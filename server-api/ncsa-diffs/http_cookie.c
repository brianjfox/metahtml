/* http_cookie.c: The minimal support required for handling HTTP Cookies. */

/* Author: Brian J. Fox (bfox@datawave.net) Wed Sep 20 17:07:43 1  */

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

extern void *malloc (int size);
extern void *realloc (void *pointer, int size);

#define whitespace(x) ((x == ' ') || (x == '\t') || (x == '\n'))

/* A NULL terminated array of COOKIE=VALUE strings. */
char **cookies = (char **)0;
static int cookies_size = 0;
static int cookies_index = 0;

/* How to remember a cookie passed in by the client. */
void
remember_cookie (char *cookie_string)
{
  register int i, start;

  for (start = 0; whitespace (cookie_string[start]); start++);
  for (i = start; cookie_string[i] && !whitespace (cookie_string[i]); i++);

  if (i != start)
    {
      char *cookie;

      cookie = (char *)malloc (1 + (i - start));
      strncpy (cookie, cookie_string + start, i - start);
      cookie[i - start] = '\0';

      if (cookies_index + 2 > cookies_size)
	{
	  if (!cookies)
	    cookies = (char **)malloc ((cookies_size = 10) * sizeof (char *));
	  else
	    cookies = (char **)realloc
	      (cookies, ((cookies_size += 10) * sizeof (char *)));

	  cookies[cookies_index++] = cookie;
	  cookies[cookies_index] = (char *)0;
	}
    }
}
