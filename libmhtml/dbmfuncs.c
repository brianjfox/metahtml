/* dbmfuncs.c: -*- C -*-  Raw read/write GDBM/DBM primitives. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Thu Nov  7 14:51:00 1996.  */

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
#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_with_open_database (PFunArgs);
static void pf_get_var (PFunArgs);
static void pf_set_var (PFunArgs);
static void pf_unset_var (PFunArgs);
static void pf_first_key (PFunArgs);
static void pf_next_key (PFunArgs);

#if defined (NOT_THIS_RELEASE)
static void pf_load_package (PFunArgs);
static void pf_save_package (PFunArgs);
#endif

static PFunDesc func_table[] =
{
  { "DBM::WITH-OPEN-DATABASE",	1, 0, pf_with_open_database },
  { "DBM::GET-VAR",		0, 0, pf_get_var },
  { "DBM::SET-VAR",		0, 0, pf_set_var },
  { "DBM::UNSET-VAR",		0, 0, pf_unset_var },
  { "DBM::FIRST-KEY"	,	0, 0, pf_first_key },
  { "DBM::NEXT-KEY",		0, 0, pf_next_key },
#if defined (NOT_THIS_RELEASE)
  { "DBM::LOAD-PACKAGE",	0, 0, pf_load_package },
  { "DBM::SAVE-PACKAGE",	0, 0, pf_save_package },
#endif
  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_dbm_functions)
DEFINE_SECTION (DIRECT-DBM-ACCESS, database; perl, 
"There are times when it is desirable to directly access DBM, NDBM, or\n\
GDBM databases, for example, when reading or writing values to a\n\
database created with a different tool, such as Perl or TCL.\n\
\n\
<Meta-HTML> provides low-level access primitives for such databases in\n\
addition to a higher-level, more generally useful abstraction\n\
(which is detailed in <secref DATABASE-COMMANDS>).", "")

/************************************************************/
/*							    */
/*		Database Manipulation Functions		    */
/*							    */
/************************************************************/

static int database_environment_level = 0;

static int
db_mode (char *modename)
{
  int mode = DB_READER;

  if (modename != (char *)NULL)
    {
      if ((strcasecmp (modename, "writer") == 0) ||
	  (strcasecmp (modename, "write") == 0))
	mode = DB_WRITER;
      else if (strcasecmp (modename, "write-create") == 0)
	mode = DB_WRCREAT;
    }

  return (mode);
}

static void
lock_database (char *dbname, int open_mode, int *lock, DBFILE *db)
{
  char *lockname = db_lockname (dbname);
  int fd;

  *lock = -1;
  *db = (DBFILE) 0;

  fd = os_open (lockname, O_CREAT | O_WRONLY | O_APPEND, 0666);

  if ((fd < 0) || (LOCKFILE (fd) == -1))
    {
      page_syserr ("%s: %s", dbname, (char *)strerror (errno));

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
      if ((*db = database_open (dbname, open_mode)) == (DBFILE)0)
	{
	  page_syserr ("%s: %s", dbname, database_strerror ());
	  /* unlink (lockname); */
	  UNLOCKFILE (fd);
	  close (fd);
	}
      else
	*lock = fd;
    }
}

static void
unlock_database (char *dbname, int *lock, DBFILE *db)
{
  if (*db != (DBFILE)0)
    database_close (*db);

  if (*lock > -1)
    {
      char *lockname = db_lockname (dbname);
      unlink (lockname);
      UNLOCKFILE (*lock);
      close (*lock);
    }
}

/* <with-open-database variable dbname mode=[writer/reader/write-create]>
   [code using the open database]
   </with-open-database>
   Opens the database specified by DBNAME, and stores a referent
   to it in VARIABLE.  The database is opened in the mode specified
   by MODE.  If MODE is not specified, the database is opened read-only.
   If the operation fails, the value of VARIABLE is the empty string.*/
static void
pf_with_open_database (PFunArgs)
{
  char *varname;
  char *dbname;
  int mode;
  int jump_again = 0;

  varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  dbname = mhtml_canonicalize_file_name_argument(get_positional_arg (vars, 1));

  if ((!empty_string_p (varname)) && (!empty_string_p (dbname)))
    {
      char *modename = mhtml_evaluate_string (get_value (vars, "MODE"));
      DBFILE db;
      int lock;

      mode = db_mode (modename);
      if (modename) free (modename);
  
      lock_database (dbname, mode, &lock, &db);

      if (db != (DBFILE *)NULL)
	{
	  char dbvalue[40];
	  PAGE *body_code = page_copy_page (body);

	  sprintf (dbvalue, "%0lX", (unsigned long)db);
	  pagefunc_set_variable (varname, dbvalue);

	  {
	    PageEnv *page_environ;

	    page_environ = pagefunc_save_environment ();
	    database_environment_level++;

	    if ((jump_again = setjmp (page_jmp_buffer)) == 0)
	      page_process_page_internal (body_code);

	    database_environment_level--;
	    pagefunc_restore_environment (page_environ);
	  }

	  if (body_code != (PAGE *)NULL)
	    {
	      if (!jump_again && (body_code->buffer != (char *)NULL))
		{
		  bprintf_insert (page, start, "%s", body_code->buffer);
		  *newstart = start + (body_code->bindex);
		}

	      page_free_page (body_code);
	    }
	}

      unlock_database (dbname, &lock, &db);
    }

  if (dbname) free (dbname);
  if (varname) free (varname);
  if (jump_again) longjmp (page_jmp_buffer, 1);
}

