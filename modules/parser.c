/* parser.c: -*- C -*-  Modify parser parameters. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Fri May  2 13:03:37 1997.

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
static void pf_change_brackets (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag	     complex? debug_level	   code    */
  { "PARSER::CHANGE-BRACKETS", 	0,	 0,	pf_change_brackets },
  { (char *)NULL,		0,	 0,	(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("parser", ftab)

/* 4) Write the actual code which implements your functionality. */

/* <parser::change-brackets "{}"> --> Allow new opener and closer. */
DOC_SECTION (LANGUAGE-OPERATORS)
DEFUNX (pf_parser::change-brackets, bracket-pair,
"Add the matched open and close bracket characters to the special
characters that are understood by the Meta-HTML parser.  For example,
calling this function like this:
<example>
<parser::change-brackets \"{}\">
</example>
causes the character \"{\" to be in the same syntax class as \"<\", such
that an expression {get-var foo} would return the value of <var foo>.

This function is only available after loading the <b>parser</b> module:
<example>
<load-module parser> --> /www/lib/parser.so
</example>

For more information on loading dynamic modules in Meta-HTML, please see
the documentation for <funref dynamic-modules load-module>.")

static void
pf_change_brackets (PFunArgs)
{
  char *brackets = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (brackets) && (brackets[0]) && (brackets[1]))
    {
      LEFT_BRACKET = brackets[0];
      RIGHT_BRACKET = brackets[1];
    }
  xfree (brackets);
}

#if defined (__cplusplus)
}
#endif
