/* relfuncs.c: -*- C -*-  Relational operators for Meta-HTML. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Thu Jul 24 10:00:52 1997.  */

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

/************************************************************/
/*							    */
/*			Relational Operators		    */
/*							    */
/************************************************************/

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_not (PFunArgs);
static void pf_and (PFunArgs);
static void pf_or (PFunArgs);

static PFunDesc func_table[] =
{
  { "NOT",		0, 0, pf_not },
  { "AND",		0, 0, pf_and },
  { "OR",		0, 0, pf_or },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_relational_functions)
DEFINE_SECTION (RELATIONAL-OPERATORS, boolean; relational; logical, 
"<i>Relational operators</i> in <Meta-HTML> return information about the\n\
relationship between two items.  There are three logical operators,\n\
<funref RELATIONAL-OPERATORS and>, <funref RELATIONAL-OPERATORS\n\
or>,  and <funref RELATIONAL-OPERATORS not>.\n\
\n\
There are several other operators which compare numeric values; the\n\
section <secref ARITHMETIC-OPERATORS> cover those in detail.", "")

DEFUN (pf_not, &optional body,
"<var body> is evaluated.  If the result is the empty string, then the\n\
string \"true\" is returned, otherwise nothing is returned.  <var body>\n\
is simply the entire contents of the simple tag, with the word \"not\"\n\
in it.  Thus, in typical usage one might write:\n\
<example>\n\
<when <not <get-var foo>>>\n\
  Hey!  You didn't set the variable FOO.\n\
</when>\n\
</example>")
{
  int offset = 0;
  char *sexp = read_sexp (body->buffer, &offset, 0);
  char *test = mhtml_evaluate_string (sexp);

  if (empty_string_p (test))
    bprintf_insert (page, start, "true");

  if (test) free (test);
  if (sexp) free (sexp);
}

DEFUN (pf_and, &unevalled &rest expr...,
"<code>and</code> evaluates each <var expr> given until one of them\n\
evaluates to the empty string, or until they are all exhausted.  The\n\
result is the result of the last evaluation.  Evaluating just <example\n\
code><and></example> returns <code>\"true\"</code>.\n\
\n\
Examples:\n\
<example>\n\
<and>                    --> true\n\
<and this that>          --> that\n\
<unset-var foo>\n\
<and <get-var foo> this> -->\n\
<and this <get-var foo>> -->\n\
<set-var foo=bar>\n\
<and this long list <get-var foo>> --> bar\n\
</example>\n\
\n\
<tag and> could have been defined in <Meta-HTML> as follows:\n\
<example>\n\
<define-tag and &unevalled &rest expressions[] whitespace=delete>\n\
  <set-var result=true>\n\
  <foreach :expr expressions>\n\
    <set-var result = <get-var-eval :expr>>\n\
    <if <not <get-var-once result>>\n\
      <break>>\n\
  </foreach>\n\
  <get-var-once result>\n\
</define-tag>\n\
</example>")
{
  register int i = 0;
  char *result = strdup ("true");
  char *temp;

  while ((temp = get_positional_arg (vars, i++)) != (char *)NULL)
    {
      char *value = mhtml_evaluate_string (temp);

      if (!empty_string_p (value))
	{
	  free (result);
	  result = value;
	}
      else
	{
	  if (value) free (value);
	  free (result);
	  result = (char *)NULL;
	  break;
	}
    }

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      free (result);
    }
}

DEFUN (pf_or, &unevalled &rest expr...,
"<code>or</code> evaluates each <var expr> given until one of them\n\
evaluates to a non-empty string, or until they are all exhausted.  The\n\
result is the result of the last evaluation.  Evaluating just <example\n\
code><or></example> returns the empty string.\n\
\n\
Examples:\n\
<example>\n\
<or>                    --> \n\
<or this that>          --> this\n\
<unset-var foo>\n\
<or <get-var foo> this> --> this\n\
<or this <get-var foo>> --> this\n\
<set-var foo=bar>\n\
<or <get-var foo> this> --> bar\n\
</example>\n\
\n\
<tag or> could have been defined in <Meta-HTML> as follows:\n\
<example>\n\
<define-tag or &unevalled &rest expressions[] whitespace=delete>\n\
  <foreach :expr expressions>\n\
    <set-var result = <get-var-eval :expr>>\n\
    <if <get-var-once result>\n\
        <break>>\n\
  </foreach>\n\
  <get-var-once result>\n\
</define-tag>\n\
</example>")
{
  register int i = 0;
  char *result = (char *)NULL;
  char *temp;

  while ((temp = get_positional_arg (vars, i++)) != (char *)NULL)
    {
      char *value = mhtml_evaluate_string (temp);
      if (!empty_string_p (value))
	{
	  result = value;
	  break;
	}
      else if (value) free (value);
    }

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      free (result);
    }
}

#if defined (__cplusplus)
}
#endif
