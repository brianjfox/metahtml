/* mysqlperfuncs.c:  MYSQL database compatibility layer. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jan 31 1996
    Author: Henry Minsky (hqm@ua.com) Wed Oct  2 16:28:36 1996.
    Hacks: Marc A. Tamsky (tamsky@as.ucsb.edu) Sat Oct 11 11:48:59 1997

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

#include <mysql.h>

/****************************************************************/

#define DEFAULT_SQL_ESCAPE_CHARACTER  '\\'
#define DEFAULT_SQL_TRUNCATE_COLUMNS  1
#define DEFAULT_SQL_PREFIX_TABLENAMES 0
#define DATABASE_SUPPORTS_LIMIT_KEYWORD 1

static void pf_database_shutdown (PFunArgs);

static PFunDesc func_table[] =
{
  { "MYSQLPER::WITH-OPEN-DATABASE",	1, 0, pf_with_open_database },
  { "MYSQLPER::DATABASE-EXEC-QUERY",	0, 0, pf_database_exec_query },
  { "MYSQLPER::DATABASE-EXEC-SQL",   	0, 0, pf_database_exec_sql },
  { "MYSQLPER::DATABASE-NEXT-RECORD",	0, 0, pf_database_next_record },
  { "MYSQLPER::DATABASE-SAVE-RECORD",	0, 0, pf_database_save_record },
  { "MYSQLPER::PACKAGE-TO-TABLE",	0, 0, pf_package_to_table },
  { "MYSQLPER::DATABASE-DELETE-RECORD",	0, 0, pf_database_delete_record },
  { "MYSQLPER::DATABASE-LOAD-RECORD",	0, 0, pf_database_load_record },
  { "MYSQLPER::DATABASE-SAVE-PACKAGE",	0, 0, pf_database_save_package },
  { "MYSQLPER::NUMBER-OF-ROWS",		0, 0, pf_database_num_rows},
  { "MYSQLPER::AFFECTED-ROWS",		0, 0, pf_database_affected_rows},
  { "MYSQLPER::SET-ROW-POSITION",	0, 0, pf_database_set_pos},
  { "MYSQLPER::DATABASE-QUERY",		0, 0, pf_database_query },
  { "MYSQLPER::HOST-DATABASES",	        0, 0, pf_host_databases },
  { "MYSQLPER::DATABASE-TABLES",     	0, 0, pf_database_tables },
  { "MYSQLPER::DATABASE-TABLES-INFO",	0, 0, pf_database_tables_info },
  { "MYSQLPER::DATABASE-COLUMNS",	0, 0, pf_database_columns },
  { "MYSQLPER::DATABASE-COLUMN-INFO",	0, 0, pf_database_column_info },
  { "MYSQLPER::DATABASE-COLUMNS-INFO",	0, 0, pf_database_columns_info },
  { "MYSQLPER::DATABASE-QUERY-INFO",	0, 0, pf_database_query_info },
  { "MYSQLPER::DATABASE-SET-OPTIONS",	0, 0, pf_database_set_options },
  { "MYSQLPER::CURSOR-GET-COLUMN",	0, 0, pf_cursor_get_column },
  { "MYSQLPER::QUERY-GET-COLUMN",	0, 0, pf_query_get_column },
  { "MYSQLPER::SQL-TRANSACT",		0, 0, pf_sql_transact },
  { "MYSQLPER::SHUTDOWN",		0, 0, pf_database_shutdown },
  { (char *)NULL,			0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_mysql_functions)
DEFINE_SECTION (MYSQL-DATABASE-EXTENSIONS, database; SQL; Minerva,
"<Meta-HTML> can use the <b>MySQL</b> database engine (residing in the
dynamic module <code>modmysql.so</code>) when requested to by the
<funref generic-sql-interface sql::set-database-type> command.
The MySQL extensions allow true SQL database interaction at many levels,
providing a clean and flexible abstraction to a MySQL database.", "")

DEFVAR (mysqlper::mysql-error-message,
"An array of strings containing any error messages generated by the
last call to the MySQL database.  Don't use this variable, use
the function <funref generic-sql-interface sql::sql-error-message> instead.")

DEFVAR (mysqlper::recent-query,
"The last query sent to the MySQL database for execution.  Don't use this
variable, use the function <funref generic-sql-interface sql::recent-query>
instead.")

DEFMACROX (pf_mysqlper::with_open_database, ,
"See <funref generic-sql-interface sql::with-open-database>.")
DEFUNX (pf_mysqlper::database_exec_query, ,
"See <funref generic-sql-interface sql::database-exec-query>.")
DEFUNX (pf_mysqlper::database_exec_sql, ,
"See <funref generic-sql-interface sql::database-exec-sql>.")
DEFUNX (pf_mysqlper::database_next_record, ,
"See <funref generic-sql-interface sql::database-next-record>.")
DEFUNX (pf_mysqlper::database_save_record, ,
"See <funref generic-sql-interface sql::database-save-record>.")
DEFUNX (pf_mysqlper::database_delete_record, ,
"See <funref generic-sql-interface sql::database-delete-record>.")
DEFUNX (pf_mysqlper::database_load_record, ,
"See <funref generic-sql-interface sql::database-load-record>.")
DEFUNX (pf_mysqlper::database_save_package, ,
"See <funref generic-sql-interface sql::database-save-package>.")
DEFUNX (pf_mysqlper::database_num_rows, ,
"See <funref generic-sql-interface sql::database-num-rows>.")
DEFUNX (pf_mysqlper::database_affected_rows, ,
"See <funref generic-sql-interface sql::database-affected-rows>.")
DEFUNX (pf_mysqlper::database_set_pos, ,
"See <funref generic-sql-interface sql::database-set-pos>.")
DEFUNX (pf_mysqlper::database_query, ,
"See <funref generic-sql-interface sql::database-query>.")
DEFUNX (pf_mysqlper::host_databases, ,
"See <funref generic-sql-interface sql::host-databases>.")
DEFUNX (pf_mysqlper::database_tables, ,
"See <funref generic-sql-interface sql::database-tables>.")
DEFUNX (pf_mysqlper::database_tables_info, ,
"See <funref generic-sql-interface sql::database-tables-info>.")
DEFUNX (pf_mysqlper::database_columns, ,
"See <funref generic-sql-interface sql::database-columns>.")
DEFUNX (pf_mysqlper::database_column_info, ,
"See <funref generic-sql-interface sql::database-column-info>.")
DEFUNX (pf_mysqlper::database_columns_info, ,
"See <funref generic-sql-interface sql::database-columns-info>.")
DEFUNX (pf_mysqlper::database_query_info, ,
"See <funref generic-sql-interface sql::database-query-info>.")
DEFUNX (pf_mysqlper::database_set_options, ,
"See <funref generic-sql-interface sql::database-set-options>.")
DEFUNX (pf_mysqlper::cursor_get_column, ,
"See <funref generic-sql-interface sql::cursor-get-column>.")
DEFUNX (pf_mysqlper::query_get_column, ,
"See <funref generic-sql-interface sql::query-get-column>.")
DEFUNX (pf_mysqlper::sql_transact, ,
"See <funref generic-sql-interface sql::sql-transact>.")

/****************************************************************
 * The Database object:
 *
 * Contains a stack of cursors, and information about the open
 * database connection.
 ****************************************************************/

typedef struct
{
  char *dsn;		/* DSN used to create this socket. */
  int reference_count;	/* Number of times that we are reusing this socket. */
  MYSQL *sock;		/* Socket, from the MySQL point of view. */
} MYSOCK;

static MYSOCK **all_sockets = (MYSOCK **)NULL;
static int as_index = 0;
static int as_slots = 0;

typedef struct
{
  IStack *cursors;
  int connected;		/* 1 if db connection open, 0 otherwise */
  char sql_escape_char;
  int  sql_truncate_columns;
  int  sql_prefix_tablenames;

  /* MYSQL-specific data */
  MYSOCK *mysock;
  char *dbname;
  char *hostname;

#if defined (CACHED_GSQL_LIST_FIELDS)
  /* Cache the saved information about fields of tables */
  void **table_cache_info;	/* An array of gsql_result. */
  int table_cache_index;
  int table_cache_slots;
#endif

} Database;

typedef struct
{
  MYSQL_FIELD *mysql_field;
  /* These are not used by anyone yet, but are for 
     future compatibility with ODBC. */
  char *qualifier;
  char *owner;
  char *type_name;
  int precision;
  int scale;
  int radix;

} gsql_field;

typedef struct
{
  MYSQL_RES *mysql_result;
  MYSQL_ROW mysql_row;
  gsql_field **fields;
  char *tablename;
} gsql_result;


/* For MYSQL, a cursor points to a m_result object. */
typedef struct
{
  gsql_result *result;		/* The results of <database-exec-query..> */
  Database *db;			/* Associated database connection. */
  int index;			/* A unique index number for this cursor within
				   the stack of cursors for a database. */
} DBCursor;

static void free_database_cursors (Database *db);

#if defined (CACHED_GSQL_LIST_FIELDS)
static void gsql_free_result (gsql_result *result);

static void
free_cached_table_info (Database *db) 
{ 
  if ((db != (Database *)NULL) && (db->table_cache_info != (void **)NULL))
    {
      register int i;

      for (i = 0; db->table_cache_info[i] != (void *)NULL; i++)
	gsql_free_result ((gsql_result *)db->table_cache_info[i]);

      free (db->table_cache_info);
    }
}
#endif

static int mysock_true_shutdown = 0;

static void
mysock_disconnect (MYSOCK *mysock)
{
  if (mysock != (MYSOCK *)NULL)
    {
      mysock->reference_count--;

      if ((mysock_true_shutdown != 0) && (mysock->reference_count < 1))
	{
	  mysock_true_shutdown = 0;

	  if (mysock->sock != (MYSQL *)NULL)
	    {
	      register int i;
	      mysql_close (mysock->sock);
	      xfree (mysock->dsn);

	      for (i = 0; i < as_index; i++)
		{
		  if (mysock == all_sockets[i])
		    {
		      while ((all_sockets[i] = all_sockets[i + 1])) i++;
		      as_index--;
		      break;
		    }
		}
	    }
	  free (mysock);
	}
    }
}

static MYSOCK *
mysock_connect (char *dsn)
{
  MYSOCK *result = (MYSOCK *)NULL;

  if (dsn != (char *)NULL)
    {
      register int i;

      for (i = 0; i < as_index; i++)
	if (strcmp (all_sockets[i]->dsn, dsn) == 0)
	  {
	    result = all_sockets[i];
	    result->reference_count++;
	    break;
	  }

      if (result == (MYSOCK *)NULL)
	{
	  char *dbhost = dsn_lookup ("host", dsn);
	  char *user = dsn_lookup ("uid", dsn);
	  char *pass = dsn_lookup ("pwd", dsn);
	  MYSQL *sock;

	  mysql_init (sock);

	  if (sock != NULL)
	    sock = mysql_real_connect
	      (sock, dbhost, user, pass, NULL, 0, NULL, 0);

	  if (sock != (MYSQL *)NULL)
	    {
	      result = (MYSOCK *)xmalloc (sizeof (MYSOCK));
	      result->sock = sock;
	      result->dsn = strdup (dsn);
	      result->reference_count = 1;

	      if (as_index + 2 >= as_slots)
		all_sockets = (MYSOCK **)
		  xrealloc (all_sockets, (as_slots += 10) * sizeof (MYSOCK *));
	      all_sockets[as_index++] = result;
	      all_sockets[as_index] = (MYSOCK *)NULL;
	    }
	}
    }
  return (result);
}

static void
free_database_resources (Database *db)
{
  /* Free cursors, namestrings, MySQL resources, etc. */
  if (db != (Database *)NULL)
    {
      free_database_cursors (db);
#if defined (CACHED_GSQL_LIST_FIELDS)
      /* Free cached table information. */
      free_cached_table_info (db);
#endif
      xfree (db->dbname);
      xfree (db->hostname);
      db->dbname = (char *)NULL;
      db->hostname = (char *)NULL;
    }
}

/* Index where the next error message should go. */
static int mysql_error_index = 0;

static void
gsql_clear_error_message (void)
{
  mysql_error_index = 0;
  pagefunc_set_variable ("mysqlper::mysql-error-message[]", "");
}

/* Pass NULL to use system's error message. */
static void
gsql_save_error_message (Database *db, char *msg)
{
  char error_variable[128];

  sprintf (error_variable, "mysqlper::mysql-error-message[%d]",
	   mysql_error_index);
  mysql_error_index++;

  if (msg == GSQL_DEFAULT_ERRMSG)
    {
      char *message = (char *)NULL;
      if ((db != (Database *)NULL) && (db->mysock != (MYSOCK *)NULL))
	message = mysql_error (db->mysock->sock);
      else
	message = "Not connected to mySQL server";

      if (!empty_string_p (pagefunc_get_variable
			   ("sql::verbose-error-reporting")))
	{
	  BPRINTF_BUFFER *b = bprintf_create_buffer ();
	  bprintf (b, "%s\n[Recent Query: %s]", message,
		   pagefunc_get_variable ("gsql::recent-query"));

	  pagefunc_set_variable (error_variable, b->buffer);
	  page_debug ("MySQL: %s", b->buffer);
	  bprintf_free_buffer (b);
	}
      else
	{
	  pagefunc_set_variable (error_variable, message);
	  page_debug ("mySQL: %s", message);
	}
    }
  else
    {
      pagefunc_set_variable (error_variable, msg);
      page_debug ("mySQL: %s", msg);
    }
}

static int
gsql_number_of_rows (gsql_result *result)
{
  if (result && result->mysql_result && result->mysql_result->handle)
    return (result->mysql_result->handle->affected_rows);
  else
    return (0);
}

static int
gsql_affected_rows (DBCursor *cursor)
{
  MYSQL *sock = cursor->db->mysock->sock;
  int nrows = mysql_affected_rows (sock);

  return (nrows);
}

static void
gsql_data_seek (gsql_result *result, int position)
{
  if (result->mysql_result != (MYSQL_RES *)NULL)
    mysql_data_seek (result->mysql_result, position);
}

static int
gsql_fetch_row (gsql_result *result)
{
  result->mysql_row =
    mysql_fetch_row (result->mysql_result);

  if (result->mysql_row != (MYSQL_ROW)NULL)
    return (GSQL_SUCCESS);
  else
    return (GSQL_NO_DATA_FOUND);
}

static gsql_field *
gsql_fetch_field (gsql_result *result, int i)
{
  if (result->fields != (gsql_field **) NULL)
    return (result->fields[i]);
  else
    return ((gsql_field *)NULL);
}

static int
gsql_query (Database *db, char *query, int save_errors_p)
{
  int status =  mysql_query (db->mysock->sock, query);

  pagefunc_set_variable ("mysqlper::recent-query", query);
  pagefunc_set_variable ("gsql::recent-query", query);

  if (status == -1)
    {
      if (save_errors_p)
	gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);

      return (GSQL_ERROR);
    }
  else
    return (GSQL_SUCCESS);
}

static void
gsql_free_result (gsql_result *result)
{
  int i;
  MYSQL_RES *mr = result->mysql_result;

  if (result->fields != (gsql_field **) NULL)
    {
      i = 0;
      while (result->fields[i] != (gsql_field *) NULL)
	{
	  /* Free the gsql_field structs.  */
	  free (result->fields[i]);
	  i++;
	}
      free (result->fields);
    }

  xfree (result->tablename);

  if (mr != (MYSQL_RES *)NULL) mysql_free_result (mr);

  free (result);
}

static void
initialize_database (Database *db)
{
  db->dbname = (char *) NULL;
  db->hostname = (char *) NULL;
  db->mysock= (MYSOCK *) NULL;

#if defined (CACHED_GSQL_LIST_FIELDS)
  db->table_cache_index = 0;
  db->table_cache_slots = 0;
  db->table_cache_info = (void **)NULL;
#endif

  db->sql_escape_char       = DEFAULT_SQL_ESCAPE_CHARACTER;
  db->sql_truncate_columns  = DEFAULT_SQL_TRUNCATE_COLUMNS;
  db->sql_prefix_tablenames = DEFAULT_SQL_PREFIX_TABLENAMES;
}

#if defined (CACHED_GSQL_LIST_FIELDS)
static void
database_add_cached_table_info (Database *db, gsql_result *gr)
{
  if (db->table_cache_index + 2 > db->table_cache_slots)
    db->table_cache_info = (void **)xrealloc
      (db->table_cache_info,
       ((db->table_cache_slots += 10) * sizeof (gsql_result *)));

  db->table_cache_info[db->table_cache_index++] = (void *)gr;
  db->table_cache_info[db->table_cache_index] = (void *)NULL;
}

/* Search for the first result with a matching tablename.  */
static gsql_result *
database_lookup_cached_table_info (Database *db, char *tablename)
{
  int i;

  for (i = 0; i < db->table_cache_index; i++)
    {
      gsql_result *gr = (gsql_result *)db->table_cache_info[i];
      if ((gr->tablename != (char *) NULL) &&
	  (strcasecmp (tablename, gr->tablename) == 0))
	return (gr);
    }

  return (gsql_result *) NULL;
}
#endif /* CACHED_GSQL_LIST_FIELDS */

static gsql_result *
make_gsql_result (void)
{
  gsql_result *g;
  g = (gsql_result *)xmalloc (sizeof (gsql_result));

  g->mysql_result = (MYSQL_RES *) NULL;
  g->mysql_row = (MYSQL_ROW) NULL;
  g->fields = (gsql_field **) NULL;
  g->tablename = (char *)NULL;

  return (g);
}

static void
initialize_gsql_field (gsql_field *gfield)
{
  gfield->qualifier = (char *) NULL;
  gfield->owner     = (char *) NULL;
  gfield->type_name  = (char *) NULL;
  gfield->precision = 0;
  gfield->scale     = 0;
  gfield->radix     = 0;
}


static int
mysql_num_fields_of_result (MYSQL_RES *result)
{
  if (result != (MYSQL_RES *)NULL)
    return (result->field_count);
  else
    return (0);
}

static gsql_result *
gsql_make_field_array (MYSQL_RES *result)
{
  gsql_result *gr = make_gsql_result ();
  gsql_field *gfield;
  MYSQL_FIELD *mfield;
  int numfields, i;

  gr->mysql_result = result;

  mysql_field_seek (result, 0);
  numfields = mysql_num_fields_of_result (result);

  if (numfields > 0)
    {
      gr->fields = (gsql_field **) xmalloc
	((numfields + 1) * sizeof (gsql_field *));

      for (i = 0; i < numfields; i++)
	{
	  gfield = (gsql_field *)xmalloc (sizeof (gsql_field));
	  initialize_gsql_field (gfield);
	  mfield = mysql_fetch_field (gr->mysql_result);
	  gfield->mysql_field = mfield;
	  gr->fields[i] = gfield;
	}

      gr->fields[i] = (gsql_field *) NULL;
    }

  return (gr);
}

/* We need to create an array of gsql_field structs which have
   the column info. We traverse the MYSQL field list, and
   wrap each field into a gsql_field object. */
static gsql_result *
gsql_store_result (Database *db)
{
  MYSQL_RES *mr = mysql_store_result (db->mysock->sock);
  gsql_result *gr = (gsql_result *) NULL;

  if (mr != (MYSQL_RES *) NULL)
    {
      gr = gsql_make_field_array (mr);
    }
  return (gr);
}

/* The options  qualifier,owner,name,type are for ODBC
   compatibility. */
static gsql_result *
gsql_db_list_tables (Database *db,
		     char *table_qualifier, 
		     char *table_owner,
		     char *table_name,
		     char *table_type)
{
  gsql_result *gr = (gsql_result *) NULL;
  MYSQL_RES *mr = mysql_list_tables (db->mysock->sock, NULL);

  if (mr != (MYSQL_RES *) NULL)
    {
      gr = gsql_make_field_array (mr);
    }
  return (gr);
}

/****************************************************************
 * Field properties
 *
 * name, length, datatype, is_primary_key, not_null
 *
 ****************************************************************/

/* MYSQL 2.0 uses UNIQUE fields instead of a primary key field. */
#ifndef IS_UNIQUE
#  define IS_UNIQUE IS_PRI_KEY
#endif

#define gsql_field_name(f) (f->mysql_field->name)
#define gsql_field_table(f) (f->mysql_field->table)
#define gsql_field_length(f) (f->mysql_field->length)
#define gsql_field_is_primary_key(f) (IS_PRI_KEY (f->mysql_field->flags))
#define gsql_field_is_unique(f) (IS_UNIQUE (f->mysql_field->flags))
#define gsql_field_is_not_null(f) (IS_NOT_NULL (f->mysql_field->flags))

/* Future ODBC compatibility */
#define gsql_field_qualifier(f) (f->qualifier)
#define gsql_field_owner(f) (f->owner)
#define gsql_field_type_name(f) (f->type_name)
#define gsql_field_precision(f) (f->precision)
#define gsql_field_scale(f) (f->scale)
#define gsql_field_radix(f) (f->radix)

static int
gsql_raw_field_type (gsql_field *field)
{
  return ((int)field->mysql_field->type);
}

static int
gsql_field_type (gsql_field *field)
{
  switch (field->mysql_field->type)
    {
    case FIELD_TYPE_CHAR:		return (GSQL_CHAR);
      /*    case FIELD_TYPE_NUMERIC:	return (GSQL_NUMERIC); */
    case FIELD_TYPE_DECIMAL:		return (GSQL_DECIMAL);
    case FIELD_TYPE_INT24:		return (GSQL_INTEGER);
      /* case FIELD_TYPE_SMALLINT:	return (GSQL_SMALLINT); */
    case FIELD_TYPE_FLOAT:		return (GSQL_FLOAT);
      /* case FIELD_TYPE_REAL:		return (GSQL_REAL); */
    case FIELD_TYPE_DOUBLE:		return (GSQL_DOUBLE);
    case FIELD_TYPE_STRING:		return (GSQL_VARCHAR);
    case FIELD_TYPE_SHORT:		return (GSQL_INTEGER);
    case FIELD_TYPE_LONG:		return (GSQL_INTEGER);
    case FIELD_TYPE_NULL:		return (GSQL_NULL);

      /* non-standard ANSI follows */
    case FIELD_TYPE_LONGLONG:		return (GSQL_INTEGER);
    case FIELD_TYPE_TINY_BLOB:		return (GSQL_TINY_BLOB);
    case FIELD_TYPE_MEDIUM_BLOB:	return (GSQL_MEDIUM_BLOB);
    case FIELD_TYPE_LONG_BLOB:		return (GSQL_LONG_BLOB);
    case FIELD_TYPE_BLOB:		return (GSQL_BLOB);
    case FIELD_TYPE_VAR_STRING:		return (GSQL_VAR_STRING);
    case FIELD_TYPE_DATE:		return (GSQL_DATE);
    case FIELD_TYPE_TIME:		return (GSQL_INTEGER);
#if defined (FIELD_TYPE_NEWDATE)
    case FIELD_TYPE_NEWDATE:		return (GSQL_INTEGER);
#endif
    case FIELD_TYPE_DATETIME:		return (GSQL_CHAR);
    default:				return (GSQL_ANY);
    }
}

/* This needs to build the array of gsql_fields. */
static gsql_result *
gsql_list_fields (Database *db, char *tablename)
{
  gsql_result *gr = (gsql_result *) NULL;
  MYSQL_RES *mr = (MYSQL_RES *)NULL;

#if defined (CACHED_GSQL_LIST_FIELDS)
  gr = database_lookup_cached_table_info (db, tablename);
  if (gr != (gsql_result *) NULL) return (gr);
#endif /* CACHED_GSQL_LIST_FIELDS */

  mr = mysql_list_fields (db->mysock->sock, tablename, NULL);

  if (mr != (MYSQL_RES *) NULL)
    {
      gr = gsql_make_field_array (mr);
      gr->tablename = strdup (tablename);
#if defined (CACHED_GSQL_LIST_FIELDS)
      database_add_cached_table_info (db, gr);
#endif
    }

  return (gr);
}

static int
gsql_num_fields (gsql_result *result)
{
  if (result->mysql_result != (MYSQL_RES *) NULL)
    return (mysql_num_fields_of_result (result->mysql_result));
  else
    return (0);
}

/* Fetch data from RESULT at column COL.
   A new string is consed.*/
static char *
gsql_get_column (gsql_result *result, int col)
{
  if ((result->mysql_row != (MYSQL_ROW) NULL) && (result->mysql_row[col]))
    return (strdup (result->mysql_row[col]));
  else
    return ((char *)NULL);
}

/* From the result set of a mysql_list_tables command,
   return the column which has the table name.  This
   is column 0. */
static char *
gsql_get_column_table_name (gsql_result *gr)
{
  return (gsql_get_column (gr, 0));
}


static void
gsql_connect (char *dsn, Database *db)
{
  char *dbhost = dsn_lookup ("host", dsn);
  char *dbname = dsn_lookup ("database", dsn);
  char *user = dsn_lookup ("uid", dsn);
  char *pass = dsn_lookup ("pwd", dsn);

  db->connected = 0;
  db->dbname = dbname;
  db->hostname = dbhost;

  if ((dbhost != (char *) NULL) && (dbname != (char *)NULL))
    {
      MYSOCK *mysock = mysock_connect (dsn);

      if (mysock != (MYSOCK *)NULL)
	{
	  if (mysql_select_db (mysock->sock, dbname) == -1)
	    {
	      BPRINTF_BUFFER *e = bprintf_create_buffer ();
	      bprintf (e, "Unable to connect to database: `%s' on `%s'",
		       dbname, dbhost);
	      gsql_save_error_message (db, e->buffer);
	      bprintf_free_buffer (e);
	    }
	  else
	    {
	      db->connected = 1;
	      db->mysock = mysock;
	    }
	}
      else
	{
	  BPRINTF_BUFFER *e = bprintf_create_buffer ();
	  bprintf (e, "Unable to connect to server! ");
	  bprintf (e, "(HOST: `%s'; DATABASE: `%s'; UID: `%s'; PWD: `%s')",
		   dbhost, dbname, user ? user : "[not supplied]",
		   pass ? "[supplied]" : "[not supplied]");
	  gsql_save_error_message (db, e->buffer);
	  bprintf_free_buffer (e);
	}
    }
  else
    {
      BPRINTF_BUFFER *e = bprintf_create_buffer ();
      bprintf (e, "Unable to connect to server! ");
      bprintf (e, "(HOST: `%s'; DATABASE: `%s'; UID: `%s'; PWD: `%s')",
	       dbhost ? dbhost : "[not supplied]",
	       dbname ? dbname : "[not supplied]",
	       user ? user : "[not supplied]",
	       pass ? "[supplied]" : "[not supplied]");
	  gsql_save_error_message (db, e->buffer);
	  bprintf_free_buffer (e);
    }

  xfree (user);
  xfree (pass);
}

static void
gsql_close (Database *db)
{
  if ((db->connected == 1) && (db->mysock != (MYSOCK *)NULL))
    mysock_disconnect (db->mysock);
}

static Database *get_dbref (Package *);

static void
pf_database_shutdown (PFunArgs)
{
  Database *db = get_dbref (vars);

  if ((database_environment_level == 0) || (db == (Database *)NULL))
    return;
  else
    mysock_true_shutdown = 1;
}
  
/* Returns an array of the databases available on HOST.
   If VARNAME is supplied, the array is placed into that
   variable instead. */
static void
pf_host_databases (PFunArgs)
{
  char *host = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *resultvar = mhtml_evaluate_string (get_value (vars, "result"));
  MYSQL *sock;

  /* No errors yet! */
  pagefunc_set_variable ("mysqlper::mysql-error-message[]", "");

  if (empty_string_p (host))
    {
      xfree (host);
      host = strdup ("localhost");
    }

  if ((mysql_init (sock) != NULL) &&
      ((sock = mysql_real_connect
	(sock, host, NULL, NULL, NULL, 0, NULL, 0)) != NULL))
    {
      MYSQL_RES *result = mysql_list_dbs (sock, NULL);
      int nrows = (result ? mysql_num_rows (result) : 0);

      if (nrows != 0)
	{
	  int count = 0;
	  char **dbnames = (char **) xmalloc ((nrows + 1) * sizeof (char *));
	  MYSQL_ROW mysql_row;

	  /* Loop over rows returned; the db name will be passed in the first
	     field of each 'row'.  Add names to the result array.  */
	  while ((mysql_row = mysql_fetch_row (result)) != (MYSQL_ROW)NULL)
	    dbnames[count++] = strdup (mysql_row[0]);

	  dbnames[count] = (char *) NULL;

	  if (!empty_string_p (resultvar))
	    {
	      symbol_store_array (resultvar, dbnames);
	    }
	  else
	    {
	      register int i;

	      for (i = 0; dbnames[i] != (char *)NULL; i++)
		{
		  bprintf_insert (page, start, "%s\n", dbnames[i]);
		  start += 1 + strlen (dbnames[i]);
		  free (dbnames[i]);
		}
	      free (dbnames);
	      *newstart = start;
	    }
	}

      if (result != (MYSQL_RES *)NULL) mysql_free_result (result);
      mysql_close (sock);
    }
  else
    {
      gsql_save_error_message
	((Database *)NULL, "HOST-DATABASES: mySQL Connect Failed");
    }

  xfree (host);
  xfree (resultvar);
}

/* Transactions are not yet implemented for MYSQL */
static int
gsql_transact_internal (Database *db, char *action_arg)
{
  return (GSQL_ERROR);
}
