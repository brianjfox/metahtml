/* create-session.c: -*- C -*-  DESCRIPTIVE TEXT. */

/* Author: Brian J. Fox (bfox@ua.com) Sat Jul  1 16:34:18 1995.  */

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

#include "session.h"
#include "parseargs.h"

int
main (int argc, char *argv[])
{
  /* Create a session with the arguments given.  Print the SID
     of the created session. */
  session_id_t sid;

  parse_session_arguments (&argc, argv);

  if (argc < 2)
    {
      fprintf (stderr, "You must supply a key to begin the session!\n");
      exit (1);
    }

  sid = session_begin (argv[1], 0);

  if (sid != (session_id_t) 0)
    printf ("Sid = %s\n", (char *)sid);
  else
    printf ("The key \"%s\" is already in use.\n", argv[1]);

  return (0);
}
