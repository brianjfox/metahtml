/* modecho.c: -*- C -*-  <echo::echo arg arg arg> echos arguments. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Thu Aug  5 18:39:23 1999.

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
static void pf_echo (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "ECHO::ECHO",       0,       0,             pf_echo },
  { (char *)NULL,       0,       0,             (PFunHandler *)NULL }
};

/* 3)  Insert the following statement, with the name of your module file
   as a string.  This allows Meta-HTML to install the functions in your
   module when it is loaded. */
MODULE_INITIALIZE ("echo", ftab)

/* 4) Write a blurb about what this module does.  The text here will make
   it directly into the documentation, as a section of its own, and the
   functions that you declare with DEFUN, DEFMACRO, etc., will be documented
   in that section. */
DEFINE_SECTION (ECHO-MODULE, testing; displaying arguments,
"ECHO::ECHO simply echoes the arguments it was passed.  This is solely for
the purposes of debugging Meta-HTML internals, but can be relatively useful
when building new language constructs.", "")

/* 5) Write the actual code which implements your functionality. */

/* <echo arg arg arg> --> the args, one per line, each surrounded by `' */

DEFUN (pf_echo, &rest args,
"Echo the passed in arguments, one per line, and each one surrounded by 
single quotes.")
{
  register int i;
  char *arg;

  for (i = 0; (arg = get_positional_arg (vars, i)) != (char *)NULL; i++)
    {
      bprintf_insert (page, start, "`%s'\n", arg);
      *newstart += strlen (arg) + 3;
      start = *newstart;
    }
}

#if defined (__cplusplus)
}
#endif
