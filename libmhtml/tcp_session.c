/* tcp_session.c: -*- C -*-  The session database manager (TCP Based). */

/* Author: Brian J. Fox (bfox@ua.com) Thu Aug 12 21:59:47 1999.  */

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

#include "mhtmlstd.h"
#include "sessiondb.h"
#include "tcp_session.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* ************************************************************ */
/*								*/
/*	  Forward Declarations for Private Implementation	*/
/*								*/
/* ************************************************************ */

/* Open the database named NAME for access as determined by FLAGS, and
   return a handle to it.  A handle of 0 indicates failure. */
static DBFILE tcp_open (char *name, int flags);

/* Close the currently open database referenced with the DBFILE handle DB. */
static void tcp_close (DBFILE db);

/* Return the object which represents the first key in the currently
   open database referenced with the DBFILE handle DB. */
static DBOBJ *tcp_firstkey (DBFILE db);

/* Return the object which represents the "next" key in the currently
   open database referenced with the DBFILE handle DB.  The "next" key
   is the one after the DBOBJ KEY. */
static DBOBJ *tcp_nextkey (DBFILE db, DBOBJ *key);

/* Return the database object in DB associated with KEY. */
static DBOBJ *tcp_fetch (DBFILE db, DBOBJ *key);

/* Associate KEY with CONTENT in the database object DB. */
static void tcp_store (DBFILE db, DBOBJ *key, DBOBJ *content);

/* Delete the entry stored in DB with key KEY. */
static void tcp_delete (DBFILE db, DBOBJ *key);

/* Return the string representation of the most recent error. */
static char *tcp_strerror (void);

/* Return non-zero if there was a recent error during database operations. */
static int tcp_had_error_p (void);

/* Reset the database error indicator. */
static void tcp_reset_error (void);

extern DBOBJ *database_setkey (char *keyval);
extern int generic_session_reaper (void);

/* ************************************************************ */
/*								*/
/*		Public Implementation Begins Here		*/
/*								*/
/* ************************************************************ */
SessionDBFuncs tcp_session_table =
{
  tcp_open,
  tcp_close,
  tcp_firstkey,
  tcp_nextkey,
  tcp_fetch,
  tcp_store,
  tcp_delete,
  database_setkey,
  generic_session_reaper,
  tcp_strerror,
  tcp_had_error_p,
  tcp_reset_error
};

/* ************************************************************ */
/*								*/
/*			The Actual Implementation		*/
/*								*/
/* ************************************************************ */

/* Structure which defines a TCP connection.  It has state, i.e., don't
   forget to close it, or you'll run out of file descriptors. */
typedef struct
{
  int fd;
} tcpdb;

static tcpdb *
make_connection (char *host_and_port)
{
  register int i;
  static char hostname[256];
  static char portname[128];
  tcpdb *result = (tcpdb *)NULL;

  if (host_and_port)
    {
      for (i = 0; host_and_port[i] != '\0'; i++)
	{
	  hostname[i] = host_and_port[i];
	  if (hostname[i] == ':')
	    break;
	}

      hostname[i] = '\0';
      host_and_port = host_and_port + i;

      if (host_and_port[0] != '\0')
	{
	  int fd;
	  host_and_port++;
	  for (i = 0; host_and_port[i] != '\0'; i++)
	    portname[i] = host_and_port[i];

	  fd = tcp_to_host (hostname, portname);

	  if (fd > -1)
	    {
	      result = (tcpdb *)xmalloc (sizeof (tcpdb));
	      result->fd = fd;
	    }
	}
    }
  return (result);
}

static int
tcpdb_write (int fd, int count, void *data)
{
  int bytes;

  bytes = write (fd, (const void *)data, (size_t)count);
  return (bytes == count);
}

static int
write_dbobj (int fd, DBOBJ *obj)
{
  int bytes, length;

  length = obj->length;
  write (fd, (const void *)&length, sizeof (length));
  bytes = write (fd, (const void *)obj->data, (size_t)length);
  return (bytes);
}

static DBOBJ *
read_dbobj (DBFILE db)
{
  tcpdb *connection = (tcpdb *)db;
  DBOBJ *result = (DBOBJ *)NULL;

  if (connection != (tcpdb *)NULL)
    {
      int count = 0;
      int bytes_read = 0;

      read (connection->fd, &count, sizeof (count));

      if (count > 0)
	{
	  result = (DBOBJ *)xmalloc (sizeof (DBOBJ));
	  result->length = count;
	  result->data = (unsigned char *)xmalloc (1 + count);

	  while (bytes_read < count)
	    {
	      int amount = read (connection->fd, result->data + bytes_read,
				 count - bytes_read);
	      if (amount < 0)
		break;
	      else
		bytes_read += amount;
	    }

	  result->data[count] = '\0';
	}
    }

  return (result);
}

