/* csv.c: -*- C -*-  Convert to/from Comma/Tab Separated Values. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sat Nov 15 05:42:50 1997.

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
static void pf_csv_to_array (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag			complex? debug_level	   code    */
  { "CSV::CSV-TO-ARRAY",	0,	 0,		pf_csv_to_array },
  { (char *)NULL,		0,	 0,		(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("csv", ftab)

DEFINE_SECTION (CSV-MODULE, , 
"The CSV module is useful for importing <b>C</b>omma <b>S</b>eparated
<b>V</b>alues from external programs which are capable of producing them.
Such programs include word processors, legacy database systems, spreadsheets,
and the like.", "")

/* Return an array of strings, each one representing data taken from the
   text in TEXT.  Each element of TEXT is separated from the next by
   SEP. */

char **
csv_to_array (char *text, char *sep)
{
  char **results = (char **)NULL;
  int r_slots = 0;
  int r_index = 0;

  if (text != (char *)NULL)
    {
      register int i = 0;
      int in_quotes = 0;
      int element_beg = 0;
      int element_end = 0;
      int element_len = 0;
      int done = 0;

      while (!done)
	{
	  int c = text[i];

	  if (((c == *sep) && (!in_quotes)) || (c == '\0'))
	    {
	      char *value;

	      element_end = i;
	      if (text[element_beg] == '"') { element_beg++; element_end--; }
	      element_len = element_end - element_beg;
	      value = (char *)xmalloc (1 + element_len);
	      strncpy (value, text + element_beg, element_len);
	      value[element_len] = '\0';

	      if (r_index + 2 > r_slots)
		results = (char **)xrealloc
		  (results, (r_slots += 10) * sizeof (char *));

	      results[r_index++] = value;
	      results[r_index] = (char *)NULL;
	      if (c != '\0')
		{
		  i++;
		  for (element_beg = i; whitespace (text[i]); i++);
		  element_beg = element_end = i;
		  i--;
		}
	    }
	  else if (c == '"')
	    {
	      in_quotes = !in_quotes;
	    }
	  else if (c == '\\')
	    i++;

	  if (c == '\0')
	    done++;
	  else
	    i++;
	}
    }

  return (results);
}

DEFUNX (pf_csv::csv_to_array, cvs-text array-name &key sep=SEPARATOR,
"Returns an array of the values specified in the comma separated values
represented by <var csv-text>.  The values are placed in <var array-name>
in the order in which they are found.")
static void
pf_csv_to_array (PFunArgs)
{
  char *text = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *sep = mhtml_evaluate_string (get_value (vars, "SEP"));
  char *array_name = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char **results = csv_to_array (text, sep ? sep : ",");

  if (results != (char **)NULL)
    {
      if (empty_string_p (array_name))
	{
	  register int i, len = 0;

	  for (i = 0; results[i] != (char *)NULL; i++)
	    {
	      int this_len = strlen (results[i]);
	      len += this_len;
	      bprintf_insert (page, start, "%s\n", results[i]);
	      free (results[i]);
	      len++;
	      start += (this_len + 1);
	    }
	  free (results);
	  *newstart = start;
	}
      else
	{
	  symbol_store_array (array_name, results);
	}
    }

  xfree (text);
  xfree (sep);
  xfree (array_name);
}

#if defined (__cplusplus)
}
#endif
