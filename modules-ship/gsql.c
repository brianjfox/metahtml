/* Gsql.c: -*- C -*-  Functions for manipulating SQL databases. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jun 26 07:27:10 1996.
            Henry Minsky (hqm@ua.com)

    This file is part of <Meta-HTML>(tm), a system for the rapid
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

#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_with_open_database (PFunArgs);
static void pf_database_query (PFunArgs);
static void pf_database_exec_query (PFunArgs);
static void pf_database_exec_sql (PFunArgs);
static void pf_database_next_record (PFunArgs);
static void pf_host_databases (PFunArgs);
static void pf_database_tables (PFunArgs);
static void pf_database_tables_info (PFunArgs);
static void pf_database_columns (PFunArgs);
static void pf_database_column_info (PFunArgs);
static void pf_database_columns_info (PFunArgs);
static void pf_database_query_info (PFunArgs);
static void pf_database_num_rows (PFunArgs);
static void pf_database_affected_rows (PFunArgs);
static void pf_database_set_pos (PFunArgs);
static void pf_database_save_record (PFunArgs);
static void pf_database_load_record (PFunArgs);
static void pf_database_delete_record (PFunArgs);
static void pf_database_save_package (PFunArgs);
static void pf_package_to_table (PFunArgs);
static void pf_database_set_options (PFunArgs);
static void pf_cursor_get_column (PFunArgs);
static void pf_query_get_column (PFunArgs);
static void pf_sql_transact (PFunArgs);

/* Return codes. */
#define GSQL_INVALID_HANDLE		(-2)
#define GSQL_ERROR			(-1)
#define GSQL_SUCCESS 			0
#define GSQL_SUCCESS_WITH_INFO		1
#define GSQL_NO_DATA_FOUND		100

/* Standard GSQL datatypes (agree with ANSI type numbering). */
#define GSQL_CHAR	1
#define GSQL_NUMERIC 	2
#define GSQL_DECIMAL 	3
#define GSQL_INTEGER 	4
#define GSQL_SMALLINT	5
#define GSQL_FLOAT	6
#define GSQL_REAL	7
#define GSQL_DOUBLE	8
#define GSQL_ODBC_DATE	9
#define GSQL_VARCHAR 	12
#define GSQL_BOOLEAN	16

/* These are not standard ANSI */
#define GSQL_IDENT	100
#define GSQL_NULL	101
#define GSQL_IDX	102
#define GSQL_SYSVAR	103
#define GSQL_UINT	104
#define GSQL_ANY	105

/* PostGres SQL extensions. */
#define GSQL_DATE	200
#define GSQL_TIME	201
#define GSQL_MONEY	202


/* MYSQL extensions. (tamsky@as.ucsb.edu: Sat Oct 11 11:55:38 1997) */
#define GSQL_TINY_BLOB      249
#define GSQL_MEDIUM_BLOB    250
#define GSQL_LONG_BLOB      251
#define GSQL_BLOB           252
#define GSQL_VAR_STRING     253
#define GSQL_STRING         254

/* Reporting of db errors to Meta-HTML environment  */
#define GSQL_DEFAULT_ERRMSG NULL

static int database_environment_level = 0;

/* *************************************************************** */
/*								   */
/*	      Growable stacks for databases and cursors		   */
/*								   */
/* *************************************************************** */

/* A stack which is implemented with a growable array. */
typedef struct
{
  void **data;
  int index;
  int slots;
} IStack;

/* A function which returns void and takes a pointer to void. */
typedef void FREEFUN (void *);

static IStack *
make_istack (void)
{
  register int i;

  IStack *stack = (IStack *) xmalloc (sizeof (IStack));
  stack->slots = 10;
  stack->index = 0;
  stack->data = (void **) xmalloc (sizeof (void *) * stack->slots);

  for (i = 0; i < stack->slots; i++)
    stack->data[i] = (void *)NULL;

  return (stack);
}

/* Returns the index of the pushed item. */
static int
istack_push (IStack *stack, void *data)
{
  if (stack->index + 2 > stack->slots)
    stack->data = (void **)xrealloc
      (stack->data, ((stack->slots += 5) * sizeof (void *)));

  stack->data[stack->index++] = data;
  stack->data[stack->index] = (void *)NULL;
  return (stack->index - 1);
}

static void *
istack_aref (IStack *stack, int i)
{
  if ((i > -1) && (i < stack->index))
    return (stack->data[i]);
  else
    return ((void *)NULL);
}

static void *
istack_top (IStack *stack)
{
  void *data = (void *)NULL;
  if (stack->index > 0)
    {
      data = stack->data[(stack->index)-1];
    }
  return (data);
}

static void *
istack_pop (IStack *stack)
{
  void *data = (void *)NULL;

  if (stack->index > 0)
    {
      data = stack->data[--(stack->index)];
      stack->data[stack->index] = (void *)NULL;
    }

  return (data);
}

/* This iterates over the elements of stack, calling FREE_FUN on each one.
   Then it frees the stack itself. */
static void
istack_free (IStack *stack, FREEFUN *free_fun)
{
  register int i;

  for (i = 0; i < stack->index; i++)
    (*free_fun) (stack->data[i]);

  free (stack->data);
  free (stack);
}

#if !defined (COMPILING_ODBCFUNCS) && !defined (COMPILING_ODBCPERFUNCS) && !defined (COMPILING_IODBCFUNCS) && !defined (COMPILING_IODBCPERFUNCS)
/* Look up a key-value pair in a ODBC style DSN string.

   Example:
       HOST=star;SVT=Informix 5;UID=demo;PWD=demo;DATABASE=stores5 */
static char *
dsn_lookup (char *key, char *str)
{
  char *dsn = strdup (str);
  char *kp = dsn;
  char *vp = (char *)NULL;
  char *val = (char *)NULL;
  int found = 0;

  while ((vp = strchr (kp, '=')) != (char *) NULL)
    {
      char *semi;

      *vp = 0;
      vp++;

      /* Trim off semicolon separator, if there is one */
      semi = strchr (vp, ';');
      if (semi != (char *)NULL)
	*semi = 0;

      if (strcasecmp (kp, key) == 0)
	{
	  found = 1;
	  break;
	}

      if (semi == (char *)NULL)
	break;
      else
	kp = ++semi;
    }

  if (found == 1)
    val = strdup (vp);
  else
    val = (char *)NULL;

  free (dsn);
  return (val);
}
#endif /* !ODBC */

#if defined (COMPILING_MSQLFUNCS)
#  include "msqlfuncs.c"
#endif

#if defined (COMPILING_MYSQLFUNCS)
#  define CACHED_GSQL_LIST_FIELDS
#  include "mysqlfuncs.c"
#endif

#if defined (COMPILING_MYSQLPERFUNCS)
#  define CACHED_GSQL_LIST_FIELDS
#  include "mysqlperfuncs.c"
#endif

#if defined (COMPILING_PGSQLFUNCS)
#  include "pgsqlfuncs.c"
#endif

#if defined (COMPILING_ODBCFUNCS)
#  define CACHED_GSQL_LIST_FIELDS
#  include "odbcfuncs.c"
#endif

#if defined (COMPILING_ODBCPERFUNCS)
#  define CACHED_GSQL_LIST_FIELDS
#  include "odbcperfuncs.c"
#endif

#if defined (COMPILING_IODBCFUNCS)
#  define CACHED_GSQL_LIST_FIELDS
#  include "iodbcfuncs.c"
#endif

#if defined (COMPILING_IODBCPERFUNCS)
#  define CACHED_GSQL_LIST_FIELDS
#  include "iodbcperfuncs.c"
#endif

/* Parameters:
   odbc::sql-escape-character-code
   odbc::truncate-column-data
   */

/* *************************************************************** */
/*								   */
/*			Cursors and Databases			   */
/*								   */
/* *************************************************************** */

static void free_cursor (DBCursor *cursor);
static IStack *open_databases = (IStack *)NULL;

/* Initialize the database stack. */
static void
initialize_database_stack (void)
{
  open_databases = make_istack ();
}

/* Return non-zero if the database connection is active, 0 otherwise. */
static int
gsql_database_connected (Database *db)
{
  return db->connected;
}

/* Create and initialize a new database object. */
static Database *
make_database (void)
{
  Database *db = (Database *) xmalloc (sizeof (Database));
  db->cursors = make_istack ();
  initialize_database (db);
  return (db);
}

/* Install this database, and return the index which is used to identify it. */
static int
add_open_database (IStack *dbstack, Database *db)
{
  int result = istack_push (dbstack, (void *) db);
  return (result);
}

/* Get the database object associated with the index DB_INDEX. */
static Database *
get_database (int db_index)
{
  Database *db = (Database *) istack_aref (open_databases, db_index);
  return (db);
}

/* Pop each cursor off the cursor's istack, and free it.
   Then free the istack itself. */
static void
free_database_cursors (Database *db)
{
  if (db->cursors)
    istack_free (db->cursors, (FREEFUN *) free_cursor);
}

/* De-install this current db which is at the top of the stack.
   Free up any cursor resources, then free the Database object.

   Note: This does not close the database connection. That is
   only done by unlock_database. */
static void
pop_database (IStack *dbstack)
{
  Database *database = (Database *) istack_pop (dbstack);

  if (database != (Database *) NULL)
    {
      /* Free all the cursors, name strings, etc. */
      free_database_resources (database);
      free (database);
    }
}

/* For a given open database, a list of cursors is kept.  A cursor is
   encoded as a 16 bit integer whose high byte indexes into the database
   list, and whose low byte indexes into the list of cursors for that
   database. */
static DBCursor *
make_cursor (Database *db)
{
  DBCursor *cursor = (DBCursor *) xmalloc (sizeof (DBCursor));
  cursor->db = db;
  return (cursor);
}

static void
free_cursor (DBCursor *cursor)
{
  if (cursor && cursor->result)
    gsql_free_result (cursor->result);
}

static DBCursor *
lookup_cursor_ref (char *cursor_ref)
{
  Database *db;
  int cursor_token = -1;		/* The combined db::cursor ids */
  int cursor_id;
  int db_id;

  /* Extract the token from the variable string value. */
  if (!empty_string_p (cursor_ref))
    {
      char *rep = pagefunc_get_variable (cursor_ref);
      char *end;

      if (!empty_string_p (rep))
	{
	  /* Get high byte of the token */
	  cursor_token = (int)strtol (rep, &end, 16);
	  if (*end != '\0')
	    cursor_token = -1;
	}
    }

  if (cursor_token == -1)
    return ((DBCursor *) NULL);

  db_id = (cursor_token & 0xFF00) >> 8;
  cursor_id = (cursor_token & 0xFF);

  db = get_database (db_id);

  if (db != (Database *) NULL)
    {
      /* OK, we found an open database.  Now find the cursor in
	 that database's cursor list. */
      return ((DBCursor *)istack_aref (db->cursors, cursor_id));
    }
  else
    return ((DBCursor *) NULL);
}


/* Dereference a Meta-HTML variable which indexes a cursor object.
   Takes the high byte, and computes an index into the open databases,
   and then uses the low byte to index into that database's cursor list. */
static DBCursor *
get_cursor_ref (Package *vars)
{
  DBCursor *cursor;
  char *cursor_ref = mhtml_evaluate_string (get_positional_arg (vars, 0));
  cursor = lookup_cursor_ref (cursor_ref);
  xfree (cursor_ref);
  return cursor;
}


/* *************************************************************** */
/*								   */
/*		     Database Manipulation Functions		   */
/*								   */
/* *************************************************************** */

/* CHAR type will be anything which is not explicitly numeric */ 
#define CHARTYPE(ftype) ((! (gsql_field_type (ftype) == GSQL_NUMERIC)) \
			 && (! (gsql_field_type (ftype) == GSQL_DECIMAL)) \
			 && (! (gsql_field_type (ftype) == GSQL_INTEGER)) \
			 && (! (gsql_field_type (ftype) == GSQL_UINT)) \
			 && (! (gsql_field_type (ftype) == GSQL_SMALLINT)) \
			 && (! (gsql_field_type (ftype) == GSQL_FLOAT)) \
			 && (! (gsql_field_type (ftype) == GSQL_REAL)) \
			 && (! (gsql_field_type (ftype) == GSQL_DOUBLE)))

#define BLOBTYPE(field) ((gsql_field_type (field) == GSQL_TINY_BLOB) || \
			 (gsql_field_type (field) == GSQL_MEDIUM_BLOB) || \
			 (gsql_field_type (field) == GSQL_LONG_BLOB) || \
			 (gsql_field_type (field) == GSQL_BLOB))

