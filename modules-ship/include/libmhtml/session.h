/* session.h: Functions for tracking an HTML "session". */

/* Author: Brian J. Fox (bfox@ua.com) Thu Jun 29 21:31:46 1995. */

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

#if !defined (_SESSION_H_)
#define _SESSION_H_

#include "database.h"

/* The default location of the session database. */
#define DEFAULT_SESSION_DATABASE "/tmp/sessions.db"

/* The location of the session database lock file. */
#define DEFAULT_SESSION_LOCKFILE "/tmp/sessions.LOCK"

/* The default number of minutes a session can be idle before it
   gets timed out.  A value of 0 indicates that the session never
   times out. */
#define SESSION_TIMEOUT 61

#if defined (__cplusplus)
extern "C"
{
#endif

/* The underlying data type of a unique session identifier. */
typedef char *session_id_t;

/* A structure encapsulating various bits of information about a
   session. */
typedef struct {
  session_id_t sid;		/* The session identifier. */
  char *key;			/* The key used to start this session. */
  time_t start;			/* Time when session_begin was called. */
  time_t access;		/* Time when session_access was last called. */
  int timeout;			/* Number of idle minutes allowed. */
  size_t length;		/* Length of the data which follows. */
  unsigned char *data;		/* Associated data. */
} SESSION_INFO;

/* The location of the session database. */
extern char *SESSION_DATABASE;

/* The location of the session database lock file. */
extern char *SESSION_LOCKFILE;

/* Change the location of the session database to PATHNAME. */
extern void set_session_database_location (char *pathname);

/* Create a location for this session in the session database, and
   return a session ID to be used in future calls.  If ALLOW_MULTIPLE
   is non-zero, it says to allow multiple sessions for KEY. */
extern session_id_t session_begin (char *key, int allow_multiple);

/* Inform the session database that this session is closed.  This
   deletes the entry from the database.  Returns 0 if the session
   was present, -1 if not. */
extern int session_end (session_id_t sid);

/* Force the session in INFO to exist.  This is the way to restore
   a saved session, for example.  If the session is already present,
   then no change takes place, otherwise, the information in INFO
   is stuffed into the session database. */
extern void session_restore (SESSION_INFO *info);

/* Inform the session database that the data is being accessed.
   Returns 0 if the session is still active, else -1. */
extern int session_access (session_id_t sid);

/* Free the data associated with INFO. */
extern void session_free (SESSION_INFO *info);

/* Return the SESSION_INFO associated with SID.  This function
   implicitly calls session_access (), thus updating the last
   access time.  Returns a pointer to the session info if
   successful, or a NULL pointer if not. */
extern SESSION_INFO *session_get_info (session_id_t sid);

/* Set the session info for this session.  This implicitly calls
   session_access (), thus updating the last access time.  Returns
   the session info passed in, or a NULL poointer if the session
   no longer exists. */
extern SESSION_INFO *session_put_info (SESSION_INFO *info);

/* Return the timeout value for the session mentioned in SID. */
extern int session_get_timeout (session_id_t sid);

/* Returns the first session found which matches KEY, the original
   argument given to session_begin.  If there is no such session,
   returns a session_id_t of 0. */
extern session_id_t session_find_key (char *key);

/* A useful function for those programs which need access to the
   entire database.  Returns an array of SESSION_INFO *, one for
   each entry in the database.  Timed out entries which have not
   yet been reaped are also returned. */
extern SESSION_INFO **session_all_sessions (void);

/* Exclusively open the database, and pack and write the contents of INFO. */
extern void session_put_info_internal (SESSION_INFO *info);

/* Reap those sessions which have timed out.
   Returns the number of sessions reaped. */
extern int session_reap (void);
extern int generic_session_reaper (void);
extern void set_session_reaping (int reap_p);

#if defined (__cplusplus)
}
#endif

#endif /* !_SESSION_H_ */

