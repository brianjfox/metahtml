/* dbfuncs.c: -*- C -*-  Functions for manipulating databases. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jan 31 20:50:36 1996.  */

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
static void pf_database_load_record (PFunArgs);
static void pf_database_delete_record (PFunArgs);
static void pf_database_save_record (PFunArgs);
static void pf_database_save_package (PFunArgs);
static void pf_database_first_key (PFunArgs);
static void pf_database_next_key (PFunArgs);
static void pf_database_unique_key (PFunArgs);
static void pf_database_query (PFunArgs);

static PFunDesc func_table[] =
{
  { "WITH-OPEN-DATABASE",	1, 0, pf_with_open_database },
  { "DATABASE-LOAD-RECORD",	0, 0, pf_database_load_record },
  { "DATABASE-DELETE-RECORD",	0, 0, pf_database_delete_record },
  { "DATABASE-SAVE-RECORD",	0, 0, pf_database_save_record },
  { "DATABASE-SAVE-PACKAGE",	0, 0, pf_database_save_package },
  { "DATABASE-FIRST-KEY",	0, 0, pf_database_first_key },
  { "DATABASE-NEXT-KEY",	0, 0, pf_database_next_key },
  { "DATABASE-UNIQUE-KEY",	0, 0, pf_database_unique_key },
  { "DATABASE-QUERY",		0, 0, pf_database_query },
  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_db_functions)