/* We pass a string which contains a decimal representation of the
   SQL escape code character, such as "39" */
static void
gsql_database_set_sql_escape_character (Database *db, char *escape_code)
{
  char sql_escape_char;

  if (escape_code)
    sql_escape_char = (char) atoi (escape_code);
  else
    sql_escape_char = DEFAULT_SQL_ESCAPE_CHARACTER;

  db->sql_escape_char = sql_escape_char;
}

static char
gsql_database_sql_escape_character (Database *db)
{
  return (db->sql_escape_char);
}

/* If this variable is set, automatically truncate strings to max
   column width in queries composed by save-record/save-package
   */
static void
gsql_database_set_truncate_columns (Database *db, char *value)
{
  int truncate_p = DEFAULT_SQL_TRUNCATE_COLUMNS;

  if (empty_string_p (value))
    truncate_p = 0;
  else
    truncate_p = 1;

  db->sql_truncate_columns = truncate_p;
}

static int
gsql_database_sql_truncate_columns (Database *db)
{
  return (db->sql_truncate_columns);
}

/* If this variable is set, then database-query and next-record will
   prefix column variables from the result set with the table to which
   the column belongs.  */
static void
gsql_database_set_prefix_tablenames (Database *db, char *value)
{
  int prefix_p;

  if (!empty_string_p (value))
    prefix_p = 1;
  else
    prefix_p = 0;

  db->sql_prefix_tablenames = prefix_p;
}

static int
gsql_database_sql_prefix_tablenames (Database *db)
{
  return (db->sql_prefix_tablenames);
}

static void
lock_database (char *dsn, int *lock, Database *db, int lock_p)
{
  char *lockname = db_lockname (dsn);
  int fd;

  *lock = -1;
  db->connected = 0;

  if (lockname == (char *)NULL)
    return;

  if (lock_p)
    {
      fd = os_open (lockname, O_CREAT | O_WRONLY | O_APPEND, 0666);

      if ((fd < 0) || (LOCKFILE (fd) == -1))
	{
	  if (fd >= 0)
	    {
	      char pid_name[100];
	      sprintf (pid_name, "%ld\n", (long)getpid ());
	      write (fd, (void *)pid_name, (size_t) strlen (pid_name));
	      close (fd);
	    }
	  return;
	}
      else
	{
	  gsql_connect (dsn, db);

	  if (db->connected != 1)
	    {
	      unlink (lockname);
	      UNLOCKFILE (fd);
	      close (fd);
	    }
	  else
	    {
	      *lock = fd;
	    }
	}
    }
  else
    {
      gsql_connect (dsn, db);
    }
}

/* Release the (exclusive?) lock associated with DSN, LOCK and DB. */
static void
unlock_database (char *dsn, int *lock, Database *db, int lock_p)
{
  if (db != (Database *)NULL)
    gsql_close (db);

  if (lock_p && (*lock > -1))
    {
      char *lockname = db_lockname (dsn);
      char *unlockname = (char *)NULL;

      if (lockname != (char *)NULL)
	{
	  unlockname = (char *)xmalloc (10 + strlen (lockname));
	  strcpy (unlockname, lockname);
	  strcat (unlockname, ".unlock");
	  rename (lockname, unlockname);
	}

      UNLOCKFILE (*lock);

      if (unlockname != (char *)NULL)
	{
	  unlink (unlockname);
	  free (unlockname);
	}

      close (*lock);
    }
}

/* Translates a Meta-HTML db token (a small int) into an index into
   the table of open databases, which returns a Database object.

   The Database object contains some db-specific connection state. */
static Database *
get_dbref_internal (Package *vars, int *dbindex)
{
  char *dbref = mhtml_evaluate_string (get_positional_arg (vars, 0));
  Database *db = (Database *)NULL;

  *dbindex = -1;

  if (!empty_string_p (dbref))
    {
      char *rep = pagefunc_get_variable (dbref);
      char *end;

      if (!empty_string_p (rep))
	{
	  *dbindex = strtol (rep, &end, 16);
	  if (*end != '\0')
	    *dbindex = -1;
	}
    }

  xfree (dbref);

  if (*dbindex != -1)
    db = get_database (*dbindex);

  return db;
}

static Database *
get_dbref (Package *vars)
{
  int dbindex;
  return (get_dbref_internal (vars, &dbindex));
}

static void
make_db_error (Database *db, char *format, ...)
{
  BPRINTF_BUFFER *b = bprintf_create_buffer ();
  va_list args;
  va_start (args, format);

  b->bindex = 0;
  vbprintf (b, format, args);
  va_end (args);
  gsql_save_error_message (db, b->buffer);
  bprintf_free_buffer (b);
}

static void
pf_with_open_database (PFunArgs)
{
  char *varname  = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *dsn      = mhtml_evaluate_string (get_value (vars, "DSN"));
  char *dbname   = mhtml_evaluate_string (get_value (vars, "DATABASE"));
  char *hostname = mhtml_evaluate_string (get_value (vars, "HOST"));
  char *nolock   = mhtml_evaluate_string (get_value (vars, "NOLOCK"));
  int jump_again = 0;

  if (!open_databases) initialize_database_stack ();

  /* No errors yet! */
  gsql_clear_error_message ();

  if (empty_string_p (hostname))
    {
      xfree (hostname);
      hostname = strdup ("localhost");
    }

  /* If the user supplied a DSN, use it, otherwise make one out of
     the hostname and dbname. */

  if ((empty_string_p (dsn)) && (!empty_string_p (dbname)))
    {
      BPRINTF_BUFFER *service = bprintf_create_buffer ();

      bprintf (service, "HOST=%s;DATABASE=%s", hostname, dbname);
      xfree (dsn);
      dsn = strdup (service->buffer);
      bprintf_free_buffer (service);
    }

  if ((!empty_string_p (varname)) && (!empty_string_p (dsn)))
    {
      Database *db = make_database ();
      int lock;
#if defined (COMPILING_MYSQLPERFUNCS)
      int lock_p = 0;
#else
      int lock_p = 1;
#endif

      if (!empty_string_p (nolock))
	lock_p = 0;

      lock_database (dsn, &lock, db, lock_p);

      if (db->connected == 1)
	{
	  char dbvalue[40];
	  PAGE *body_code = page_copy_page (body);
	  int dbindex;

	  /* Encode pointer to this database as an index into the table
	     of open databases. */
	  dbindex = add_open_database (open_databases, db);
	  sprintf (dbvalue, "%x", dbindex);

	  pagefunc_set_variable (varname, dbvalue);

	  {
	    PageEnv *page_environ;

	    page_environ = pagefunc_save_environment ();
	    database_environment_level++;

	    if ((jump_again = setjmp (page_jmp_buffer)) == 0)
	      page_process_page_internal ((PAGE *)body_code);

	    database_environment_level--;
	    pagefunc_restore_environment (page_environ);

	    if (jump_again && (body_code != (PAGE *)NULL))
	      {
		page_free_page ((PAGE *)body_code);
		body_code = (PAGE *)NULL;
	      }
	  }

	  if (body_code != (PAGE *)NULL)
	    {
	      if (body_code->buffer != (char *)NULL)
		{
		  bprintf_insert (page, start, "%s", body_code->buffer);
		  *newstart = start + (body_code->bindex);
		}

	      page_free_page ((PAGE *)body_code);
	    }
	}
      else
	{
	  gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	}

      unlock_database (dsn, &lock, db, lock_p);
      pop_database (open_databases);
    }

  xfree (nolock);
  xfree (dbname);
  xfree (varname);
  xfree (hostname);
  xfree (dsn);

  if (jump_again)
    longjmp (page_jmp_buffer, 1);
}

/* <database-exec-sql dbvar query-string>
   Executes an SQL query, within the scope of with-open-database.

   returns TRUE if no error (GSQL_ERROR), NULL otherwise. */
static void
pf_database_exec_sql (PFunArgs)
{
  int dbindex;
  gsql_result *query_result = (gsql_result *)NULL;

  if (database_environment_level != 0)
    {
      Database *db = get_dbref_internal (vars, &dbindex);

      if (db != (Database *)NULL)
	{
	  char *dbquery = mhtml_evaluate_string (get_value (vars, "query"));

	  if (dbquery == (char *)NULL)
	    dbquery = mhtml_evaluate_string (get_positional_arg (vars, 1));

	  if (dbquery != (char *)NULL)
	    {
	      if (gsql_query (db, dbquery, 0) == GSQL_ERROR)
		{
		  gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
		}
	      else
		{
		  bprintf_insert (page, start, "true");
		  *newstart += 4;
		}

	      query_result = gsql_store_result (db);

	      if (query_result != (gsql_result *)NULL)
		{
		  gsql_fetch_row (query_result);
		  gsql_free_result (query_result);
		}
	      
	      xfree (dbquery);
	    }
	}
    }
}

/* <database-exec-query dbvar query-string [cursor=cursor]>
   Executes an SQL query, within the scope of with-open-database.

   Returns a CURSOR object.

   If a cursor is passed in the cursor attribute, it is re-used,
   rather than a new one being allocated. */
static void
pf_database_exec_query (PFunArgs)
{
  gsql_result *query_result;
  int dbindex;

  if (database_environment_level != 0)
    {
      Database *db = get_dbref_internal (vars, &dbindex);

      if (db != (Database *)NULL)
	{
	  char *dbquery = mhtml_evaluate_string (get_value (vars, "query"));

	  if (dbquery == (char *)NULL)
	    dbquery = mhtml_evaluate_string (get_positional_arg (vars, 1));

	  if (dbquery != (char *)NULL)
	    {
	      if (gsql_query (db, dbquery, 0) == GSQL_ERROR)
		{
		  gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
		}
	      else
		{
		  DBCursor *cursor;
		  int cursor_index;
		  int cursor_token;
		  char cursor_string[16];
		  char *cursorvar;

		  query_result = gsql_store_result (db);

		  /* If the used passed in a valid cursor,
		     we free its resources and re-use it. Otherwise we
		     cons up a new one.
		     */
		  cursorvar =
		    mhtml_evaluate_string (get_value (vars, "cursor"));

		  cursor = lookup_cursor_ref (cursorvar);

		  if (cursor == (DBCursor *)NULL)
		    {
		      /* Make a cursor for this result */
		      cursor = make_cursor (db);
		      cursor->result = query_result;

		      /* Push the cursor on the databases's cursor stack. */
		      cursor_index = istack_push
			(db->cursors, (void *) cursor);

		      /* Save the stack index of the cursor, so we can
			 reference it in the stack if we need to. */
		      cursor->index = cursor_index;
		    }
		  else
		    {
		      /* The cursor already exists, so free its resources
			 (previously allocated db-specific HSTMT or cursors)
			 and then store the newly allocated ones.*/
		      gsql_free_result (cursor->result);
		      cursor->result = query_result;
		    }

		  /* Make a token of db::cursor id and print it. */
		  cursor_token =
		    ((dbindex & 0xff) << 8) | (cursor->index & 0xff);

		  sprintf (cursor_string, "0X%04X", cursor_token);

		  if (!empty_string_p (cursorvar))
		    {
		      pagefunc_set_variable (cursorvar, cursor_string);
		    }
		  else
		    {
		      bprintf_insert (page, start, "%s", cursor_string);
		      *newstart += strlen (cursor_string);
		    }
		  xfree (cursorvar);
		}

	      xfree (dbquery);
	    }
	}
    }
}

/* Returns the number of rows which were returned from the last query.
   This operates on a CURSOR object. */
static void
pf_database_num_rows (PFunArgs)
{
  DBCursor *cursor = get_cursor_ref (vars);

  if ((cursor != (DBCursor *)NULL) && (cursor->result != (gsql_result *)NULL))
    {
      int nrows = gsql_number_of_rows (cursor->result);

      bprintf_insert (page, start, "%d", nrows);
    }
}

static void
pf_database_affected_rows (PFunArgs)
{
  DBCursor *cursor = get_cursor_ref (vars);

  if (cursor != (DBCursor *)NULL)
    {
      int nrows = gsql_affected_rows (cursor);
      bprintf_insert (page, start, "%d", nrows);
    }
}

/* Set the current row number position in the result set from the last query.
   This operates on a CURSOR object. */
static void
pf_database_set_pos (PFunArgs)
{
  DBCursor *cursor = get_cursor_ref (vars);
  long position = -1;

  if (cursor != (DBCursor *)NULL)
    {
      char *index_ref =  mhtml_evaluate_string (get_positional_arg (vars, 1));

      /* Get the position argument. */
      if (!empty_string_p (index_ref))
	{
	  char *rep = index_ref;
	  char *ends;

	  if (!empty_string_p (rep))
	    {
	      /* Get high byte of the token */
	      position = strtol (rep, &ends, 10);
	      if (*ends != '\0')
		{
		  if (debug_level)
		    page_debug ("Invalid position in set-row-position");
		  position = -1;
		}
	    }
	}

      xfree (index_ref);

      if ((position > -1) && (cursor->result != (gsql_result *)NULL))
	gsql_data_seek (cursor->result, position);
    }
}

