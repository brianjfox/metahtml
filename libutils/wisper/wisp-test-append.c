/*  wisp-test-append.c: Test the append function. */

/* Author: Brian J. Fox (bfox@ua.com) Sun Jun  4 09:10:11 1995.  */

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
#include <string.h>

#include "wisp.h"

#if defined (__cplusplus)
extern "C"
{
#endif
/* Test a few Lisp-like replacement functions. */
int
main (int argc, char *argv[])
{
  WispObject *list, *object, *result, *tem;

  if (argc < 3)
    {
      fprintf (stderr, "Supply two arguments which are the string\n");
      fprintf (stderr, "representations of wisp objects to append.\n");
      fprintf (stderr, "\n  e.g.  %s \"(foo)\" bar\n", argv[0]);
      exit (2);
    }

  list = wisp_from_string (argv[1]);
  tem = copy_object (list);
  object = wisp_from_string (argv[2]);
  result = wisp_append (list, object);

  /* The following cruft is necessary because of the way printf () works.
     It first collects all of the arguments, and remembers them, and then
     prints them out.  If I don't stash away new copies of the strings
     that are being printed, it just prints the contents of the pointer,
     which simply points to STRING_BUFFER, which contains the last value
     converted. */
  {
    char *rep_tem, *rep_obj, *rep_res;

    rep_tem = strdup (string_from_wisp (tem));
    rep_obj = strdup (string_from_wisp (object));
    rep_res = strdup (string_from_wisp (result));
		      
    printf ("(append '%s '%s) --> %s\n", rep_tem, rep_obj, rep_res);
  }

  exit (0);
}

#if defined (__cplusplus)
}
#endif
