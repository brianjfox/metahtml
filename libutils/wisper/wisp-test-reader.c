/*  wisp-test.c: Test the reader and printer of the Wisp Object library. */

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

/*******************************************************************/
/*								   */
/*		  Testing the Reader and Printer		   */
/*								   */
/*******************************************************************/

#if defined (__cplusplus)
extern "C"
{
#endif

int
main (int argc, char *argv[])
{
  register int i = 1;

  while (i < argc)
    {
      WispObject *wisp_representation;
      char *string_representation;

      wisp_representation = wisp_from_string (argv[i]);
      string_representation = string_from_wisp (wisp_representation);

      printf ("argv[%d] = `%s' (length is %d)\n",
	      i, string_representation, wisp_length (wisp_representation));
      if (wisp_representation == NIL)
	printf ("  (Value was NIL)\n");
      
      i++;
    }
  return (0);
}

#if defined (__cplusplus)
}
#endif
