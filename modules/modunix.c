/* modunix.c: -*- C -*-  Some Unixisms directly exported to Meta-HTML. */

/*  Copyright (c) 2001 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sun Feb 25 20:24:43 2001.

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

static void pf_unix_fork (PFunArgs);
static void pf_unix_kill (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "UNIX::FORK",	0,       0,             pf_unix_fork },
  { "UNIX::KILL",	0,       0,             pf_unix_kill },
  { (char *)NULL,       0,       0,             (PFunHandler *)NULL }
};

MODULE_INITIALIZE ("modunix", ftab)

DEFINE_SECTION (UNIX-MODULE, fork;exec;kill;process,
"The functions in the Unix module implement (in a very direct way) some
low-level C-library function calls which are only readily available in 
a Unix environment.", "")

DEFUNX (UNIX::FORK, ,
"Fork a new process and return the pid of the child to the parent, and
a zero to the just forked child.")

static void
pf_unix_fork (PFunArgs)
{
  pid_t child = fork ();

  bprintf (page, "%ld", (long)child);
}

DEFUNX (UNIX::KILL, pid &optional signal, "Kill <var pid> with <var signal>")

static void
pf_unix_kill (PFunArgs)
{
}

#if defined (__cplusplus)
}
#endif
