/* binary-session.c: Test the sessions database with binary data. */

/* Author: Brian J. Fox (bfox@ua.com) Sat Jul  1 17:10:11 1995.  */

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
#include <xmalloc/xmalloc.h>

#include "session.h"
#include "parseargs.h"

typedef struct
{
  char tag[20];
  int number;
  double a_double;
} DATA;

int
main (int argc, char *argv[])
{
  session_id_t sid;
  SESSION_INFO *info;
  DATA data, data1;

  parse_session_arguments (&argc, argv);
  sid = session_begin ("test-session", 1);
  info = session_get_info (sid);
  strcpy (data.tag, "Tag Value");
  data.number = 135;
  data.a_double = 3.1415;

  info->length = sizeof (DATA);
  info->data = (unsigned char *)&data;

  session_put_info (info);

  info = session_get_info (sid);
  memcpy ((unsigned char *)&data1, info->data, info->length);

  printf ("Data values from db: tag %s, number %d, a_double %f.\n",
	  data1.tag, data1.number, data1.a_double);
  return (0);
}