/* <cursor-get-column cursor column-number>
   This should be called after a call to database-exec-query.
   It gets the column numbered colnum (starting at column 0).  */
static void
pf_cursor_get_column (PFunArgs)
{
  char *colnum = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((database_environment_level != 0) && !(empty_string_p (colnum)))
    {
      DBCursor *cursor = get_cursor_ref (vars);

      if (cursor != (DBCursor *) NULL)
	{
	  gsql_result *query_result = cursor->result;

	  if (gsql_database_connected (cursor->db) &&
	      (query_result != (gsql_result *)NULL))
	    {
	      int col = atoi (colnum);
	      char *value = gsql_get_column (query_result, col);
	      int len = value ? strlen (value) : 0;

	      if (len)
		{
		  bprintf_insert (page, start, value);
		  *newstart += len;
		}
	      xfree (value);
	    }
	}
    }
  xfree (colnum);
}

/* This is a global variable bound to the currently executing query
   result into a <database-query> form. */
static gsql_result *g_current_query_result;

/* <query-get-column column-number>
   This can only be called inside the scope of database-query.
   It gets value of the SQL column in the current result set,
   where column-number starts at 0.  */
static void
pf_query_get_column (PFunArgs)
{
  char *colnum = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((database_environment_level != 0) && !(empty_string_p (colnum)))
    {
      Database *db = (Database *)istack_top (open_databases);

      if ((db != (Database *) NULL) &&
	  ((gsql_database_connected (db)) &&
	   (g_current_query_result != (gsql_result *)NULL)))
	{
	  int col = atoi (colnum);
	  char *value = gsql_get_column (g_current_query_result, col);
	  int len = value ? strlen (value) : 0;

	  if (len)
	    {
	      bprintf_insert (page, start, value);
	      *newstart += len;
	    }
	  xfree (value);
	}
    }
  xfree (colnum);
}

/* Free the individual pointers in ARRAY, and the ARRAY itself. */
static void
free_array (char **array)
{
  if (array != (char **)NULL)
    {
      register int i;

      for (i = 0; array[i] != (char *)NULL; i++)
	free (array[i]);

      free (array);
    }
}

/* Split TEXT into an array of words.  The boundaries are whitespace
   and commas. Returns the number of words found in NCOLS.*/
static char **
split_colnames (char *text, int *ncols)
{
  char **result = (char **)NULL;
  int result_slots = 0;
  int result_index = 0;

  if (text != (char *)NULL)
    {
      register int i = 0;

      while (1)
	{
	  /* Skip leading separator characters. */
	  while ((whitespace (text[i])) || (text[i] == ',')) i++;

	  if (text[i] == '\0')
	    break;
	  else
	    {
	      int start = i;

	      while ((text[i] != '\0') &&
		     (!whitespace (text[i])) &&
		     (text[i] != ','))
		i++;

	      if (start < i)
		{
		  if (result_index + 2 > result_slots)
		    result = (char **)xrealloc
		      (result, (result_slots += 10) * sizeof (char *));

		  result[result_index] = (char *)xmalloc (1 + (i - start));
		  strncpy (result[result_index], text + start, i - start);
		  result[result_index][i - start] = '\0';
		  result_index++;
		  result[result_index] = (char *)NULL;
		}
	      else
		break;
	    }
	}
    }

  *ncols = result_index;
  return (result);
}

static char *alist_of_field_info (gsql_field *field, Database *db);

/* <database-query-info cursor [result=varname]> 

   Returns an array of alists of information about the columns of the
   given cursor's query result set. */
static void
pf_database_query_info (PFunArgs)
{
  if (database_environment_level != 0)
    {
      DBCursor *cursor = get_cursor_ref (vars);

      if (cursor != (DBCursor *) NULL)
	{
	  gsql_result *result = cursor->result;

	  if (gsql_database_connected (cursor->db) &&
	      (result != (gsql_result *)NULL))
	    {
	      int i;
	      int cols = gsql_num_fields (result);
	      char *varname;
	      char **colnames;

	      /* The name of the variable to stuff the results into. */
	      varname = mhtml_evaluate_string (get_value (vars, "RESULT"));
	      colnames = (char **)xmalloc ((cols + 1) * sizeof (char *));

	      /* Reset the field descriptor cursor to start of record */

	      for (i = 0; i < cols; i++)
		{
		  gsql_field *field = gsql_fetch_field (result, i);

		  colnames[i] = alist_of_field_info (field, cursor->db);
		}
	      colnames[i] = (char *) NULL;

	      if (!empty_string_p (varname))
		{
		  symbol_store_array (varname, colnames);
		}
	      else
		{
		  for (i = 0; i < cols; i++)
		    {
		      bprintf_insert (page, start, "%s\n", colnames[i]);
		      start += 1 + strlen (colnames[i]);
		      free (colnames[i]);
		    }
		  free (colnames);

		  *newstart = start;
		}

	    }
	}
    }
}

/* <database-next-record cursor [package=PKGNAME] [colnames=name1,name2.]
                                [prefixtablenames=true] [appendvars=true]>
   This should be called after a call to database-exec-query.
   It fetches the next result row, and binds all the fields of the
   result row to variables in PKGNAME if specified, or returns an alist
   of the variables if no PKGNAME is given.

   If prefixtablenames is non-null, or db->sql_prefix_tablenames is
   true, then for each column in the result set, use the column's
   table name, if it exists, as a prefix to the column name as the
   variable name.

   If colnames is supplied, then the column values are bound
   sequentially to these names instead of the field names in the
   result set.

   If appendvars is supplied, then values are appended to variables,
   to form an array, rather than overwriting the previous values.

   Return "true" if successful, or "" if not. */
static void
pf_database_next_record (PFunArgs)
{
  if (database_environment_level != 0)
    {
      DBCursor *cursor = get_cursor_ref (vars);

      if (cursor != (DBCursor *) NULL)
	{
	  gsql_result *query_result = cursor->result;

	  if (gsql_database_connected (cursor->db) &&
	      (query_result != (gsql_result *)NULL) &&
	      (gsql_fetch_row (query_result) == GSQL_SUCCESS))
	    {
	      register int i, limit = gsql_num_fields (query_result);
	      char *pkgname = mhtml_evaluate_string
		(get_one_of (vars, "PACKAGE", "PREFIX", (char *)NULL));
	      char *prefixtablenames =
		mhtml_evaluate_string (get_value (vars, "prefixtablenames"));
	      char *appendvars =
		mhtml_evaluate_string (get_value (vars, "appendvars"));
	      char **fieldnames = (char **)NULL;
	      char *prev_fieldname = (char *) NULL;
	      char *fieldname= (char *) NULL;
	      int found_next = 0;
	      int ncolnames = 0;
	      Package *package;

	      if (empty_string_p (pkgname))
		{
		  /* The results should show up in an alist. */
		  package = symbol_get_package ((char *)NULL);
		}
	      else
		{
		  package = symbol_get_package (pkgname);
		}

	      {
		char *t = mhtml_evaluate_string (get_value (vars, "COLNAMES"));

		if (!empty_string_p (t))
		  fieldnames = split_colnames (t, &ncolnames);

		xfree (t);

		/* If the user does not want variables to have their
		   appended values persist across calls to next-record,
		   then we loop here to null out all the symbols which
		   are going to be set in this result row. */
		if (empty_string_p (appendvars))
		  {
		    for (i = 0; i < limit; i++)
		      {
			gsql_field *field = gsql_fetch_field
			  (query_result, i);
			char buf[256];
			Symbol *sym;

			/* If the user supplied column names, use them. */
			if (fieldnames != (char **)NULL)
			  {
			    if (i < ncolnames)
			      fieldname = fieldnames[i];
			    else
			      fieldname = (char *) NULL;
			  }
			else
			  fieldname = gsql_field_name (field);

			if (empty_string_p (fieldname))
			  break;
			else
			  {
			    if (gsql_field_table (field) &&
				(gsql_database_sql_prefix_tablenames
				 (cursor->db)
				 || !(empty_string_p (prefixtablenames))))
			      {
				strcpy (buf, gsql_field_table (field));
				strcat (buf, ".");
				strcat (buf, fieldname);
			      }
			    else
			      {
				strcpy (buf, fieldname);
			      }

			    sym = symbol_intern_in_package (package, buf);
			    symbol_reset (sym);
			  }
		      }
		  }

		/* Loop over fields of the row, binding variables.
		   All data is already coerced to strings, so we
		   do not do any type conversion. */
		for (i = 0; i < limit; i++)
		  {
		    gsql_field *field = gsql_fetch_field (query_result, i);
		    char buf[256];
		    Symbol *sym;

		    if (!empty_string_p (fieldname))
		      prev_fieldname = fieldname;

		    /* If the user supplied column names, use them. */
		    if (fieldnames != (char **)NULL)
		      {
			if (i < ncolnames)
			  fieldname = fieldnames[i];
			else
			  fieldname = (char *) NULL;
		      }
		    else
		      fieldname = gsql_field_name (field);
		      
		    if (empty_string_p (fieldname) &&
			empty_string_p (prev_fieldname))
		      break;
		    else
		      {
			char *value;
			found_next++;
			  
			if (empty_string_p (fieldname))
			  fieldname = prev_fieldname;

			if (gsql_field_table (field) &&
			    (gsql_database_sql_prefix_tablenames (cursor->db)
			     || !(empty_string_p (prefixtablenames))))
			  {
			    strcpy (buf, gsql_field_table (field));
			    strcat (buf, ".");
			    strcat (buf, fieldname);
			  }
			else
			  {
			    strcpy (buf, fieldname);
			  }

			sym = symbol_intern_in_package (package, buf);
			value = gsql_get_column (query_result, i);
			symbol_add_value (sym, value ? value : "");

			xfree (value);
		      }
		  }
	      }

	      if (found_next)
		{
		  if (empty_string_p (pkgname))
		    {
		      char *alist = package_to_alist (package, 1);

		      if (alist != (char *)NULL)
			{
			  bprintf_insert (page, start, "%s", alist);
			  *newstart += strlen (alist);
			  free (alist);
			}

		      symbol_destroy_package (package);
		    }
		  else
		    {
		      bprintf_insert (page, start, "true");
		      *newstart += 4;
		    }
		}

	      free_array (fieldnames);
	      xfree (pkgname);
	      xfree (prefixtablenames);
	      xfree (appendvars);
	    }
	}
    }
}

/*  <database-set-options dbvar keyword=value keyword=value ...>

    supported options are:

    SQL-ESCAPE-CHARACTER-CODE: Sets the escape character used to quote
    single quotes in SQL strings.

    SQL-TRUNCATE-COLUMNS: For database-save-record and
    database-save-package, automatically truncate data strings which
    are passed to the INSERT or UPDATE statements to the max field
    length for each column.

    SQL-PREFIX-TABLENAMES: For database-next-record and
    database-query, prefix the names of symbols for column of the
    result set with the name table to which the column belongs. */
static void
pf_database_set_options (PFunArgs)
{
  Database *db = get_dbref (vars);
  char *escape_code =
    mhtml_evaluate_string (get_value (vars, "SQL-ESCAPE-CHARACTER-CODE"));
  char *truncate_columns =
    mhtml_evaluate_string (get_value (vars, "SQL-TRUNCATE-COLUMNS"));
  char *prefix_tablenames =
    mhtml_evaluate_string (get_value (vars, "SQL-PREFIX-TABLENAMES"));

  if (db != (Database *) NULL)
    {
      if (symbol_lookup_in_package (vars, "SQL-ESCAPE-CHARACTER-CODE"))
	gsql_database_set_sql_escape_character (db, escape_code);

      if (symbol_lookup_in_package (vars, "SQL-TRUNCATE-COLUMNS"))
	gsql_database_set_truncate_columns (db, truncate_columns);

      if (symbol_lookup_in_package (vars, "SQL-PREFIX-TABLENAMES"))
	gsql_database_set_prefix_tablenames (db, prefix_tablenames);
    }

  xfree (escape_code);
  xfree (truncate_columns);
  xfree (prefix_tablenames);
}

