/* mdb.c: -*- C -*-  Meta-HTML Debugger. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Sep 30 07:10:16 1995.  */

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
#include "language.h"
#include "mdb.h"
#include "commands.h"

#if !defined (MHTML_SYSTEM_TYPE)
#  define MHTML_SYSTEM_TYPE "Incorrectly Compiled"
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

typedef struct
{
  char *line;
  char *data;
} HIST_ENTRY;

extern char *readline (char *prompt);
extern int history_truncate_file (char *file, int lines);
extern void add_history (char *string);
extern int read_history (char *filename);
extern int write_history (char *filename);
extern void using_history (void);
extern HIST_ENTRY *previous_history (void);
extern int debugging_with_mdb;
extern int mhtml_parser_interrupted;
extern void mdb_reset_pushed_functions (void);

extern char *tilde_expand (char *string);
extern void mdb_initialize_readline (void);

extern char **environ;

static void initialize_mdb (int argc, char *argv[]);
PAGE *mdb_page;

#if !defined (MHTML_VERSION_STRING)
static char *mhtml_version_string = "";
#else
static char *mhtml_version_string = MHTML_VERSION_STRING;
#endif

/* Non-zero when --config has been seen. */
static int config_arg_given = 0;

int MDB_Interrupted = 0;

jmp_buf mdb_top_level_jmp_buffer;

int mdb_loop_level = 0;
void mdb_loop (void);
static char *find_history_create (char *name);

static int mdb_major = 3;
static int mdb_minor = 0;

int
main (int argc, char *argv[])
{
  char *history_file = (char *)NULL;

  if (!*metahtml_copyright_string)
    abort ();

  printf ("MDB %d.%d: Interactive Debugger for Meta-HTMLv%s, by Brian J. Fox.",
	  mdb_major, mdb_minor, mhtml_version_string);
  printf ("\n%s\n", metahtml_copyright_string);
  printf ("Compiled for (%s)\n", MHTML_SYSTEM_TYPE);

  initialize_mdb (argc, argv);

  {
    char *ignore = mhtml_evaluate_string ("<%%bootstrap-metahtml true>");
    xfree (ignore);
  }

  history_file = find_history_create ((char *)NULL);
  history_truncate_file (history_file, 500);
  read_history (history_file);

  while (!MDB_QuitFlag)
    {
      /* signal (SIGINT, restart); */

      if (setjmp (mdb_top_level_jmp_buffer))
	{
	  mhtml_parser_interrupted = 0;
	  mdb_loop_level = 0;
	  mdb_reset_pushed_functions ();
	}

      if (MDB_Interrupted)
	{
	  printf ("Quit\n");
	  MDB_Interrupted = 0;
	}

      mdb_loop ();
    }

  /* Save the history. */
  if (history_file != (char *)NULL)
    write_history (history_file);

  return (0);
}

static char *
find_history_create (char *name)
{
  static char full_history_path[2048];
  char *use_name;
  int free_name = 0;

  if (name == (char *)NULL)
    {
      name = strdup (".mdb_history");
      free_name++;
    }

  if (*name != '/')
    {
      char *temp = (char *)xmalloc (10 + strlen (name));
      sprintf (temp, "~/%s", name);
      use_name = tilde_expand (temp);
    }
  else
    use_name = strdup (name);

  strcpy (full_history_path, use_name);
  free (use_name);
  if (free_name) free (name);
  return (full_history_path);
}

void
mdb_loop (void)
{
  register int i, j;
  char prompt[100];
  char *line;
  int done = 0;

  if (MDB_QuitFlag) return;

  mdb_loop_level++;

  for (i = 0, j = 0; j < mdb_loop_level; j++)
    prompt[i++] = '(';

  prompt[i++] = 'm';
  prompt[i++] = 'd';
  prompt[i++] = 'b';

  for (j = 0; j < mdb_loop_level; j++)
    prompt[i++] = ')';

  prompt[i++] = ' ';
  prompt[i++] = '\0';

  while (!done && !MDB_Interrupted)
    {
      char *result = (char *)NULL;
      int add_to_hist = 1;

      line = readline (prompt);

      if (line == (char *)NULL)
	{
	  line = strdup ("quit");
	  add_to_hist = 0;
	}

      printf ("\r");
      fflush (stdout);

      if (*line)
	{
	  if (add_to_hist)
	    {
	      HIST_ENTRY *entry;

	      using_history ();

	      entry = previous_history ();

	      if ((entry == (HIST_ENTRY *)NULL) ||
		  ((entry->line != (char *)NULL) &&
		   ((entry->line[0] != line[0]) ||
		    (strcmp (entry->line, line) != 0))))
		{
		  using_history ();
		  add_history (line);
		}
	    }
	  result = mdb_command (line);
	}
      else
	result = mdb_redo ();

      if (result)
	{
	  for (i = strlen (result) - 1;
	       (i > -1) && (whitespace (result[i])); i--);

	  if ((i == -1) || !whitespace (result[i]))
	    result[i + 1] = '\0';
	  else
	    result[i] = '\0';

	  if (*result && !MDB_QuitFlag && !MDB_Interrupted)
	    printf ("%s\n", result);

	  free (result);
	}

      free (line);

      if (MDB_QuitFlag || MDB_ContFlag || MDB_Interrupted)
	break;
    }

  if (MDB_ContFlag)
    MDB_ContFlag--;

  mdb_loop_level--;
}

#define MDB_BROKEN_SIGINT_HANDLER 1

void
mdb_signal_restart (int sig)
{
  MDB_Interrupted++;
#if defined (MDB_BROKEN_SIGINT_HANDLER)
  mdb_throw_to_top_level ();
#else
  mhtml_parser_interrupted = 1;
#endif
}