/* For the database functions which take a DB and a KEY as args, this
   processes the local varlist and returns these values. */
static void
dbkey_function_args (Package *vars, DBFILE *db, DBOBJ **key, int key_required)
{
  char *dbref;
  char *dbkey;

  *db = (DBFILE)0;
  *key = (DBOBJ *)NULL;

  if (database_environment_level == 0)
    return;

  dbref  = mhtml_evaluate_string (get_positional_arg (vars, 0));
  dbkey  = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((dbref != (char *)NULL) &&
      (!key_required || (dbkey != (char *)NULL)))
    {
      char *rep = pagefunc_get_variable (dbref);

      if (rep != (char *)NULL)
	{
	  long dbval = strtol (rep, (char **)NULL, 16);

	  *db = (DBFILE)dbval;
	}

      if (*db != (DBFILE)0)
	{
	  if (dbkey)
	    {
	      *key = (DBOBJ *)xmalloc (sizeof (DBOBJ));
	      (*key)->data = (unsigned char *)dbkey;
	      (*key)->length = (size_t) 1 + strlen (dbkey);
	    }
	}
    }

  if (dbref) free (dbref);
  if (dbkey && !key) free (dbkey);
}

static void
dbobj_free (DBOBJ *obj)
{
  if (obj != (DBOBJ *)NULL)
    {
      if (obj->data) free (obj->data);
      free (obj);
    }
}

/* <dbm::get-var db key> */
static void
pf_get_var (PFunArgs)
{
  DBFILE db;
  DBOBJ *key;

  dbkey_function_args (vars, &db, &key, 1);

  if (db && key)
    {
      DBOBJ *value = database_fetch (db, key);

      if ((value != (DBOBJ *)NULL) && (value->length > 0 ))
	{
	  if (value->length < 255)
	    {
	      char value_buffer[256];
	      memmove (value_buffer, value->data, value->length);
	      value_buffer[value->length] = '\0';
	      bprintf_insert (page, start, "%s", value_buffer);
	    }
	  else
	    {
	      char *temp = (char *)xmalloc (1 + value->length);
	      memmove (temp, value->data, value->length);
	      temp[value->length] = '\0';
	      bprintf_insert (page, start, "%s", temp);
	      free (temp);
	    }

	  start += value->length;
	  *newstart += value->length;
	}

      dbobj_free (value);
    }

  dbobj_free (key);
}

/* <dbm::set-var db key=value ...> */
static void
pf_set_var (PFunArgs)
{
  DBFILE db;
  DBOBJ *key;

  dbkey_function_args (vars, &db, &key, 0);

  if (db)
    {
      char **names = get_vars_names (vars);
      char **vals = get_vars_vals (vars);

      dbobj_free (key);

      if ((names != (char **)NULL) && (names[0] != (char *)NULL))
	{
	  register int i;
	  char *sym_name;

	  for (i = 1; (sym_name = names[i]) != (char *)NULL; i++)
	    {
	      char *name = sym_name;
	      char *value = vals[i];
	      int free_value = 0;

	      name = mhtml_evaluate_string (sym_name);

	      if (value == (char *)NULL)
		{
		  value = "";
		  if (debug_level)
		    page_debug ("<set-var %s ...> missing `='", sym_name);
		}
	      else
		{
		  value = mhtml_evaluate_string (value);
		  if (value) free_value++;
		}

	      if (name)
		{
		  DBOBJ *content = database_setkey (value);
		  key = database_setkey (name);

		  database_store (db, key, content);

		  /* database_setkey () doesn't cons the string value.
		     Don't free it, just the surrounding structure. */
		  free (key);
		  free (content);
		}

	      if (free_value) free (value);
	      if (name != sym_name) free (name);
	    }
	}
    }
}

/* <dbm::unset-var dbvar key> */
static void
pf_unset_var (PFunArgs)
{
  DBFILE db;
  DBOBJ *key;

  dbkey_function_args (vars, &db, &key, 1);

  if (db && key)
    database_delete (db, key);

  if (key)
    {
      if (key->data) free (key->data);
      free (key);
    }
}

/* <dbm::first-key dbvar> */
static void
pf_first_key (PFunArgs)
{
  DBFILE db = (DBFILE)0;
  DBOBJ *key;

  dbkey_function_args (vars, &db, &key, 0);

  if (db != (DBFILE)0)
    {
      key = database_firstkey (db);

      if (key != (DBOBJ *)NULL)
	{
	  bprintf_insert (page, start, "%s", key->data);
	  *newstart = start + (key->length - 1);

	  free (key->data);
	  free (key);
	}
    }
}

/* <dbm::next-key dbvar key> */
static void
pf_next_key (PFunArgs)
{
  DBFILE db;
  DBOBJ *inkey, *outkey = (DBOBJ *)NULL;

  dbkey_function_args (vars, &db, &inkey, 1);

  if (db && inkey)
    outkey = database_nextkey (db, inkey);

  if (outkey)
    {
      bprintf_insert (page, start, "%s", outkey->data);
      *newstart = start + (outkey->length - 1);
      free (outkey->data);
      free (outkey);
    }

  if (inkey)
    {
      free (inkey->data);
      free (inkey);
    }
}

#if defined (__cplusplus)
}
#endif