/* List all the table names in this database. This must be done
   with a database already open, i.e., within the scope of a
   <with-open-database>.

   Args: <odbc::database-tables dbvar [result=VAR] 
            [tablequalifier=VAL]
	    [tableowner=VAL]
	    [tablename=VAL]
	    [tabletype=VAL]>

   Returns resulting array, or places it into VAR if supplied. */
static void
pf_database_tables (PFunArgs)
{
  char *qualifier = mhtml_evaluate_string (get_value (vars, "tablequalifier"));
  char *owner     = mhtml_evaluate_string (get_value (vars, "tableowner"));
  char *name      = mhtml_evaluate_string (get_value (vars, "tablename"));
  char *type      = mhtml_evaluate_string (get_value (vars, "tabletype"));

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  char *resultvar = mhtml_evaluate_string (get_value (vars, "RESULT"));
	  gsql_result *result = gsql_db_list_tables
	                            (db, qualifier, owner,name, type);

	  if (result != (gsql_result *)NULL)
	    {
	      int arraysize = 10;
	      char **tablenames;
	      int count = 0;

	      tablenames = (char **)
		xmalloc ((1 + arraysize) * sizeof (char *));

	      /* Loop over rows returned; the table name will be
		 passed in the first field (column 0) of each 'row'.
		 Add names to the result array.  */
	      while ((gsql_fetch_row (result)) == GSQL_SUCCESS)
		{
		  if (count + 2 > arraysize)
		    tablenames = (char **)xrealloc
		      (tablenames, ((arraysize += 5) * sizeof (char *)));

		  tablenames[count++] =
		    gsql_get_column_table_name (result);
		}

	      tablenames[count] = (char *)NULL;

	      if (!empty_string_p (resultvar))
		{
		  symbol_store_array (resultvar, tablenames);
		}
	      else
		{
		  register int i;

		  for (i = 0; tablenames[i] != (char *)NULL; i++)
		    {
		      bprintf_insert (page, start, "%s\n", tablenames[i]);
		      start += 1 + strlen (tablenames[i]);
		      free (tablenames[i]);
		    }
		  free (tablenames);

		  *newstart = start;
		}
	      gsql_free_result (result);
	    }

	  xfree (resultvar);
	}
    }
}

/* List all the fieldnames for a table. This must be done
   with a database already open, i.e., within the scope of a
   <msql::with-open-database>.

   Args: <msql::database-columns dbvar tablename [result=VAR]>
   Returns an array of column names, or sets the value of VAR
   if supplied. */
static void
pf_database_columns (PFunArgs)
{
  /* No errors yet! */
  gsql_clear_error_message ();

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  char *tablename;

	  /* The name of the table we are enquiring about. */
	  tablename = mhtml_evaluate_string (get_positional_arg (vars, 1));

	  if (!empty_string_p (tablename))
	    {
	      gsql_result *result = gsql_list_fields (db, tablename);

	      if (result != (gsql_result *)NULL)
		{
		  register int i;
		  int cols = gsql_num_fields (result);
		  char *varname;
		  char **colnames;

		  /* The name of the variable to stuff the results into. */
		  varname = mhtml_evaluate_string (get_value (vars, "RESULT"));
		  colnames = (char **)xmalloc ((cols + 1) * sizeof (char *));

		  /* Reset the field descriptor cursor to start of record */

		  for (i = 0; i < cols; i++)
		    {
		      gsql_field *field = gsql_fetch_field (result, i);

		      colnames[i] = strdup (gsql_field_name (field));
		    }
		  colnames[i] = (char *) NULL;

		  if (!empty_string_p (varname))
		    {
		      symbol_store_array (varname, colnames);
		    }
		  else
		    {
		      for (i = 0; i < cols; i++)
			{
			  bprintf_insert (page, start, "%s\n", colnames[i]);
			  start += 1 + strlen (colnames[i]);
			  free (colnames[i]);
			}
		      free (colnames);

		      *newstart = start;
		    }

		  xfree (varname);

#if !defined (CACHED_GSQL_LIST_FIELDS)
		  gsql_free_result (result);
#endif
		}
	      else
		gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);

	    }
	  xfree (tablename);
	}
    }
}

/* Convert a gsql_result struct into a meta-html package.

   If colnames[] is not null, read sequential column names from it
   instead of from the result set. colnames[] is a null terminated
   list of column names.

   If prefixtablenames is non-null, or db->sql_prefix_tablenames is
   true, then use variable names which are use the table name, if it
   exists, as a prefix to the column name for each column in the
   result set. */
static Package *
package_from_row (gsql_result *query_result,
		  char **colnames, int ncolnames,
		  int prefixtablenames)
{
  register int i;
  int num_fields = 0;
  Package *package = (Package *)NULL;
  char *prev_fieldname = (char *) NULL;
  char *fieldname = (char *) NULL;

  /* Loop over fields of the row, binding variables.  All results
     should already be strings, so we don't need to worry about
     converting INT, REAL, or other datatypes to strings. */
  num_fields = gsql_num_fields (query_result);

  for (i = 0; i < num_fields; i++)
    {
      char buf[256];
      gsql_field *field;
      char *value;
      Symbol *sym;

      field = gsql_fetch_field (query_result, i);

      if (!empty_string_p (fieldname))
	prev_fieldname = fieldname;

      /* If the user supplied column names, use them. */
      if (colnames != (char **)NULL)
	{
	  if (i < ncolnames)
	    fieldname = colnames[i];
	  else
	    fieldname = (char *) NULL;
	}
      else
	fieldname = gsql_field_name (field);

      if (empty_string_p (fieldname) &&
	  empty_string_p (prev_fieldname))
	break;
      else
	{
	  if (empty_string_p (fieldname))
	    fieldname = prev_fieldname;

	  if (prefixtablenames && gsql_field_table (field))
	    {
	      strcpy (buf, gsql_field_table (field));
	      strcat (buf, ".");
	      strcat (buf, fieldname);
	    }
	  else
	    {
	      strcpy (buf, fieldname);
	    }

	  value = gsql_get_column (query_result, i);
	  if (!package) package = symbol_get_package ((char *)NULL);
	  sym = symbol_intern_in_package (package, buf);
	  symbol_add_value (sym, value ? value : "");
	}

      xfree (value);
    }

  return (package);
}

/* <database-query db expr query=query-string [format=<expr>]
                        [keys=varname keyname=name]
			colnames="name1,name2,...">
   Select and optionally format records in the database according
   to the SQL query QUERY-STRING.   If the result of the query is
   not empty, and if FORMAT is present, it is an expression to evaluate
   in the context of the database fields.

   If a list of comma separated names is supplied as the COLNAMES
   argument, then these names will be used as the symbol names to
   which the column values are bound, in order of their appearance in
   the query.

   If you want to collect 'keys', you need to specify a variable to
   put them into, and a key field name collect. Technically there is nothing
   which distinguishes this field from any other, it is just that we
   need to specify *some* field to collect the values of. */
typedef struct
{
  char *key;
  Package *contents;
} DBRecord;

static void
pf_database_query (PFunArgs)
{
  Database *db = get_dbref (vars);

  if ((database_environment_level == 0) || (db == (Database *)NULL))
    return;
  else
    {
      register int i;
      char *query = mhtml_evaluate_string (get_value (vars, "QUERY"));
      char *keys_var = mhtml_evaluate_string (get_value (vars, "KEYS"));
      char *keyname = mhtml_evaluate_string
	(get_one_of (vars, "KEYNAME", "KEY", (char **)NULL));
      char *colnames = mhtml_evaluate_string (get_value (vars, "COLNAMES"));
      char *prefixtablenames =
	mhtml_evaluate_string (get_value (vars, "PrefixTableNames"));
      char **fieldnames = (char **)NULL;
      gsql_result *query_result = (gsql_result *)NULL;
      int window_start = 0;
      int window_length = 9999999;
      int window_requested = 0;

      {
	char *temp;

	temp = mhtml_evaluate_string (get_value (vars, "window-start"));
	if (!empty_string_p (temp))
	  {
	    window_requested = 1;
	    window_start = atoi (temp);
	  }
	xfree (temp);

	temp = mhtml_evaluate_string (get_value (vars, "window-length"));
	if (!empty_string_p (temp))
	  {
	    window_requested = 1;
	    window_length = atoi (temp);
	  }

	xfree (temp);
      }

      if (query == (char *)NULL)
	query = mhtml_evaluate_string (get_positional_arg (vars, 2));

      /* Execute the query. */
      if (!empty_string_p (query))
	{
	  int query_result_code;

#if defined (DATABASE_SUPPORTS_LIMIT_KEYWORD)
	  if (window_requested)
	    {
	      BPRINTF_BUFFER *temp_query = bprintf_create_buffer ();
	      bprintf (temp_query, "%s limit %d, %d",
		       query, window_start, window_length);
	      query_result_code = gsql_query (db, temp_query->buffer, 1);
	      bprintf_free_buffer (temp_query);
	    }
	  else
#endif
	    query_result_code = gsql_query (db, query, 1);

	  if (query_result_code == GSQL_SUCCESS)
	    {
	      query_result = gsql_store_result (db);
	      g_current_query_result = query_result;
	    }
	}

      /* If anything was found, deal with it now. */
      if (query_result != (gsql_result *)NULL)
	{
	  char *expr = get_positional_arg (vars, 1);
	  DBRecord **records = (DBRecord **)NULL;
	  int rec_slots = 0;
	  int rec_index = 0;
	  int rec_count = 0;
	  int ncolnames = 0;
	  int window_limit = window_length;

	  /* Optimize most common case. */
	  if ((expr != (char *)NULL) && (strcasecmp (expr, "true") == 0))
	    expr = (char *)NULL;

	  if (!empty_string_p (colnames))
	    fieldnames = split_colnames (colnames, &ncolnames);

#if !defined (DATABASE_SUPPORTS_LIMIT_KEYWORD)
	  /* Discard records until we get to the start of the window. */
	  {
	    if (window_start < 0) window_start = 0;
	    window_limit = window_start + window_length;

	    while (rec_count < window_start)
	      {
		int status = gsql_fetch_row (query_result);

		if ((status != GSQL_SUCCESS) &&
		    (status != GSQL_SUCCESS_WITH_INFO))
		  break;
		else
		  rec_count++;
	      }
	  }
#endif

	  while (rec_count < window_limit)
	    {
	      int status = gsql_fetch_row (query_result);
	      Package *db_fields = (Package *)NULL;

	      /* If at end of data, we're done with this loop. */
	      if ((status != GSQL_SUCCESS) &&
		  (status != GSQL_SUCCESS_WITH_INFO))
		break;
	      else
		rec_count++;

	      /* Otherwise, get the field data. */
	      db_fields = package_from_row
		(query_result, fieldnames, ncolnames,
		 (!(empty_string_p (prefixtablenames) ||
		    gsql_database_sql_prefix_tablenames (db))));

	      if (db_fields)
		{
		  char *expr_result = (char *)NULL;

		  /* If the user supplied an expression, evaluate it now, in
		     the context of this record. */
		  if (expr != (char *)NULL)
		    {
		      symbol_push_package (db_fields);
		      expr_result = mhtml_evaluate_string (expr);
		      symbol_pop_package ();
		    }

		  /* If satisfied, save this record. */
		  if ((expr == (char *)NULL) ||
		      (!empty_string_p (expr_result)))
		    {
		      DBRecord *rec = (DBRecord *)xmalloc (sizeof (DBRecord));

		      rec->key = (char *)NULL;
		      rec->contents = db_fields;

		      /* If a keyname field was specified, get its value. */
		      if (!empty_string_p (keyname))
			{
			  Symbol *sym;

			  sym = symbol_lookup_in_package (db_fields, keyname);
			  if (sym != (Symbol *)NULL)
			    rec->key = strdup (sym->values[0]);
			}

		      if (rec_index + 2 > rec_slots)
			records = (DBRecord **)xrealloc
			  (records, (rec_slots += 30) * sizeof (DBRecord *));

		      records[rec_index++] = rec;
		      records[rec_index] = (DBRecord *)NULL;
		    }
		  else
		    symbol_destroy_package (db_fields);
		  xfree (expr_result);
		}
	    }

	  /* Count the rest of the records? */
	  {
	    char *countvar = mhtml_evaluate_string (get_value (vars, "COUNT"));

	    if (!empty_string_p (countvar))
	      {
		char num[50];

		while (1)
		  {
		    int status = gsql_fetch_row (query_result);

		    if ((status != GSQL_SUCCESS) &&
			(status != GSQL_SUCCESS_WITH_INFO))
		      break;
		    else
		      rec_count++;
		  }

		sprintf (num, "%d", rec_count);
		pagefunc_set_variable (countvar, num);
	      }

	    xfree (countvar);
	  }

	  free_array (fieldnames);

	  /* If there are any matching records, then format, and/or
	     return the keys. */
	  if (rec_index != 0)
	    {
	      char *format_expr = get_value (vars, "format");

	      /* If there is a format operator, evaluate it now. */
	      if (format_expr != (char *)NULL)
		{
		  int format_limit = -1;
		  char *temp;
		  char *fl;

		  fl =
		    mhtml_evaluate_string (get_value (vars, "format-limit"));

		  if (!empty_string_p (fl))
		    {
		      format_limit = atoi (fl);
		      if (format_limit == 0) format_limit = -1;
		    }

		  xfree (fl);

		  for (i = 0; ((i < rec_index) &&
			       ((format_limit < 0) || (i < format_limit)));
		       i++)
		    {
		      symbol_push_package (records[i]->contents);
		      temp = mhtml_evaluate_string (format_expr);
		      symbol_pop_package ();

		      if (temp)
			{
			  bprintf_insert (page, start, "%s", temp);
			  start += strlen (temp);
			  free (temp);
			}
		    }
		}

	      /* We've processed every record.  If the caller has specified a
		 place to put the keys for this record, then do so now. */
	      if (!empty_string_p (keys_var))
		{
		  register int j;
		  Symbol *sym;
		  char **keys;
		  char *tname;

		  tname = strchr (keys_var, '[');
		  if (tname)
		    *tname = '\0';

		  keys = (char **) xmalloc ((1 + rec_index) * sizeof (char *));

		  for (i = j = 0; i < rec_index; i++)
		    {
		      if (records[i]->key != (char *) NULL)
			keys[j++] = strdup (records[i]->key);
		    }

		  keys[j] = (char *)NULL;

		  sym = symbol_remove (keys_var);
		  symbol_free (sym);
		  sym = symbol_intern (keys_var);
		  sym->values = keys;
		  sym->values_index = j;
		  sym->values_slots = rec_index;
		}
 	      else if (empty_string_p (format_expr))
 		{
 		  /* No variable specified to collect the keys, so simply
		     place the keys in the page. */
 		  for (i = 0; i < rec_index; i++)
 		    {
 		      if (records[i]->key != (char *)NULL)
 			{
 			  bprintf_insert (page, start,
					  "%s\n", records[i]->key);
 			  start += 1 + strlen (records[i]->key);
 			}
 		    }
 		  *newstart = start;
 		}

	      /* Finally, free the memory that we have used. */
	      for (i = 0; i < rec_index; i++)
		{
		  symbol_destroy_package (records[i]->contents);
		  xfree (records[i]->key);
		  xfree (records[i]);
		}
	      free (records);
	    }
	}

      if (query_result != (gsql_result *) NULL)
	gsql_free_result (query_result);

      g_current_query_result = (gsql_result *) NULL;

      xfree (query);
      xfree (keys_var);
      xfree (keyname);
      xfree (colnames);
      xfree (prefixtablenames);
    }
}

