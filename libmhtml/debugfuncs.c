/* debugfuncs.c: -*- C -*-  Functions which facilitate debugging Meta-HTML. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Tue Jul 22 13:59:02 1997.  */

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
/************************************************************/
/*							    */
/*		   Debugger Facilitation Functions	    */
/*							    */
/************************************************************/

static void pf_debugging_on (PFunArgs);
static void pf_page_debug (PFunArgs);
static void pf_debugging_output (PFunArgs);
static void pf_system_error_output (PFunArgs);
static void pf_stack_trace (PFunArgs);

static PFunDesc func_table[] =
{
  { "DEBUGGING-ON",	0, 0, pf_debugging_on },
  { "PAGE-DEBUG",	0, 0, pf_page_debug },
  { "DEBUGGING-OUTPUT",	0, 0, pf_debugging_output },
  { "SYSTEM-ERROR-OUTPUT", 0, 0, pf_system_error_output },
  { "%%STACK-TRACE",	0, 0, pf_stack_trace },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_debugger_functions)
DEFINE_SECTION (DEBUGGING-COMMANDS, debug; variables; trouble-shooting,
"Debugging a CGI application executing under a running Web server can be\n\
quite problematic.  <Meta-HTML> provides the following functions, macros,\n\
and variables in an effort to alleviate the problems associated with this\n\
situation.",
"In addition to the functions and variables above, <Meta-HTML> has a complete\n\
source language debugger called <code>mdb</code>, which can be used to\n\
interactively execute expressions, to place breakpoints in files and then\n\
run them examining local and global variables, and to single-step through\n\
source code that you have written in <Meta-HTML>.  Instructions on the\n\
use of the debugger is beyong the scope of <i>this</i> manual -- please see\n\
<b>MDB: A User's Guide to Debugging <Meta-HTML> Programs</b> for more\n\
information specific to the debugger.")

DEFUN (pf_debugging_on, &optional function-name=level...,
"Turns on debugging for the <var function-name>s mentioned, setting\n\
the level of output to <var level>.  <var level> is a number between\n\
<code>0</code> (the least amount of debugging info) and\n\
<code>10</code> (the maximum amount of debugging info).\n\
\n\
The output is placed into the <Meta-HTML> internal debugger buffer, and\n\
can be placed into an output page by simply placing the tag\n\
<tag DEBUGGING-OUTPUT> somewhere in the page, or can be explicity\n\
retrieved using <example code><debugging-output retrieve></example>.")
{
  if (vars)
    {
      register int i;
      Symbol **symbols = symbols_of_package (vars);
      Symbol *sym;

      for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	{
	  mhtml_set_debugging_on (sym);
	}
    }
}

DEFUN (pf_page_debug, &rest body,
"Cause <var body> to be inserted into the debugger output.\n\
\n\
The output is placed into the <Meta-HTML> internal debugger buffer, and\n\
can be placed into an output page by simply placing the tag\n\
<tag DEBUGGING-OUTPUT> somewhere in the page, or can be explicity\n\
retrieved using <example code><debugging-output retrieve></example>.")
{
  char *value;

  value = mhtml_evaluate_string (body->buffer ? body->buffer : "");
  if (!empty_string_p (value))
    page_debug ("%s", value);
  if (value) free (value);
}

