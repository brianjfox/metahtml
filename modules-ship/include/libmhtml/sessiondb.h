/* sessiondb.h: -*- C -*-  Dispatch to session database handler. */

/* Author: Brian J. Fox (bfox@ua.com) Fri Aug 13 07:49:50 1999. */

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
#if !defined (_SESSIONDB_H_)
#define _SESSIONDB_H_

#if !defined (_DATABASE_H_)
#  include <database.h>
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

/* The following typedefs are used to make the remainder of the code more
   readable.  Each one is really a function prototype, so that we can use
   the name of the prototype when defining pointers to these functions, as
   in the structure containing the pointers below. */

/* Open the database named NAME for access as determined by FLAGS, and
   return a handle to it.  A handle of 0 indicates failure. */
typedef DBFILE _db_open (char *name, int flags);

/* Close the currently open database referenced with the DBFILE handle DB. */
typedef void _db_close (DBFILE db);

/* Return the object which represents the first key in the currently
   open database referenced with the DBFILE handle DB. */
typedef DBOBJ *_db_firstkey (DBFILE db);

/* Return the object which represents the "next" key in the currently
   open database referenced with the DBFILE handle DB.  The "next" key
   is the one after the DBOBJ KEY. */
typedef DBOBJ *_db_nextkey (DBFILE db, DBOBJ *key);

/* Return the database object in DB associated with KEY. */
typedef DBOBJ *_db_fetch (DBFILE db, DBOBJ *key);

/* Associate KEY with CONTENT in the database object DB. */
typedef void _db_store (DBFILE db, DBOBJ *key, DBOBJ *content);

/* Delete the entry stored in DB with key KEY. */
typedef void _db_delete (DBFILE db, DBOBJ *key);

/* Create a DBOBJ from keyval. */
typedef DBOBJ *_db_setkey (char *keyval);

/* Reap sessions which have timed out. Return the number of sessions reaped. */
typedef int _db_session_reap (void);

/* Return the string representation of the most recent error. */
typedef char *_db_strerror (void);

/* Return non-zero if there was a recent error during database operations. */
typedef int _db_had_error_p (void);

/* Reset the database error indicator. */
typedef void _db_reset_error (void);

/* Here is a structure which contains pointers to all of the functions
   iterated above.  It is a dispatch table for the session database
   functions. */

typedef struct
{
  _db_open *open;
  _db_close *close;
  _db_firstkey *firstkey;
  _db_nextkey *nextkey;
  _db_fetch *fetch;
  _db_store *store;
  _db_delete *delete;
  _db_setkey *setkey;
  _db_session_reap *reaper;
  _db_strerror *strerror;
  _db_had_error_p *had_error_p;
  _db_reset_error *reset_error;
} SessionDBFuncs;

#if defined (__cplusplus)
}
#endif

#endif /* !_SESSIONDB_H_ */