/* Quote single quotes for SQL string.
   Takes a field specifier, used to decide where to truncate the output
   to CHAR fields.

   If truncate_p is true, strings are truncated to the max field
   length for that column.

   If field == NULL, don't actually truncate anything. */
static void
bprintf_gsql_escape (BPRINTF_BUFFER *out, char *src, gsql_field *field,
		     char escape_code, int truncate_p)
{
  char *p = src;
  char c;
  int i = 0;
  int maxlength = 0;
  int fieldtype = GSQL_INTEGER;
  char escape_string[2];

  escape_string[0] = escape_code;
  escape_string[1] = (char) 0;

  /*  We only truncate character types. */
  if (field != (gsql_field *)NULL)
    {
      fieldtype = gsql_field_type (field);
      if ((CHARTYPE (field)) && (!BLOBTYPE (field)))
	maxlength = gsql_field_length (field);
    }

  if ((src == (char *) NULL) || (*src == '\0'))
    {
      /* If the src string is empty,
	 SQL fields with numeric types get NULL values. */
      if (!(CHARTYPE (field)))
	bprintf (out, "NULL");
      return;
    }

  while ((c = *p++) != 0)
    {
      i++;
      if ((truncate_p != 0) && ((maxlength != 0) && (i > maxlength)))
	return;

      /* Oracle doesn't need parens quoted, but mSQL and MySQL do. */
#if defined (COMPILING_MSQLFUNCS) || defined (COMPILING_MYSQLFUNCS)
      if ((c == '\'') || (c == '(') || (c == ')') || (c == escape_code))
	bprintf (out, "%s", escape_string);
#else
      if ((c == '\'') || (c == escape_code))
	bprintf (out, "%s", escape_string);
#endif

      bprintf (out, "%c",c);
    }
}

/* Do a case insensitive lookup of name in fields.
   Return a matching field, or NULL.  */
static gsql_field *
lookup_fieldname (char *name, gsql_result *result)
{
  register int i, num_fields;

#if 0
  static char namebuff[1024];
  /* We can't do this, because alists which are returned from the ...info
     commands would have different names than those which we allowed in the
     storage commands.  It is too confusing for the user, so we just don't
     do it. */

  /* Pleasant on the eye.  Allow dashes in the field name. */
  {
    char *temp;

    strcpy (namebuff, name);
    while ((temp = strchr (namebuff, '-')) != (char *)NULL) *temp = '_';
    name = namebuff;
  }
#endif

  num_fields = gsql_num_fields (result);

  for (i = 0; i < num_fields; i++)
    {
      gsql_field *field;

      field = gsql_fetch_field (result, i);
      if (strcasecmp (name, gsql_field_name (field)) == 0)
	return (field);
    }

  return ((gsql_field *)NULL);
}

/* <database-save-record db key var1 var2 ... varN
   table=table keyname=fieldname [method=insert|update]>

   Does an INSERT and UPDATE to try to store the data into table
   using single key keyname, with value key.

   If method is specified, then only one of INSERT or UPDATE
   is attempted.

   NOTE: If the key field for table you are using is not
   configured as the PRIMARY KEY, then you can get duplicate
   entries in the database.

   We use msqlListFields() to get a list of field names, so we
   can make sure to only try to set valid field names in the
   table.

   We also need the field type to decide whether to use single
   quotes around the data values (for char type). */

#define QUOTE_KEYVAL_IF_NEEDED \
	if ((gsql_field_type (keyfield) == GSQL_CHAR) || \
	    (gsql_field_type (keyfield) == GSQL_VARCHAR) || \
	    (gsql_field_type (keyfield) == GSQL_BLOB) || \
	    (gsql_field_type (keyfield) == GSQL_VAR_STRING) || \
	    (gsql_field_type (keyfield) == GSQL_STRING)) \
	  bprintf (query, "'")

#define QUOTE_VALUE_IF_NEEDED(field) \
	if ((gsql_field_type (field) == GSQL_CHAR) || \
	    (gsql_field_type (field) == GSQL_VARCHAR) || \
	    (gsql_field_type (field) == GSQL_VAR_STRING) || \
	    (gsql_field_type (field) == GSQL_BLOB) || \
	    (gsql_field_type (field) == GSQL_STRING)) \
	  bprintf (query, "'")

