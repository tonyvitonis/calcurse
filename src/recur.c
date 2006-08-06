/*	$calcurse: recur.c,v 1.3 2006/08/06 14:30:46 culot Exp $	*/

/*
 * Calcurse - text-based organizer
 * Copyright (c) 2004-2006 Frederic Culot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Send your feedback or comments to : calcurse@culot.org
 * Calcurse home page : http://culot.org/calcurse
 *
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "i18n.h"
#include "utils.h"
#include "apoint.h"
#include "event.h"
#include "recur.h"
#include "day.h"
#include "vars.h"

struct recur_apoint_s *recur_alist;
struct recur_event_s *recur_elist;

/* Insert a new recursive appointment in the general linked list */
struct recur_apoint_s *recur_apoint_new(char *mesg, long start, long dur, 
	int type, int freq, long until)
{
	struct recur_apoint_s *o, **i;
	o = (struct recur_apoint_s *) malloc(sizeof(struct recur_apoint_s));
	o->rpt = (struct rpt_s *) malloc(sizeof(struct rpt_s));
	o->mesg = (char *) malloc(strlen(mesg) + 1);
	strcpy(o->mesg, mesg);
	o->start = start;
	o->dur = dur;
	o->rpt->type = type;
	o->rpt->freq = freq;
	o->rpt->until = until;
	i = &recur_alist;
	for (;;) {
		if (*i == 0 || (*i)->start > start) {
			o->next = *i;
			*i = o;
			break;
		}
		i = &(*i)->next;
	}
	return o;
}

/* Insert a new recursive event in the general linked list */
struct recur_event_s *recur_event_new(char *mesg, long day, int id, 
	int type, int freq, long until)
{
	struct recur_event_s *o, **i;
	o = (struct recur_event_s *) malloc(sizeof(struct recur_event_s));
	o->rpt = (struct rpt_s *) malloc(sizeof(struct rpt_s));
	o->mesg = (char *) malloc(strlen(mesg) + 1);
	strcpy(o->mesg, mesg);
	o->day = day;
	o->id = id;
	o->rpt->type = type;
	o->rpt->freq = freq;
	o->rpt->until = until;
	i = &recur_elist;
	for (;;) {
		if (*i == 0 || (*i)->day > day) {
			o->next = *i;
			*i = o;
			break;
		}
		i = &(*i)->next;
	}
	return o;
}

/* 
 * Correspondance between the defines on recursive type, 
 * and the letter to be written in file. 
 */
char recur_def2char(int define){
	char recur_char;

	switch (define) {
	case 1:
		recur_char = 'D';
		break;	
	case 2:
		recur_char = 'W';
		break;
	case 3:
		recur_char = 'M';
		break;
	case 4:
		recur_char = 'Y';
		break;
	}
	return recur_char;
}

/* 
 * Correspondance between the letters written in file and the defines
 * concerning the recursive type.
 */
int recur_char2def(char type){
	int recur_def;

	switch (type) {
	case 'D':
		recur_def = RECUR_DAILY;
		break;	
	case 'W':
		recur_def = RECUR_WEEKLY;
		break;
	case 'M':
		recur_def = RECUR_MONTHLY;
		break;
	case 'Y':
		recur_def = RECUR_YEARLY;
		break;
	}
	return recur_def;
}

/* Write days for which recurrent items should not be repeated. */
void recur_write_exc(struct days_s *exc, FILE *f) {
	struct days_s *day;
	struct tm *lt;
	time_t t;
	int st_mon, st_day, st_year;
	int end_mon, end_day, end_year;

	for (day = exc; day != 0; day = exc->next) {
		t = exc->st;
		lt = localtime(&t);
		st_mon = lt->tm_mon + 1;
		st_day = lt->tm_mday;
		st_year = lt->tm_year + 1900;
		fprintf(f, " !%02u/%02u/%04u",
			st_mon, st_day, st_year);	
	}
}

