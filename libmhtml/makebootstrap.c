/* makebootstrap.c: -*- C -*-  Standalone program creates a C file which
   when called installs packages and functions which were defined in the
   input files to this program. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jan 29 10:20:38 1997.  */

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

#include "mhtmlstd.h"

static char *prefix = "bootstrap";

static void
usage (void)
{
  fprintf (stderr, "Usage: makebootstrap [bootstrap libraries ...]\n");
  exit (2);
}

static int bootstrap_code_len = 0;

static void
open_library (void)
{
  static int library_open = 0;

  if (!library_open)
    {
      fprintf (stdout, "char %s_code[] = {\n", prefix);
      library_open++;
    }
}

static void
close_library (void)
{
  fprintf (stdout,
	   " 0 };\nint %s_code_len = %d;\n", prefix, bootstrap_code_len);
}

static void
dump_library (char *filename)
{
  int fd = os_open (filename, O_RDONLY, 0666);

  open_library ();	/* Open the output lib if not already opened. */

  if (fd > -1)
    {
      struct stat finfo;

      if (stat (filename, &finfo) != -1)
	{
	  char *buffer = (char *)malloc (1 + (int)finfo.st_size);
	  register int i;
	  int chars_this_line = 0;

	  read (fd, buffer, (size_t)finfo.st_size);

	  for (i = 0; i < finfo.st_size; i++)
	    {
	      /* Start a new line if that is the right thing. */

	      if (!chars_this_line)
		fprintf (stdout, "\n  ");

	      fprintf (stdout, "'\\%03o', ", (unsigned char)buffer[i]);

	      chars_this_line++;
	      if (chars_this_line == 9)
		{
		  fprintf (stdout, "\n");
		  chars_this_line = 0;
		}
	    }

	  free (buffer);
	  bootstrap_code_len += i;
	}
    }
}

int
main (int argc, char *argv[])
{
  int arg_index = 1;

  if (argc < 2)
    usage ();

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if (strcasecmp (arg, "--prefix") == 0)
	prefix = argv[arg_index++];
      else
	dump_library (arg);
    }

  close_library ();
  return (0);
}
