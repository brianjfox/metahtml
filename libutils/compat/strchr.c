/* strchr.c: Replacement for systems that don't have it in libc.a. */

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

#if !defined (HAVE_STRCHR)
#include <sys/types.h>
#include <ctype.h>

#if !defined (NULL)
#  define NULL 0x0
#endif

char *
strchr (const char *string, int c)
{
  char *result = string;

  while ((*result != '\0') && (*result != c))
    result++;

  if (*result)
    return (result);
  else
    return ((char *)NULL);
}

char *
strrchr (const char *string, int c)
{
  char *result = (string + strlen (string)) - 1;

  while ((result >= string) && (*result != c))
    result--;

  if (result >= string)
    return (result);
  else
    return ((char *)NULL);
}

#endif /* !HAVE_STRCHR */
