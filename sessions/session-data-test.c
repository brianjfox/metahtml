/* session-data-test.c: Program tests the session data stuff. */

/* Author: Brian J. Fox (bfox@ua.com) Fri Jul  7 11:55:58 1995.  */

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
#include <session_data.h>
#include "parseargs.h"

int
main (int argc, char *argv[])
{
  char *key;
  SESSION_INFO *info;
  session_id_t sid;
  Package *package;

  parse_session_arguments (&argc, argv);

  if (argc < 2)
    {
      fprintf (stderr, "You must supply a key value.\n");
      exit (2);
    }
  else
    key = argv[1];

  forms_input_data (argc, argv);
  package = symbol_get_package ("");

  sid = session_begin (key, 0);
  if (sid == (session_id_t)0)
    {
      fprintf (stderr, "There is already a session with the key %s.\n", key);
      exit (1);
    }

  info = session_get_info (sid);
  sd_package_to_info (info, package);

  fprintf (stderr, "For key `%s', sid `%s', the session data is: %s\n",
	   key, (char *)sid, info->data);

  session_put_info (info);
  info = session_get_info (sid);
  package = symbol_get_package ((char *)NULL);
  sd_info_to_package (info, package);

  if (package->table->entries == 0)
    {
      fprintf (stderr, "Couldn't retrieve the posted items!\n");
      exit (1);
    }
  else
    {
      register int i;
      Symbol **symbols = symbols_of_package (package);

      fprintf (stderr, "Result of reading back the items:\n");

      for (i = 0; symbols[i] != (Symbol *)NULL; i++)
	{
	  char **values = symbols[i]->values;

	  fprintf (stderr, "%s: ", symbols[i]->name);

	  if (values != (char **)NULL)
	    {
	      register int j;

	      for (j = 0; values[j] != (char *)NULL; j++)
		fprintf (stderr, "%s%s", values[j],
			 values[j + 1] ? ", " : ".");
	    }

	  fprintf (stderr, "\n");
	}
    }
  return (0);
}

  
    
