/* odbcfuncs.c:  ODBC database compatibility layer. */

/* Author: Henry Minsky (hqm@ua.com) Wed Oct  2 16:28:36 1996.
   Heavy Hacks: Brian J. Fox (bfox@ai.mit.edu) */

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

/* ODBC compatibility layer */
/* ODBC includes from OpenLink Software drivers */

#if defined (OPENLINK_UDBC_CLIENT)
# include <libudbc.h>
  /* The OpenLink SunOS UDBC libraries need this function for some reason. */
  int __ansi_fflush (FILE *f) { return (fflush (f)); }

# define SQLRETCODE int
  extern RETCODE _UDBC_GetInfo (HDBC, UWORD, PTR, SWORD, SWORD FAR *);

/* C to SQL datatype mapping */
# define SQL_SIGNED_OFFSET              (-20)
# define SQL_UNSIGNED_OFFSET            (-22)

#if !defined (SQL_C_SLONG)
# define SQL_C_SLONG                    (SQL_C_LONG  + SQL_SIGNED_OFFSET)
#endif

#if !defined (SQL_C_SSHORT)
# define SQL_C_SSHORT                   (SQL_C_SHORT + SQL_SIGNED_OFFSET)
#endif

/* options for SQLSetConnectOption/SQLGetConnectOption */
# define SQL_CURRENT_QUALIFIER          109
# define SQL_ODBC_CURSORS               110
# define SQL_QUIET_MODE                 111
# define SQL_PACKET_SIZE                112

/* SQLGetInfo infor number */
# define SQL_NON_NULLABLE_COLUMNS       75
# define SQL_DRIVER_HLIB                76
# define SQL_QUALIFIER_LOCATION         114
#else
# include <sqlext.h>
#endif /* OPENLINK_UDBC */

/****************************************************************/

#define DEFAULT_SQL_ESCAPE_CHARACTER  '\''
#define DEFAULT_SQL_TRUNCATE_COLUMNS  1
#define DEFAULT_SQL_PREFIX_TABLENAMES 0

/* ODBC-specific commands */
static void pf_odbc_get_info (PFunArgs);
static void pf_odbc_get_connect_option (PFunArgs);
static void pf_odbc_set_connect_option (PFunArgs);

