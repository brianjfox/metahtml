/* cleaner.c: -*- C -*-  Cleanup excess whitespace before point. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Tue Apr 15 13:01:17 1996.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1995, 2001, Brian J. Fox (bfox@ai.mit.edu).

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

#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_cleanup_whitespace (PFunArgs);
static void pf_cleanup_beautify (PFunArgs);

static PFunDesc func_table[] =
{
  /*   tag		complex? debug_level	   code    */
  { "CLEANUP-WHITESPACE", 0,	 0,		pf_cleanup_whitespace },
  { "CLEANUP-BEAUTIFY",	  0,	 0,		pf_cleanup_beautify },
  { (char *)NULL,	  0,	 0,		(PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_cleaner_funcs)
DOC_SECTION (PAGE-OPERATORS)

#define newline(x) ((x) == '\r' || (x) == '\n')
static char *
cleanup_html_whitespace (char *string, int len, int beautify_p)
{
  register int src, dst;
  char *result = (char *)xmalloc (1 + len);
  int quoted = 0;
  int brace_level = 0;
  int in_pre = 0;
  int in_textarea = 0;
  int in_script = 0;
  int accum = 0, newlines = 0;

  src = dst = 0;

  while (src < len)
    {
      if (!quoted && !in_pre && !in_textarea && !in_script)
	{
	  if (newline (string[src]))
	    newlines++;
	  else if (whitespace (string[src]))
	    accum++;
	  else
	    {
	      if (newlines)
		{
		  if (string[src] != '>')
		    {
		      result[dst++] = '\n';
		      if (beautify_p && (newlines > 1))
			result[dst++] = '\n';
		    }
		  newlines = 0;
		  accum = 0;
		}
	      else if (accum)
		{
		  accum = 0;
		  if (string[src] != '>')
		    result[dst++] = ' ';
		}
	      result[dst++] = string[src];
	    }
	}
      else
	{
	  result[dst++] = string[src];
	}

      if (string[src] == '"')
	quoted = !quoted;

      if (!quoted)
	{
	  if (string[src] == '<')
	    {
	      brace_level++;
	      /* Check for <TEXTAREA ....
		 Do NOT check for "<TEXTAREA>" as that is handled below. */
	      if ((string[src + 1] == 'T' || string[src + 1] == 't') &&
		  (string[src + 2] == 'E' || string[src + 2] == 'e') &&
		  (string[src + 3] == 'X' || string[src + 3] == 'x') &&
		  (string[src + 4] == 'T' || string[src + 4] == 't') &&
		  (string[src + 5] == 'A' || string[src + 5] == 'a') &&
		  (string[src + 6] == 'R' || string[src + 6] == 'r') &&
		  (string[src + 7] == 'E' || string[src + 7] == 'e') &&
		  (string[src + 8] == 'A' || string[src + 8] == 'a') &&
		  (whitespace (string[src + 9]) ||
		   (newline (string[src + 9]))))
		in_textarea++;
	      else if ((string[src + 1] == 'S' || string[src + 1] == 's') &&
		       (string[src + 2] == 'C' || string[src + 2] == 'c') &&
		       (string[src + 3] == 'R' || string[src + 3] == 'r') &&
		       (string[src + 4] == 'I' || string[src + 4] == 'i') &&
		       (string[src + 5] == 'P' || string[src + 5] == 'p') &&
		       (string[src + 6] == 'T' || string[src + 6] == 't') &&
		       (whitespace (string[src + 7]) ||
			(newline (string[src + 7]))))
		in_script++;
	    }
	  else if (string[src] == '>')
	    {
	      brace_level--;
	      if (src > 3)
		{
		  /* Check explicitly for <pre> and </pre> */
		  if ((string[src - 1] == 'E' || string[src - 1] == 'e') &&
		      (string[src - 2] == 'R' || string[src - 2] == 'r') &&
		      (string[src - 3] == 'P' || string[src - 3] == 'p'))
		    {
		      if (string[src - 4] == '<')
			in_pre++;
		      else if (in_pre &&
			       (string[src - 4] == '/' &&
				string[src - 5] == '<'))
			in_pre--;
		    }

		  if (src > 6)
		    {
		      /* Check explicitly for <script> and </script>. */
		      if ((string[src - 1] == 'T' || string[src - 1] == 't') &&
			  (string[src - 2] == 'P' || string[src - 2] == 'p') &&
			  (string[src - 3] == 'I' || string[src - 3] == 'i') &&
			  (string[src - 4] == 'R' || string[src - 4] == 'r') &&
			  (string[src - 5] == 'C' || string[src - 5] == 'c') &&
			  (string[src - 6] == 'S' || string[src - 6] == 's'))
			{
			  if (string[src - 7] == '<')
			    in_script++;
			  else if (in_script &&
				   (string[src - 7] == '/' && (src > 7) &&
				    string[src - 8] == '<'))
			    in_script--;
			}
		    }

		  if (src > 8)
		    {
		      /* Check explicitly for </textarea>. */
		      if ((string[src - 1] == 'A' || string[src - 1] == 'a') &&
			  (string[src - 2] == 'E' || string[src - 2] == 'e') &&
			  (string[src - 3] == 'R' || string[src - 3] == 'r') &&
			  (string[src - 4] == 'A' || string[src - 4] == 'a') &&
			  (string[src - 5] == 'T' || string[src - 5] == 't') &&
			  (string[src - 6] == 'X' || string[src - 6] == 'x') &&
			  (string[src - 7] == 'E' || string[src - 7] == 'e') &&
			  (string[src - 8] == 'T' || string[src - 8] == 't'))
			{
			  if (string[src - 9] == '<')
			    in_textarea++;
			  else if (in_textarea &&
				   (string[src - 9] == '/' && (src > 9) &&
				    string[src - 10] == '<'))
			    in_textarea--;
			}
		    }
		}
	    }
	}

      src++;
    }

  result[dst] = '\0';
  return (result);
}

DEFUN (pf_cleanup_whitespace, &optional varname &key beautify,
"Reduces the amount of whitespace in the output page to the minimum
required.  This function carefully avoids the contents of <example
code><pre> ... </pre></example> constructs, as well as any material
appearing in a <example code><textarea ...> ... </textarea></example>
construct.

Given <var varname>, operates on the contents of that variable.

Placed as the last instruction on a page, it is an effective way to
reduce the amount of data sent back over the network to the connecting
browser.")
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *beautify = mhtml_evaluate_string (get_value (vars, "beautify"));
  int beautify_p = !empty_string_p (beautify);
  char *cleaned = (char *)NULL;

  xfree (beautify);

  if (varname != (char *)NULL)
    {
      char *temp = pagefunc_get_variable (varname);

      if (temp != (char *)NULL)
	{
	  cleaned = cleanup_html_whitespace (temp, strlen (temp), beautify_p);
	  pagefunc_set_variable (varname, cleaned);
	  xfree (cleaned);
	}
    }
  else
    {
      PagePDL *top_page = page_pdl_page (0);
      page = top_page->page;
      start = *top_page->search_start_modified;
      if (start == -1)
	start = top_page->start;
      else
	top_page->start = start;

      cleaned = cleanup_html_whitespace (page->buffer, start, beautify_p);
      bprintf_delete_range (page, 0, start);
      bprintf_insert_text (page, 0, cleaned);
      start = strlen (cleaned);
      *top_page->search_start_modified = start;
      free (cleaned);
    }
}

static void
pf_cleanup_beautify (PFunArgs)
{
  forms_set_tag_value_in_package (vars, "beautify", "true");
  pf_cleanup_whitespace (PassPFunArgs);
}
#if defined (__cplusplus)
}
#endif
