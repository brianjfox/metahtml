/* session.c: -*- C -*-  The session database manager. */

/* Author: Brian J. Fox (bfox@ua.com) Fri Jun 30 11:47:31 1995.  */

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

#define ENABLE_TCP_SESSIONS 1

#include "mhtmlstd.h"
#include "session.h"
#include "sessiondb.h"
#include "gdbm_session.h"
#if defined (ENABLE_TCP_SESSIONS)
#include "tcp_session.h"
#endif
#if defined (ENABLE_USERLAND_SESSIONS)
#include "userland_session.h"
#endif

#define SESSION_READ  1
#define SESSION_WRITE 2

#if defined (__cplusplus)
extern "C"
{
#endif

/* When non-zero, this indicates that timed sessions should be reaped
   upon the creation of a new session.  This can be time consuming. */
static int session_reaping = 0;

static SessionDBFuncs *session_manager_table = &gdbm_session_table;

#define sessiondb_open     (session_manager_table->open)
#define sessiondb_close    (session_manager_table->close)
#define sessiondb_setkey   (session_manager_table->setkey)
#define sessiondb_firstkey (session_manager_table->firstkey)
#define sessiondb_nextkey  (session_manager_table->nextkey)
#define sessiondb_fetch    (session_manager_table->fetch)
#define sessiondb_store    (session_manager_table->store)
#define sessiondb_delete   (session_manager_table->delete)
#define sessiondb_reap	   (session_manager_table->reaper)

/* ************************************************************ */
/*								*/
/*	  Forward Declarations for Private Implementation	*/
/*								*/
/* ************************************************************ */

static SESSION_INFO *content_to_info (DBOBJ *content);
static DBOBJ *info_to_content (SESSION_INFO *info);
static session_id_t session_create_sid (void);
static SESSION_INFO *session_allocate_info (void);
static void session_lock_database (int *lock, DBFILE *db, int mode);
static void session_unlock_database (int *lock, DBFILE *db);
static SESSION_INFO *session_sequential_access (DBFILE, SESSION_INFO *);
static void session_delete (session_id_t sid);
static SESSION_INFO *session_get_info_internal (session_id_t sid);
void session_put_info_internal (SESSION_INFO *info);
#if defined (MHTML_CRYPTOGRAPHY)
static unsigned char *session_encrypt (char *key, unsigned char *data, int *l);
static unsigned char *session_decrypt (char *key, unsigned char *data, int *l);
char *session_encryption_key = (char *)NULL;
#endif /* MHTML_CRYPTOGRAPHY */

/* ************************************************************ */
/*								*/
/*		Public Implementation Begins Here		*/
/*								*/
/* ************************************************************ */

/* The location of the session database. */
char *SESSION_DATABASE = DEFAULT_SESSION_DATABASE;

/* The location of the session database lock file. */
char *SESSION_LOCKFILE = DEFAULT_SESSION_LOCKFILE;

/* Change the location of the session database to PATHNAME. */
void
set_session_database_location (char *pathname)
{
  if (pathname == (char *)NULL || *pathname == '\0')
    pathname = DEFAULT_SESSION_DATABASE;

  SESSION_DATABASE = strdup (pathname);

  /* Now decide if this is using TCP sessions, USERLAND sessions, or
     local sessions. */
#if defined (ENABLE_USERLAND_SESSIONS)
  if (strcasecmp (SESSION_DATABASE, "userland") == 0)
    {
      session_manager_table = &userland_session_table;
    }
  else
#endif
#if defined (ENABLE_TCP_SESSIONS)
  if (strchr (SESSION_DATABASE, ':') != (char *)NULL)
    {
      /* TCP based sessioning.  Set up pointers. */
      session_manager_table = &tcp_session_table;
      SESSION_LOCKFILE = (char *)NULL;
    }
  else
#endif
    {
      /* Local GDBM based sessions. */
      char *temp;

      session_manager_table = &gdbm_session_table;

      SESSION_LOCKFILE = (char *)xmalloc (6 + strlen (pathname));
      strcpy (SESSION_LOCKFILE, SESSION_DATABASE);

      temp = strrchr (SESSION_LOCKFILE, '.');
      if (temp == (char *)NULL)
	temp = SESSION_LOCKFILE + strlen (SESSION_LOCKFILE);

      strcpy (temp, ".LOCK");
      set_session_reaping (1);
    }
}

void
set_session_reaping (int reap_p)
{
  session_reaping = reap_p;
}

/* Create a location for this session in the session database, and
   return a session ID to be used in future calls.  If ALLOW_MULTIPLE
   is non-zero, it says to allow multiple sessions for KEY. */
session_id_t
session_begin (char *key, int allow_multiple)
{
  session_id_t sid = (session_id_t) 0;

  /* If we don't allow multiple sessions with the same initial key,
     and there is already a session present in the database with this
     key, then fail. */
  if (allow_multiple || (session_find_key (key) == (session_id_t)0))
    {
      SESSION_INFO *info;

      sid = session_create_sid ();
      info = session_allocate_info ();
      info->sid = strdup (sid);
      info->key = strdup (key);
      session_put_info_internal (info);
      session_free (info);
    }

  /* New sessions cause the reaping of old ones. */
  if (session_reaping)
    session_reap ();

  return (sid);
}

/* Force the session in INFO to exist.  This is the way to restore
   a saved session, for example.  If the session is already present,
   then no change takes place, otherwise, the information in INFO
   is stuffed into the session database. */
void
session_restore (SESSION_INFO *info)
{
  SESSION_INFO *existing;
  time_t now = time ((time_t *)NULL);

  existing = session_get_info_internal (info->sid);

  if (existing == (SESSION_INFO *)NULL)
    existing = info;

  existing->access = now;

  session_put_info_internal (existing);
}

/* Inform the session database that this session is closed.  This
   deletes the entry from the database.  Returns 0 if the session
   was present, -1 if not. */
int
session_end (session_id_t sid)
{
  SESSION_INFO *info;

  info = session_get_info (sid);

  if (info == (SESSION_INFO *)NULL)
    return (-1);

  session_delete (sid);

  return (0);
}

/* Reap those sessions which have timed out.
   Returns the number of sessions reaped. */
int
session_reap (void)
{
  int result = 0;

  if ((session_reaping != 0) && (sessiondb_reap != NULL))
    result =  sessiondb_reap ();

  return (result);
}

int
generic_session_reaper (void)
{    
  SESSION_INFO *info = (SESSION_INFO *)NULL;
  SESSION_INFO *newinfo = (SESSION_INFO *)NULL;
  DBFILE db;
  int the_lock;
  int reaped = 0;
  time_t now;

  session_lock_database (&the_lock, &db, SESSION_WRITE);
  now = time ((time_t *)0);

  if ((the_lock != -1) && (db != (DBFILE)0))
    {
      SESSION_INFO **to_be_reaped = (SESSION_INFO **)NULL;
      int tbr_index = 0, tbr_size = 0;
      int free_it = 0;

      while ((newinfo = session_sequential_access (db, info)) !=
	     (SESSION_INFO *)NULL)
	{
	  if (free_it) session_free (info);
	  free_it = 1;
	  info = newinfo;

	  if (info->timeout != 0)
	    {
	      long seconds_unaccessed;

	      seconds_unaccessed = (long) now - info->access;
	      if (seconds_unaccessed >= (info->timeout * 60))
		{
		  if (tbr_index + 2 >= tbr_size)
		    to_be_reaped = (SESSION_INFO **)xrealloc
		      (to_be_reaped,
		       (tbr_size += 20) * sizeof (SESSION_INFO *));

		  to_be_reaped[tbr_index++] = info;
		  to_be_reaped[tbr_index] = (SESSION_INFO *)NULL;
		  free_it = 0;
		}
	    }
	}

      if (free_it && info) session_free (info);

      if (tbr_index)
	{
	  register int i;

	  for (i = 0; i < tbr_index; i++)
	    {
	      DBOBJ *key;

	      key = sessiondb_setkey ((char *)(to_be_reaped[i]->sid));
	      sessiondb_delete (db, key);
	      free (key);
	      session_free (to_be_reaped[i]);
	      reaped++;
	    }
	  free (to_be_reaped);
	}
    }

  session_unlock_database (&the_lock, &db);

  return (reaped);
}

/* Inform the session database that the data is being accessed.
   Returns 0 if the session is still active, else -1. */
int
session_access (session_id_t sid)
{
  SESSION_INFO *info;
  int result = 0;

  info = session_get_info_internal (sid);

  if (info != (SESSION_INFO *)NULL)
    {
      /* Possibly timeout. */
      if (info->timeout != 0)
	{
	  long seconds_unaccessed;
	  time_t now;

	  now = time ((time_t *)NULL);
	  seconds_unaccessed = (long) now - info->access;

	  if (seconds_unaccessed >= (info->timeout * 60))
	    {
	      session_delete (sid);
	      result = -1;
	    }
	  else
	    info->access = now;
	}
      session_free (info);
    }
  else
    result = -1;

  return (result);
}

/* Return the SESSION_INFO associated with SID.  This function
   implicitly calls session_access (), thus updating the last
   access time.  Returns a pointer to the session info if
   successful, or a NULL pointer if not. */
SESSION_INFO *
session_get_info (session_id_t sid)
{
  SESSION_INFO *info = (SESSION_INFO *)NULL;

  if (session_access (sid) == 0)
    info = session_get_info_internal (sid);

  return (info);
}

/* Set the session info for this session.  This implicitly calls
   session_access (), thus updating the last access time.  Returns
   the session info passed in, or a NULL pointer if the session
   no longer exists. */
SESSION_INFO *
session_put_info (SESSION_INFO *info)
{
  SESSION_INFO *result = (SESSION_INFO *)NULL;
  SESSION_INFO *current;

  current = session_get_info_internal (info->sid);

  if (current != (SESSION_INFO *)NULL)
    {
      time_t now;

      now = time ((time_t *)NULL);

      result = info;
      result->access = now;

      /* Note that you cannot change the key that this session was
	 created with. */
      if (result->key) free (result->key);
      result->key = current->key ? strdup (current->key) : (char *)NULL;
      session_put_info_internal (result);
      session_free (current);
    }

  return (result);
}

/* Return the timeout value for the session mentioned in SID. */
int session_get_timeout (session_id_t sid)
{
  SESSION_INFO *info = session_get_info_internal (sid);
  int timeout = info ? info->timeout : -1;

  if (info)
    session_free (info);

  return (timeout);
}

/* Returns the first session found which matches KEY, the original
   argument given to session_begin.  If there is no such session,
   returns a session_id_t of 0. */
session_id_t
session_find_key (char *key)
{
  session_id_t result = (session_id_t)0;
  SESSION_INFO *info = (SESSION_INFO *)NULL;
  DBFILE db;
  int the_lock;

  session_lock_database (&the_lock, &db, SESSION_READ);

  if ((the_lock != -1) && (db != (DBFILE)0))
    {
      while ((info = session_sequential_access (db, info)) !=
	     (SESSION_INFO *)NULL)
	{
	  if ((info->key != (char *)NULL) && (strcmp (key, info->key) == 0))
	    {
	      result = info->sid;
	      break;
	    }
	}
    }

  session_unlock_database (&the_lock, &db);

  return (result);
}

/* A useful function for those programs which need access to the
   entire database.  Returns an array of SESSION_INFO *, one for
   each entry in the database.  Timed out entries which have not
   yet been reaped are also returned. */
SESSION_INFO **
session_all_sessions (void)
{
  SESSION_INFO **result = (SESSION_INFO **)NULL;
  SESSION_INFO *info = (SESSION_INFO *)NULL;
  DBFILE db;
  int the_lock;
  int si_size = 0, si_index = 0;

  session_lock_database (&the_lock, &db, SESSION_READ);

  if ((the_lock != -1) && (db != (DBFILE)0))
    {
      while ((info = session_sequential_access (db, info)) !=
	     (SESSION_INFO *)NULL)
	{
	  if (si_index + 2 >= si_size)
	    result = (SESSION_INFO **)xrealloc
	      (result, (si_size += 100) * sizeof (SESSION_INFO *));

	  result[si_index++] = info;
	  result[si_index] = (SESSION_INFO *)NULL;
	}
    }

  session_unlock_database (&the_lock, &db);

  return (result);
}

/* ************************************************************ */
/*								*/
/*		Private Implementation Begins Here		*/
/*								*/
/* ************************************************************ */

/* Create a unique session identifier and return it. */
static session_id_t
session_create_sid (void)
{
  BPRINTF_BUFFER *b;
  session_id_t sid;
  time_t now;
  unsigned long value;

  now = time ((time_t *)NULL);
  srandom ((unsigned) (now & 0x0000FFFFFFFF));
  value = (unsigned long) random ();
  value = (unsigned long) random ();
  value = (unsigned long) random ();
  b = bprintf_create_buffer ();
  bprintf (b, "%d%d", getpid (), value);
  sid = (session_id_t)(b->buffer);
  free (b);
  return (sid);
}

/* Allocate space for a SESSION_INFO, filling in the start and access
   times, and defaulting other values where appropriate.  Returns a
   pointer to the newly created structure. */
static SESSION_INFO *
session_allocate_info (void)
{
  SESSION_INFO *info;

  info = (SESSION_INFO *)xmalloc (sizeof (SESSION_INFO));
  memset (info, 0, sizeof (SESSION_INFO));
  info->start = time ((time_t *)NULL);
  info->access = info->start;
  info->timeout = SESSION_TIMEOUT;

  return (info);
}

/* Free the data associated with INFO. */
void
session_free (SESSION_INFO *info)
{
  if (info != (SESSION_INFO *)NULL)
    {
      xfree (info->sid);
      xfree (info->key);
      xfree (info->data);
      free (info);
    }
}

static void
session_lock_database (int *lock, DBFILE *db, int mode)
{
  int fd = -1;
  int dbmode;

  *lock = -1;
  *db = (DBFILE) 0;

  if (mode == SESSION_READ)
    dbmode = DB_READER;
  else
    dbmode = DB_WRCREAT;

  if (SESSION_LOCKFILE == (char *)NULL)
    {
      *db = sessiondb_open (SESSION_DATABASE, dbmode);
      if (*db != (DBFILE)NULL)
	*lock = 99;
    }
  else
    {
      fd = os_open (SESSION_LOCKFILE, O_CREAT | O_WRONLY | O_APPEND, 0666);

      if ((fd == -1) ||
	  (((mode == SESSION_READ) && (READLOCKFILE (fd) == -1)) ||
	   ((mode == SESSION_WRITE) && (LOCKFILE (fd) == -1))))
	{
	  if (fd >= 0)
	    close (fd);
	  return;
	}
      else
	{
	  if ((*db = sessiondb_open (SESSION_DATABASE, dbmode)) == (DBFILE)0)
	    {
	      if (fd != -1)
		{
		  UNLOCKFILE (fd);
		  close (fd);
		}
	    }
	  else
	    *lock = fd;
	}
    }
}

static void
session_unlock_database (int *lock, DBFILE *db)
{
  if (*db != (DBFILE)0)
    sessiondb_close (*db);

  if ((SESSION_DATABASE != (char *)NULL) && (*lock > -1))
    {
      UNLOCKFILE (*lock);
      close (*lock);
    }
}

static SESSION_INFO *
session_sequential_access (DBFILE db, SESSION_INFO *initial)
{
  SESSION_INFO *info = (SESSION_INFO *)NULL;
  DBOBJ *key = (DBOBJ *)NULL;

  if (!initial)
    key = sessiondb_firstkey (db);
  else
    {
      DBOBJ *key1 = sessiondb_setkey (initial->sid);
      key = sessiondb_nextkey (db, key1);
      free (key1);
    }

  if (key != (DBOBJ *)NULL)
    {
      DBOBJ *content = sessiondb_fetch (db, key);

      if (key->data)
	{
	  free (key->data);
	  free (key);
	}

      if (content)
	{
	  if ((content->data) && (content->length))
	    {
	      info = content_to_info (content);
	      free (content->data);
	    }
	  free (content);
	}
    }

  return (info);
}

static void
session_delete (session_id_t sid)
{
  int the_lock;
  DBFILE db;

  session_lock_database (&the_lock, &db, SESSION_WRITE);

  if ((the_lock != -1) && (db != (DBFILE)0))
    {
      DBOBJ *key;

      key = sessiondb_setkey ((char *)sid);
      sessiondb_delete (db, key);
    }

  session_unlock_database (&the_lock, &db);
}

/* Exclusively open the database, and read and unpack the contents of
   the entry indexed by SID. */
static SESSION_INFO *
session_get_info_internal (session_id_t sid)
{
  SESSION_INFO *info = (SESSION_INFO *)NULL;
  int the_lock;
  DBFILE db;

  session_lock_database (&the_lock, &db, SESSION_READ);

  if ((the_lock != -1) && (db != (DBFILE)0))
    {
      DBOBJ *key, *content;

      key = sessiondb_setkey ((char *)sid);
      content = sessiondb_fetch (db, key);
      xfree (key);
      if (content)
	{
	  info = content_to_info (content);
	  xfree (content->data);
	  free (content);
	}
    }

  session_unlock_database (&the_lock, &db);

  return (info);
}

/* Exclusively open the database, and pack and write the contents of INFO. */
void
session_put_info_internal (SESSION_INFO *info)
{
  int the_lock;
  DBFILE db;

  session_lock_database (&the_lock, &db, SESSION_WRITE);

  if ((the_lock != -1) && (db != (DBFILE)0))
    {
      DBOBJ *key, *content;

      key = sessiondb_setkey (info->sid);
      content = info_to_content (info);
      sessiondb_store (db, key, content);

      free  (key);
      if (content)
	{
	  xfree (content->data);
	  free (content);
	}
    }

  session_unlock_database (&the_lock, &db);
}

#include <wisper/wisp.h>

static char *
strassoc (char *tag, WispObject *list)
{
  WispObject *alist, *cdr;

  alist = assoc (tag, list);

  if ((alist == (WispObject *)NULL) ||
      (alist == NIL) ||
      ((cdr = CDR (alist)) == NIL) ||
      (!STRING_P (cdr)))
    return ((char *)NULL);
  else
    return (STRING_VALUE (cdr));
}

static int
get_lisp_int (char *tag, WispObject *list)
{
  char *string = (char *)NULL;
  int value = 0;

  string = strassoc (tag, list);
  if (string != (char *)NULL)
    sscanf (string, "%d", &value);

  return (value);
}

static long
get_lisp_long (char *tag, WispObject *list)
{
  char *string = (char *)NULL;
  long value = 0;

  string = strassoc (tag, list);
  if (string != (char *)NULL)
    sscanf (string, "%ld", &value);

  return (value);
}

static char *
get_lisp_string (char *tag, WispObject *list)
{
  char *value = (char *)NULL;

  if ((value = strassoc (tag, list)) != (char *)NULL)
    value = strdup (value);

  return (value);
}

#if defined (MHTML_CRYPTOGRAPHY)
extern unsigned char *triple_des (unsigned char *, char *, int *, int);

static unsigned char *
session_encrypt (char *key, unsigned char *data, int *len)
{
  unsigned char *result = (unsigned char *)NULL;

  if (*len != 0)
    result = triple_des (data, key, len, 1);

  return (result);
}

static unsigned char *
session_decrypt (char *key, unsigned char *data, int *len)
{
  unsigned char *result = (unsigned char *)NULL;

  if (*len != 0)
    result = triple_des (data, key, len, 0);

  return (result);
}
#endif /* MHTML_CRYPTOGRAPHY */

static char *
encode_data (unsigned char *data, int length)
{
  register int i;
  unsigned char *result;

#if defined (MHTML_CRYPTOGRAPHY)
  int free_data = 0;

  /* If encrypted sessioning is turned on, then handle the
     encryption of the data now. */
  if ((length != 0) &&
      (session_encryption_key != (char *)NULL) &&
      (session_encryption_key[0] != '\0'))
    {
      int len = 1 + length;
      data = session_encrypt (session_encryption_key, data, &len);
      length = len;
      free_data++;
    }
#endif /* MHTML_CRYPTOGRAPHY */

  result = (unsigned char *)xmalloc (1 + (2 * length));

  for (i = 0; i < length; i++)
    sprintf ((char *)result + (2 * i), "%02x", data[i]);

  result[2 * i] = '\0';

#if defined (MHTML_CRYPTOGRAPHY)
  if (free_data) free (data);
#endif

  return ((char *)result);
}

static char *
decode_data (char *data)
{
  register int i;
  unsigned char *result;

  result = (unsigned char *)xmalloc (1 + (strlen (data) / 2));

  for (i = 0; data[i] != '\0'; i += 2)
    {
      unsigned char value = 0;

      if (isdigit (data[i]))
	value = data[i] - '0';
      else
	value = data[i] - 'a' + 10;

      value *= 16;

      if (isdigit (data[i + 1]))
	value += data[i + 1] - '0';
      else
	value += data[i + 1] - 'a' + 10;

      result[i / 2] = value;
    }

  result[i / 2] = '\0';

#if defined (MHTML_CRYPTOGRAPHY)
  /* If encrypted sessioning is turned on, then handle the
     decryption of the data now. */
  if ((i != 0) &&
      (session_encryption_key != (char *)NULL) &&
      (session_encryption_key[0] != '\0'))
    {
      int len = i / 2;
      unsigned char *decrypted;

      decrypted = session_decrypt (session_encryption_key, result, &len);
      free (result);
      result = decrypted;
    }
#endif /* MHTML_CRYPTOGRAPHY */

  return ((char *)result);
}

static SESSION_INFO *
content_to_info (DBOBJ *content)
{
  SESSION_INFO *info = (SESSION_INFO *)NULL;

  if (content != (DBOBJ *)NULL)
    {
      WispObject *list;

      list = wisp_from_string ((char *)content->data);

      if (list != (WispObject *)NULL)
	{
	  info = (SESSION_INFO *)xmalloc (sizeof (SESSION_INFO));
	  info->sid = (session_id_t)get_lisp_string ("sid", list);
	  info->key = get_lisp_string ("key", list);
	  info->start = (time_t)get_lisp_long ("start", list);
	  info->access = (time_t)get_lisp_long ("access", list);
	  info->timeout = get_lisp_int ("timeout", list);
	  info->length = (size_t)get_lisp_long ("length", list);
	  {
	    char *temp;
	    temp = get_lisp_string ("data", list);
	    info->data = (unsigned char *)decode_data (temp);
	    free (temp);
	  }

	  gc_wisp_objects ();
	}
    }
  return (info);
}

static DBOBJ *
info_to_content (SESSION_INFO *info)
{
  DBOBJ *content = (DBOBJ *)NULL;
  BPRINTF_BUFFER *buffer;

  buffer = bprintf_create_buffer ();

  bprintf (buffer, "(");

  bprintf (buffer, "(sid . %s)", wisp_readable (info->sid));
  bprintf (buffer, "(key . %s)", wisp_readable (info->key));
  bprintf (buffer, "(start . %ld)", (long)info->start);
  bprintf (buffer, "(access . %ld)", (long)info->access);
  bprintf (buffer, "(timeout . %d)", info->timeout);
  bprintf (buffer, "(length . %ld)", (long)info->length);

  {
    char *temp;

    temp = encode_data (info->data, info->length);
    bprintf (buffer, "(data . %s)", wisp_readable (temp));
    free (temp);
  }

  bprintf (buffer, ")");

  content = (DBOBJ *)xmalloc (sizeof (DBOBJ));
  content->data = (unsigned char *)buffer->buffer;
  content->length = 1 + strlen ((char *)content->data);

  free (buffer);
  return (content);
}

#if defined (__cplusplus)
}
#endif