static PFunDesc func_table[] =
{
  { "ODBCPER::WITH-OPEN-DATABASE",	1, 0, pf_with_open_database },
  { "ODBCPER::DATABASE-EXEC-QUERY",	0, 0, pf_database_exec_query },
  { "ODBCPER::DATABASE-EXEC-SQL",	0, 0, pf_database_exec_sql },
  { "ODBCPER::DATABASE-NEXT-RECORD",	0, 0, pf_database_next_record },
  { "ODBCPER::DATABASE-SAVE-RECORD",	0, 0, pf_database_save_record },
  { "ODBCPER::DATABASE-DELETE-RECORD",	0, 0, pf_database_delete_record },
  { "ODBCPER::DATABASE-LOAD-RECORD",	0, 0, pf_database_load_record },
  { "ODBCPER::DATABASE-SAVE-PACKAGE",	0, 0, pf_database_save_package },
  { "ODBCPER::PACKAGE-TO-TABLE",	0, 0, pf_package_to_table },
  { "ODBCPER::NUMBER-OF-ROWS",		0, 0, pf_database_num_rows },
  { "ODBCPER::AFFECTED-ROWS",		0, 0, pf_database_affected_rows },
  { "ODBCPER::SET-ROW-POSITION",	0, 0, pf_database_set_pos },
  { "ODBCPER::DATABASE-QUERY",		0, 0, pf_database_query },
  { "ODBCPER::HOST-DATABASES",		0, 0, pf_host_databases },
  { "ODBCPER::DATABASE-TABLES",		0, 0, pf_database_tables },
  { "ODBCPER::DATABASE-TABLES-INFO",	0, 0, pf_database_tables_info },
  { "ODBCPER::DATABASE-TABLE-INFO",	0, 0, pf_database_tables_info },
  { "ODBCPER::DATABASE-COLUMNS",	0, 0, pf_database_columns },
  { "ODBCPER::DATABASE-COLUMN-INFO",	0, 0, pf_database_column_info },
  { "ODBCPER::DATABASE-COLUMNS-INFO",	0, 0, pf_database_columns_info },
  { "ODBCPER::DATABASE-QUERY-INFO",	0, 0, pf_database_query_info },
  { "ODBCPER::DATABASE-SET-OPTIONS",	0, 0, pf_database_set_options },
  { "ODBCPER::CURSOR-GET-COLUMN",	0, 0, pf_cursor_get_column },
  { "ODBCPER::QUERY-GET-COLUMN",	0, 0, pf_query_get_column },
  { "ODBCPER::SQL-TRANSACT",		0, 0, pf_sql_transact },
  { "ODBCPER::GET-INFO",		0, 0, pf_odbc_get_info },
  { "ODBCPER::SET-CONNECT-OPTION",	0, 0, pf_odbc_set_connect_option },
  { "ODBCPER::GET-CONNECT-OPTION",	0, 0, pf_odbc_get_connect_option },
  { (char *)NULL,			0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_iodbc_functions)
DEFINE_SECTION (ODBC-DATABASE-EXTENSIONS, ODBC; database; records,
"<Meta-HTML> PRO is optionally delivered with a complete ODBC\n\
compliant database module, providing a useful set of database commands\n\
for operating on commercial databases, such as ORACLE or SyBase. These\n\
extensions allow true SQL database interaction at many levels,\n\
providing a clean and flexible abstraction.", "")

DEFVAR (odbcper::odbc-error-message,
"An array of strings containing any error messages generated by the\n\
last call to an ODBC database.  Don't use this variable, use\n\
the function <funref generic-sql-interface sql::sql-error-message> instead.")

DEFVAR (odbcper::recent-query,
"The last query sent to an ODBC database for execution.  Don't use this\n\
variable, use the function <funref generic-sql-interface sql::recent-query>\n\
instead.")

DEFMACROX (pf_odbcper::with_open_database, ,
"See <funref generic-sql-interface sql::with-open-database>.")
DEFUNX (pf_odbcper::database_exec_query, ,
"See <funref generic-sql-interface sql::database-exec-query>.")
DEFUNX (pf_odbcper::database_exec_sql, ,
"See <funref generic-sql-interface sql::database-exec-sql>.")
DEFUNX (pf_odbcper::database_next_record, ,
"See <funref generic-sql-interface sql::database-next-record>.")
DEFUNX (pf_odbcper::database_save_record, ,
"See <funref generic-sql-interface sql::database-save-record>.")
DEFUNX (pf_odbcper::database_delete_record, ,
"See <funref generic-sql-interface sql::database-delete-record>.")
DEFUNX (pf_odbcper::database_load_record, ,
"See <funref generic-sql-interface sql::database-load-record>.")
DEFUNX (pf_odbcper::database_save_package, ,
"See <funref generic-sql-interface sql::database-save-package>.")
DEFUNX (pf_odbcper::database_num_rows, ,
"See <funref generic-sql-interface sql::database-num-rows>.")
DEFUNX (pf_odbcper::database_affected_rows, ,
"See <funref generic-sql-interface sql::database-affected-rows>.")
DEFUNX (pf_odbcper::database_set_pos, ,
"See <funref generic-sql-interface sql::database-set-po>.")
DEFUNX (pf_odbcper::database_query, ,
"See <funref generic-sql-interface sql::database-query>.")
DEFUNX (pf_odbcper::host_databases, ,
"See <funref generic-sql-interface sql::host-databases>.")
DEFUNX (pf_odbcper::database_tables, ,
"See <funref generic-sql-interface sql::database-tables>.")
DEFUNX (pf_odbcper::database_tables_info, ,
"See <funref generic-sql-interface sql::database-tables-info>.")
DEFUNX (pf_odbcper::database_tables_info, ,
"See <funref generic-sql-interface sql::database-tables-info>.")
DEFUNX (pf_odbcper::database_columns, ,
"See <funref generic-sql-interface sql::database-columns>.")
DEFUNX (pf_odbcper::database_column_info, ,
"See <funref generic-sql-interface sql::database-column-info>.")
DEFUNX (pf_odbcper::database_columns_info, ,
"See <funref generic-sql-interface sql::database-columns-info>.")
DEFUNX (pf_odbcper::database_query_info, ,
"See <funref generic-sql-interface sql::database-query-info>.")
DEFUNX (pf_odbcper::database_set_options, ,
"See <funref generic-sql-interface sql::database-set-options>.")
DEFUNX (pf_odbcper::cursor_get_column, ,
"See <funref generic-sql-interface sql::cursor-get-column>.")
DEFUNX (pf_odbcper::query_get_column, ,
"See <funref generic-sql-interface sql::query-get-column>.")
DEFUNX (pf_odbcper::sql_transact, ,
"See <funref generic-sql-interface sql::sql-transact>.")

/****************************************************************
 * The Database object:
 *
 * Contains a stack of cursors, and information about the open
 * database connection.
 ****************************************************************/

typedef struct
{
  /* ODBC-specific data */
  char *dsn;
  int reference_count;
  HENV    henv;
  HDBC    hdbc;
  HSTMT   hstmt;
} MYODBC;

static MYODBC **all_socks = (MYODBC **)NULL;
static int as_index = 0;
static int as_slots = 0;

typedef struct
{
  IStack *cursors;
  int connected;		/* 1 if db connection open, 0 otherwise */
  char sql_escape_char;
  int  sql_truncate_columns;
  int  sql_prefix_tablenames;

  MYODBC *myodbc;

#if defined (CACHED_GSQL_LIST_FIELDS)
  /* Cache the saved information about fields of tables */
  void **table_cache_info;	/* An array of gsql_result. */
  int table_cache_index;
  int table_cache_slots;
#endif
} Database;

typedef struct
{
  char *name;
  char *table;
  char *qualifier;
  char *owner;
  char *type_name;
  int precision;
  int scale;
  int radix;
  int type;
  int length;
  int nullable;
} gsql_field;

typedef struct
{
  HSTMT hstmt;
  int numfields;
  Database *db;
  gsql_field **fields;
  char *tablename;
} gsql_result;

/* For ODBC, a cursor points to a gsql_result object. */
typedef struct
{
  gsql_result *result;		/* The results of <database-exec-query..> */
  Database *db;			/* Associated database connection. */
  int index;			/* A unique index number for this cursor within
				   the stack of cursors for a database. */
} DBCursor;

static void free_database_cursors (Database *db);
static void gsql_free_result (gsql_result *result);

#if defined (CACHED_GSQL_LIST_FIELDS)
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

static void
odbc_disconnect (MYODBC *myodbc)
{
  if (myodbc != (MYODBC *)NULL)
    {
      myodbc->reference_count--;
#if 0
      if (myodbc->reference_count < 1)
        {
	  register int i;

          if (myodbc->hdbc != (HDBC)NULL)
	    SQLDisconnect (myodbc->hdbc);

	  if (myodbc->hdbc)
	    SQLFreeConnect (myodbc->hdbc);

	  if (myodbc->henv)
	    SQLFreeEnv (myodbc->henv);

	  xfree (myodbc->dsn);

	  for (i = 0; i < as_index; i++)
	    {
	      if (myodbc == all_socks[i])
		{
		  while ((all_socks[i] = all_socks[i + 1])) i++;
		  as_index--;
		  break;
		}
	    }
	}
      free (myodbc);
#endif
    }
}

static MYODBC *
odbc_connect (char *dsn)
{
  MYODBC *result = (MYODBC *)NULL;

  if (dsn != (char *)NULL)
    {
      register int i;

      for (i = 0; i < as_index; i++)
	if (strcmp (all_socks[i]->dsn, dsn) == 0)
	  {
	    result = all_socks[i];
	    result->reference_count++;
	    break;
	  }

      if (result == (MYODBC *)NULL)
	{
	  MYODBC *temp = (MYODBC *)xmalloc (sizeof (MYODBC));
	  char buf[257];
	  short buflen;

	  if (SQLAllocEnv (&(temp->henv)) == SQL_SUCCESS)
	    if (SQLAllocConnect (temp->henv, &(temp->hdbc)) == SQL_SUCCESS)
	      {
		int status = SQLDriverConnect (temp->hdbc, 0,
					       (UCHAR *) dsn, SQL_NTS,
					       (UCHAR *) buf, sizeof (buf),
					       &buflen, SQL_DRIVER_COMPLETE);

		if ((status == SQL_SUCCESS) ||
		    (status == SQL_SUCCESS_WITH_INFO))
		  {
		    result = temp;
		    result->reference_count = 1;
		    result->dsn = strdup (dsn);

		    if (as_index + 2 >= as_slots)
		      all_socks = (MYODBC **)
			xrealloc (all_socks,
				  (as_slots += 10) * sizeof (MYODBC *));
		    all_socks[as_index++] = result;
		    all_socks[as_index] = (MYODBC *)NULL;
		  }
		else
		  {
		    free (temp);
		  }
	      }
	}
    }

  return (result);
}

static void
free_database_resources (Database *db)
{
  /* Free cursors, namestrings, hdbc resources, etc. */
  free_database_cursors (db);

#if defined (CACHED_GSQL_LIST_FIELDS)
  /* Free cached table information. */
  free_cached_table_info (db);
#endif
}

/* Some accessors for parsing database references from metahtml variables */
static Database * get_dbref_internal (Package *vars, int *dbindex);
static Database * get_dbref (Package *vars);

static int gsql_database_connected (Database *db);

/* The index of where the next error message should be stored. */
static int odbc_error_index = 0;

static void
gsql_clear_error_message (void)
{
  odbc_error_index = 0;
  pagefunc_set_variable ("odbcper::odbc-error-message[]", "");
}

/* Pass NULL to use system's error message. */
static void
gsql_save_error_message (Database *db, char *msg)
{
  unsigned char buf[256];
  unsigned char sqlstate[15];
  char odbc_error_variable[128];
  BPRINTF_BUFFER *err;
  MYODBC *myodbc = db->myodbc;

  if (myodbc != (MYODBC *)NULL)
    {
      /* Get statement errors */
      while (SQLError (myodbc->henv, myodbc->hdbc, myodbc->hstmt,
		       sqlstate, NULL, buf, sizeof(buf), NULL) == SQL_SUCCESS)
	{
	  sprintf (odbc_error_variable, "odbcper::odbc-error-message[%d]",
		   odbc_error_index);
	  odbc_error_index++;

	  err = bprintf_create_buffer ();
	  bprintf (err, "HSTMT: SQLSTATE=%s %s\n", sqlstate, buf);
	  pagefunc_set_variable (odbc_error_variable, err->buffer);
	  page_debug ("ODBC: %s", err->buffer);
	  bprintf_free_buffer (err);
	}

      /* Get connection errors */
      while (SQLError (myodbc->henv, myodbc->hdbc, SQL_NULL_HSTMT,
		       sqlstate, NULL, buf, sizeof(buf), NULL) == SQL_SUCCESS)
	{
	  sprintf (odbc_error_variable, "odbcper::odbc-error-message[%d]",
		   odbc_error_index);
	  odbc_error_index++;
	  err = bprintf_create_buffer ();
	  bprintf (err, "Connection: SQLSTATE=%s %s\n", sqlstate, buf);
	  pagefunc_set_variable (odbc_error_variable,  err->buffer);
	  page_debug ("ODBC: %s", err->buffer);
	  bprintf_free_buffer (err);
	}

      /* Get environmental errors */
      while (SQLError (myodbc->henv, SQL_NULL_HDBC, SQL_NULL_HSTMT,
		       sqlstate, NULL, buf, sizeof(buf), NULL) == SQL_SUCCESS)
	{
	  sprintf (odbc_error_variable, "odbcper::odbc-error-message[%d]",
		   odbc_error_index);
	  odbc_error_index++;
	  err = bprintf_create_buffer ();
	  bprintf (err, "Environment: SQLSTATE=%s %s\n", sqlstate, buf);
	  pagefunc_set_variable (odbc_error_variable, err->buffer);
	  page_debug ("ODBC: %s", err->buffer);
	  bprintf_free_buffer (err);
	}
    }

  if (msg == GSQL_DEFAULT_ERRMSG)
    msg =  "[mhtml: Bad DSN or no database]";

  sprintf (odbc_error_variable, "odbcper::odbc-error-message[%d]", 
	   odbc_error_index);
  odbc_error_index++;
  page_debug ("ODBCPER: %s", msg);
  pagefunc_set_variable (odbc_error_variable, msg);
}

static int
gsql_number_of_rows (gsql_result *result)
{
  SDWORD nrows = 0;

  gsql_clear_error_message ();

  if (result->hstmt != (HSTMT) NULL)
    {
      if (SQLRowCount (result->hstmt, &nrows) != SQL_SUCCESS)
	gsql_save_error_message (result->db, "SQLRowCount");
    }

  return ((int) nrows);
}

static int
gsql_affected_rows (DBCursor *cursor)
{
  if ((cursor != (DBCursor *)NULL) &&
      (cursor->result != (gsql_result *)NULL))
    return (gsql_number_of_rows (cursor->result));
  return (0);
}  

static void
gsql_data_seek (gsql_result *result, int position)
{
  /* Use SQLSetPos, if you can figure out how. */
}

static int
odbc_to_gsql_status (int status)
{
  switch (status)
    {
    case SQL_INVALID_HANDLE:	return (GSQL_INVALID_HANDLE);
    case SQL_ERROR:		return (GSQL_ERROR);
    case SQL_SUCCESS:		return (GSQL_SUCCESS);
    case SQL_SUCCESS_WITH_INFO:	return (GSQL_SUCCESS_WITH_INFO);
    case SQL_NO_DATA_FOUND:	return (GSQL_NO_DATA_FOUND);
    default:			return (GSQL_ERROR);
    }
}

static int
gsql_fetch_row (gsql_result *result)
{
  int status = SQLFetch (result->hstmt);
  return (odbc_to_gsql_status (status));
}

static gsql_field *
gsql_fetch_field (gsql_result *result, int i)
{
  if (result->fields != (gsql_field **) NULL)
    return (result->fields[i]);
  else
    return ((gsql_field *)NULL);
}

/* We allocate a new hstmt for this query, and leave it sitting in the
   db->myodbc->hstmt slot.

   We need to free these things eventually, but that is the job of
   gsql_store_result() and gsql_free_result().  gsql_store_result() copies
   the hstmt pointer to a gsql_result structure, and sets the db->myodbc->hstmt
   slot to NULL.  gsql_free_result is responsible for calling SQLFreeStmt().

   Thus, a call to gsql_query() should always be followed by a call to
   gsql_store_result(), and the result must eventually be freed using
   gsql_free_result(). */
static int
gsql_query (Database *db, char *query, int save_errors_p)
{
  MYODBC *myodbc = db->myodbc;
  int result = GSQL_ERROR;

  pagefunc_set_variable ("odbcper::recent-query", query);
  pagefunc_set_variable ("gsql::recent-query", query);

  if (db->connected)
    {
      /* If there is an old hstmt sitting here, free it. */
      if (myodbc->hstmt)
	SQLFreeStmt (myodbc->hstmt, SQL_DROP);

      if (SQLAllocStmt (myodbc->hdbc, &(myodbc->hstmt)) == SQL_SUCCESS)
	{
	  if (SQLExecDirect (myodbc->hstmt, (UCHAR *) query, SQL_NTS)
	      != SQL_SUCCESS)
	    {
	      if (save_errors_p)
		gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	    }
	  else
	    result = GSQL_SUCCESS;
	}
    }

  return (result);
}

static void
free_gsql_field (gsql_field *field)
{
  if (field != (gsql_field *)NULL)
    {
      xfree (field->name);
      xfree (field->table);
      free (field);
    }
}

static void
gsql_free_result (gsql_result *result)
{
  int i;
  HSTMT hstmt = result->hstmt;

  if (result->fields != (gsql_field **) NULL)
    {
      i = 0;
      while (result->fields[i] != (gsql_field *) NULL)
	{
	  /* Free the gsql_field structs.  */
	  free_gsql_field (result->fields[i]);
	  i++;
	}
      free (result->fields);
    }

  xfree (result->tablename);

  if (hstmt)
    SQLFreeStmt (hstmt, SQL_DROP);

  free (result);
}

/* Initialize the ODBC-specific portions of the database structure. */
static void
initialize_database (Database *db)
{
  db->myodbc = (MYODBC *)NULL;
  db->connected = 0;

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

  memset (g, 0, sizeof (gsql_result));

  g->hstmt = (HSTMT) NULL;
  g->fields = (gsql_field **) NULL;
  g->numfields = -1;
  g->tablename = (char *) NULL;

  return (g);
}

static void
initialize_gsql_field (gsql_field *gfield)
{

  gfield->table    = (char *)NULL;
  gfield->name     = (char *) NULL;
  gfield->type     = 0;
  gfield->length   = 0;
  gfield->nullable = 0;

  gfield->qualifier = (char *) NULL;
  gfield->owner     = (char *) NULL;
  gfield->type_name  = (char *) NULL;
  gfield->precision = 0;
  gfield->scale     = 0;
  gfield->radix     = 0;

}


/* Reads the field info using SQLDescribeCol and creates
   a gsql_field with the descriptive information filled in. */
static gsql_field *
get_gsql_field_from_result (gsql_result *result, int colnum)
{
  gsql_field *gfield = (gsql_field *)xmalloc (sizeof (gsql_field));
  short colType;
  UDWORD colPrecision;
  short colScale;
  short colNullable;
  char colName[512];
  RETCODE status;

  colName[0] = '\000';

  initialize_gsql_field (gfield);

  /* Get column name */
  status = SQLDescribeCol
    (result->hstmt, colnum, (UCHAR *) colName,
     sizeof (colName), NULL, &colType, &colPrecision,
     &colScale, &colNullable);

  if (status != SQL_SUCCESS)
    {
      gsql_save_error_message (result->db, "SQLDescribeCol");
    }
  else
    {
#if defined (SQL_COLUMN_TABLE_NAME)
      char tablename[512];
      SWORD FAR tablename_len = 0;
      SDWORD FAR ignore_return = 0;

      status = SQLColAttributes
	(result->hstmt, colnum, SQL_COLUMN_TABLE_NAME,
	 (PTR)tablename, sizeof (tablename), &tablename_len, &ignore_return);

      if (status == SQL_SUCCESS)
	{
	  tablename[tablename_len] = '\0';
	  gfield->table = strdup (tablename);
	}
#endif /* SQL_COLUMN_TABLE_NAME */

      gfield->name = strdup (colName);
      gfield->type = colType;
      gfield->length = colPrecision;
      gfield->nullable = colNullable;
    }

  return (gfield);
}

/* Loop over the columns of this set, and create a field structure for
   each column. This is done by using the SQLDescribeCol() function,
   to probe each column in sequence.*/
static gsql_result *
gsql_make_field_array (gsql_result *gr)
{
  gsql_field *gfield;
  SWORD numfields;
  int i;

  if (SQLNumResultCols (gr->hstmt, &numfields) != SQL_SUCCESS)
    return (NULL);

  gr->numfields = numfields;

  if (numfields > 0)
    {
      gr->fields = (gsql_field **)xmalloc
	((numfields + 1) * sizeof (gsql_field *));

      for (i = 0; i < numfields; i++)
	{
	  /* ODBC column numbers start at 1, not 0 */
	  gfield = get_gsql_field_from_result (gr, i + 1);
	  gr->fields[i] = gfield;
	}

      gr->fields[i] = (gsql_field *) NULL;
    }

  return (gr);
}

/* We need to create an array of gsql_field structs which have the
   column info. We iterate over the columns, and create a gsql_field
   object containing the info about that field. */
static gsql_result *
gsql_store_result (Database *db)
{
  gsql_result *gr = (gsql_result *)NULL;

  if (db->myodbc->hstmt != (HSTMT)NULL)
    {
      gr = make_gsql_result ();
      gr->db = db;
      gr->hstmt = db->myodbc->hstmt;	/* Copy the hstmt to the result */
      db->myodbc->hstmt = (HSTMT)NULL;
      gsql_make_field_array (gr);
    }
  return (gr);
}

/* We need also a separate database-table-info command, like the
   database-field-info command, which you can use to get more info
   about a specific table. */
static gsql_result *
gsql_db_list_tables (Database *db, 
		     char *table_qualifier, 
		     char *table_owner,
		     char *table_name,
		     char *table_type)
{
  gsql_result *gr = (gsql_result *) NULL;
  HSTMT hstmt;
  RETCODE status;

  /* Issue an SQLTables request, and then create a result set. */
  gsql_clear_error_message ();

  if (table_qualifier == (char *) NULL)
    table_qualifier = "";
  if (table_owner == (char *) NULL)
    table_owner = "";
  if (table_name == (char *) NULL)
    table_name = "";
  if (table_type == (char *) NULL)
    table_type = "";

  if (SQLAllocStmt (db->myodbc->hdbc, &hstmt) != SQL_SUCCESS)
    {
      gsql_save_error_message (db, "SQLAllocStmt");
      return ((gsql_result *)NULL);
    }

  status = SQLTables (hstmt,
		      (unsigned char *)table_qualifier, SQL_NTS,
		      (unsigned char *)table_owner, SQL_NTS,
		      (unsigned char *)table_name, SQL_NTS,
		      (unsigned char *)table_type, SQL_NTS);

  if (status != SQL_SUCCESS)
    {
      gsql_save_error_message (db, "SQLTables");
      return ((gsql_result *)NULL);
    }

  /* gsql_store_result() will copy the HSTMT to a result struct,
     and it will eventually be freed when someone calls gsql_free_result. */
  db->myodbc->hstmt = hstmt;
  gr = gsql_store_result (db);

  return (gr);
}

/****************************************************************
 * Field properties
 *
 * name, length, datatype, is_primary_key, not_null
 *
 ****************************************************************/

#define gsql_field_name(field) (field->name)
#define gsql_field_table(field) (field->table)
#define gsql_field_type(field) (field->type)
#define gsql_raw_field_type(field) (field->type)
#define gsql_field_length(field) (field->length)
#define gsql_field_is_unique(field) 0
#define gsql_field_is_not_null(field) (field->nullable == SQL_NO_NULLS)

#define gsql_field_qualifier(f) (f->qualifier)
#define gsql_field_owner(f) (f->owner)
#define gsql_field_type_name(f) (f->type_name)
#define gsql_field_precision(f) (f->precision)
#define gsql_field_scale(f) (f->scale)
#define gsql_field_radix(f) (f->radix)


/* Destructively modifies the input string, putting
   null terminators in place of the separator char.
   Returns a pointer to the start of next token, or NULL.
   */
static char *
gettoken (char *str, char sepchar)
{
  if ((str == (char *) NULL) || (*str == (char) NULL)) return (char *) NULL;

  /* Go until we find a separator char, and zero it out. */
  while (*str) {
    if (*str == sepchar)
      {
	*str = '\000';
	return str+1;
      }
    str++;
  }

  return (char *) NULL;
}

/* This needs to build the array of gsql_fields. 

   The resulting array of fields is cached in the database struct,
   if, and only if, the define CACHED_GSQL_LIST_FIELDS is set.

   In that case, the caller should *not* attempt to free the gsql_result
   object which is returned from this function, since it needs to persist
   in the database table info cache until the database is closed. */
static gsql_result *
gsql_list_fields (Database *db, char *tablename)
{
  register int i;
  gsql_result *gr = (gsql_result *)NULL;

#if defined (CACHED_GSQL_LIST_FIELDS)
  gr = database_lookup_cached_table_info (db, tablename);
  if (gr != (gsql_result *) NULL) return (gr);
#endif /* CACHED_GSQL_LIST_FIELDS */

  {
    char *p1, *p2, *p3;
    char *owner, *basenam, *qualifier;

#define STR_LEN 128+1
#define REM_LEN 254+1

    HSTMT hstmt;
    UCHAR szQualifier[STR_LEN], szOwner[STR_LEN];
    UCHAR szTableName[STR_LEN], szColName[STR_LEN];
    UCHAR szTypeName[STR_LEN], szRemarks[REM_LEN];
    SDWORD Precision, Length;
    SWORD DataType, Scale, Radix, Nullable;

    SDWORD cbQualifier, cbOwner, cbTableName, cbColName;
    SDWORD cbTypeName, cbRemarks, cbDataType, cbPrecision;
    SDWORD cbLength, cbScale, cbRadix, cbNullable;

    RETCODE retcode;

    /* Pick out the table parts. The full name could be:
       [[[qualifier].owner].basename]

       foo.bix.address
       p1 = foo,  p2 = bix, p3 = address

       bix.address
       p1 = bix, p2 = address, p3 = NULL;

       address
       p1 = address, p2, p3 = nULL; */

    p1 = strdup (tablename);
    p2 = gettoken (p1, '.');
    p3 = gettoken (p2, '.');

    if ((p2 == (char *)NULL) && (p3 == (char *)NULL))
      {
	/* basename only */
	qualifier = NULL;  owner = NULL;  basenam = p1; 
      }
    else if (p3 == (char *)NULL)
      {
	qualifier = NULL; owner = p1; basenam = p2; 
      } 
    else
      {
	qualifier = p1; owner = p2; basenam = p3;     
      }
  
    gr = make_gsql_result ();

    if (SQLAllocStmt (db->myodbc->hdbc, &hstmt) != SQL_SUCCESS)
      {
	free (p1);
	return ((gsql_result *)NULL);
      }

    retcode = SQLColumns
      (hstmt,
       (unsigned char *)qualifier,
       (qualifier != NULL) ? SQL_NTS : 0, /* All qualifiers */
       (unsigned char *)owner, (owner != NULL) ? SQL_NTS : 0, /* All owners */
       (unsigned char *)basenam, SQL_NTS, /* Table name */
       (unsigned char *)NULL, 0); /* All columns */

    if (retcode == SQL_SUCCESS)
      {
	int arraysize = 10;
	/* We get back one descriptive row for each column in the table. */
	gr->fields = (gsql_field **) xmalloc
	  ((arraysize + 1) * sizeof (gsql_field *));

	SQLBindCol (hstmt,  1, SQL_C_CHAR, szQualifier, STR_LEN, &cbQualifier);
	SQLBindCol (hstmt,  2, SQL_C_CHAR, szOwner, STR_LEN, &cbOwner);
	SQLBindCol (hstmt,  3, SQL_C_CHAR, szTableName, STR_LEN, &cbTableName);
	SQLBindCol (hstmt,  4, SQL_C_CHAR, szColName, STR_LEN, &cbColName);
	SQLBindCol (hstmt,  5, SQL_C_SHORT, &DataType, 0, &cbDataType);
	SQLBindCol (hstmt,  6, SQL_C_CHAR, szTypeName, STR_LEN, &cbTypeName);
	SQLBindCol (hstmt,  7, SQL_C_LONG, &Precision, 0, &cbPrecision);
	SQLBindCol (hstmt,  8, SQL_C_LONG, &Length, 0, &cbLength);
	SQLBindCol (hstmt,  9, SQL_C_SHORT,&Scale, 0, &cbScale);
	SQLBindCol (hstmt, 10, SQL_C_SHORT,&Radix, 0, &cbRadix);
	SQLBindCol (hstmt, 11, SQL_C_SHORT,&Nullable, 0, &cbNullable);
	SQLBindCol (hstmt, 12, SQL_C_CHAR, szRemarks, REM_LEN, &cbRemarks);

	for (i = 0; ; i++)
	  {
	    retcode = SQLFetch (hstmt);
	    if (retcode == SQL_SUCCESS)
	      {
		/* create fields... */

		gsql_field *gfield = (gsql_field *)
		  xmalloc (sizeof (gsql_field));

		gfield->name = strdup ((char *)szColName);
		gfield->type = DataType;
		gfield->length = Length;
		gfield->nullable = Nullable;
		gfield->table = (char *) NULL;

		gfield->qualifier = strdup ((char *)szQualifier);
		gfield->owner     = strdup ((char *)szOwner);
		gfield->type_name  = strdup ((char *)szTypeName);
		gfield->precision = Precision;
		gfield->scale     = Scale;
		gfield->radix     = Radix;

		if (i + 2 > arraysize)
		  gr->fields = (gsql_field  **)xrealloc
		    (gr->fields, ((arraysize += 5) * sizeof (gsql_field *)));

		gr->fields[i] = gfield;
	      }
	    else
	      break;
	  }

      gr->fields[i] = (gsql_field *) NULL;
      gr->numfields = i;
      gr->tablename = strdup (tablename);
    }

    if (hstmt)
      SQLFreeStmt (hstmt, SQL_DROP);

#if defined (CACHED_GSQL_LIST_FIELDS)
    database_add_cached_table_info (db, gr);
#endif

    free (p1);
  }

  return (gr);
}

#if defined (MACRO_REFERENCE_DISALLOWED)
/* The number of fields in a result row. */
static int gsql_num_fields (gsql_result *r) { return (r->numfields); }
#else
#  define gsql_num_fields(r) (r->numfields)
#endif

/* Fetch data from RESULT at column COL.  The result string is limited
   to 64k.  The return code SQL_SUCCESS_WITH_INFO is supposed be used
   if needed to get sequential chunks of data which are longer than
   the fetchBuffer can hold. A realloc loop should be implemented as a
   future improvement to get arbitrary length data, but be sure to
   establish that the ODBC drivers you are using actually support for
   this functionality.

   Always conses a new string or NULL. */
static char *
gsql_get_column (gsql_result *result, int col)
{
  SDWORD colIndicator;
  char fetchBuffer[65536];
  RETCODE retcode;

  /*  Fetch this column as string data. */
  retcode = SQLGetData (result->hstmt, col + 1, SQL_CHAR, fetchBuffer,
			sizeof (fetchBuffer), &colIndicator);

  if ((retcode == SQL_SUCCESS) || (retcode == SQL_SUCCESS_WITH_INFO))
    return (strdup (fetchBuffer));
  else
    return ((char *)NULL);
}

/* From the result set of a msqlListTables command,
   return the column which has the table name.  */
static char *
gsql_get_column_table_name (gsql_result *gr)
{
  /* The columns in a result set returned by SQLTables() are
       TABLE_QUALIFER
       TABLE_OWNER
       TABLE_NAME
       TABLE_TYPE

   So, we want to return column 2 because we number columns starting
   at 0, not 1, and gsql_get_column () takes care of fixing the index. */
  return (gsql_get_column (gr, 2));
}

static void
gsql_connect (char *dsn, Database *db)
{
  db->connected = 0;

  if (dsn != (char *) NULL)
    {
      char *mydsn = dsn;
      MYODBC *myodbc;

      if (strchr (dsn, '=') == (char *)NULL)
	{
	  mydsn = (char *)xmalloc (5 + strlen (dsn));
	  sprintf (mydsn, "DSN=%s", dsn);
	}

      myodbc = odbc_connect (mydsn);

      if (myodbc != (MYODBC *)NULL)
	{
	  db->myodbc = myodbc;
	  db->connected = 1;
	}

      if (mydsn != dsn)
	free (mydsn);
    }
}

static void
gsql_close (Database *db)
{
  if ((db->connected == 1) && (db->myodbc != (MYODBC *)NULL))
    {
      odbc_disconnect (db->myodbc);
    }
}

/* <odbcper::host-databases [hostname] [result=varname]>
   Returns an array of the sources available from the SQL driver.

   The HOSTNAME is for compatibility with msql functions and has no
   effect. All information is retrieved from the ODBC client driver
   on the local system.

   If VARNAME is supplied, the array is placed into that variable instead. */
static void
pf_host_databases (PFunArgs)
{
#if defined (USE_OPENLINK_SQLDATASOURCES_BROKEN_DRIVERS)
  char *resultvar = mhtml_evaluate_string (get_value (vars, "result"));
  RETCODE status;
  HENV    henv;
  UCHAR source_name[1024];
  UCHAR source_desc[1024];
  SWORD size1, size2;

  /* No errors yet! */
  gsql_clear_error_message ();

  /* SQLDataSources doesn't seem to work in OpenLink's drivers.  -- hqm */

  if (SQLAllocEnv (&henv) == SQL_SUCCESS)
    {

      status = SQLDataSources (henv, SQL_FETCH_FIRST,
			       source_name, sizeof (source_name), &size1, 
			       source_desc, sizeof (source_desc), &size2);

      if (status == SQL_SUCCESS)
	{
	  int count = 0;
	  int namesize = 10;
	  char result[1024];
	  char **names = (char **) xmalloc ((namesize + 1) * sizeof (char *));
	  
	  while (1) 
	    {
	      if (count + 2 > namesize)
		    names = (char  **)xrealloc
		      (names, ((namesize += 5) * sizeof (char *)));

	      result[0] = 0;
	      strcat (result, source_name);
	      strcat (result, ":");
	      strcat (result, source_desc);
	      
	      names[count] = strdup (result);
	      count++;

	      status = 
		SQLDataSources (henv, SQL_FETCH_NEXT,
				source_name, sizeof (source_name), &size1, 
				source_desc, sizeof (source_desc), &size2);

	      if (status != SQL_SUCCESS)
		break;
	    }

	  if (!empty_string_p (resultvar))
	    {
	      symbol_store_array (resultvar, names);
	    }
	  else
	    {
	      register int i;

	      for (i = 0; i < count; i++)
		{
		  bprintf_insert (page, start, "%s\n", names[i]);
		  start += 1 + strlen (names[i]);
		  free (names[i]);
		}
	      free (names);
	      *newstart = start;
	    }
	}

	SQLFreeEnv (henv);
    }

  xfree (resultvar);
#endif /* USE_OPENLINK_SQLDATASOURCES_BROKEN_DRIVERS */
}

static int
gsql_transact_internal (Database *db, char *action_arg)
{
  int action;
  RETCODE status;

  if (!empty_string_p (action_arg) &&
      (strcasecmp (action_arg, "ROLLBACK") == 0))
    action = SQL_ROLLBACK;
  else
    action = SQL_COMMIT;

  status = SQLTransact (db->myodbc->henv, db->myodbc->hdbc, action);
  return (odbc_to_gsql_status (status));
}



#define ODBC_OPT_STRING 1
#define ODBC_OPT_INT32  2
#define ODBC_OPT_INT16  3

typedef struct
{
  char *name;
  int code;
  int type;
} ODBC_option;

static ODBC_option odbc_options[] =
{
/* options for SQLSetConnectOption/SQLGetConnectOption */
  { "SQL_ACCESS_MODE",		SQL_ACCESS_MODE,	ODBC_OPT_INT32 },
  { "SQL_AUTOCOMMIT",		SQL_AUTOCOMMIT,		ODBC_OPT_INT32 },
  { "SQL_LOGIN_TIMEOUT",        SQL_LOGIN_TIMEOUT,	ODBC_OPT_INT32 },
  { "SQL_OPT_TRACE",            SQL_OPT_TRACE,		ODBC_OPT_INT32 },
  { "SQL_OPT_TRACEFILE",        SQL_OPT_TRACEFILE,	ODBC_OPT_STRING },
  { "SQL_TRANSLATE_DLL",        SQL_TRANSLATE_DLL,	ODBC_OPT_STRING },
  { "SQL_TRANSLATE_OPTION",     SQL_TRANSLATE_OPTION,	ODBC_OPT_INT32 },
  { "SQL_TXN_ISOLATION",        SQL_TXN_ISOLATION,	ODBC_OPT_INT32 },
  { "SQL_CURRENT_QUALIFIER",	SQL_CURRENT_QUALIFIER,	ODBC_OPT_STRING },
  { "SQL_ODBC_CURSORS",		SQL_ODBC_CURSORS,	ODBC_OPT_INT32 },
  { "SQL_QUIET_MODE",		SQL_QUIET_MODE,		ODBC_OPT_INT32 },
  { "SQL_PACKET_SIZE",		SQL_PACKET_SIZE,	ODBC_OPT_INT32 },

  /* SQLGetInfo options */
  { "SQL_DRIVER_NAME",		SQL_DRIVER_NAME,	ODBC_OPT_STRING },
  { "SQL_ODBC_VER",		SQL_ODBC_VER,		ODBC_OPT_STRING },
  { "SQL_CURSOR_COMMIT_BEHAVIOR",  SQL_CURSOR_COMMIT_BEHAVIOR,	ODBC_OPT_INT16 },
  { "SQL_CURSOR_ROLLBACK_BEHAVIOR",SQL_CURSOR_ROLLBACK_BEHAVIOR,ODBC_OPT_INT16 },
  { "SQL_DEFAULT_TXN_ISOLATION",   SQL_DEFAULT_TXN_ISOLATION,	ODBC_OPT_INT16 },

  { "SQL_TXN_ISOLATION_OPTION", SQL_TXN_ISOLATION_OPTION, ODBC_OPT_INT16 },
  { "SQL_NON_NULLABLE_COLUMNS", SQL_NON_NULLABLE_COLUMNS, ODBC_OPT_INT16 },

  { "SQL_DRIVER_HLIB",		SQL_DRIVER_HLIB,	ODBC_OPT_INT16 },
  { "SQL_DRIVER_ODBC_VER",	SQL_DRIVER_ODBC_VER,	ODBC_OPT_STRING },

  { "SQL_QUALIFIER_LOCATION",	SQL_QUALIFIER_LOCATION, ODBC_OPT_INT32 },

  { (char *)NULL, 0, 0 }
};

/* Returns 0 if no match found. */
static int
find_odbc_option (char *name, ODBC_option *opt)
{
  int i;
  for (i = 0; odbc_options[i].type != 0; i++)
    {
      if (!strcasecmp (name, odbc_options[i].name))
	{
	  *opt = odbc_options[i];
	  return 1;
	}
    }
  return 0;
}

/* <odbcper::get-info db fInfoType>
   
   Performs ODBC GetInfo call.  Returns a string. */

static void
pf_odbc_get_info (PFunArgs)
{
  char *finfo = mhtml_evaluate_string (get_positional_arg (vars, 1));

  /* No errors yet! */
  gsql_clear_error_message ();

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  RETCODE status = SQL_ERROR;
	  SWORD nbytes;
	  char buf[1024];
	  int  numval;
	  ODBC_option opt;
	  MYODBC *myodbc = db->myodbc;

	  strcpy (buf, "");

	  if (find_odbc_option (finfo, &opt) != 0)
	    {
	      switch (opt.type)
		{
		case ODBC_OPT_STRING:
		  status = SQLGetInfo
		    (myodbc->hdbc, opt.code, buf, (SWORD) sizeof (buf),
		     &nbytes);
		  break;

		case ODBC_OPT_INT32:
		  status = SQLGetInfo
		    (myodbc->hdbc, opt.code, &numval, sizeof (numval),
		     &nbytes); 
		  sprintf (buf, "%d", numval);
		  break;

		case ODBC_OPT_INT16:
		  status = SQLGetInfo
		    (myodbc->hdbc, opt.code, &numval, sizeof (numval),
		     &nbytes); 
		  sprintf (buf, "%d", numval & 0xffff);
		  break;
		}
	    }

	  if (status == SQL_SUCCESS)
	    {
	      bprintf_insert (page, start, "%s", buf);
	      *newstart += strlen (buf);
	    }
	  else
	    gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	}
    }
  xfree (finfo);
}

