/* lockname.c: -*- C -*-  Functions for file locking. */

/* Copyright (c) 1996 Brian J. Fox
   Author: Brian J. Fox (bfox@ai.mit.edu) Thu Jan 16 08:41:39 1997.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1995, 2001, Brian J. Fox (bfox@ai.mit.edu).

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

#include "mhtmlstd.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* Return a hash code for NAME. */
static int
hashname (char *name)
{
  register unsigned int i, j;

  for (i = 0, j = 0; name[i] != '\0'; i++)
    j = (j << 2) +  name[i];

  return ((j & ~(0xffffffff << 31)) % 919);
}

char *
db_lockname (char *name)
{
  static char lockname[1024];

  if (name == (char *)NULL)
    return ((char *)NULL);
  else
    {
      int hashcode = hashname (name);
      char *tail = strrchr (name, '/');

      if (tail == (char *)NULL)
	tail = name;
      else
	tail++;

      sprintf (lockname, "/tmp/%08X-%s.LOCK", hashcode, tail);
      return (lockname);
    }
}

#if !defined (HAVE_FLOCK)
int
LOCKFILE (int fd)
{
  struct flock f;
  f.l_type = F_WRLCK;
  f.l_whence = SEEK_SET;
  f.l_start = (long) 0;
  f.l_len = (long) 0;
  return (fcntl (fd, F_SETLKW, &f));
}

#if defined (Solaris) || defined (hpux) || defined (_SCO_DS)
int
READLOCKFILE (int fd)
{
  struct flock f;
  f.l_type = F_WRLCK;
  f.l_whence = SEEK_SET;
  f.l_start = (long) 0;
  f.l_len = (long) 0;
  return (fcntl (fd, F_SETLKW, &f));
}
#else
int
READLOCKFILE (int fd)
{
  struct flock f;
  f.l_type = F_RDLCK;
  f.l_whence = SEEK_SET;
  f.l_start = (long) 0;
  f.l_len = (long) 0;
  return (fcntl (fd, F_SETLKW, &f));
}
#endif /* !os's that can read lock. */

int
UNLOCKFILE (int fd)
{
  struct flock f;
  f.l_type = F_UNLCK;
  f.l_whence = SEEK_SET;
  f.l_start = (long) 0;
  f.l_len = (long) 0;
  return (fcntl (fd, F_SETLKW, &f));
}

#endif /* !HAVE_FLOCK */

#if defined (__cplusplus)
}
#endif
