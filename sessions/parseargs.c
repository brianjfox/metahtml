/* parseargs.c: -*- C -*-  Parse arguments to a session command. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jun 29 10:21:59 1996.  */

/* This file is part of <Meta-HTML>(tm), a system for the rapid
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

	http://www.metahtml.com/COPYING
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if defined (__cplusplus)
extern "C"
{
#endif

extern void set_session_database_location (char *pathname);

#if defined (__cplusplus)
}
#endif

static void
remove_arg (int which, char *argv[])
{
  register int i;

  for (i = which; argv[i] != (char *)NULL; i++)
    argv[i] = argv[i + 1];
}

void
parse_session_arguments (int *argcp, char *argv[])
{
  int argc = *argcp;
  int arg_index = 1;

  /* Gobble up and act on arguments that we understand.  Move other arguments
     around so that the caller can still read the remaining ones. */

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if ((strcmp (arg, "-f") == 0) || (strcmp (arg, "--filename") == 0))
	{
	  --arg_index;
	  remove_arg (arg_index, argv); --argc;
	  set_session_database_location (argv[arg_index]);
	  remove_arg (arg_index, argv); --argc;
	}
    }

  *argcp = argc;
}
