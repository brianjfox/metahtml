/* logs.c: -*- C -*-  Snarfing data from logfiles. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Tue Jun 17 12:32:35 1997.

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

/* 0) #include any files that are specific to your module. */

/* 1) Declare the functions which implement the Meta-HTML functionality. */
static void pf_parse_line (PFunArgs);
static void pf_read_logfile (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag		    complex? debug_level   code    */
  { "LOGS::PARSE-LINE",		0,	0,	pf_parse_line },
  { "LOGS::READ-LOGFILE",	0,	0,	pf_read_logfile },
  { (char *)NULL,		0,	0,	(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("logs", ftab)

/* 4) Write the actual code which implements your functionality. */

/* <LOGS::PARSE-LINE line>

   Parse a line of text in Common Logfile Format, and return an alist
   containing the broken out parts of that line.

   The date is broken out into component parts consisting of:
   day, month, year, hour, minute, and second. */
#define skip_whitespace(line, i) while (whitespace (line[i])) i++

static void
pf_parse_line (PFunArgs)
{
  char *line = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (line))
    {
      register int i = 0;

      BPRINTF_BUFFER *output = bprintf_create_buffer ();

      bprintf (output, "(");

      /* Hostname. */
      skip_whitespace (line, i);
      bprintf (output, "(\"host\" . \"");
      while (line[i] && line[i] != ' ')
	bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

	/* Find the domain.  It's everything from point back to the
	   most recent period. */
      {
	int dot = i - 1;

	while (dot > 0 && line[dot] != '.') dot--;
	if (line[dot] == '.')
	  {
	    char *temp = (char *)xmalloc (i - dot);
	    dot++;
	    strncpy (temp, line + dot, i - dot);
	    temp[i - dot] = '\0';

	    bprintf (output, "(\"domain\" . \"%s\")", temp);
	  }
      }

      /* User. */
      skip_whitespace (line, i);
      bprintf (output, "(\"user\" . \"");
      while (line[i] && line[i] != ' ') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      /* Auth. */
      skip_whitespace (line, i);
      bprintf (output, "(\"auth\" . \"");
      while (line[i] && line[i] != ' ') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      /* Date, fully parsed. */
      skip_whitespace (line, i);

      /* Expecting "[". */
      if (line[i]) i++;

      /* Day. */
      bprintf (output, "(\"day\" . \"");
      while (line[i] && line[i] != '/') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      if (line[i]) i++;

      /* Month. */
      bprintf (output, "(\"month\" . \"");
      while (line[i] && line[i] != '/') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      if (line[i]) i++;

      /* Year. */
      bprintf (output, "(\"year\" . \"");
      while (line[i] && line[i] != ':') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      if (line[i]) i++;

      /* Hour. */
      bprintf (output, "(\"hour\" . \"");
      while (line[i] && line[i] != ':') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      if (line[i]) i++;

      /* Minute. */
      bprintf (output, "(\"minute\" . \"");
      while (line[i] && line[i] != ':') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      if (line[i]) i++;

      /* Second. */
      bprintf (output, "(\"second\" . \"");
      while (line[i] && line[i] != ' ') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      if (line[i]) i++;

      /* Zone. */
      bprintf (output, "(\"zone\" . \"");
      while (line[i] && line[i] != ']') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      if (line[i]) i++;

      skip_whitespace (line, i);

      /* The Request. */
      if (line[i]) i++;

      /* Request Type. */
      bprintf (output, "(\"req_type\" . \"");
      while (line[i] && line[i] != ' ') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");


      /* Request URL. */
      skip_whitespace (line, i);
      bprintf (output, "(\"req_url\" . \"");
      while (line[i] && line[i] != ' ') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      /* Request Protocol. */
      skip_whitespace (line, i);
      bprintf (output, "(\"req_protocol\" . \"");
      while (line[i] && line[i] != '"') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");
      if (line[i]) i++;


      /* Result Code. */
      skip_whitespace (line, i);
      bprintf (output, "(\"result_code\" . \"");
      while (line[i] && line[i] != ' ') bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      /* Bytes Transferred. */
      skip_whitespace (line, i);
      bprintf (output, "(\"bytes\" . \"");
      while (line[i] && !whitespace (line[i]))
	bprintf (output, "%c", line[i++]);
      bprintf (output, "\")");

      bprintf (output, ")");

      bprintf_insert (page, start, "%s", output->buffer);
      *newstart += strlen (output->buffer);
      bprintf_free_buffer (output);
    }
}

static void
pf_read_logfile (PFunArgs)
{
}

#if defined (__cplusplus)
}
#endif