/* <odbcper::get-connect-option db fInfoType>
   
   Performs ODBC SQLGetConnectOption call.  Returns a string. */
static void
pf_odbc_get_connect_option (PFunArgs)
{
  char *finfo = mhtml_evaluate_string (get_positional_arg (vars, 1));

  /* No errors yet! */
  gsql_clear_error_message ();

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  RETCODE status = SQL_ERROR;
	  char buf[1024];
	  int  numval;
	  ODBC_option opt;
	  MYODBC *myodbc = (MYODBC *)NULL;

	  strcpy (buf, "");

	  if (find_odbc_option (finfo, &opt) != 0)
	    {
	      switch (opt.type)
		{
		case ODBC_OPT_STRING:
		  status = SQLGetConnectOption (myodbc->hdbc, opt.code, buf);
		  break;

		case ODBC_OPT_INT32:
		  status = SQLGetConnectOption(myodbc->hdbc,opt.code,&numval); 
		  sprintf (buf, "%d", numval);
		  break;

		case ODBC_OPT_INT16:
		  status = SQLGetConnectOption(myodbc->hdbc,opt.code,&numval); 
		  sprintf (buf, "%d", numval & 0xffff);
		  break;
		}
	    }

	  if (status == SQL_SUCCESS)
	    {
	      bprintf_insert (page, start, "%s", buf);
	      *newstart += strlen (buf);
	    }
	  else
	    gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	}
    }
  xfree (finfo);
}