#if defined (NEVER_USED)
static char *
tcpdb_opname (int op)
{
  char *opname = (char *)NULL;

  switch (op)
    {
    case _tcpdb_OPEN: opname = _tcpdb_opname_OPEN; break;
    case _tcpdb_STORE: opname = _tcpdb_opname_STORE; break;
    case _tcpdb_FETCH: opname = _tcpdb_opname_FETCH; break;
    case _tcpdb_DELETE: opname = _tcpdb_opname_DELETE; break;
    case _tcpdb_CLOSE: opname = _tcpdb_opname_CLOSE; break;
    case _tcpdb_FIRSTKEY: opname = _tcpdb_opname_FIRSTKEY; break;
    case _tcpdb_NEXTKEY: opname = _tcpdb_opname_NEXTKEY; break;
    default: opname = "????";
    }

  return (opname);
}
#endif /* NEVER_USED */

static int
tcpdb_tell (DBFILE db, int op, ...)
{
  tcpdb *connection = (tcpdb *)db;
  int result = 0;
  va_list args;
  va_start (args, op);

  if (connection != (tcpdb *)NULL)
    {
      int count = 0;

      result = tcpdb_write (connection->fd, sizeof (op), &op);

      switch (op)
	{
	case _tcpdb_FIRSTKEY:
	case _tcpdb_CLOSE:
	  break;

	case _tcpdb_NEXTKEY:
	case _tcpdb_FETCH:
	case _tcpdb_DELETE:
	  {
	    DBOBJ *obj = va_arg (args, DBOBJ *);
	    result = (write_dbobj (connection->fd, obj) == obj->length);
	  }
	  break;

	case _tcpdb_STORE: 
	  {
	    DBOBJ *obj = va_arg (args, DBOBJ *);
	    result = (write_dbobj (connection->fd, obj) == obj->length);

	    if (result > 0)
	      {
		obj = va_arg (args, DBOBJ *);
		result = (write_dbobj (connection->fd, obj) == obj->length);
	      }
	  }
	  break;
	}
    }

  va_end (args);
  return (result);
}

/* Open the database named NAME for access as determined by FLAGS, and
   return a handle to it.  A handle of 0 indicates failure. */
static DBFILE
tcp_open (char *name, int flags)
{
  tcpdb *connection = make_connection (name);
  return ((DBFILE)connection);
}

/* Close the currently open database referenced with the DBFILE handle DB. */
static void
tcp_close (DBFILE db)
{
  tcpdb *connection = (tcpdb *)db;

  if (connection != (tcpdb *)NULL)
    {
      tcpdb_tell (db, _tcpdb_CLOSE);
      close (connection->fd);
      free (connection);
    }
}

/* Return the object which represents the first key in the currently
   open database referenced with the DBFILE handle DB. */
static DBOBJ *
tcp_firstkey (DBFILE db)
{
  DBOBJ *result = (DBOBJ *)NULL;

  if (tcpdb_tell (db, _tcpdb_FIRSTKEY))
    result = read_dbobj (db);

  return (result);
}

/* Return the object which represents the "next" key in the currently
   open database referenced with the DBFILE handle DB.  The "next" key
   is the one after the DBOBJ KEY. */
static DBOBJ *
tcp_nextkey (DBFILE db, DBOBJ *key)
{
  DBOBJ *result = (DBOBJ *)NULL;

  if (tcpdb_tell (db, _tcpdb_NEXTKEY, key))
    result = read_dbobj (db);

  return (result);
}

/* Return the database object in DB associated with KEY. */
static DBOBJ *
tcp_fetch (DBFILE db, DBOBJ *key)
{
  DBOBJ *result = (DBOBJ *)NULL;

  if (tcpdb_tell (db, _tcpdb_FETCH, key))
    result = read_dbobj (db);

  return (result);
}

/* Associate KEY with CONTENT in the database object DB. */
static void
tcp_store (DBFILE db, DBOBJ *key, DBOBJ *content)
{
  tcpdb_tell (db, _tcpdb_STORE, key, content);
}

/* Delete the entry stored in DB with key KEY. */
static void
tcp_delete (DBFILE db, DBOBJ *key)
{
  tcpdb_tell (db, _tcpdb_DELETE, key);  
}

/* Return the string representation of the most recent error. */
static char *
tcp_strerror (void)
{
  return ("Are you kidding?  This doesn't even work yet!");
}

/* Return non-zero if there was a recent error during database operations. */
static int
tcp_had_error_p (void)
{
  return (0);
}

/* Reset the database error indicator. */
static void
tcp_reset_error (void)
{
}

#if defined (__cplusplus)
}
#endif