/* Writting of a recursive appointment into file. */
void recur_apoint_write(struct recur_apoint_s *o, FILE *f)
{
	struct tm *lt;
	time_t t;

	t = o->start;
	lt = localtime(&t);
	fprintf(f, "%02u/%02u/%04u @ %02u:%02u",
		lt->tm_mon + 1, lt->tm_mday, 1900 + lt->tm_year,
		lt->tm_hour, lt->tm_min);

	t = o->start + o->dur;
	lt = localtime(&t);
	fprintf(f, " -> %02u/%02u/%04u @ %02u:%02u",
		lt->tm_mon + 1, lt->tm_mday, 1900 + lt->tm_year,
		lt->tm_hour, lt->tm_min);

	t = o->rpt->until;
	if (t == 0) { /* We have an endless recurrent appointment. */
		fprintf(f, " {%d%c", o->rpt->freq, 
			recur_def2char(o->rpt->type)); 
		if (o->exc != 0) recur_write_exc(o->exc, f);
		fprintf(f, "} |%s\n", o->mesg);
	} else {
		lt = localtime(&t);
		fprintf(f, " {%d%c -> %02u/%02u/%04u",
			o->rpt->freq, recur_def2char(o->rpt->type),
			lt->tm_mon + 1, lt->tm_mday, 1900 + lt->tm_year);
		fprintf(f,"} |%s\n", o->mesg);
	}
}

/* Writting of a recursive event into file. */
void recur_event_write(struct recur_event_s *o, FILE *f)
{
	struct tm *lt;
	time_t t;
	int st_mon, st_day, st_year;
	int end_mon, end_day, end_year;
	
	t = o->day;
	lt = localtime(&t);
	st_mon = lt->tm_mon + 1;
	st_day = lt->tm_mday;
	st_year = lt->tm_year + 1900;
	t = o->rpt->until;
	if (t == 0) { /* We have an endless recurrent event. */
		fprintf(f, "%02u/%02u/%04u [%d] {%d%c",
			st_mon, st_day, st_year, o->id, o->rpt->freq,
			recur_def2char(o->rpt->type));
		if (o->exc != 0) recur_write_exc(o->exc, f);
		fprintf(f,"} %s\n", o->mesg);
	} else {
		lt = localtime(&t);
		end_mon = lt->tm_mon + 1;
		end_day = lt->tm_mday;
		end_year = lt->tm_year + 1900;
		fprintf(f, "%02u/%02u/%04u [%d] {%d%c -> %02u/%02u/%04u",
			st_mon, st_day, st_year, o->id, 
			o->rpt->freq, recur_def2char(o->rpt->type),
			end_mon, end_day, end_year);
		if (o->exc != 0) recur_write_exc(o->exc, f);
		fprintf(f, "} %s\n", o->mesg);
	}		
}

/* Load the recursive appointment description */
struct recur_apoint_s *recur_apoint_scan(FILE * f, struct tm start, 
	struct tm end, char type, int freq, struct tm until)
{
	struct tm *lt;
	char buf[MESG_MAXSIZE], *nl;
	time_t tstart, tend, t, tuntil;

	t = time(NULL);
	lt = localtime(&t);

        /* Read the appointment description */
	fgets(buf, MESG_MAXSIZE, f);
	nl = strchr(buf, '\n');
	if (nl) {
		*nl = '\0';
	}

	start.tm_sec = end.tm_sec = 0;
	start.tm_isdst = end.tm_isdst = -1;
	start.tm_year -= 1900;
	start.tm_mon--;
	end.tm_year -= 1900;
	end.tm_mon--;
	tstart = mktime(&start);
	tend = mktime(&end);

	if (until.tm_year != 0) {
		until.tm_hour = 12;
        	until.tm_min = 0;
		until.tm_sec =  0;
		until.tm_isdst = -1;
		until.tm_year -= 1900;
		until.tm_mon--;
		tuntil = mktime(&until);
	} else {
		tuntil = 0;
	}

	if (tstart == -1 || tend == -1 || tstart > tend || tuntil == -1) {
		fputs(_("FATAL ERROR in apoint_scan: date error in the appointment\n"), stderr);
		exit(EXIT_FAILURE);
	}
      
	return recur_apoint_new(buf, tstart, tend - tstart, 
		recur_char2def(type), freq, tuntil);
}

/* Load the recursive events from file */
struct recur_event_s *recur_event_scan(FILE * f, struct tm start, int id, 
	char type, int freq, struct tm until)
{
	struct tm *lt;
	char buf[MESG_MAXSIZE], *nl;
	time_t tstart, t, tuntil;

	t = time(NULL);
	lt = localtime(&t);

        /* Read the event description */
	fgets(buf, MESG_MAXSIZE, f);
	nl = strchr(buf, '\n');
	if (nl) {
		*nl = '\0';
	}
      
      	start.tm_hour = until.tm_hour = 12;
        start.tm_min = until.tm_min = 0;
	start.tm_sec = until.tm_sec =  0;
	start.tm_isdst = until.tm_isdst = -1;
	start.tm_year -= 1900;
	start.tm_mon--;
	if (until.tm_year != 0) {
		until.tm_year -= 1900;
		until.tm_mon--;
		tuntil = mktime(&until);
	} else {
		tuntil = 0;
	}
	tstart = mktime(&start);
	if ( (tstart == -1) || (tuntil == -1) ) {
		fputs(_("FATAL ERROR in recur_event_scan: date error in the event\n"), stderr);
		exit(EXIT_FAILURE);
	}
	
	return recur_event_new(buf, tstart, id, recur_char2def(type), freq, tuntil);
}