void
mdb_throw_to_top_level (void)
{
  longjmp (mdb_top_level_jmp_buffer, 1);
}

static void
initialize_mdb (int argc, char *argv[])
{
  char working_dir[1024];
  char *result = getcwd (working_dir, 1023);

  mhtml_system_preload (1);
  debugging_with_mdb = 1;

  signal (SIGINT, mdb_signal_restart);

  /* Generate PAGEFUNC_FUNCTION_PACKAGE by evaluating something. */
  {
    PAGE *page = page_create_page ();
    bprintf (page, "<defsubst *mdb-version*>%d.%d</defsubst>\n",
	     mdb_major, mdb_minor);
    page_process_page (page);
    if (page)
      page_free_page (page);
  }

  mdb_initialize_readline ();

  pagefunc_set_variable ("mhtml::version", mhtml_version_string);
  pagefunc_set_variable ("mhtml::system-type", MHTML_SYSTEM_TYPE);
  pagefunc_set_variable ("mhtml::path-info", "");
  pagefunc_set_variable ("mhtml::http-to-host", "");
  pagefunc_set_variable ("mhtml::current-doc", "");
  pagefunc_set_variable ("mhtml::include-prefix", result);
  pagefunc_set_variable ("mhtml::relative-prefix", "");

  /* Try to get smart here.  If the include-prefix contains "/docs/...",
     then the relative prefix is "/...", and the include-prefix goes up
     to "/docs". */
  if (result != (char *)NULL)
    {
      register int i;

      for (i = 0; result[i] != '\0'; i++)
	if (strncasecmp (result + i, "/docs", 5) == 0)
	  {
	    if ((result[i + 5] == '\0') || (result[i + 5] == '/'))
	      {
		char *incpref = (char *)xmalloc (1 + (i + 5));
		strncpy (incpref, result, i + 5);
		incpref[i + 5] = '\0';
		pagefunc_set_variable ("mhtml::include-prefix", incpref);
		free (incpref);
		pagefunc_set_variable ("mhtml::relative-prefix",
				       result + i +5);
		break;
	      }
	  }
    }

  pagefunc_set_variable ("mhtml::current-url", "");
  pagefunc_set_variable ("mhtml::current-url-sans-sid", "");
  pagefunc_set_variable ("mhtml::full-url", "");
  pagefunc_set_variable ("mhtml::full-url-sans-sid", "");
  pagefunc_set_variable ("mhtml::http-prefix", "");
  pagefunc_set_variable ("mhtml::http-prefix-sans-sid", "");
  pagefunc_set_variable ("mhtml::cookie-compatible", "true");
  pagefunc_set_variable ("mhtml::url-to-dir", "");
  pagefunc_set_variable ("mhtml::url-to-dir-sans-sid", "");
  pagefunc_set_variable ("mhtml::version", mhtml_version_string);
  pagefunc_set_variable ("mhtml::exec-path", (char *)getenv ("PATH"));
  pagefunc_set_variable ("mdb::running-in-mdb", "true");

      /* Set mhtml::require-directories[], to a reasonable value here. */
      pagefunc_set_variable ("mhtml::require-directories[]",
".\ntagsets\nmacros\nincludes\n\
..\n../tagsets\n../macros\n../includes\n\
../..\n../../tagsets\n../../macros\n../../includes");

  /* Get the environment into MDB. */
  {
    register int i;
    Package *pack = symbol_get_package ("ENV");
    Symbol *sym;

    for (i = 0; environ[i] != (char *)NULL; i++)
      {
	char *name = strdup (environ[i]);
	char *value = name;

	while ((*value != '\0') && (*value != '=')) value++;
	if (*value == '=')
	  *value++ = '\0';

	sym = symbol_intern_in_package (pack, name);
	if (*value)
	  symbol_add_value (sym, value);

	free (name);
      }
  }

  {
    PAGE *page = page_read_template (".mdb_init");

    if (!page)
      page = page_read_template (".mdbinit");

    if (page)
      {
	page_process_page (page);
	page_free_page (page);
      }
  }

  if (argc > 1)
    {
      register int i;
      char load_command[1000];
      char *temp;

      for (i = 1; i < argc; i++)
	{
	  char *filename = argv[i];

	  if (strcmp (filename, "--config") == 0)
	    {
	      PAGE *page;

	      config_arg_given++;

	      if ((i + 1) < argc)
		{
		  filename = argv[++i];

		  page = page_read_template (filename);
		  if (page != (PAGE *)NULL)
		    {
		      char *orig_dir = strdup (result);

		      fprintf (stderr, "Processing %s...", filename);
		      page_process_page (page);
		      temp = pagefunc_get_variable ("mhtml::document-root");

		      if (temp != (char *)NULL)
			{
			  int tlen;

			  chdir (temp);
			  result = getcwd (working_dir, 1023);
			  chdir (orig_dir);
			  temp = result;
			  tlen = strlen (temp);

			  pagefunc_set_variable
			    ("mhtml::include-prefix", temp);

			  if (strncmp (orig_dir, temp, tlen) == 0)
			    pagefunc_set_variable
			      ("mhtml::relative-prefix", orig_dir + tlen);
			}

		      set_session_database_location
			(pagefunc_get_variable
			 ("mhttpd::session-database-file"));

		      free (orig_dir);
		    }
		  else
		    {
		      fprintf (stderr, "Couldn't locate config file %s!",
			       filename);
		    }
		  fprintf (stderr, "\n");
		}
	      else
		{
		  fprintf (stderr, "`--config' requires a filename!\n");
		}
	    }
	  else
	    {
	      sprintf (load_command, "load %s", filename);
	      temp = mdb_command (load_command);
	      if (temp)
		free (temp);
	    }
	}
    }
}


#if defined (__cplusplus)
}
#endif