static void
pf_database_save_record (PFunArgs)
{
  Database *db = get_dbref (vars);
  char *key     = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *keyname = mhtml_evaluate_string
    (get_one_of (vars, "keyname", "key", (char *)NULL));
  char *table   = mhtml_evaluate_string (get_value (vars, "table"));
  char *method   = mhtml_evaluate_string (get_value (vars, "method"));
  BPRINTF_BUFFER *query = (BPRINTF_BUFFER *) NULL;
  char *result = (char *)NULL;
  gsql_result *table_fields = (gsql_result *) NULL;
  int items_printed = 0;
  int errors_found = 0;

  /* No errors yet! */
  gsql_clear_error_message ();

  if ((db != (Database *)NULL) &&
      (db->connected == 1) &&
      (database_environment_level != 0) &&
      !(empty_string_p (key)) &&
      !(empty_string_p (table)) &&
      !(empty_string_p (keyname)))
    {
      char *name;
      int position;
      int status;
      gsql_field *field = (gsql_field *)NULL;
      gsql_field *keyfield = (gsql_field *)NULL;
      char escape_char = gsql_database_sql_escape_character (db);
      int truncate_p = gsql_database_sql_truncate_columns (db);

      table_fields = gsql_list_fields (db, table);

      if (table_fields == (gsql_result *) NULL)
	{
	  gsql_save_error_message
	    (db, "table does not exist or has no columns");
	  errors_found++;
	}

      if (!errors_found)
	{
	  keyfield = lookup_fieldname (keyname, table_fields);
	  if (keyfield == (gsql_field *) NULL)
	    {
	      gsql_save_error_message
		(db, "primary key column name not found");
	      errors_found++;
	    }
	}

      /* If no method is specified, or if the method=insert,
	 then attempt to do an INSERT query. */
      if (!errors_found &&
	  (empty_string_p (method) || (strcasecmp (method, "INSERT") == 0)))
	{
	  /* Try to perform an SQL INSERT.  If that fails, do an UPDATE.
	     INSERT INTO tablename (var1, var2, ...) VALUES ('val1', 'val2'...)
	     We need to escape single quotes in the char values. */

	  query = bprintf_create_buffer ();
	  bprintf (query, "INSERT INTO %s (", table);
	  position = 2;

	  /* Comma-separated list of all the variable names, including key. */
	  bprintf (query, "%s", gsql_field_name (keyfield));

	  while ((name = get_positional_arg (vars, position)) != (char *)NULL)
	    {
	      Symbol *sym;
	      position++;

	      sym = symbol_lookup (name);
	      if (sym == (Symbol *)NULL) continue;

	      field = lookup_fieldname (sym->name, table_fields);

	      /* If the field exists in the table, print its name */
	      if  ((field != (gsql_field *) NULL) &&
		   (strcasecmp (gsql_field_name (field),
				gsql_field_name (keyfield)) != 0))
		{
		  bprintf (query, ",");
		  bprintf (query, "%s", gsql_field_name (field));
		}
	    }

	  bprintf (query, ") VALUES (");

	  QUOTE_KEYVAL_IF_NEEDED;
	  bprintf_gsql_escape (query, key, keyfield, escape_char, truncate_p);
	  QUOTE_KEYVAL_IF_NEEDED;

	  /* A comma-separated list of values, including the key value.
	     We check the field data type and use single-quotes if it
	     is a CHAR field. */
	  position = 2;
	  while ((name = get_positional_arg (vars, position)) != (char *)NULL)
	    {
	      char *value = pagefunc_get_variable (name);
	      Symbol *sym;

	      position++;
	      sym = symbol_lookup (name);
	      if (sym == (Symbol *)NULL) continue;

	      field = lookup_fieldname (sym->name, table_fields);

	      /* If the field exists in the table, print its name.
		 Exception: Don't print this field if it is the
		 keyfield. */
	      if ((field != (gsql_field *) NULL) &&
		  (strcasecmp (gsql_field_name (field),
			       gsql_field_name (keyfield)) != 0))
		{
		  bprintf (query, ",");

		  /* We only use single quotes around CHAR type data. */
		  if (CHARTYPE (field))
		    bprintf (query, "'");

		  bprintf_gsql_escape
		    (query, value, field, escape_char, truncate_p);

		  if (CHARTYPE (field))
		    bprintf (query, "'");
		}
	    }

	  bprintf (query, ")");

	  /* Execute the query. */
	  if (debug_level)
	    page_debug ("  Trying: [%s]", query->buffer);

	  status = gsql_query (db, query->buffer, 0);
	  bprintf_free_buffer (query);

	  if (status == GSQL_SUCCESS)
	    result = "true";
	  else
	    {
	      if (!empty_string_p (method))
		gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	    }
	}

      /* If no method is specified, and if the previous insert
	 operation failed, or if the method is UPDATE, then attempt
	 an UPDATE query. */
      if (!errors_found &&
	  ((empty_string_p (method) && result == (char *)NULL)
	   || (!empty_string_p (method) && !(strcasecmp (method, "UPDATE")))))
	{
	  /* If the INSERT failed, try an UPDATE. */
	  query = bprintf_create_buffer ();

	  /* UPDATE emp_details SET salary=30000, age=20
	     WHERE emp_id = 1234 */
	  bprintf (query, "UPDATE %s SET ", table);

	  position = 2;

	  /* This is used to tell if we need to print a comma */
	  items_printed = 0;

	  while ((name = get_positional_arg (vars, position)) != (char *)NULL)
	    {
	      char *value = pagefunc_get_variable (name);
	      Symbol *sym;

	      position++;

	      sym = symbol_lookup (name);
	      if (sym == (Symbol *)NULL) continue;

	      field = lookup_fieldname (sym->name, table_fields);

	      if ((field != (gsql_field *) NULL) &&
		  (strcasecmp (gsql_field_name (field),
			       gsql_field_name (keyfield)) != 0))
		{
		  if (items_printed > 0) bprintf (query, ",");
		  items_printed++;

		  bprintf (query, "%s = ", gsql_field_name (field));
		  if (CHARTYPE(field))
		    bprintf (query, "'");
		  bprintf_gsql_escape
		    (query, value, field, escape_char, truncate_p);
		  if (CHARTYPE(field))
		    bprintf (query, "'");

		}
	    }

	  bprintf (query, " WHERE ");
	  bprintf (query, "%s", gsql_field_name (keyfield));

	  bprintf (query, " = ");
	  QUOTE_KEYVAL_IF_NEEDED;
	  bprintf_gsql_escape (query, key, keyfield, escape_char, truncate_p);
	  QUOTE_KEYVAL_IF_NEEDED;

	  /* Execute the query. */
	  if (debug_level)
	    page_debug ("  Trying: [%s]", query->buffer);

	  status = gsql_query (db, query->buffer, 1);
	  bprintf_free_buffer (query);

	  if (status == GSQL_SUCCESS)
	    result = "true";
	}
    }

#if !defined (CACHED_GSQL_LIST_FIELDS)
  if (table_fields != (gsql_result *)NULL)
    gsql_free_result (table_fields);
#endif

  xfree (key);
  xfree (keyname);
  xfree (method);
  xfree (table);

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

/* <database-load-record db key table=table keyname=fieldname [package=name]>

   Does a SELECT to recover data from the table, using
   WHERE keyname='key' as the query. */
static void
pf_database_load_record (PFunArgs)
{
  Database *db = get_dbref (vars);
  char *key     = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *keyname = mhtml_evaluate_string
    (get_one_of (vars, "keyname", "key", (char *)NULL));
  char *table   = mhtml_evaluate_string (get_value (vars, "table"));
  BPRINTF_BUFFER *query = (BPRINTF_BUFFER *) NULL;
  char *pkgname = (char *) NULL;
  char *result = (char *)NULL;
  gsql_result *table_fields = (gsql_result *)NULL;
  int errors_found = 0;

  /* No errors yet! */
  gsql_clear_error_message ();

  if ((db != (Database *)NULL) &&
      (db->connected == 1) &&
      (database_environment_level != 0) &&
      !(empty_string_p (key)) &&
      !(empty_string_p (table)) &&
      !(empty_string_p (keyname)))
    {
      int status = -1;
      gsql_field *keyfield = (gsql_field *) NULL;
      gsql_result *query_result = (gsql_result *) NULL;
      char escape_char = gsql_database_sql_escape_character (db);
      int truncate_p = gsql_database_sql_truncate_columns (db);

      pkgname = mhtml_evaluate_string
	(get_one_of (vars, "PACK", "PACKAGE", "PREFIX", (char *)NULL));

      table_fields = gsql_list_fields (db, table);

      if (table_fields == (gsql_result *) NULL)
	{
	  gsql_save_error_message
	    (db, "table does not exist or has no columns");
	  errors_found++;
	}

      if (!errors_found)
	{
	  keyfield = lookup_fieldname (keyname, table_fields);

	  if (keyfield == (gsql_field *) NULL)
	    {
	      gsql_save_error_message
		(db, "primary key column name not found");
	      errors_found++;
	    }
	}

      if (!errors_found)
	{
	  query = bprintf_create_buffer ();
	  bprintf (query, "SELECT * FROM %s where ", table);
	  bprintf (query, "%s", gsql_field_name (keyfield));
	  bprintf (query, " = ");

	  QUOTE_KEYVAL_IF_NEEDED;
	  bprintf_gsql_escape (query, key, keyfield, escape_char, truncate_p);
	  QUOTE_KEYVAL_IF_NEEDED;

	  if (debug_level)
	    page_debug ("  Trying: [%s]", query->buffer);

	  status = gsql_query (db, query->buffer, 0);
	  bprintf_free_buffer (query);
	}

      if (status == GSQL_SUCCESS)
	{
	  /* Query had no errors */
	  query_result = gsql_store_result (db);

	  if (query_result != (gsql_result *)NULL)
	    {
	      if (gsql_fetch_row (query_result) == GSQL_NO_DATA_FOUND)
		{
		  /* End of data has been reached. */
		}
	      else
		{
		  /* Loop over fields of the row, binding variables.
		     In the case of MSQL, everything is a string, so
		     we don't need to worry about converting INT or
		     REAL to string datatypes */
		  register int i;

		  for (i = 0; i < gsql_num_fields (query_result); i++)
		    {
		      BPRINTF_BUFFER *varname = bprintf_create_buffer ();
		      gsql_field *field = gsql_fetch_field (query_result, i);
		      char *value;

		      if (empty_string_p (pkgname))
			bprintf (varname, "%s", gsql_field_name (field));
		      else
			bprintf (varname, "%s::%s",
				 pkgname, gsql_field_name (field));

		      value = gsql_get_column (query_result, i);
		      pagefunc_set_variable (varname->buffer, value);
		      xfree (value);

		      bprintf_free_buffer (varname);
		    }
		  result = "true";
		}
	    }
	  gsql_free_result (query_result);
	}
      else if (!errors_found)
	{
	  gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	}
    }

#if !defined (CACHED_GSQL_LIST_FIELDS)
  if (table_fields != (gsql_result *)NULL)
    gsql_free_result (table_fields);
#endif

  xfree (key);
  xfree (keyname);
  xfree (table);
  xfree (pkgname);

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

/* <database-delete-record DB KEY table=tablename keyname=fieldname>
   table and keyname must be supplied, where keyname is the name
   of the primary key field.  */
static void
pf_database_delete_record (PFunArgs)
{
  Database *db = get_dbref (vars);
  char *key     = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *keyname = mhtml_evaluate_string
    (get_one_of (vars, "keyname", "key", (char *)NULL));
  char *table   = mhtml_evaluate_string (get_value (vars, "table"));
  BPRINTF_BUFFER *query = (BPRINTF_BUFFER *) NULL;
  char *result = (char *)NULL;
  gsql_result *table_fields = (gsql_result *) NULL;
  int errors_found = 0;

  gsql_clear_error_message ();

  if ((db != (Database *)NULL) &&
      (db->connected == 1) &&
      (database_environment_level != 0) &&
      !(empty_string_p (key)) &&
      !(empty_string_p (table)) &&
      !(empty_string_p (keyname)))
    {
      gsql_field *keyfield;
      int status = -1;
      char escape_char = gsql_database_sql_escape_character (db);
      int truncate_p = gsql_database_sql_truncate_columns (db);

      table_fields = gsql_list_fields (db, table);
      if (table_fields == (gsql_result *) NULL)
	{
	  gsql_save_error_message
	    (db, "table does not exist or has no columns");
	  errors_found++;
	}

      if (!errors_found)
	{
	  keyfield = lookup_fieldname (keyname, table_fields);

	  if (keyfield == (gsql_field *) NULL)
	    {
	      gsql_save_error_message
		(db, "primary key column name not found");
	      errors_found++;
	    }
	}

      if (!errors_found)
	{
	  query = bprintf_create_buffer ();
	  /* DELETE FROM table where keyname='keyval' */
	  bprintf (query, "DELETE FROM %s where ", table);
	  bprintf (query, "%s", gsql_field_name (keyfield));
	  bprintf (query, " = ");

	  QUOTE_KEYVAL_IF_NEEDED;
	  bprintf_gsql_escape (query, key, keyfield, escape_char, truncate_p);
	  QUOTE_KEYVAL_IF_NEEDED;

	  if (debug_level)
	    page_debug ("  Trying: [%s]", query->buffer);

	  status = gsql_query (db, query->buffer, 0);
	  bprintf_free_buffer (query);
	}

      if (status == GSQL_SUCCESS)
	result = "true";
      else if (!errors_found)
	gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
    }

#if !defined (CACHED_GSQL_LIST_FIELDS)
  if (table_fields != (gsql_result *)NULL)
    gsql_free_result (table_fields);
#endif

  xfree (key);
  xfree (keyname);
  xfree (table);

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

/* <database-save-package db key package keyname=fieldname table=table
                          [method=insert|update]>

   This gets the fieldnames and datatypes using msqlListFields, and
   only saves variables who have names which match (case insensitive)
   with table fields.

   If METHOD is specified, then only one of INSERT or UPDATE
   is attempted. */
static void
pf_database_save_package (PFunArgs)
{
  Database *db = get_dbref (vars);
  char *key     = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *keyname = mhtml_evaluate_string (get_one_of (vars, "keyname", "key",
						     (char *)NULL));
  char *table   = mhtml_evaluate_string (get_value (vars, "table"));
  char *method   = mhtml_evaluate_string (get_value (vars, "method"));
  BPRINTF_BUFFER *query = (BPRINTF_BUFFER *) NULL;
  char *result = (char *)NULL;
  gsql_result *table_fields = (gsql_result *) NULL;
  gsql_field *field = (gsql_field *)NULL;
  gsql_field *keyfield = (gsql_field *)NULL;
  Symbol **symbols = (Symbol **)NULL;
  int status = 0;
  int items_printed = 0;
  int errors_found = 0;

  /* No errors yet! */
  gsql_clear_error_message ();

  if (db != (Database *)NULL)
    {
      if (empty_string_p (key))
	gsql_save_error_message (db, "Missing KEY argument");

      if (empty_string_p (table))
	gsql_save_error_message (db, "Missing TABLE=tablename argument");

      if (empty_string_p (keyname))
	gsql_save_error_message (db, "Missing keyname=field argument");
    }

  if ((db != (Database *)NULL) &&
      (db->connected == 1) &&
      (database_environment_level != 0) &&
      !(empty_string_p (key)) &&
      !(empty_string_p (table)) &&
      !(empty_string_p (keyname)))
    {
      register int i;
      Symbol *sym;
      char escape_char = gsql_database_sql_escape_character (db);
      int truncate_p = gsql_database_sql_truncate_columns (db);

      table_fields = gsql_list_fields (db, table);

      if (table_fields == (gsql_result *) NULL)
	{
	  gsql_save_error_message
	    (db, "table does not exist or has no columns");
	  errors_found++;
	}

      if (!errors_found)
	{
	  keyfield = lookup_fieldname (keyname, table_fields);
	  if (keyfield == (gsql_field *) NULL)
	    {
	      gsql_save_error_message
		(db, "primary key column name not found");
	      errors_found++;
	    }
	}

      {
	char *packname = mhtml_evaluate_string (get_positional_arg (vars, 2));

	if (!empty_string_p (packname))
	  {
	    symbols = symbol_package_symbols (packname);

	    if (symbols == (Symbol **)NULL)
	      {
		gsql_save_error_message (db, "package contains no variables");
		errors_found++;
	      }
	  }
	else
	  {
	    gsql_save_error_message (db, "missing package-name");
	    errors_found++;
	  }

	xfree (packname);
      }

      if  (!errors_found &&
	   (empty_string_p (method) || (strcasecmp (method, "INSERT") == 0)))
	{
	  query = bprintf_create_buffer ();

	  /* Let's try an INSERT on this key and keyvalue:
	     "INSERT INTO table (key, name, name..) VALUES
	     ('keyval', 'val', 'val', ...)" */
	  bprintf (query, "INSERT INTO %s (", table);

	  /* Comma-separated list of all the variable names, including key. */
	  bprintf (query, "%s", gsql_field_name (keyfield));

	  for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	    {
	      if (sym->type != symtype_STRING)
		continue;

	      field = lookup_fieldname (sym->name, table_fields);

	      /* If the field exists in the table, print its name.
		 Exception: if this is the keyfield, we have already
		 printed it. */
	      if ((field != (gsql_field *) NULL) &&
		  (strcasecmp (gsql_field_name (field),
			       gsql_field_name (keyfield)) != 0))
		{
		  bprintf (query, ",");
		  bprintf (query, "%s", gsql_field_name (field));
		}
	    }

	  bprintf (query, ") VALUES (");
	  QUOTE_KEYVAL_IF_NEEDED;
	  bprintf_gsql_escape (query, key, keyfield, escape_char, truncate_p);
	  QUOTE_KEYVAL_IF_NEEDED;

	  /* Print symbol values */
	  for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	    {
	      if (sym->type != symtype_STRING)
		continue;

	      field = lookup_fieldname (sym->name, table_fields);

	      /* If the field exists in the table, print its name, unless
		 it is the key field. */
	      if ((field != (gsql_field *) NULL) &&
		  (strcasecmp (gsql_field_name (field),
			       gsql_field_name (keyfield)) != 0))
		{
		  bprintf (query, ",");

		  /* We only use single quotes around CHAR type data. */
		  if (CHARTYPE (field))
		    bprintf (query, "'");

		  /* Print out the various values.  If there are none,
		     print the empty string. */
		  if (sym->values_index != 0)
		    {
		      if ((sym->values_index == 1) || (!CHARTYPE (field)))
			bprintf_gsql_escape (query, sym->values[0], field,
					     escape_char, truncate_p);
		      else
			{
			  register int j;
			  BPRINTF_BUFFER *tmpbuf = bprintf_create_buffer ();

			  for (j = 0; j < sym->values_index; j++)
			    bprintf (tmpbuf, "%s%s",
				     sym->values[j],
				     sym->values[j + 1] ? " " : "");

			  bprintf_gsql_escape (query, tmpbuf->buffer, field,
					       escape_char, truncate_p);

			  bprintf_free_buffer (tmpbuf);
			}
		    }

		  if (CHARTYPE (field))
		    bprintf (query, "'");
		}
	    }

	  bprintf (query, ")");

	  /* Execute the query. */
	  if (debug_level)
	    page_debug ("  Trying: [%s]", query->buffer);

	  status = gsql_query (db, query->buffer, 0);
	  bprintf_free_buffer (query);

	  if (status == GSQL_SUCCESS)
	    result = "true";
	  else
	    {
	      if (!empty_string_p (method))
		{
		  errors_found++;
		  gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
		}
	    }
	}

      if (!errors_found &&
	  ((empty_string_p (method) && result == (char *)NULL) ||
	   (!empty_string_p (method) && (strcasecmp (method, "UPDATE") == 0))))
	{
	  /* The INSERT failed for some reason, so let's try an UPDATE. */
	  query = bprintf_create_buffer ();

	  /* Let's try an UPDATE on this key and keyvalue:
	     UPDATE emp_details SET salary=30000 WHERE emp_id = 1234 */

	  bprintf (query, "UPDATE %s SET ", table);

	  /* Comma-separated list of all the variable names and values. */
	  items_printed = 0;

	  for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	    {
	      if (sym->type != symtype_STRING)
		continue;

	      field = lookup_fieldname (sym->name, table_fields);

	      if ((field != (gsql_field *) NULL) &&
		  (strcasecmp (gsql_field_name (field),
			       gsql_field_name (keyfield)) != 0))
		{
		  if (items_printed > 0) bprintf (query, ",");
		  items_printed++;

		  bprintf (query, "%s = ", gsql_field_name (field));

		  if (CHARTYPE (field))
		    bprintf (query, "'");

		  if (sym->values_index != 0)
		    {
		      if ((sym->values_index == 1) || (!CHARTYPE (field)))
			bprintf_gsql_escape (query, sym->values[0], field,
					     escape_char, truncate_p);
		      else
			{
			  /* Collect up the array values into a string. */
			  register int j;
			  BPRINTF_BUFFER *tmpbuf = bprintf_create_buffer ();

			  for (j = 0; j < sym->values_index; j++)
			    bprintf (tmpbuf, "%s%s", sym->values[j],
				     sym->values[j + 1] ? " " : "");

			  bprintf_gsql_escape (query, tmpbuf->buffer, field,
					       escape_char, truncate_p);

			  bprintf_free_buffer (tmpbuf);
			}
		    }

		  if (CHARTYPE (field))
		    bprintf (query, "'");
		}
	    }

	  bprintf (query, " WHERE ");
	  bprintf (query, "%s", gsql_field_name (keyfield));
	  bprintf (query, " = ");
	  QUOTE_KEYVAL_IF_NEEDED;
	  bprintf_gsql_escape (query, key, keyfield, escape_char, truncate_p);
	  QUOTE_KEYVAL_IF_NEEDED;

	  /* Execute the query. */
	  if (debug_level)
	    page_debug ("  Trying: [%s]", query->buffer);

	  status = gsql_query (db, query->buffer, 1);
	  bprintf_free_buffer (query);

	  if (status == GSQL_SUCCESS)
	    result = "true";
	}

      if (result)
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart += strlen (result);
	}

    }

#if !defined (CACHED_GSQL_LIST_FIELDS)
  if (table_fields != (gsql_result *)NULL)
    gsql_free_result (table_fields);
#endif

  xfree (symbols);
  xfree (key);
  xfree (keyname);
  xfree (table);
  xfree (method);
}

static void
maybe_make_where_clause (BPRINTF_BUFFER *query, gsql_field **fields, Package *p)
{
  register int i;
  int printed = 0;

  for (i = 0; fields[i] != (gsql_field *)NULL; i++)
    {
      if (gsql_field_is_unique (fields[i]))
	{
	  gsql_field *f = fields[i];
	  char *column_name = gsql_field_name (f);
	  Symbol *sym = symbol_lookup_in_package (p, column_name);
	  char *val = (char *)NULL;

	  if ((sym != (Symbol *)NULL) && (sym->type == symtype_STRING) &&
	      (sym->values_index != 0))
	    val = sym->values[0];
	  
	  if (val != (char *)NULL)
	    {
	      bprintf (query, " %s %s=", printed? "AND":"WHERE", column_name);
	      QUOTE_VALUE_IF_NEEDED (f);
	      bprintf_gsql_escape (query, val, f, '\'', 1);
	      QUOTE_VALUE_IF_NEEDED (f);
	      printed++;
	    }
	}
    }
}

/* <package-to-table db package table &key method>
   This gets the fieldnames and datatypes using msqlListFields, and
   only saves variables who have names which match (case insensitive)
   with table fields. */
static void
pf_package_to_table (PFunArgs)
{
  Database *db = get_dbref (vars);
  char *packname = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *table = mhtml_evaluate_string (get_positional_arg (vars, 2));
  Package *pack = (Package *)NULL;
  char *result = (char *)NULL;
  int errors_found = 0;

  /* No errors yet! */
  gsql_clear_error_message ();

  if (db != (Database *)NULL)
    {
      if (empty_string_p (packname))
	gsql_save_error_message (db, "Missing PACKAGE argument");
      else
	pack = symbol_get_package (packname);

      if (empty_string_p (table))
	gsql_save_error_message (db, "Missing TABLE argument");
    }

  if ((db != (Database *)NULL) &&
      (db->connected == 1) &&
      (database_environment_level != 0) &&
      !(empty_string_p (packname)) &&
      !(empty_string_p (table)))
    {
      register int i;
      char escape_char = gsql_database_sql_escape_character (db);
      int truncate_p = gsql_database_sql_truncate_columns (db);
      gsql_result *table_fields;
      int status = -1;

      table_fields = gsql_list_fields (db, table);

      if (table_fields == (gsql_result *) NULL)
	{
	  make_db_error
	    (db, "table `%s' does not exist or has no columns", table);
	  errors_found++;
	}

      if (!errors_found)
	{
	  BPRINTF_BUFFER *query = bprintf_create_buffer ();
	  int num_fields = gsql_num_fields (table_fields);
	  int update_p = 0, force_update = 0, force_insert = 0;
	  gsql_field **fields = (gsql_field **)
	    xmalloc ((1 + num_fields) * sizeof (gsql_field *));
	  char *method = mhtml_evaluate_string (get_value (vars, "method"));

	  if (!empty_string_p (method))
	    {
	      if (strcasecmp (method, "UPDATE") == 0)
		force_update = 1;
	      else if (strcasecmp (method, "INSERT") == 0)
		force_insert = 1;
	    }

	  xfree (method);

	  for (i = 0; i < num_fields; i++)
	    fields[i] = gsql_fetch_field (table_fields, i);

	  fields[i] = (gsql_field *)NULL;

	  if (force_update)
	    {
	      update_p = 1;
	    }
	  else if (!force_insert)
	    {
	      /* Find out if we should do an update or an insert.  We will do
		 an update if, and only if, there is a record in the database
		 which already has all of the keyfields that this record has.*/
	      int temp_status;

	      bprintf (query, "SELECT COUNT(*) AS mhtml_num_recs FROM %s",
		       table);
	      maybe_make_where_clause (query, fields, pack);
	      temp_status = gsql_query (db, query->buffer, 0);
	      bprintf_free_buffer (query);
	      query = bprintf_create_buffer ();
	      if ((temp_status == GSQL_SUCCESS) ||
		  (temp_status == GSQL_SUCCESS_WITH_INFO))
		{
		  gsql_result *temp_result = gsql_store_result (db);

		  if (temp_result != (gsql_result *)NULL)
		    {
		      temp_status = gsql_fetch_row (temp_result);

		      if ((temp_status == GSQL_SUCCESS) ||
			  (temp_status == GSQL_SUCCESS_WITH_INFO))
			{
			  gsql_field *temp_field =
			    gsql_fetch_field (temp_result, 0);

			  if (temp_field != (gsql_field *)NULL)
			    {
			      char *value = gsql_get_column (temp_result, 0);
			      int mhtml_num_rows = 0;

			      if (value != (char *)NULL)
				{
				  mhtml_num_rows = atoi (value);
				  free (value);
				}

			      if (mhtml_num_rows > 0)
				update_p = 1;
			    }
			}

		      gsql_free_result (temp_result);
		    }
		}
	    }

	  /* Do either an UPDATE or an INSERT. */
	  if (update_p)
	    {
	      int printed = 0;

	      /* UPDATE emp_details SET salary=30000 WHERE emp_id = 1234 */
	      bprintf (query, "UPDATE %s SET", table);

	      for (i = 0; fields[i] != (gsql_field *)NULL; i++)
		{
		  gsql_field *f = fields[i];
		  char *column_name = gsql_field_name (f);
		  Symbol *sym = symbol_lookup_in_package (pack, column_name);
		  char *val = (char *)NULL;

		  if ((sym != (Symbol *)NULL) &&
		      (sym->type == symtype_STRING) &&
		      (sym->values_index != 0))
		    val = sym->values[0];

		  if ((val != (char *)NULL) || !(gsql_field_is_unique (f)))
		    {
		      bprintf (query, "%s%s=", printed? ",":" ", column_name);
		      QUOTE_VALUE_IF_NEEDED (f);
		      bprintf_gsql_escape (query, val, f,
					   escape_char, truncate_p);
		      QUOTE_VALUE_IF_NEEDED (f);
		      printed++;
		    }
		}

	      /* Now the WHERE clause. */
	      maybe_make_where_clause (query, fields, pack);

	      if (debug_level)
		page_debug ("  Trying: [%s]", query->buffer);

	      status = gsql_query (db, query->buffer, 1);
	      bprintf_free_buffer (query);

	      if (status == GSQL_SUCCESS)
		result = "true";
	    }
	  else
	    {
	      /* Let's try an INSERT on this data:
		 INSERT INTO table (n, n, ...) VALUES ('v', 'v',...) */
	      bprintf (query, "INSERT INTO %s (", table);

	      /* Comma-separated list of all the variable names. */
	      for (i = 0; fields[i] != (gsql_field *)NULL; i++)
		bprintf (query, "%s%s", gsql_field_name (fields[i]),
			 fields[i + 1] ? "," : "");

	      bprintf (query, ") VALUES (");

	      /* Now some values. */
	      for (i = 0; fields[i] != (gsql_field *)NULL; i++)
		{
		  gsql_field *f = fields[i];
		  char *column_name = gsql_field_name (f);
		  Symbol *sym = symbol_lookup_in_package (pack, column_name);
		  char *val = (char *)NULL;

		  if ((sym != (Symbol *)NULL) &&
		      (sym->type == symtype_STRING) &&
		      (sym->values_index != 0))
		    val = sym->values[0];

		  QUOTE_VALUE_IF_NEEDED (f);
		  bprintf_gsql_escape (query, val, f, escape_char, truncate_p);
		  QUOTE_VALUE_IF_NEEDED (f);

		  if (fields[i + 1]) bprintf (query, ",");
		}

	      bprintf (query, ")");

	      /* Execute the query. */
	      if (debug_level)
		page_debug ("  Trying: [%s]", query->buffer);

	      status = gsql_query (db, query->buffer, 0);
	      bprintf_free_buffer (query);

	      if ((status == GSQL_SUCCESS) ||
		  (status == GSQL_SUCCESS_WITH_INFO))
		result = "true";
	    }

	  xfree (fields);
	}

      if (result)
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart += strlen (result);
	}

#if !defined (CACHED_GSQL_LIST_FIELDS)
      if (table_fields != (gsql_result *)NULL)
	gsql_free_result (table_fields);
#endif
    }

  xfree (packname);
  xfree (table);
}

static char *
pretty_field_type (int fieldtype)
{
  switch (fieldtype)
    {
    case  GSQL_CHAR:       return ("GSQL_CHAR");
    case  GSQL_NUMERIC:    return ("GSQL_NUMERIC");
    case  GSQL_DECIMAL:    return ("GSQL_DECIMAL");
    case  GSQL_INTEGER:    return ("GSQL_INTEGER");
    case  GSQL_SMALLINT:   return ("GSQL_SMALLINT");
    case  GSQL_FLOAT:      return ("GSQL_FLOAT");
    case  GSQL_REAL:       return ("GSQL_REAL");
    case  GSQL_DOUBLE:     return ("GSQL_DOUBLE");
    case  GSQL_VARCHAR:    return ("GSQL_VARCHAR");

      /* MYSQL Extensions. */
    case GSQL_TINY_BLOB:   return ("GSQL_TINY_BLOB");
    case GSQL_MEDIUM_BLOB: return ("GSQL_MEDIUM_BLOB");
    case GSQL_LONG_BLOB:   return ("GSQL_LONG_BLOB");
    case GSQL_BLOB:	   return ("GSQL_BLOB");
    case GSQL_VAR_STRING:  return ("GSQL_VAR_STRING");
    case GSQL_STRING:	   return ("GSQL_STRING");

      /* Non-ANSI Extensions. */
    case GSQL_IDENT:       return ("GSQL_IDENT");
    case GSQL_NULL:        return ("GSQL_NULL");
    case GSQL_IDX:         return ("GSQL_IDX");
    case GSQL_SYSVAR:      return ("GSQL_SYSVAR");
    case GSQL_UINT:        return ("GSQL_UINT");
    case GSQL_ANY:         return ("GSQL_ANY");
      
    default:
      return ("GSQL_CHAR");
    }
}

static char *
string_from_boolean (int the_bool)
{
  if (the_bool == 0) return "";
  else return "true";
}

/* Create a Meta-HTML string representation of an alist, which contains
   table field information. */
static char *
alist_of_field_info (gsql_field *field, Database *db)
{
  BPRINTF_BUFFER *tmpbuf = (BPRINTF_BUFFER *)NULL;
  char *val = (char *)NULL;

  if ((db != (Database *)NULL) && gsql_database_connected (db))
    {
      tmpbuf = bprintf_create_buffer ();

      bprintf (tmpbuf, "(");
      bprintf (tmpbuf, "(NAME . \"%s\")", gsql_field_name (field));
      bprintf (tmpbuf, "(LENGTH . \"%d\")", gsql_field_length (field));
      bprintf (tmpbuf, "(TYPE  \"%s\" \"%d\")",
	       pretty_field_type (gsql_field_type (field)),
	       gsql_raw_field_type (field));
      bprintf (tmpbuf, "(IS_UNIQUE . \"%s\")",
	       string_from_boolean (gsql_field_is_unique (field)));
      bprintf (tmpbuf, "(IS_NULLABLE . \"%s\")",
	       string_from_boolean (! (gsql_field_is_not_null (field))));
      bprintf (tmpbuf, "(QUALIFIER . \"%s\")", gsql_field_qualifier (field));
      bprintf (tmpbuf, "(OWNER . \"%s\")", gsql_field_owner (field));
      bprintf (tmpbuf, "(TYPENAME . \"%s\")", gsql_field_type_name (field));
      bprintf (tmpbuf, "(PRECISION . \"%d\")", gsql_field_precision (field));
      bprintf (tmpbuf, "(SCALE . \"%d\")", gsql_field_scale (field));
      bprintf (tmpbuf, "(RADIX . \"%d\")", gsql_field_radix (field));
      bprintf (tmpbuf, "(TABLE . \"%s\")", gsql_field_table (field));
      bprintf (tmpbuf, ")");
    }
  
  if (tmpbuf != (BPRINTF_BUFFER *)NULL)
    {
      if (tmpbuf->buffer != (char *)NULL)
	val = strdup (tmpbuf->buffer);
      else
	val = (char *)NULL;

      bprintf_free_buffer (tmpbuf);
    }

  return (val);
}

/* List all the column information for a table. This must be done
   with a database already open, i.e., within the scope of a
   <msql::with-open-database>.

   Args: <msql::database-columns-info dbvar tablename [result=VAR]>
   Returns an array of alists, one per table column, 
   or sets the value of VAR if supplied. 

   Each alist contains 
   ((name . "column_name") (length . "max byte length") (type . "sqlDatatype")
    (is_unique . "true") (is_not_null . "true")) */
static void
pf_database_columns_info (PFunArgs)
{
  /* No errors yet! */
  gsql_clear_error_message ();

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  char *tablename;

	  /* The name of the table we are enquiring about. */
	  tablename = mhtml_evaluate_string (get_positional_arg (vars, 1));

	  if (!empty_string_p (tablename))
	    {
	      gsql_result *result = gsql_list_fields (db, tablename);

	      if (result != (gsql_result *)NULL)
		{
		  register int i;
		  int cols = gsql_num_fields (result);
		  char *varname;
		  char **colnames;

		  /* The name of the variable to stuff the results into. */
		  varname = mhtml_evaluate_string (get_value (vars, "RESULT"));
		  colnames = (char **)xmalloc ((cols + 1) * sizeof (char *));

		  /* Reset the field descriptor cursor to start of record */
		  for (i = 0; i < cols; i++)
		    {
		      gsql_field *field = gsql_fetch_field (result, i);

		      colnames[i] = alist_of_field_info (field, db);
		    }
		  colnames[i] = (char *) NULL;

		  if (!empty_string_p (varname))
		    {
		      symbol_store_array (varname, colnames);
		    }
		  else
		    {
		      for (i = 0; i < cols; i++)
			{
			  bprintf_insert (page, start, "%s\n", colnames[i]);
			  start += 1 + strlen (colnames[i]);
			  free (colnames[i]);
			}
		      free (colnames);

		      *newstart = start;
		    }

		  xfree (varname);
#if !defined (CACHED_GSQL_LIST_FIELDS)
		  gsql_free_result (result);
#endif
		}
	    }
	  xfree (tablename);
	}
    }
}


