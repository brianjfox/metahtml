/* modhtml.c: -*- C -*-  HTML Helper functions written in C.

   Functions in this file were initially moved from other files in the
   main source.  The file was created in 2001, but the functions were
   created in 1995.*/

/*  Copyright (c) 2001 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Jan 29 12:31:08 2001.

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

/* <small-caps [upper="+0"] [lower="-1"]>This is a list</small-caps> */
static void pf_small_caps (PFunArgs);

/* <verbatim> .... </verbatim> Insert the contents verbatim. */
static void pf_verbatim (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "SMALL-CAPS",	1,	0,		pf_small_caps },
  { "VERBATIM",		1,	0,		pf_verbatim },

  { (char *)NULL,       0,       0,             (PFunHandler *)NULL }
};

MODULE_INITIALIZE ("modhtml", ftab)

DEFINE_SECTION (HTML-HELPERS, HTML; helper; convenience, 
"The following functions all produce HTML as output and are defined in
order to help with the creation of forms and tables.", "")

#define OTHER 1
#define UPPER 2
#define LOWER 3

#define CLOSE_STATE \
  switch (state) \
    { \
    case OTHER: bprintf (buffer, "%s", other_close); break; \
    case UPPER: bprintf (buffer, "%s", upper_close); break; \
    case LOWER: bprintf (buffer, "%s", lower_close); break; \
    }

static char *
wrap_by_character_class (char *string, int small_caps_p, int leave_braces,
			 char *upper_open, char *upper_close,
			 char *lower_open, char *lower_close,
			 char *other_open, char *other_close)
{
  register int i, c, state;
  char *result;
  BPRINTF_BUFFER *buffer;

  /* Handle easiest case first. */
  if (!string)
    return ((char *)NULL);

  if (!upper_open) upper_open = "";
  if (!upper_close) upper_close = "";
  if (!lower_open) lower_open = "";
  if (!lower_close) lower_close = "";
  if (!other_open) other_open = "";
  if (!other_close) other_close = "";

  buffer = bprintf_create_buffer ();

  state = 0;

  for (i = 0; (c = string[i]) != '\0'; i++)
    {
      if (isupper (c) && state != UPPER)
	{
	  CLOSE_STATE;
	  state = UPPER;
	  bprintf (buffer, "%s", upper_open);
	}
      else if (islower (c) && state != LOWER)
	{
	  CLOSE_STATE;
	  state = LOWER;
	  bprintf (buffer, "%s", lower_open);
	}
      else if (isspace (c))
	{
	}
      else if (leave_braces && ((c == '<') || (c == '>')))
	{
	  int point = i;
	  char *sexp;

	  CLOSE_STATE;
	  state = 0;
	  sexp = read_sexp_1 (string, &point, 0, 1);
	  if (sexp != (char *)NULL)
	    {
	      bprintf (buffer, "%s", sexp);
	      free (sexp);
	      c = '\0';
	      i = point - 1;
	    }
	}
      else if (!(isupper (c) || islower (c)) && state != OTHER)
	{
	  CLOSE_STATE;
	  state = OTHER;
	  bprintf (buffer, "%s", other_open);
	}

      if (small_caps_p && islower (c))
	c = toupper (c);

      if (c)
	bprintf (buffer, "%c", c);
    }

  CLOSE_STATE;

  result = buffer->buffer;
  free (buffer);

  return (result);
}

DEFMACRO (pf_small_caps, &key upper=size lower=size other=size,
"Modify the characters in <var body> raising lower-case
characters to upper-case, and changing the size of fonts as directed.

For example, this is how the text of \"Hello There\" can be displayed with
lowercase characters as smaller uppercase versions of themselves.

<complete-example>
  <small-caps lower=-1>Hello There</small-caps>
</complete-example>")
{
  char *string = mhtml_evaluate_string (body->buffer);

  if (string)
    {
      char *upper_size = get_one_of (vars, "upper", "upper-size", (char *)0);
      char *lower_size = get_one_of (vars, "lower", "lower-size", (char *)0);
      char *other_size = get_one_of (vars, "other", "other-size", (char *)0);
      char uo[100], lo[100], oo[1000], *cl = "</FONT>";
      char *result;

      if (!upper_size) upper_size = "+0";
      if (!lower_size) lower_size = "-1";
      if (!other_size) other_size = "+0";

      sprintf (uo, "<FONT SIZE=\"%s\">", upper_size);
      sprintf (lo, "<FONT SIZE=\"%s\">", lower_size);
      sprintf (oo, "<FONT SIZE=\"%s\">", other_size);

      result = wrap_by_character_class (string, 1, 1, uo, cl, lo, cl, oo, cl);
      bprintf_insert (page, start, "%s", result);
      free (string);
      free (result);
    }
}

DEFUN (pf_verbatim, &key quote,
"Insert <var body> verbatim, avoiding doing any processing on
the contents.  If the keyword argument <var quote> is given,
occurrences of characters with special meaning to <b>HTML</b> are
replaced with the <b>HTML</b> code to produce that character in the
output page.

Contrast this with <funref page-operators comment>.")
{
  int quote_p = var_present_p (vars, "quote");

  /* Insert the contents, and then skip past them. */
  if (body && body->buffer)
    {
      char *string = body->buffer;

      if (quote_p)
	string = html_quote_string (string);

      if (string != (char *)NULL)
	{
	  bprintf_insert (page, start, "%s", string);
	  *newstart = start + strlen (string);

	  if (quote_p)
	    free (string);
	}
    }
}

#if defined (__cplusplus)
}
#endif
