/* moddate.c: -*- C -*-  Module allows the parsing of random date inputs. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Jul 12 01:32:17 1999.

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

#if defined (__cplusplus)
extern "C"
{
#endif

#include "parsedate.c"

static void pf_date_to_time (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag		     complex? debug_level          code    */
  { "DATE::DATE-TO-TIME",	0,	0,		pf_date_to_time },
  { (char *)NULL,		0,	0,		(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("moddate", ftab)

DEFINE_SECTION (MODDATE-MODULE, dates; times; date; parsing,
"Complex date parsing.  Not normally included in the straight Meta-HTML binary,
DATE::DATE-TO-TIME attempts to understand all kinds of dates, including those
that are relative, such as 'next tuesday', or '2 hours ago'.", "")

DEFUNX (date::date-to-time, &optional date-string pack &key parse zone,
"Parse <var DATE-STRING>, and return the seconds since the Epoch.")

int getdate_print_parts = 0;

static void
set_with_padding (int pad, Package *p, char *tag, int val)
{
  char digits[20];
  sprintf (digits, "%0*d", pad, val);
  forms_set_tag_value_in_package (p, tag, digits);
}

static void
pf_date_to_time (PFunArgs)
{
  char *string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *parse_arg = mhtml_evaluate_string (get_value (vars, "PARSE"));
  char *zone_arg = mhtml_evaluate_string (get_value (vars, "zone"));
  time_t seconds = (time_t)time ((time_t *)NULL);
  struct tm *d = (struct tm *)NULL;
  char *use_zone = zone_arg;

  if (empty_string_p (use_zone))
    use_zone = pagefunc_get_variable ("*date*::timezone");

  if (empty_string_p (use_zone))
    use_zone = pagefunc_get_variable ("env::tz");

  if (empty_string_p (use_zone))
    use_zone = (char *)getenv ("TZ");

  getdate_print_parts = debug_level;
  seconds = get_date (string, use_zone);
  d = &input_date;

  if (!empty_string_p (parse_arg))
    {
      Package *p;
      char temp[256], year[5];
      static char *weekdays[] =
      { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
      static char *months[] = 
      { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
      char *pack_arg = mhtml_evaluate_string (get_value (vars, "PACK"));

      /* Sanity check values from parsedate.y.  Who knows what crap I wrote
	 in there? */
      if (d->tm_mon > 11) d->tm_mon = 0;

      if (!empty_string_p (pack_arg))
	p = symbol_get_package (pack_arg);
      else
	p = symbol_get_package ((char *)NULL);

      sprintf (year, "%4d", d->tm_year + 1900);
      forms_set_tag_value_in_package (p, "WEEKDAY", weekdays[d->tm_wday]);
      set_with_padding (2, p, "DAY", d->tm_mday);
      set_with_padding (2, p, "CDAY", d->tm_mday);
      sprintf (temp, "%02d/%02d/%s", d->tm_mon + 1, d->tm_mday, year + 2);
      forms_set_tag_value_in_package (p, "CANON", temp);
      sprintf (temp, "%02d/%02d/%s", d->tm_mon + 1, d->tm_mday, year);
      forms_set_tag_value_in_package (p, "CANON1", temp);
      forms_set_tag_value_in_package (p, "YEAR", year);
      forms_set_tag_value_in_package (p, "YY", year + 2);
      mhtml_set_numeric_variable_in_package (p, "MINDEX", d->tm_mon);
      forms_set_tag_value_in_package (p, "MONTH", months[d->tm_mon]);
      set_with_padding (3, p, "JULIAN", d->tm_yday);
      forms_set_tag_value_in_package (p, "ISDST", d->tm_isdst ? "true" : "");
      set_with_padding (2, p, "MM", d->tm_mon + 1);
      set_with_padding (2, p, "HOUR", d->tm_hour);
      set_with_padding (2, p, "MINUTE", d->tm_min);
      set_with_padding (2, p, "SECOND", d->tm_sec);
      set_with_padding (2, p, "WDAY", d->tm_wday);
#if defined (linux)
      mhtml_set_numeric_variable_in_package (p, "gmtoff", d->tm_gmtoff);
      forms_set_tag_value_in_package (p, "ZONE", (char *)d->tm_zone);
#endif
      if (empty_string_p (pack_arg))
	{
	  char *alist = package_to_alist (p, 1);
	  bprintf_insert (page, start, "%s", alist);
	  *newstart += strlen (alist);
	  free (alist);
	  symbol_destroy_package (p);
	}

      xfree (pack_arg);
    }
  else
    bprintf_insert (page, start, "%ld", (long)seconds);

  xfree (string);
  xfree (zone_arg);
}

#if defined (__cplusplus)
}
#endif
