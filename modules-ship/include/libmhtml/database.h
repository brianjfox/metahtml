/* database.h: Code used to hide the underlying database implementation. */

/* Author: Brian J. Fox (bfox@ua.com) Sat Jul  1 14:41:09 1995. */

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

#if !defined (_DATABASE_H_)
#define _DATABASE_H_

#if defined (__cplusplus)
extern "C"
{
#endif
/* An opaque handle to a database. */
typedef void *DBFILE;

typedef struct
{
  unsigned char *data;
  size_t length;
} DBOBJ;

#define DB_WRCREAT 1
#define DB_READER  2
#define DB_WRITER  4

/* Open the database named NAME for access as determined by FLAGS, and
   return a handle to it.  A handle of 0 indicates failure. */
extern DBFILE database_open (char *name, int flags);

/* Close the currently open database referenced with the DBFILE handle DB. */
extern void database_close (DBFILE db);

/* Return the object which represents the first key in the currently
   open database referenced with the DBFILE handle DB. */
extern DBOBJ *database_firstkey (DBFILE db);

/* Return the object which represents the "next" key in the currently
   open database referenced with the DBFILE handle DB.  The "next" key
   is the one after the DBOBJ KEY. */
extern DBOBJ *database_nextkey (DBFILE db, DBOBJ *key);

/* Return the database object in DB associated with KEY. */
extern DBOBJ *database_fetch (DBFILE db, DBOBJ *key);

/* Associate KEY with CONTENT in the database object DB. */
extern void database_store (DBFILE db, DBOBJ *key, DBOBJ *content);

/* Delete the entry stored in DB with key KEY. */
extern void database_delete (DBFILE db, DBOBJ *key);

/* Create a DBOBJ from keyval. */
extern DBOBJ *database_setkey (char *keyval);

/* Return the string representation of the most recent error. */
extern char *database_strerror (void);

/* Return non-zero if there was a recent error during database operations. */
extern int database_had_error_p (void);

/* Reset the database error indicator. */
extern void database_reset_error (void);

#if defined (__cplusplus)
}
#endif

#endif /* !_DATABASE_H_ */
