/* logging.h: Structures and defines used in the Mhttpd logging system. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Tue Nov 14 19:14:52 1995. */

/* This file is part of <Meta-HTML>(tm), a system for the rapid
   deployment of Internet and Intranet applications via the use
   of the Meta-HTML language.

   Copyright (c) 1995, 2000, Brian J. Fox (bfox@ai.mit.edu).

   Meta-HTML is free software; you can redistribute it and/or modify
   it under the terms of the General Public License as published
   by the Free Software Foundation; either version 1, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   FSF GPL for more details.

   You should have received a copy of the FSF General Public License
   along with this program; if you have not, you may obtain one
   electronically at the following URL:

	http://www.metahtml.com/COPYING
*/

#if !defined (_LOGGING_H_)
#define _LOGGING_H_ 1

#define log_ERROR   0
#define log_NOTIFY  1
#define log_ACCESS  2
#define log_DEBUG   3
#define log_REFERER 4
#define log_AGENT   5

#if defined (__cplusplus)
extern "C"
{
#endif

typedef struct
{
  int which;			/* One of the above defines. */
  char *logfile;		/* Full path to the log file. */
} LOG_MAPPING;

extern LOG_MAPPING **mhttpd_logfiles;

extern void mhttpd_set_logfile (int which, char *name);
extern char *mhttpd_get_logfile (int which);
extern void mhttpd_log (int which, char *format, ...);
extern void mhttpd_access_log (char *name, char *format, ...);
extern void mhttpd_log_verbatim (int which, char *format, ...);
extern char *mhttpd_date_format (long ticks);

#if defined (__cplusplus)
}
#endif

#endif /* !_LOGGING_H_ */
