/* strcasecmp.c: Replacement for systems that don't have it in libc.a. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jul 22 14:32:13 1995.  */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#if !defined (HAVE_STRCASECMP)
#include <sys/types.h>
#include <ctype.h>

int
strcasecmp (const char *s1, const char *s2)
{
  register int i = 0;
  int c1, c2;
  int result = -1;

  if (s1 == s2)
    return (0);

  while (1)
    {
      c1 = s1[i];
      c2 = s2[i];
      i++;

      if (c1 == 0 && c2 == 0)
	return (0);

      if (c1 == 0)
	return (-1);

      if (c2 == 0)
	return (1);

      if (isupper (c1)) c1 = tolower (c1);
      if (isupper (c2)) c2 = tolower (c2);

      if ((result = (c2 - c1)) != 0)
	break;
    }

  return (result);
}

int
strncasecmp (const char *s1, const char *s2, size_t len)
{
  register int i = 0;
  int c1, c2;
  int result = 0;

  /* Optimize simplest case. */
  if ((len == 0) || (s1 == s2))
    return (0);

  while (len)
    {
      len--;
      c1 = s1[i];
      c2 = s2[i];
      i++;

      if (c1 == 0 && c2 == 0)
	return (0);

      if (c1 == 0)
	return (-1);

      if (c2 == 0)
	return (1);

      if (isupper (c1)) c1 = tolower (c1);
      if (isupper (c2)) c2 = tolower (c2);

      if ((result = (c2 - c1)) != 0)
	break;
    }

  return (result);
}

#endif /* !HAVE_STRCASECMP */

