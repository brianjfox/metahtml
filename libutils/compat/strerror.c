/* strerror.c: Replacement for systems that don't have it in libc.a. */

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

#include <stdio.h>

#if !defined (HAVE_STRERROR)
extern int sys_nerr;
extern char *sys_errlist[];

char *
strerror (int error_num)
{
  char *result;
  static char num_buff[50];

  if (error_num > sys_nerr)
    {
      sprintf (num_buff, "Error %d", error_num);
      result = num_buff;
    }
  else
    result = sys_errlist[error_num];

  return (result);
}
#endif /* !HAVE_STRERROR */

  