/* <odbcper::set-connect-option db fInfoType value>
   
   Performs ODBC SetInfo call. Returns true if successful. */
static void
pf_odbc_set_connect_option (PFunArgs)
{
  char *finfo = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *val = mhtml_evaluate_string (get_positional_arg (vars, 2));

  /* No errors yet! */
  gsql_clear_error_message ();

  if (database_environment_level != 0)
    {
      Database *db = get_dbref (vars);

      if ((db != (Database *)NULL) && gsql_database_connected (db))
	{
	  RETCODE status = SQL_ERROR;
	  int  numval;
	  ODBC_option opt;

	  if (find_odbc_option (finfo, &opt) != 0)
	    {
	      switch (opt.type)
		{
		case ODBC_OPT_STRING:
		  status = SQLSetConnectOption
		    (db->myodbc->hdbc, opt.code, (UDWORD) val);
		  break;

		case ODBC_OPT_INT32:
		  numval = atoi (val);
		  status = SQLSetConnectOption
		    (db->myodbc->hdbc, opt.code, (UDWORD) numval); 
		  break;

		case ODBC_OPT_INT16:
		  numval = 0xffff & atoi (val);
		  status = SQLSetConnectOption
		    (db->myodbc->hdbc, opt.code, (UDWORD) numval); 
		  break;
		}
	    }

	  if (status == SQL_SUCCESS)
	    {
	      bprintf_insert (page, start, "true");
	      *newstart += strlen ("true");
	    }
	  else
	    gsql_save_error_message (db, GSQL_DEFAULT_ERRMSG);
	}
    }
  else
    page_debug ("<odbcper::set-connect-option ...>: Not in open-database!");

  xfree (val);
  xfree (finfo);
}