DEFINE_SECTION (DATABASE-COMMANDS, database; long-term storage, 
"<meta-html> contains commands for performing operations on a
database. A database created by <meta-html> contains <i>records</i>
stored by <i>key</i>, where each record consists of a set of
<i>name/value</i> pairs.

There is a single command for specifying which database you will be
operating on: <funref database-commands with-open-database>.  All of
the remaining database commands only have an effect when executed
within the scope of this function.

<funref \"database commands\" database-query> is the command used to
perform queries on a database, or to simply select a range of records
to operate on.

Functions which load, store, or delete a record as a single atomic
operation, return the string <code>\"true\"</code> when they succeed,
and store an error message in <funref Language-Operators
SYSTEM-ERROR-OUTPUT> when they do not.", "")

static long
random_number (void)
{
  register int i;
  long value = 0;		/* Shut UP, Gcc! */
  static int times_called = 0;
  static unsigned int seed;
#if defined (HAVE_SETSTATE)
  static char new_state[256];
  char *old_state;
#endif
  
  if (!times_called)
    seed = (unsigned int)time ((time_t *)0);

  times_called++;

#if defined (HAVE_SETSTATE)
  old_state = (char *) initstate (seed, new_state, sizeof (new_state));
  srandom (seed);

  for (i = 0; i < times_called; i++)
#endif
    value = random ();

#if defined (HAVE_SETSTATE)
  setstate (old_state);
#endif

  return (value);
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
	  if (database_had_error_p ())
	    page_syserr ("%s: %s", dbname, database_strerror ());
	  unlink (lockname);
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

DEFMACRO (pf_with_open_database, dbvar dbname &key mode=open-mode,
"Create an environment in which other database commands can be given.
First, the database referenced by <var dbname> is locked and opened in
the mode specified by <var open-mode>, and the resultant database
handle is bound to the variable named by <var dbvar>.  Then, the <var
body> code is executed.  Finally, the database is closed, and further
references to <var dbvar> are meaningless.

Please note that the file name specified by <var dbname> should be a
full pathname; it is not relative to Web space in any way.

<var open-mode> should evaluate to one of the following:

<ul>
  <li> <var reader><br>
The caller wishes only to have read access to the specified database.

<li> <var writer><br>
The caller wishes to have both read and write access to the specified
database.

<li> <var write-create><br>
The caller wishes both read and write access to the specified
database.  If the database does not exist, it is created.
</ul>")
{
  char *varname;
  char *dbname;
  int mode;
  int jump_again = 0;

  varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  dbname = mhtml_canonicalize_file_name_argument 
    (get_positional_arg (vars, 1));

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

  xfree (dbname);
  xfree (varname);
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

  xfree (dbref);
  if (dbkey && !key) free (dbkey);
}

/* <database-load-record DB KEY [PREFIX=string]>
   Set page variables perhaps prefixed with PREFIX from the record
   referenced by KEY in DB.  DB, KEY and PREFIX are evaluated.
   For DB, the value of the resultant string is looked up as a
   variable name in the current page environment.  It must refer to a
   database variable assigned with <with-open-database>.
   The record is loaded, and page variables are assigned according to
   the contents of that record.  The page variable names are the same
   as those used in database-save-record, perhaps additionally
   prefixed with the string PREFIX.  If the key couldn't be found, the
   empty string is returned, otherwise, the string "true" is returned.

   This function can only be run from within <with-open-database ...>. */
static char *varname = (char *)NULL;
static int varname_size = 0;

DEFUN (pf_database_load_record, dbvar key &key package=package-name,
"Load the variables from the record specified by <var key> in the
database referenced by <var dbvar>.  If <var package=package-name> is
given, the record variables are stored into the specified package,
instead of the current package.

Upon success, this function returns the string <code>\"true\"</code>.

<example>
<with-open-database db /tmp/file.db mode=read>
  <set-var loaded? = <database-load-record db <get-var name> package=foo>>
</with-open-database>

<when <get-var loaded?>>
  The record was loaded successfully, and the
  value of NAME is <get-var foo::name>.
</when>
</example>")
{
  char *prefix;
  int prefix_len;
  char *result = (char *)NULL;
  DBFILE db;
  DBOBJ *key, *content = (DBOBJ *)NULL;

  dbkey_function_args (vars, &db, &key, 1);

  if ((db == (DBFILE)0) || (key == (DBOBJ *)NULL))
    return;

  prefix = mhtml_evaluate_string
    (get_one_of (vars, "PACKAGE", "PREFIX", (char *)NULL));
  prefix_len = prefix ? strlen (prefix) : 0;

  content = database_fetch (db, key);

  /* Unpack the record if there is one. */
  if ((content != (DBOBJ *)NULL) && (content->data != (unsigned char *)NULL))
    {
      WispObject *list = wisp_from_string ((char *)content->data);

      while (list != NIL)
	{
	  WispObject *pair;
	  char *name, *value;

	  pair = CAR (list);
	  list = CDR (list);

	  name = STRING_VALUE (CAR (pair));

	  if (STRING_P (CDR (pair)))
	    {
	      value = STRING_VALUE (CDR (pair));

	      if ((name != (char *)NULL) && (name[0] != '\0'))
		{
		  if (prefix_len)
		    {
		      int name_len = strlen (name);
		  
		      while ((prefix_len + name_len + 10) > varname_size)
			varname = (char *)xrealloc
			(varname, (varname_size += 100));

		      sprintf (varname, "%s::%s", prefix, name);

		      pagefunc_set_variable (varname, value);
		    }
		  else
		    pagefunc_set_variable (name, value);
		}
	    }
	  else
	    {
	      register int which = 0;

	      if ((name != (char *)NULL) && (name[0] != '\0'))
		{
		  int name_len = strlen (name);

		  while (CONS_P (CDR (pair)))
		    {
		      pair = CDR (pair);
		      value = STRING_VALUE (CAR (pair));

		      while ((prefix_len + name_len + 10) > varname_size)
			varname = (char *)xrealloc
			(varname, (varname_size += 100));

		      if (prefix_len)
			sprintf (varname, "%s::%s[%d]", prefix, name, which);
		      else
			sprintf (varname, "%s[%d]", name, which);

		      pagefunc_set_variable (varname, value);
		      which++;
		    }
		}
	    }
	}

      gc_wisp_objects ();
      result = "true";
    }

  dbobj_free (content);
  dbobj_free (key);

  if (prefix) free (prefix);

  if (result != (char *)NULL)
    bprintf_insert (page, start, "%s", result);
}

DEFUN (pf_database_delete_record, dbvar key,
"Remove the record associated with <var key> from the database
referenced by <var dbvar>.  This functions returns the string
<code>true</code> upon success.

<example>
<with-open-database db /tmp/file.db mode=write>
  <set-var deleted? = <database-delete-record db <get-var key>>>
</with-open-database>
</example>")
{
  DBFILE db;
  DBOBJ *key;
  char *result = (char *)NULL;

  dbkey_function_args (vars, &db, &key, 1);

  if (db && key)
    {
      database_delete (db, key);

      if (database_had_error_p ())
	page_syserr ("<database-delete-record %s>: %s",
		     mhtml_funargs (vars), database_strerror ());
      else
	result = "true";
    }

  dbobj_free (key);

  if (result != (char *)NULL)
    bprintf_insert (page, start, "%s", result);
}

DEFUN (pf_database_save_record, dbvar key &optional var... &key
       package=package-name,
"Save the variables <var var1...varn> in the database referenced by
<var dbvar> in the record specified by the key <var key>.  If <var
package-name> is supplied, then the names of the variables written to
the database are written as if they belonged to that package.

Upon success, this function returns the string <code>true</code>.

Also see <funref database-commands database-save-package>, and
<funref database-commands database-load-record>.

<example>
<with-open-database db /tmp/file.db mode=write-create>
  <set-var saved? =
     <database-save-record db <get-var name> name age size>>
</with-open-database>

<when <get-var saved?>>
  The record was stored successfully.
</when>
</example>")
{
  DBFILE db;
  DBOBJ *key;
  char *result = (char *)NULL;

  dbkey_function_args (vars, &db, &key, 1);

  if (db && key)
    {
      int position = 2;
      char *prefix = mhtml_evaluate_string
	(get_one_of (vars, "PACKAGE", "PREFIX", (char *)NULL));
      int prefix_len = prefix ? strlen (prefix) : 0;
      char *name;
      BPRINTF_BUFFER *data = bprintf_create_buffer ();

      bprintf (data, "(");

      while ((name = mhtml_evaluate_string
	      (get_positional_arg (vars, position))) != (char *)NULL)
	{
	  char *value = pagefunc_get_variable (name);

	  position++;
	  if (prefix_len)
	    {
	      int name_len = strlen (name);

	      while ((name_len + prefix_len + 4) > varname_size)
		varname = (char *)xrealloc (varname, varname_size += 100);

	      sprintf (varname, "%s::%s", prefix, name);
	      bprintf (data, "(%s . ", wisp_readable (varname));
	    }
	  else
	    bprintf (data, "(%s . ", wisp_readable (name));

	  bprintf (data, "%s)", value ? wisp_readable (value) : "\"\"");
	  free (name);
	}

      bprintf (data, ")");

      {
	DBOBJ content;

	content.data = (unsigned char *)data->buffer;
	content.length = (size_t)data->bindex;

	database_store (db, key, &content);
	if (database_had_error_p ())
	  page_syserr ("%s", database_strerror ());
      }
      bprintf_free_buffer (data);

      result = "true";
    }

  dbobj_free (key);

  if (result)
    bprintf_insert (page, start, "%s", result);
}

DEFUN (pf_database_save_package, dbvar key package &key strip=true,
"Save the variables from the package <var package> in the database
referenced by <var dbvar> in the record specified by <var key>.

If <var strip=true> is supplied, then the names of the variables
written to the database have their package prefix removed as they are
written.

Upon success, this function returns the string <code>true</code>.

<example>
<set-var mykey=\"The Key\" foo::x=x foo::array[0]=val0 foo::array[1]=val1>
<with-open-database db /tmp/file.db mode=write-create>
  <database-save-package db <get-var mykey> foo strip=true>
</with-open-database>
</example>

produces
<example>
true
</example>")
{
  DBFILE db;
  DBOBJ *key;
  char *result = (char *)NULL;

  dbkey_function_args (vars, &db, &key, 1);

  if (db && key)
    {
      int strip = !empty_string_p (get_value (vars, "strip"));
      char *package_arg = mhtml_evaluate_string (get_positional_arg (vars, 2));
      Symbol **symbols = (Symbol **)NULL;

      if (!empty_string_p (package_arg))
	symbols = symbol_package_symbols (package_arg);

      if (package_arg != (char *)NULL)
	free (package_arg);

      if (symbols != (Symbol **)NULL)
	{
	  register int i;
	  BPRINTF_BUFFER *data = bprintf_create_buffer ();
	  Symbol *sym;
	  char *packname = SYMBOL_PACKAGE_NAME (symbols[0]);
	  int plen = SYMBOL_PACKAGE_NAME_LEN (symbols[0]);

	  bprintf (data, "(");

	  for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	    {
	      if (sym->type != symtype_STRING)
		continue;

	      while ((sym->name_len + plen + 4) > varname_size)
		varname = (char *)xrealloc (varname, varname_size += 100);

	      if (strip == 0)
		{
		  sprintf (varname, "%s::%s", packname, sym->name);
		  bprintf (data, "(%s ", wisp_readable (varname));
		}
	      else
		bprintf (data, "(%s ", wisp_readable (sym->name));

	      /* Print out the various values.  If there is none, print
		 the empty string. */
	      if (sym->values_index == 0)
		bprintf (data, ". \"\")");
	      else
		{
		  if (sym->values_index == 1)
		    bprintf (data, ". %s)", wisp_readable (sym->values[0]));
		  else
		    {
		      register int j;

		      for (j = 0; j < sym->values_index; j++)
			bprintf (data, " %s", wisp_readable (sym->values[j]));

		      bprintf (data, ")");
		    }
		}
	    }

	  bprintf (data, ")");

	  {
	    DBOBJ content;

	    content.data = (unsigned char *)data->buffer;
	    content.length = (size_t)data->bindex;

	    database_store (db, key, &content);

	    if (database_had_error_p ())
	      page_syserr ("%s", database_strerror ());
	    else
	      result = "true";
	  }

	  bprintf_free_buffer (data);
	}

      dbobj_free (key);

      if (result)
	bprintf_insert (page, start, "%s", result);
    }
}

DEFUN (pf_database_first_key, dbvar,
"Return a string representing the \"first\" key found in the database
referenced by <var dbvar>.  The key is suitable for input to any of
the database functions which takes a key as input.  The order in which
keys are returned appears random, but the return value is always a
suitable argument to <funref database-commands database-next-key>.")
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

	  dbobj_free (key);
	}
    }
}

DEFUN (pf_database_next_key, dbvar after-key,
"Return the \"next\" key in the database.  The key is found by looking
at <var after-key>, which makes this function suitable for calling in
a loop.  For example, the following code iterates over an entire
database.

<example>
<with-open-database db /file.db mode=read>
  <set-var key=<database-first-key db>>
  <while <get-var key>>
    <package-delete record>
    <set-var loaded? =
      <database-load-record db <get-var key> package=record>>
    Key: <get-var key>, Name: <get-var record::name>
    <set-var key = <database-next-key db <get-var key>>>
  </while>
</with-open-database>
</example>")
{
  DBFILE db;
  DBOBJ *inkey = (DBOBJ *)NULL;
  DBOBJ *outkey = (DBOBJ *)NULL;

  dbkey_function_args (vars, &db, &inkey, 1);

  if (db && inkey)
    outkey = database_nextkey (db, inkey);

  if (outkey)
    {
      bprintf_insert (page, start, "%s", outkey->data);
      *newstart = start + (outkey->length - 1);
      dbobj_free (outkey);
    }

  dbobj_free (inkey);
}

DEFUN (pf_database_unique_key, dbvar &optional suggestion,
"Return a key which is guaranteed to not already exist in the the
database referenced by <var db>.  If <var suggestion> is supplied,
that key is tried first, and then subsequent attempts are various
modifications to the suggestion.")
{
  DBFILE db;
  DBOBJ *key;

  dbkey_function_args (vars, &db, &key, 0);

  if (db != (DBFILE)0)
    {
      char *unique_id = (char *)NULL;
      DBOBJ *content = (DBOBJ *)NULL;
      int done = 0, tried = 0;
      char *suggestion;

      if (key)
	suggestion = strdup ((char *)key->data);
      else
	suggestion = strdup ("dbkey");

      unique_id = (char *)xmalloc (20 + strlen (suggestion));

      while (!done)
	{
	  if (tried)
	    {
	      sprintf (unique_id, "%s-%ld",
		       suggestion, (unsigned long)random_number ());
	    }
	  else
	    {
	      tried++;
	      sprintf (unique_id, "%s", suggestion);
	    }

	  key = database_setkey (unique_id);
	  content = database_fetch (db, key);
	  free (key);

	  if (content == (DBOBJ *)NULL)
	    {
	      bprintf_insert (page, start, "%s", unique_id);
	      *newstart += strlen (unique_id);
	      done = 1;
	    }
	  else
	    dbobj_free (content);
	}
      free (unique_id);
      free (suggestion);
    }
}

/* Read the record in DB at KEY, and return a package containing the
   fields and values. */
static Package *
database_package_contents (DBFILE db, DBOBJ *key)
{
  DBOBJ *content = database_fetch (db, key);
  Package *package = (Package *)NULL;

  /* Unpack the record if there is one. */
  if ((content != (DBOBJ *)NULL) && (content->data != (unsigned char *)NULL))
    {
      package = alist_to_package ((char *)content->data);
      forms_set_tag_value_in_package (package, "key", (char *)key->data);
      free (content->data);
    }

  if (content)
    free (content);

  return (package);
}

/* <database-query db <expr> [format=<expr>] [keys=varname] sort=field,field>
   Select and optionally format records in the database according
   to the criterion in EXPR.  EXPR is evaluated with the fields of
   the database as the current package.  If the result of that
   evaluation is not an empty string, then that record is selected
   for further processing by either FORMAT, KEYS, or to return
   in plain text the list of keys.
   If FORMAT is present, it is an expression to evaluate in the
   context of the database fields, (as with EXPR).
   If OUTPUT is specified, it is the name of a variable to receive
   the list of keys which satisfied EXPR. */
typedef struct {
  char *key;
  Package *contents;
  char **sort_fields;
} DBRecord;

static int
dbrec_comp (const void *arg1, const void *arg2)
{
  register int i;
  DBRecord *rec1 = *(DBRecord **)arg1;
  DBRecord *rec2 = *(DBRecord **)arg2;
  char *string1 = (char *)NULL;
  char *string2 = (char *)NULL;
  int result = 0;

  if (rec1 && rec1->sort_fields && rec1->contents)
    {
      BPRINTF_BUFFER *buff = bprintf_create_buffer ();

      for (i = 0; rec1->sort_fields[i]; i++)
	{
	  char *temp;

	  temp = forms_get_tag_value_in_package
	    (rec1->contents, rec1->sort_fields[i]);

	  if (temp)
	    bprintf (buff, "%s", temp);
	}

      string1 = buff->buffer;
      free (buff);
    }

  if (rec2 && rec2->sort_fields && rec2->contents)
    {
      BPRINTF_BUFFER *buff = bprintf_create_buffer ();

      for (i = 0; rec2->sort_fields[i]; i++)
	{
	  char *temp;

	  temp = forms_get_tag_value_in_package
	    (rec2->contents, rec2->sort_fields[i]);

	  if (temp)
	    bprintf (buff, "%s", temp);
	}

      string2 = buff->buffer;
      free (buff);
    }

  if (string1 && !string2)
    result = -1;
  else if (!string1 && string2)
    result = 1;
  else if (string1 && string2)
    {
      /* Check for both strings all digits.  If so, sort numerically. */
      if (mhtml_all_digits (string1) && mhtml_all_digits (string2))
	{
	  long val1 = atol (string1);
	  long val2 = atol (string2);

	  result = val2 - val1;
	}
      else
	result = strcasecmp (string1, string2);
    }

  if (string1) free (string1);
  if (string2) free (string2);

  return (result);
}

static char *global_dbrec_sortfun = (char *)NULL;

static int
dbrec_callout (const void *arg1, const void *arg2)
{
  register int i;
  DBRecord *rec1 = *(DBRecord **)arg1;
  DBRecord *rec2 = *(DBRecord **)arg2;
  int result = 0;

  if (empty_string_p (global_dbrec_sortfun))
    return (0);
  else if (!rec1)
    return (-1);
  else if (!rec2)
    return (1);
  else
    {
      BPRINTF_BUFFER *buff = bprintf_create_buffer ();
      char *compare_result;
      char *temp;

      bprintf (buff, "<%s", global_dbrec_sortfun);

      for (i = 0; rec1->sort_fields[i]; i++)
	{
	  temp = forms_get_tag_value_in_package
	    (rec1->contents, rec1->sort_fields[i]);

	  bprintf (buff, " <prog %s>", temp ? temp : "");
	}

      for (i = 0; rec2->sort_fields[i]; i++)
	{
	  temp = forms_get_tag_value_in_package
	    (rec2->contents, rec2->sort_fields[i]);

	  bprintf (buff, " <prog %s>", temp ? temp : "");
	}

      bprintf (buff, ">");
      compare_result = mhtml_evaluate_string (buff->buffer);
      bprintf_free_buffer (buff);

      if (empty_string_p (compare_result))
	result = 0;
      else
	{
	  for (i = 0; whitespace (compare_result[i]); i++);

	  switch (compare_result[i])
	    {
	    case 'l':
	    case 'L':
	    case '-':
	    case '<':
	      result = -1;
	      break;

	    case 'g':
	    case 'G':
	    case '1':
	    case '>':
	      result = 1;
	      break;

	    case 'e':
	    case 'E':
	    case '0':
	    case '=':
	      result = 0;
	      break;
	    }
	}

      if (compare_result != (char *)NULL)
	free (compare_result);
    }
  return (result);
}

DEFUN (pf_database_query, dbvar expr &key format=fexpr keys=varname
       sort=field... sortorder=reverse,
"Select records from the database referenced by <var dbvar>, based on
<var expr>, perhaps sorting them based on the data in <var FIELD>s,
and optionally formatting them with the <Meta-HTML> expression in <var
fexpr>.

Both <var expr> and <var fexpr> are evaluated with the fields of the
database as the current package, so references to variables which are
not part of the record structure must be fully qualified (i.e., by
prefixing them with a package name. See <secref Packages> for more
information.) If the result of evaluating <var expr> is not the empty
string, then the record is selected for further processing by either
<var format>, <var keys>, or to be returned in a  plain text list of
keys. 

The order of evaluation is as follows:

First, the database is queried, and an internal list of selected
records is created.  Then, if <var sort=field1,field2...> is present,
the records are sorted using the values of the specified fields.  The
order of the sort can be reversed by passed <var sortorder=reverse>. 

Next, if <var format=fexpr> is present, <var fexpr> is executed for
each record, in an environment where the current package consists of
bindings for each field's name to each field's value, and the special
binding of <code>key</code> to the key of that record.  Finally, if
<var keys=varname> is present, <var varname> is the name of a variable
into which the matched keys are stored as an array.

The <var sort> parameters indicates which fields to sort the resultant
keys on; the field names <var field1,field2...> are separated by
commas.

Examples:

This simple query stores the key of every record in the database whose
<b>name</b> field contains \"Fox\" into the variable named
<b>dbkeys</b>:

<example>
<with-open-database db \"/www/data/employee.db\" mode=read>
  <database-query db <match <get-var name> \"Fox\"> keys=dbkeys>
</with-open-database>
</example>

This more complex query formats an alphabetical list of all the
members of the database who are employed as carpenters, and are
between the ages of 20 and 45, by creating links to a page which will
(presumably) get a more detailed listing of the individual record:

<example>
<with-open-database db \"employee.db\" mode=read>
  <database-query db sort=LastName
    <and <match <get-var job> \"carpent\">
         <gt age 20>
         <lt age 45>>
    format = <prog
               <a href=\"detail-display.mhtml?<cgi-encode key>\">
               <get-var LastName>, <get-var FirstName>
               </a>: <get-var Job>, <get-var Age>>>
</with-open-database>
</example>")
{
  DBFILE db = (DBFILE)0;
  char *dbref = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *expr = get_positional_arg (vars, 1);
  char *format_expr = get_value (vars, "format");
  char *sort_args = mhtml_evaluate_string (get_value (vars, "sort"));
  char *sortfun = mhtml_evaluate_string (get_value (vars, "predicate"));
  char *keys_var = get_value (vars, "keys");

  if (database_environment_level == 0 || empty_string_p (dbref))
    {
      goto clean_up;
    }
  else
    {
      char *rep = pagefunc_get_variable (dbref);

      if (rep != (char *)NULL)
	{
	  long dbval = strtol (rep, (char **)NULL, 16);
	  db = (DBFILE)dbval;
	}
    }

  if ((db != (DBFILE)0) && (expr != (char *)NULL))
    {
      register int i;
      DBOBJ *key = database_firstkey (db);
      DBOBJ *nextkey;
      DBRecord **records = (DBRecord **)NULL;
      int rec_slots = 0;
      int rec_index = 0;
      char *search_limit_string;
      int search_limit = -1;

      search_limit_string =
	mhtml_evaluate_string (get_value (vars, "search-limit"));

      if (!empty_string_p (search_limit_string))
	{
	  search_limit = atoi (search_limit_string);
	  if (search_limit == 0) search_limit = -1;
	}

      if (search_limit_string) free (search_limit_string);

      /* Build the list of matching records. */
      while ((key != (DBOBJ *)NULL) &&
	     ((search_limit < 0) || (rec_index < search_limit)))
	{
	  Package *db_fields = database_package_contents (db, key);

	  if (db_fields)
	    {
	      char *expr_result;

	      symbol_push_package (db_fields);
	      expr_result = mhtml_evaluate_string (expr);
	      symbol_pop_package ();

	      /* If satisfied, save this key. */
	      if (!empty_string_p (expr_result))
		{
		  DBRecord *rec = (DBRecord *)xmalloc (sizeof (DBRecord));
		  rec->key = strdup ((char *)key->data);
		  rec->contents = db_fields;

		  if (rec_index + 2 > rec_slots)
		    records = (DBRecord **)xrealloc
		      (records, (rec_slots += 30) * sizeof (DBRecord *));

		  records[rec_index++] = rec;
		  records[rec_index] = (DBRecord *)NULL;
		}
	      else
		symbol_destroy_package (db_fields);

	      if (expr_result) free (expr_result);
	    }
	  nextkey = database_nextkey (db, key);
	  free (key->data);
	  free (key);
	  key = nextkey;
	}

      dbobj_free (key);

      /* If there are any matched keys, then sort, format, and/or return
	 the keys. */
      if (rec_index != 0)
	{
	  if (!empty_string_p (sort_args))
	    {
	      char **sort_fields = (char **)NULL;
	      char *sortorder;
	      int sf_index = 0;
	      int sf_slots = 0;
	      char *temp_name;
	      int offset = 0;

	      sortorder = mhtml_evaluate_string(get_value (vars, "sortorder"));

	      while (sort_args[offset])
		{
		  /* Skip whitespace and commas between field names. */
		  while (whitespace (sort_args[offset]) ||
			 (sort_args[offset] == ',')) offset++;

		  /* Snarf everything upto a comma or end of text. */
		  for (i = offset; sort_args[i] && sort_args[i] != ','; i++);

		  temp_name = (char *)xmalloc (1 + (i - offset));
		  strncpy (temp_name, sort_args + offset, i - offset);
		  temp_name[i - offset] = '\0';
		  offset = i;

		  if (sf_index + 2 > sf_slots)
		    sort_fields = (char **)xrealloc
		      (sort_fields, (sf_slots += 10) * sizeof (char *));

		  sort_fields[sf_index++] = temp_name;
		  sort_fields[sf_index] = (char *)NULL;
		}

	      /* Set every record to use the same set of sort fields. */
	      for (i = 0; i < rec_index; i++)
		records[i]->sort_fields = sort_fields;

	      /* Sort the keys. */
	      global_dbrec_sortfun = sortfun;
	      if (empty_string_p (sortfun))
		qsort (records, rec_index, sizeof (DBRecord *), dbrec_comp);
	      else
		qsort (records, rec_index, sizeof (DBRecord *), dbrec_callout);

	      if (sortfun) free (sortfun);
	      global_dbrec_sortfun = (char *)NULL;

	      /* Free the sort fields. */
	      if (sort_fields)
		{
		  for (i = 0; i < sf_index; i++)
		    free (sort_fields[i]);
		  free (sort_fields);
		}

	      if ((sortorder != (char *)NULL) &&
		  ((strcasecmp (sortorder, "reverse") == 0) ||
		   (strcasecmp (sortorder, "descending") == 0)))
		{
		  DBRecord **reversed = (DBRecord **)
		    xmalloc ((1 + rec_index) * sizeof (DBRecord *));
		  for (i = 0; i < rec_index; i++)
		    reversed[(rec_index - 1) - i] = records[i];
		  free (records);
		  reversed[rec_index] = (DBRecord *)NULL;
		  records = reversed;
		}
	    }

	  /* If there is a format operator, evaluate it now. */
	  if (format_expr != (char *)NULL)
	    {
	      char *temp;
	      int format_limit = -1;
	      int window_start = 0, window_length = 1000000;

	      temp = mhtml_evaluate_string (get_value (vars, "window-start"));
	      if (!empty_string_p (temp)) window_start = atoi (temp);
	      xfree (temp);

	      temp = mhtml_evaluate_string (get_value (vars, "window-length"));
	      if (!empty_string_p (temp)) window_length = atoi (temp);
	      xfree (temp);

	      temp = mhtml_evaluate_string (get_value (vars, "format-limit"));
	      if (!empty_string_p (temp))
		{
		  format_limit = atoi (temp);
		  if (format_limit == 0) format_limit = -1;
		}
	      xfree (temp);

	      for (i = window_start;
		   ((i < rec_index) && (i < window_length) &&
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
	  if (keys_var != (char *)NULL)
	    {
	      char *sym_name = mhtml_evaluate_string (keys_var);

	      if (!empty_string_p (sym_name))
		{
		  Symbol *sym;
		  char **keys;
		  char *tname;

		  tname = strchr (sym_name, '[');
		  if (tname)
		    *tname = '\0';

		  if (!empty_string_p (sym_name))
		    {
		      keys = (char **)
			xmalloc ((1 + rec_index) * sizeof (char *));

		      for (i = 0; i < rec_index; i++)
			keys[i] = strdup (records[i]->key);

		      keys[i] = (char *)NULL;

		      sym = symbol_remove (sym_name);
		      symbol_free (sym);
		      sym = symbol_intern (sym_name);
		      sym->values = keys;
		      sym->values_index = rec_index;
		      sym->values_slots = 1 + rec_index;
		    }
		}

	      if (sym_name) free (sym_name);
	    }
	  else if (empty_string_p (format_expr))
	    {
	      /* Nothing specified for output.  Dump the keys right here. */
	      for (i = 0; i < rec_index; i++)
		{
		  bprintf_insert (page, start, "%s\n", records[i]->key);
		  start += 1 + strlen (records[i]->key);
		}
	      *newstart = start;
	    }

	  /* Finally, free the memory that we have used. */
	  for (i = 0; i < rec_index; i++)
	    {
	      symbol_destroy_package (records[i]->contents);
	      free (records[i]->key);
	      free (records[i]);
	    }

	  free (records);
	}
      else
	{
	  /* There weren't any records that matched.  But there might
	     have been a key variable specified.  In that case, set it
	     to no keys. */
	  if (keys_var != (char *)NULL)
	    {
	      char *sym_name = mhtml_evaluate_string (keys_var);

	      if (!empty_string_p (sym_name))
		{
		  Symbol *sym;
		  char *tname;

		  tname = strchr (sym_name, '[');
		  if (tname)
		    *tname = '\0';

		  sym = symbol_remove (sym_name);
		  symbol_free (sym);
		  sym = symbol_intern (sym_name);
		}

	      if (sym_name) free (sym_name);
	    }
	}
    }

 clean_up:
  if (dbref) free (dbref);
  if (sort_args) free (sort_args);
}

#if defined (__cplusplus)
}
#endif
