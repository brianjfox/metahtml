/* dbdelete.c: Delete an entry from a GDBM database. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Mar 29 09:32:51 1996.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "database.h"

static char *progname;

static void
usage (void)
{
  fprintf (stderr, "Usage: %s dbfile.db key-string\n", progname);
  exit (1);
}

int
main (int argc, char *argv[])
{
  DBFILE db;
  char *temp;

  progname = argv[0];
  if ((temp = strrchr (progname, '/')) != (char *)NULL)
    progname = temp + 1;

  if (argc < 2)
    usage ();

  db = database_open (argv[1], DB_WRITER);
  if (!db)
    {
      fprintf (stderr, "%s: Couldn't open `%s' to delete a record\n",
	       progname, argv[1]);
    }
  else
    {
      DBOBJ *key = database_setkey (argv[2]);
      database_delete (db, key);
    }

  return (0);
}
