/* get-session-var.c: Get the value of a session variable. */

/* Author: Brian J. Fox (bfox@datawave.net) Fri Jul  7 14:29:31 1995.  */

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
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include <forms.h>
#include <session_data.h>
#include "parseargs.h"

int
main (int argc, char *argv[])
{
  session_id_t sid;
  SESSION_INFO *info;

  parse_session_arguments (&argc, argv);

  if (argc < 2)
    {
      fprintf (stderr, "Usage: get-session-var SID NAME ...\n");
      exit (1);
    }

  sid = (session_id_t)argv[1];
  info = session_get_info (sid);

  if (info)
    {
      int arg_index = 2;
      Package *package = symbol_get_package ("");

      sd_info_to_package (info, package);

      while (arg_index < argc)
	{
	  char *name = argv[arg_index++];
	  char *value;

	  if (!name)
	    continue;

	  value = forms_get_tag_value_in_package (package, name);

	  if (value) printf ("%s ", value);
	}
    }
  else
    fprintf (stderr, "No such session id (%s)\n", (char *)sid);

  return (0);
}