/* <database-column-info dbvar table fieldname>

   Returns an alist of properties of a column of a table:
   ((name . "column_name") (length . "max byte length") (type . "sqlDatatype")
    (is_unique . "true") (is_not_null . "true"))

    This is actually implemented by fetching information on *all*
    columns and selecting the specified one. It is thus more
    efficient, if you are looking for info on several columns, to use
    database-columns-info, and get all the column information in a
    single array. */
static void
pf_database_column_info (PFunArgs)
{
  /* No errors yet! */
  gsql_clear_error_message ();

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  char *tablename = (char *)NULL;
	  char *fieldname = (char *)NULL;

	  /* The name of the table we are inquiring about. */
	  tablename = mhtml_evaluate_string (get_positional_arg (vars, 1));
	  fieldname = mhtml_evaluate_string (get_positional_arg (vars, 2));

	  if ((!empty_string_p (tablename)) && (!empty_string_p (fieldname)))
	    {
	      gsql_result *result = gsql_list_fields (db, tablename);

	      if (result != (gsql_result *)NULL)
		{
		  register int i;
		  int cols = gsql_num_fields (result);
		  char *alist = (char *)NULL;

		  for (i = 0; i < cols; i++)
		    {
		      gsql_field *field = gsql_fetch_field (result, i);

		      if (strcasecmp (gsql_field_name (field), fieldname) == 0)
			{
			  alist = alist_of_field_info (field, db);
			  break;
			}
		    }

		  if (alist != (char *)NULL)
		    {
		      bprintf_insert (page, start, "%s", alist);
		      *newstart += strlen (alist);
		      free (alist);
		    }
#if !defined (CACHED_GSQL_LIST_FIELDS)
		  gsql_free_result (result);
#endif
		}
	    }
	  xfree (tablename);
	  xfree (fieldname);
	}
    }
}

