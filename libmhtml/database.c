/* database.c: Code used to hide the underlying database implementation. */

/* Author: Brian J. Fox (bfox@ua.com) Sat Jul  1 14:56:57 1995.  */

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

/* This version is based on using a database file in GDBM format. */

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <xmalloc/xmalloc.h>

#include "database.h"

#include <gdbm.h>

#if !defined (errno)
extern int errno;
#endif

int
database_had_error_p (void)
{
  return (gdbm_errno != GDBM_NO_ERROR);
}

void
database_reset_error (void)
{
  gdbm_errno = GDBM_NO_ERROR;
  errno = 0;
}

char *
database_strerror (void)
{
  if (gdbm_errno != GDBM_NO_ERROR)
    return ((char *)gdbm_strerror (gdbm_errno));
  else
    return ((char *)strerror (errno));
}

/* Open the database named NAME for access as determined by FLAGS, and
   return a handle to it.  A handle of 0 indicates failure. */
DBFILE
database_open (char *name, int flags)
{
  GDBM_FILE db;
  int gdbm_flags = 0;

  if (flags & DB_WRCREAT) gdbm_flags |= GDBM_WRCREAT | GDBM_FAST; 
  if (flags & DB_READER)  gdbm_flags |= GDBM_READER;
  if (flags & DB_WRITER)  gdbm_flags |= GDBM_WRITER | GDBM_FAST;

  db = gdbm_open (name, 0, gdbm_flags, 0644, NULL);

  return ((DBFILE)db);
}

/* Close the currently open database referenced with the DBFILE handle DB. */
void
database_close (DBFILE db)
{
  if (db != (DBFILE)0)
    gdbm_close ((GDBM_FILE)db);
}

/* Return the object which represents the first key in the currently
   open database referenced with the DBFILE handle DB. */
DBOBJ *
database_firstkey (DBFILE db)
{
  DBOBJ *key = (DBOBJ *)NULL;

  if (db != (DBFILE)0)
    {
      datum gkey;

      gkey = gdbm_firstkey ((GDBM_FILE)db);

      if (gkey.dptr)
	{
	  key = (DBOBJ *)xmalloc (sizeof (DBOBJ));
	  key->data = (unsigned char *)gkey.dptr;
	  key->length = gkey.dsize;
	}
    }

  return (key);
}

/* Return the object which represents the "next" key in the currently
   open database referenced with the DBFILE handle DB.  The "next" key
   is the one after the DBOBJ KEY. */
DBOBJ *
database_nextkey (DBFILE db, DBOBJ *key)
{
  DBOBJ *result = (DBOBJ *)NULL;

  if (db != (DBFILE)0)
    {
      datum currkey, nextkey;

      currkey.dptr = (char *)key->data;
      currkey.dsize = key->length;

      nextkey = gdbm_nextkey ((GDBM_FILE)db, currkey);

      if (nextkey.dptr)
	{
	  result = (DBOBJ *)xmalloc (sizeof (DBOBJ));
	  result->data = (unsigned char *)nextkey.dptr;
	  result->length = nextkey.dsize;
	}
    }
  return (result);
}

/* Return the database object in DB associated with KEY. */
DBOBJ *
database_fetch (DBFILE db, DBOBJ *key)
{
  DBOBJ *result = (DBOBJ *)NULL;

  if (db != (DBFILE)0)
    {
      datum gkey, content;

      gkey.dptr = (char *)key->data;
      gkey.dsize = key->length;

      content = gdbm_fetch ((GDBM_FILE)db, gkey);

      if (content.dptr)
	{
	  result = (DBOBJ *)xmalloc (sizeof (DBOBJ));

	  result->data = (unsigned char *)content.dptr;
	  result->length = content.dsize;
	}
    }

  return (result);
}

/* Associate KEY with CONTENT in the database object DB. */
void
database_store (DBFILE db, DBOBJ *key, DBOBJ *content)
{
  if (db != (DBFILE)0)
    {
      datum gkey, gcontent;

      gkey.dptr = (char *)key->data;
      gkey.dsize = key->length;
      gcontent.dptr = (char *)content->data;
      gcontent.dsize = content->length;

      gdbm_store ((GDBM_FILE)db, gkey, gcontent, GDBM_REPLACE);
    }
}

/* Delete the entry stored in DB with key KEY. */
void
database_delete (DBFILE db, DBOBJ *key)
{
  if (db != (DBFILE)0)
    {
      datum gkey;

      gkey.dptr = (char *)key->data;
      gkey.dsize = key->length;

      gdbm_delete ((GDBM_FILE)db, gkey);
    }
}

/* Create a DBOBJ from keyval. */
DBOBJ *
database_setkey (char *keyval)
{
  DBOBJ *key;

  key = (DBOBJ *)xmalloc (sizeof (DBOBJ));
  key->data = (unsigned char *)keyval;
  key->length = 1 + strlen ((char *)key->data);

  return (key);
}

#if defined (__cplusplus)
}
#endif
