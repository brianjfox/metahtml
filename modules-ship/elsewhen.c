/* elsewhen.c: -*- C -*-  Provide a <when> ... <elsewhen> </when> tag. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jun 18 18:44:04 1997.

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

static void pf_whenelse (PFunArgs);
static void pf_complex_if (PFunArgs);

/* #define COMPLEX_ELSE_IF */

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag	     complex? debug_level	   code    */
  { "WHENELSE",		1,	 0,		pf_whenelse },
#if defined (COMPLEX_ELSE_IF)
  { "IF",		1,	 0,		pf_complex_if },
#else
  { "CIF",		1,	 0,		pf_complex_if },
#endif
  { (char *)NULL,	0,	 0,		(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("elsewhen", ftab)

DEFINE_SECTION (ELSEWHEN-MODULE, ,
"This module is provided for those programmers who feel more comfortable with
a balanced tag syntax for flow control over a balanced brace syntax.
Using the tags in this package will greatly reduce the number of
<funref flow-control group>, or <funref flow-control concat> tags, at the
slight cost of interpreter speed.  In many cases, the clarity of the code
is more important, so if you are one of those programmers that feel more
comfortable with the tag-based syntax, you might consider the use of this
module.", "")

DEFMACRO (pf_whenelse, test,
"Evaluate <var test>.  If the result is a non-empty string,
then execute the <var body> statements.  If the tag &lt;elsewhen&gt;
appears in the body, then the commands between that tag and the closing
&lt;/whenelse&gt; will be executed if, and only if, the <var test> evaluates
to the empty string.  This is a cleaner way to handle optional multiple
statement execution rather than dealing with quoting everything inside
of an <funref FLOW-CONTROL if> form.")
{
  char *test = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *else_text = (char *)NULL;
  int e_start = 0, else_start = 0;

  /* Find *all* occurences of <elsewhen> until there are none left, or
     until we found one that belongs to us. */
  while (1)
    {
      int e_end, ours_p;

      /* Is it within our <when> body? */
      ours_p = page_indicator_owned_by
	(body, "elsewhen", "whenelse", &e_start, &e_end, else_start);

      if (ours_p == 1)
	{
	  body->buffer[e_start] = '\0';
	  while ((e_end < body->bindex) && (whitespace (body->buffer[e_end])))
	    e_end++;
	  else_text = body->buffer + e_end;
	  break;
	}
      else if (ours_p == 0)
	{
	  /* Not found at all. Quit looking. */
	  break;
	}
      else
	{
	  /* Not within our body.  Try finding the next one. */
	  else_start = e_end;
	  e_start = 0;
	}
    }

  if (!empty_string_p (test))
    bprintf_insert (page, start, "%s", body->buffer);
  else if (else_text)
    bprintf_insert (page, start, "%s", else_text);

  xfree (test);
}

DEFMACROX (pf_cif, test &optional then-clause <else> else-clause,
"First <var test> is evaluated. If the result is a non-whitespace only string,
the statements between the <var then-clause> and the <var else-clause> are
evaluated.  Otherwise, if the the <var else-clause> is present, it is
evaluated. Although Meta-HTML has an <code>or</code> function, you can
efficiently test for the presence of any of a group of variables with code
similar to the following:
<example>
<if <get-var foo bar>>
  \"Either FOO or BAR is present\"
<else>
  \"Neither FOO nor BAR is present\"
</if>
</example>")

/* A non-lisp friendly <if> <else> </if> tag. */
static void
pf_complex_if (PFunArgs)
{
  char *test = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *else_text = (char *)NULL;
  int e_start = 0, else_start = 0;

  /* Find *all* occurences of <else> until there are none left, or
     until we found one that belongs to us. */
  while (1)
    {
      int e_end, ours_p;

      /* Is it within our <when> body? */
      ours_p = page_indicator_owned_by
	(body, "else", "cif", &e_start, &e_end, else_start);

      if (ours_p == 1)
	{
	  body->buffer[e_start] = '\0';
	  while ((e_end < body->bindex) && (whitespace (body->buffer[e_end])))
	    e_end++;
	  else_text = body->buffer + e_end;
	  break;
	}
      else if (ours_p == 0)
	{
	  /* Not found at all. Quit looking. */
	  break;
	}
      else
	{
	  /* Not within our body.  Try finding the next one. */
	  else_start = e_end;
	  e_start = 0;
	}
    }

  if (!empty_string_p (test))
    bprintf_insert (page, start, "%s", body->buffer);
  else if (else_text)
    bprintf_insert (page, start, "%s", else_text);

  xfree (test);
}

#if defined (__cplusplus)
}
#endif
