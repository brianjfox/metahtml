/* modsyslog.c: -*- C -*-  Write to system logger. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sun Aug 15 17:40:24 1999.

    This file is part of <Meta-HTML>(tm), a system for the rapid
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

	 http://www.metahtml.com/COPYING  */

#include "modules.h"
#include "syslog.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_syslog (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "SYSLOG",		0,	0,		pf_syslog },
  { (char *)NULL,	0,	0,		(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("modsyslog", ftab)

DEFINE_SECTION (SYSLOG-MODULE, logging,
"Provides an interface to the lower level system logging facility.","")

typedef struct
{
  char *label;
  int mask;
} trans;

#if !defined (LOG_AUTHPRIV)
#  define LOG_AUTHPRIV LOG_AUTH
#endif

static trans facilities[] =
{
  { "AUTH",	LOG_AUTH },
  { "AUTHPRIV",	LOG_AUTHPRIV },
  { "CRON",	LOG_CRON },
  { "DAEMON",	LOG_DAEMON },
  { "KERN",	LOG_KERN },
  { "LOCAL0",	LOG_LOCAL0 },
  { "LOCAL1",	LOG_LOCAL1 },
  { "LOCAL2",	LOG_LOCAL2 },
  { "LOCAL3",	LOG_LOCAL3 },
  { "LOCAL4",	LOG_LOCAL4 },
  { "LOCAL5",	LOG_LOCAL5 },
  { "LOCAL6",	LOG_LOCAL6 },
  { "LOCAL7",	LOG_LOCAL7 },
  { "LPR",	LOG_LPR },
  { "MAIL",	LOG_MAIL },
  { "NEWS",	LOG_NEWS },
  { "SYSLOG",	LOG_SYSLOG },
  { "USER",	LOG_USER },
  { "UUCP",	LOG_UUCP },
  { (char *)NULL, -1 }
};

static trans priorities[] =
{
  { "EMERG",	LOG_EMERG },
  { "ALERT",	LOG_ALERT },
  { "CRIT",	LOG_CRIT },
  { "ERR",	LOG_ERR },
  { "WARNING",	LOG_WARNING },
  { "NOTICE",	LOG_NOTICE },
  { "INFO",	LOG_INFO },
  { "DEBUG",	LOG_DEBUG },
  { (char *)NULL, -1 }
};

static int
find_mask (char *label, trans *array)
{
  register int i;
  int mask = -1;

  for (i = 0; array[i].label != (char *)NULL; i++)
    if (strcasecmp (label, array[i].label) == 0)
      {
	mask = array[i].mask;
	break;
      }
  return (mask);
}

DEFUN (pf_syslog, &optional who message &key facility priority,
"<var syslog> generates a log message, which will be distributed by
syslogd(8).")
{
  char *whoarg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *facility_arg = mhtml_evaluate_string (get_value (vars, "FACILITY"));
  char *priority_arg = mhtml_evaluate_string (get_value (vars, "PRIORITY"));
  char *who = whoarg ? whoarg : "<Meta-HTML>";
  char *msg = mhtml_evaluate_string (get_positional_arg (vars, 1));
  int facility = LOG_USER, priority = LOG_ERR;
  
  if (!empty_string_p (facility_arg))
    {
      int mask = find_mask (facility_arg, &facilities[0]);
      if (mask > -1)
	facility = mask;
    }

  if (!empty_string_p (priority_arg))
    {
      int mask = find_mask (priority_arg, &priorities[0]);
      if (mask > -1)
	priority = mask;
    }

  openlog (who, LOG_PID | LOG_CONS, facility);
  if (!empty_string_p (msg))
    syslog (priority, "%s", msg);
  closelog ();

  xfree (facility_arg);
  xfree (priority_arg);
}

#if defined (__cplusplus)
}
#endif