DEFUN (pf_debugging_output, &optional action...,
"<Meta-HTML> stores debugging information in an internal buffer.  You\n\
may directly place information into this buffer using the <funref\n\
language-operators page-debug> command, and you may retrieve or clear\n\
this buffer using <example code><debugging-output></example>.\n\
\n\
Possible values for <var action> are:\n\
<ul>\n\
<li> <b>retrieve</b><br>\n\
Inserts the current contents of the debugging buffer into the page at\n\
the current location.\n\
<li> <b>clear</b><br>\n\
Empties the debugging buffer of all stored information.\n\
</ul>\n\
\n\
If you place  <example code><debugging-output></example> into your\n\
page without passing any arguments, <Meta-HTML> treats this invocation\n\
specially; it marks the location at which any debugging statements\n\
which have been collected during the processing of the entire page\n\
should be placed.\n\
\n\
We recommend that you always place this tag somewhere in the output\n\
page, whenever that output is an HTML document, as opposed to a\n\
standalone script.")
{
  register int i = 0;
  char *text;
  static char *token_name = "<DEBUGGING-OUTPUT>";

  while ((text = get_positional_arg (vars, i)) != (char *)NULL)
    {
      char *arg = mhtml_evaluate_string (text);

      if (!empty_string_p (arg))
	{
	  if (strcasecmp (arg, "clear") == 0)
	    {
	      page_debug_clear ();
	    }
	  else if (strcasecmp (arg, "retrieve") == 0)
	    {
	      char *contents = page_debug_buffer ();

	      if (contents != (char *)NULL)
		{
		  bprintf_insert (page, start, "%s", contents);
		  *newstart = start + strlen (contents) - 1;
		}
	    }
	}
      xfree (arg);
      i++;
    }

  if (i == 0)
    {
      bprintf_insert (page, start, token_name);
      *newstart = start + strlen (token_name) - 1;
    }
}

DEFUN (pf_system_error_output, &optional action...,
"<Meta-HTML> stores system error information in an internal buffer.\n\
Such information may be the results of an error which occurred during\n\
the execution of an external command (such as with <funref\n\
file-operators cgi-exec>), or other information which is generated by\n\
the inability of <Meta-HTML> to access a particular file (such as with\n\
the <funref file-operators require> command).\n\
\n\
You may retrieve or clear this buffer using <example\n\
code><system-error-output></example>.\n\
\n\
Possible values for <var action> are:\n\
<ul>\n\
<li> <b>retrieve</b><br>\n\
Inserts the current contents of the system error buffer into the page at\n\
the current location.\n\
<li> <b>clear</b><br>\n\
Empties the system error buffer of all stored information.\n\
</ul>\n\
\n\
If you place  <example code><system-error-output></example> into your\n\
page without passing any arguments, <Meta-HTML> treats this invocation\n\
specially; it marks the location at which any system errors\n\
which have been collected during the processing of the entire page\n\
should be placed.\n\
\n\
We recommend that you always place this tag somewhere in the output\n\
page, whenever that output is an HTML document, as opposed to a\n\
standalone script.")
{
  register int i = 0;
  char *text;
  static char *token_name = "<SYSTEM-ERROR-OUTPUT>";

  while ((text = get_positional_arg (vars, i)) != (char *)NULL)
    {
      char *arg = mhtml_evaluate_string (text);

      if (!empty_string_p (arg))
	{
	  if (strcasecmp (arg, "clear") == 0)
	    {
	      page_syserr_clear ();
	    }
	  else if (strcasecmp (arg, "retrieve") == 0)
	    {
	      char *contents = page_syserr_buffer ();

	      if (contents != (char *)NULL)
		{
		  bprintf_insert (page, start, "%s", contents);
		  *newstart = start + strlen (contents) - 1;
		}
	    }
	}
      xfree (arg);
      i++;
    }

  if (i == 0)
    {
      bprintf_insert (page, start, token_name);
      *newstart = start + strlen (token_name) - 1;
    }
}

DEFUNX (pf_%%stack_trace, ,
"Return an array of function calls that are pending the return of this one.\n\
This functionality can only be turned on by setting the variable\n\
mhtml::remember-function-calls to a non-empty value.")

extern char **mdb_pushed_function_names (void);

static void
pf_stack_trace (PFunArgs)
{
  char **calls = mdb_pushed_function_names ();

  if (calls != (char **)NULL)
    {
      register int i;

      for (i = 0; calls[i] != (char *)NULL; i++)
	{
	  bprintf_insert (page, start, "%s\n", calls[i]);
	  start += strlen (calls[i]) + 1;
	}
      *newstart = start;
    }
}

#if defined (__cplusplus)
}
#endif
