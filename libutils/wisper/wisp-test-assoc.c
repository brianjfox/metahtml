/*  wisp-test-assoc.c: Test the assoc and sassoc functions. */

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
/* Test the association functions. */
int
main (int argc, char *argv[])
{
  char *key, *string_result, *temp;
  WispObject *list, *result;

  key = argv[1];
  list = wisp_from_string (argv[2]);
  result = assoc (key, list);
  temp = strdup (result ? string_from_wisp (result) : "");

  printf ("(assoc %s '%s) --> `%s'\n", key, string_from_wisp (list), temp);

  string_result = sassoc (key, list);
  
  printf ("(sassoc %s '%s) --> `%s'\n", key, string_from_wisp (list),
	  string_result ? string_result : "");
  return (0);
}

#if defined (__cplusplus)
}
#endif
