/* example.c: -*- C -*-  An example of writing a Meta-HTML module extension. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Tue Dec 24 11:58:24 1996.

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
static void pf_apropos (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "APROPOS",          0,       0,             pf_apropos },
  { (char *)NULL,       0,       0,             (PFunHandler *)NULL }
};

/* 3)  Insert the following statement, with the name of your module file
   as a string.  This allows Meta-HTML to install the functions in your
   module when it is loaded. */
MODULE_INITIALIZE ("example", ftab)

/* 4) Write a blurb about what this module does.  The text here will make
   it directly into the documentation, as a section of its own, and the
   functions that you declare with DEFUN, DEFMACRO, etc., will be documented
   in that section. */
DEFINE_SECTION (EXAMPLE-MODULE, example-keywords; more keywords,
"The functions in this module are simply here to demonstrate how to write\n\
new dynamically loadable modules for <Meta-HTML> for your own application\n\
needs, and don't really perform any services that couldn't be better \n\
performed in other ways.  However, the <tag apropos> function in this\n\
module might be of use in an interactive Web page -- see\n\
<secref html-helpers> for more information, as well as reading\n\
the source code in <code>modules/example.c</code>.", "")

/* 5) Write the actual code which implements your functionality. */

/* <apropos regexp> --> array of symbol names containing regexp. */

DEFUN (pf_apropos, regexp,
"Search through all <Meta-HTML> symbols for <var regexp>, and return a\n\
newline separated list of those symbols which match.\n\
\n\
Generally meant for interactive use.\n\
\n\
Note that this is simply a long documentation comment in the source, to\n\
demonstrate how to document new commands that you might write in a\n\
<Meta-HTML> dynamically loadable module.")
{
  char *regexp = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (regexp != (char *)NULL)
    {
      regex_t re;
      regmatch_t offsets[2];

      regcomp (&re, regexp, REG_EXTENDED | REG_ICASE);

      if (AllPackages)
	{
	  register int pi;
	  Package *pack;

	  for (pi = 0; (pack = AllPackages[pi]) != (Package *)NULL; pi++)
	    {
	      /* Operate on global packages only. */
	      if (pack->name != (char *)NULL)
		{
		  register int i;
		  Symbol **syms = symbols_of_package (pack);
		  int len = strlen (pack->name);

		  for (i = 0; (syms && syms[i]); i++)
		    {
		      char *name = syms[i]->name;

		      if (regexec (&re, name, 1, offsets, 0) == 0)
			{
			  bprintf_insert
			    (page, start, "%s::%s\n", pack->name, name);
			  start += 3 + len + strlen (name);
			}
		    }
		}
	    }
	}

      *newstart = start;
      regfree (&re);
      free (regexp);
    }
}

#if defined (__cplusplus)
}
#endif