/* <sql-transact db [action=COMMIT|ROLLBACK]>

   Perform transaction, to either commit or rollback all operations on
   the current database connection.

   ACTION can be one of COMMIT or ROLLBACK.
   If unspecified, ACTION defaults to COMMIT.  */
static void
pf_sql_transact (PFunArgs)
{
  char *action_arg   = mhtml_evaluate_string (get_value (vars, "ACTION"));

  /* No errors yet! */
  gsql_clear_error_message ();

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);
      int status;

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  char *result;
	  status = gsql_transact_internal (db, action_arg);

	  if (status == GSQL_SUCCESS)
	    {
	      result = "true";
	      bprintf_insert (page, start, "%s", result);
	      *newstart += strlen (result);
	    }
	  else
	    gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	}
    }
  xfree (action_arg);
}

/* Convert a gsql_result struct into a Meta-HTML alist. */
static char *
alist_from_row (gsql_result *query_result)
{
  register int i;
  int num_fields = 0;
  char *fieldname = (char *) NULL;
  char *val;
  BPRINTF_BUFFER *alist;

  alist = bprintf_create_buffer ();
  /* Loop over fields of the row, adding alist entries.  All results
     should already be strings, so we don't need to worry about
     converting INT, REAL, or other datatypes to strings. */

  num_fields = gsql_num_fields (query_result);

  bprintf (alist, "(");
  for (i = 0; i < num_fields; i++)
    {
      gsql_field *field;
      char *value;

      field = gsql_fetch_field (query_result, i);

      fieldname = gsql_field_name (field);

      value = gsql_get_column (query_result, i);
      bprintf (alist, "(\"%s\" . \"%s\")", fieldname, value);
      xfree (value);
    }

  bprintf (alist, ")");
  val = strdup (alist->buffer);
  bprintf_free_buffer (alist);
  return val;

}

/* List all the table information for a database. This must be done
   with a database already open, i.e., within the scope of a
   <msql::with-open-database>.

   Args: <msql::database-tables-info dbvar [result=VAR]>
   Returns an array of alists, one per table, 
   or sets the value of VAR if supplied. 

   Args: <odbc::database-tables-info dbvar [result=VAR] 
   [tabletype=type] [tablequalifier=qualifier] [tablename=name] 
   [tableowner=owner]>

   The optional tablemumble args are ANSI SQL regexp patterns.

   Each alist contains database-specific information, including at least
   the NAME of the table.
   ((name . "table_name")) */
static void
pf_database_tables_info (PFunArgs)
{
  char *qualifier = mhtml_evaluate_string
    (get_one_of (vars, "tablequalifier", "qualifier", (char *)NULL));
  char *owner = mhtml_evaluate_string
    (get_one_of (vars, "tableowner", "owner", (char *)NULL));
  char *name = mhtml_evaluate_string
    (get_one_of (vars, "tablename", "name", (char *)NULL));
  char *type = mhtml_evaluate_string
    (get_one_of (vars, "tabletype", "type", (char *)NULL));

  /* No errors yet! */
  gsql_clear_error_message ();

  if (name == (char *)NULL)
    name = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  /* This returns a result set, with one row of info 
	     for each table in the database. */
	  gsql_result *result =
	    gsql_db_list_tables (db, qualifier, owner, name, type);

	  if (result != (gsql_result *)NULL)
	    {
	      register int i;
	      char **alists;
	      char *varname;
	      int rows;
	      int arraysize = 10;

	      alists = (char **)
		xmalloc ((1 + arraysize) * sizeof (char *));

	      /* The name of the variable to stuff the results into. */
	      varname = mhtml_evaluate_string (get_value (vars, "RESULT"));

	      /* Loop, fetching each row of the result set which
		 was returned by gsql_db_list_tables. */
	      i = 0;
	      while ((gsql_fetch_row (result)) == GSQL_SUCCESS)
		{
		  if (i + 2 > arraysize)
		    alists = (char **)xrealloc
		      (alists, ((arraysize += 5) * sizeof (char *)));

		  /* Create an alist from this result row.*/
		  alists[i++] = alist_from_row (result);
		}

	      alists[i] = (char *) NULL;
	      rows = i;

	      if (!empty_string_p (varname))
		{
		  symbol_store_array (varname, alists);
		}
	      else
		{
		  for (i = 0; i < rows; i++)
		    {
		      bprintf_insert (page, start, "%s\n", alists[i]);
		      start += 1 + strlen (alists[i]);
		      free (alists[i]);
		    }
		  free (alists);

		  *newstart = start;
		}

	      xfree (varname);
	    }
	}
    }
  xfree (qualifier);
  xfree (owner);
  xfree (name);
  xfree (type);
}

#if defined (__cplusplus)
}
#endif
