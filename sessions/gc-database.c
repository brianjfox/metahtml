/* gc-database.c: -*- C -*-  Quickly reorganize the session database. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Nov 17 22:56:31 1995.  */

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
#include <time.h>
#include "session.h"
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include <gdbm.h>

int
main (int argc, char *argv[])
{
  GDBM_FILE db = (GDBM_FILE)NULL;
  char *database_name = SESSION_DATABASE;
  int result;

  if (argc > 1)
    database_name = argv[2];

  if (database_name != (char *)NULL)
    db = gdbm_open (database_name, 0, GDBM_WRITER, 0644, NULL);

  if (db != (GDBM_FILE)NULL)
    {
      printf ("Starting reorganization of %s...", database_name);
      fflush (stdout);
      gdbm_reorganize (db);
      printf ("done!\n");
      fflush (stdout);
      gdbm_close (db);
      result = 0;
    }
  else
    {
      printf ("Usage: gc-database -f DATABASE-FILENAME\n");
      printf ("Cannot reorganize %s; maybe try again as root?\n",
	      database_name ? database_name : SESSION_DATABASE);
      result = 1;
    }
  return (result);
}
