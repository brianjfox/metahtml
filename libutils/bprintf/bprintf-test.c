/*  bprintf-test.c: Some simple code to test the bprintf library. */

/* Author: Brian J. Fox (bfox@ua.com) Thu Apr 20 19:38:48 1995.  */

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
#include <sys/errno.h>
#include <stdarg.h>
#include "bprintf.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* Thank you, SunOS 4.1.3. */
extern int errno;

int
main (int argc, char *argv[])
{
  BPRINTF_BUFFER *buffer;
  int point;

  buffer = bprintf_create_buffer ();

  bprintf (buffer, "Testing a static string without printf args.\n");
  bprintf (buffer, "Testing %%%% and %%c with character of `%c'.\n", 'x');
  bprintf (buffer, "Testing %%x with 32767: result (%x), that is: (%X)\n",
	   32767, 32767);
  bprintf (buffer, "Testing scientific notation with 123456789.0: %g\n",
	   1234567890.0);
  bprintf (buffer, "Testing strings with width specifiers: `%24s'\n",
	   "24 spaces wide");
  
  bprintf (buffer, "Testing formatted decimal notation: $%6.2f\n", 34.55);
  fprintf (stdout, "%s", buffer->buffer);

  errno = 3;
  bprintf (buffer, "Testing an error message string: Error: `%m', (%d)\n",
	   errno);
  fprintf (stdout, "%s", buffer->buffer);

  point = buffer->bindex;
  bprintf (buffer, "Testing insertion at the start of this line.\n");
  bprintf_insert (buffer, point, "%s is the start: ", "HERE");
  fprintf (stdout, "%s", buffer->buffer);

  bprintf (buffer, "Deleting the word \"HERE\" from the start of that line\n");
  bprintf_delete_range (buffer, point, point + strlen ("HERE"));
  fprintf (stdout, "%s", buffer->buffer);

  fprintf (stdout, "Testing word wrap function: width=60\n");
  bprintf_word_wrap (buffer, 60);
  fprintf (stdout, "%s", buffer->buffer);

  bprintf_free_buffer (buffer);
  return (0);
}

#if defined (__cplusplus)
}
#endif