/* Write recursive items to file. */
void recur_save_data(FILE *f)
{
	struct recur_event_s *re;
	struct recur_apoint_s *ra;

	for (re = recur_elist; re != 0; re = re->next)
		recur_event_write(re, f);
	for (ra = recur_alist; ra != 0; ra = ra->next)
		recur_apoint_write(ra, f);
}

/* Check if the recurrent item belongs to the selected day. */
unsigned recur_item_inday(long item_start, struct days_s *item_exc,
				int rpt_type, int rpt_freq,
			  	long rpt_until, long day_start)
{
	const int DAYINSEC = 86400; 
	long day_end = day_start + DAYINSEC;
	int inday = 0;
	struct tm *lt;
	struct days_s *exc;
	time_t t;
	char *error = 
		_("FATAL ERROR in recur_item_inday: unknown item type\n");

	for (exc = item_exc; exc != 0; exc = exc->next)
		if (exc->st < day_end && exc->st >= day_start) 
			return 0;
	if (rpt_until == 0) /* we have an endless recurrent item */
		rpt_until = day_end;
	while (item_start <= day_end && item_start <= rpt_until) {
		if (item_start < day_end && item_start >= day_start) {
			inday = 1;
			break;
		}
		t = item_start;
		lt = localtime(&t);
		if (rpt_type == RECUR_DAILY) {
			lt->tm_mday += rpt_freq;
		} else if (rpt_type == RECUR_WEEKLY) {
			lt->tm_mday += rpt_freq * 7;
		} else if (rpt_type == RECUR_MONTHLY) {
			lt->tm_mon += rpt_freq;
		} else if (rpt_type == RECUR_YEARLY) {
			lt->tm_year += rpt_freq;
		} else { /* NOT REACHED */
			fputs(error, stderr);
			exit(EXIT_FAILURE);
		}
		item_start = date2sec(lt->tm_year + 1900, lt->tm_mon + 1, 
			lt->tm_mday, 0, 0);
	}	
	return inday;
}

/* Returns a structure of type apoint_s given a structure of type recur_apoint_s */
struct apoint_s *recur_apoint_s2apoint_s(struct recur_apoint_s *p)
{
	struct apoint_s *a;

	a = (struct apoint_s *) malloc(sizeof(struct apoint_s));
	a->mesg = (char *) malloc(strlen(p->mesg) + 1);
	a->start = p->start;
	a->dur = p->dur;
	a->mesg = p->mesg;
	return a;
}

/* 
 * Delete a recurrent event from the list (if delete_whole is not null),
 * or delete only one occurence of the recurrent event. 
 */
void recur_event_erase(long start, unsigned num, unsigned delete_whole)
{
        unsigned n;
        struct recur_event_s *i, **iptr;
	struct days_s *o, **j;

        n = 0;
        iptr = &recur_elist;
        for (i = recur_elist; i != 0; i = i->next) {
                if (recur_item_inday(i->day, i->exc, i->rpt->type,
			i->rpt->freq, i->rpt->until, start)) {
                        if (n == num) {
				if (delete_whole) {
                                	*iptr = i->next;
                                	free(i->mesg);
					free(i->rpt);
					free(i->exc);
                                	free(i);
                                	return;
				} else {
					o = (struct days_s *) 
					    malloc(sizeof(struct days_s));
					o->st = start;
					j = &i->exc;
					for (;;) {
						if(*j==0 || (*j)->st > start) {
							o->next = *j;
							*j = o;
							break;
						}
						j = &(*j)->next;
					}
					return;
				}
                        }
                        n++;
                }
                iptr = &i->next;
        }
        /* NOTREACHED */
        fputs(_("FATAL ERROR in recur_event_erase: no such event\n"),
		stderr);
        exit(EXIT_FAILURE);
}

/*
 * Delete a recurrent appointment from the list (if delete_whole is not null),
 * or delete only one occurence of the recurrent appointment. 
 */
void recur_apoint_erase(long start, unsigned num, unsigned delete_whole)
{
        unsigned n;
        struct recur_apoint_s *i, **iptr;
	struct days_s *o, **j;

        n = 0;
        iptr = &recur_alist;
        for (i = recur_alist; i != 0; i = i->next) {
                if (recur_item_inday(i->start, i->exc, i->rpt->type,
			i->rpt->freq, i->rpt->until, start)) {
                        if (n == num) {
				if (delete_whole) {
					*iptr = i->next;
					free(i->mesg);
					free(i->rpt);
					free(i->exc);
					free(i);
					return;
				} else {
					o = (struct days_s *) 
					    malloc(sizeof(struct days_s));
					o->st = start;
					j = &i->exc;
					for (;;) {
						if(*j==0 || (*j)->st > start) {
							o->next = *j;
							*j = o;
							break;
						}
						j = &(*j)->next;
					}
					return;
				}
			}
                        n++;
                }
                iptr = &i->next;
        }
        /* NOTREACHED */
        fputs(_("FATAL ERROR in recur_apoint_erase: no such appointment\n"),
		stderr);
        exit(EXIT_FAILURE);
}

/*
 * Ask user for repetition characteristics:
 * 	o repetition type: daily, weekly, monthly, yearly
 *	o repetition frequence: every X days, weeks, ...
 *	o repetition end date
 * and then delete the selected item to recreate it as a recurrent one
 */
void recur_repeat_item(int sel_year, int sel_month, int sel_day, 
    int item_nb, int colr) {
	int i, ch = 0;
	int valid_date = 0, date_entered = 0;
	int year = 0, month = 0, day = 0;
	char user_input[MAX_LENGTH];
	char *mesg_type_1 = 
	_("Enter the repetition type: (D)aily, (W)eekly, (M)onthly, (Y)early");
	char *mesg_type_2 = _("[D/W/M/Y] ");
	char *mesg_freq_1 =
	_("Enter the repetition frequence:");
	char *mesg_until_1 = 
	_("Enter the ending date: [mm/dd/yyyy] or '0' for an endless repetition");
	char *mesg_wrong_1 = _("The entered date is not valid.");
	char *mesg_wrong_2 =
	_("Possible formats are [mm/dd/yyyy] or '0' for an endless repetetition");
	char *wrong_type_1 = _("This item is already a repeated one.");
	char *wrong_type_2 = _("Press [ENTER] to continue.");
	int type = 0, freq = 0, id;
	struct day_item_s *p; 
	struct recur_apoint_s *ra;
	struct recur_event_s *re;
	long until, date;

	p = day_get_item(item_nb);
	if (p->type != APPT && p->type != EVNT) {
		status_mesg(wrong_type_1, wrong_type_2);
		return;
	}

	while ( (ch != 'D') && (ch != 'W') && (ch != 'M') 
	    && (ch != 'Y') && (ch != ESCAPE) ) {
		status_mesg(mesg_type_1, mesg_type_2);
		ch = wgetch(swin);	 		
		ch = toupper(ch);
	}
	if (ch == ESCAPE) {
		return;
	} else {
		type = recur_char2def(ch);
		ch = 0;
	}

	status_mesg(mesg_freq_1, "");
	getstring(swin, colr, user_input, 0, 1);
	if (strlen(user_input) != 0) {
		freq = atoi(user_input);
		strcpy(user_input, "");
	} else {
		return;
	}

	while (!date_entered) {
		status_mesg(mesg_until_1, "");
		getstring(swin, colr, user_input, 0, 1);
		if (strlen(user_input) != 0) {
			if (strlen(user_input) == 1 && 
			    strncmp(user_input, "0", 1) == 0 )  {
				until = 0;
				date_entered = 1;
			} else { 
				valid_date = check_date(user_input);
				if (valid_date) {
					sscanf(user_input, "%d / %d / %d", 
						&month, &day, &year);	
					until = date2sec(year, month, day, 0, 0);
					date_entered = 1;
				} else {
					status_mesg(mesg_wrong_1, mesg_wrong_2);
					date_entered = 0;
				}
			}
		} else {
			return;
		}
	}
	
	date = date2sec(sel_year, sel_month, sel_day, 0, 0);
	day_erase_item(date, item_nb);
	if (p->type == EVNT) {
		re = recur_event_new(p->mesg, p->start, p->evnt_id, 
			type, freq, until);
	} else if (p->type = APPT) {
		ra = recur_apoint_new(p->mesg, p->start, p->appt_dur, 
			type, freq, until);
	} else { /* NOTREACHED */
		fputs(_("FATAL ERROR in recur_repeat_item: wrong item type\n"),
			stderr);
		exit(EXIT_FAILURE);
	}
} 
