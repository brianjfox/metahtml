/* commands.c: -*- C -*-  Implementation of the commands for MDB. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Sep 30 13:27:28 1995.  */

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
#include "breakpoints.h"

#if defined (__cplusplus)
extern "C"
{
#endif
/* ../libmhtml/parser.c */
extern char **mdb_pushed_function_names (void);

/* readline/readline.c */
extern char *readline (char *prompt);

/* An array of MDB_File pointers. */
static MDB_File **mdb_files = (MDB_File **)NULL;
static int mdb_files_slots = 0;
static int mdb_files_index = 0;
static int mdb_files_offset = 0;

/* Information about the recent file and line number. */
static char *mdb_recent_file = (char *)NULL;
static int mdb_recent_line_number = -1;

/* The "current" breakpoint. */
static MDB_Breakpoint *mdb_current_bp = (MDB_Breakpoint *)NULL;

/* When non-zero, it is time to quit. */
int MDB_QuitFlag = 0;

/* When non-zero, it is time to continue. */
int MDB_ContFlag = 0;

static MFunction *MDB_LastCommand = (MFunction *)NULL;
static int mdb_allow_redo = 1;
#define CANNOT_REDO mdb_allow_redo = 0

static void initialize_page_handlers (void);
static void parse_file_and_line (char *string, char **file,
				 int *line, int *defaulted_p);
static void find_file_and_line_for_function (char *, char **, int *);
static char *mdb_quit (MDBArgs);
static char *mdb_eval (MDBArgs);
static char *mdb_help (MDBArgs);
static char *mdb_exec (MDBArgs);
static char *mdb_load (MDBArgs);
static char *mdb_list (MDBArgs);
static char *mdb_disasm (MDBArgs);
static char *mdb_compile (MDBArgs);
static char *mdb_info (MDBArgs);
static char *mdb_breakpoint (MDBArgs);
static char *mdb_delete (MDBArgs);
static char *mdb_cont (MDBArgs);
static char *mdb_next (MDBArgs);
static char *mdb_step (MDBArgs);
static char *mdb_print (MDBArgs);
static char *mdb_apropos (MDBArgs);
static char *mdb_where (MDBArgs);
static char *mdb_history (MDBArgs);
static char *mdb_cd (MDBArgs);
static char *mdb_pwd (MDBArgs);

MDBCommand mdb_command_table[] =
{
  { "quit",	(char *)NULL,	mdb_quit,	"quit",
      "Quit using the debugger" },

  { "eval",	(char *)NULL,	mdb_eval,	"eval EXPR",
      "Evaluate EXPR in the current context" },

  { "print",	"p",		mdb_print,	"print VARIABLE",
      "Display the contents of VARIABLE in the current context" },

  { "help",	"?",		mdb_help,	"help [COMMAND]",
      "Provide help on using MDB" },

  { "exec",	"r",		mdb_exec,	"exec FILENAME",
      "Load FILENAME, and evaluate the contents" },

  { "break",	"b",		mdb_breakpoint,	"break [FILENAME:]LINE",
      "Set a breakpoint for the RUN command" },

  { "cd",	(char *)NULL,	mdb_cd,		"cd [DIRECTORY]",
      "Change the current working directory, and mhtml::include-prefix" },

  { "delete",	"d",		mdb_delete,	"delete BREAKPOINT",
      "Delete an existing breakpoint" },
  
  { "cont",	"c",		mdb_cont,	"cont [COUNT]",
      "Continue from a breakpoint, perhaps skipping COUNT more" },

  { "where",	"w",		mdb_where,	"where",
      "Display the current stack state" },

  { "next",	"n",		mdb_next,	"next [COUNT]",
      "Executes COUNT more statements" },

  { "pwd",	(char *)NULL,	mdb_pwd,	"pwd",
      "Show the actual and virtual directory locations" },

  { "step",	"s",		mdb_step,	"step [COUNT]",
      "Steps into current expression" },

  { "load",	(char *)NULL,	mdb_load,	"load FILENAME",
      "Load FILENAME, making it the current file" },

  { "list",	"l",		mdb_list,	"list FUNC|[FILENAME:]LINE",
      "List the page around LINE-NUMBER, or the text of FUNC" },

  { "disassemble",	"dis",	mdb_disasm,	"dis FUNC",
      "Disassemble the compiled function FUNC" },

  { "compile",		"comp",	mdb_compile,	"comp FUNC",
      "Compiles the function FUNC" },

  { "info",	"i",		mdb_info,	"info [file|fun|breakpoint]",
      "Get information about files, functions, or breakpoints" },

  { "apropos",	(char *)NULL,	mdb_apropos,	"apropos STRING",
      "List variables and functions that have STRING in their names" },

  { "history",	(char *)NULL,	mdb_history,	"history [count]",
      "Show COUNT lines from the history" },

  { (char *)NULL, (char *)NULL, (MFunction *)NULL, (char *)NULL, (char *)NULL }
};

static MDBCommand *
find_command (char *name)
{
  register int i;
  int possibles = 0;
  MDBCommand *close_match = (MDBCommand *)NULL;
  int namelen = strlen (name);
  
  for (i = 0; mdb_command_table[i].name; i++)
    {
      if ((strcasecmp (mdb_command_table[i].name, name) == 0) ||
	  (mdb_command_table[i].alias &&
	   (strcasecmp (mdb_command_table[i].alias, name) == 0)))
	return (&mdb_command_table[i]);
      else if (strncasecmp (mdb_command_table[i].name, name, namelen) == 0)
	{
	  possibles++;
	  close_match = &mdb_command_table[i];
	}
    }      

  /* Okay, that failed.  If the command is uniquely expressed by what was
     typed, then use that command. */
  if (possibles == 1)
    return (close_match);
  else
    return ((MDBCommand *)NULL);
}

static char *
make_error (char *format, ...)
{
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *result;
  va_list args;

  va_start (args, format);
  bprintf (buffer, "Error: ");
  vbprintf (buffer, format, args);

  if (buffer->buffer && buffer->bindex &&
      buffer->buffer[buffer->bindex - 1] != '\n')
    bprintf (buffer, "\n");

  result = buffer->buffer;
  free (buffer);

  return (result);
}

static char *
make_message (char *format, ...)
{
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *result;
  va_list args;

  va_start (args, format);
  vbprintf (buffer, format, args);

  if (buffer->buffer && buffer->bindex &&
      buffer->buffer[buffer->bindex - 1] != '\n')
    bprintf (buffer, "\n");

  result = buffer->buffer;
  free (buffer);

  return (result);
}

typedef struct
{
  MDB_File *file;
  int lineno;
  char *fname;
} MDB_FunctionDef;

static MDB_FunctionDef **snarfed_functions = (MDB_FunctionDef **)NULL;
static int snarfed_slots = 0;
static int snarfed_index = 0;

static void
mdb_scan_for_defuns (MDB_File *file, PAGE *page)
{

  if (page != (PAGE *)NULL)
    {
      register int i = 0;
      register int lineno = 1;

      while  (i < (page->bindex - 4))
	{
	  if (page->buffer[i] == '\n')
	    {
	      lineno++; i++;
	      continue;
	    }

	  if (page->buffer[i] != '<')
	    {
	      i++;
	      continue;
	    }

	  if ((strncasecmp ((page->buffer + i), "<defun", 6) == 0) ||
	      (strncasecmp ((page->buffer + i), "<define-", 8) == 0) ||
	      (strncasecmp ((page->buffer + i), "<defmacro", 9) == 0) ||
	      (strncasecmp ((page->buffer + i), "<defsubst", 9) == 0))
	    {
	      int thisline = lineno + 1;
	      int start, end;
	      char *fname = (char *)NULL;
	      MDB_FunctionDef *fdef = (MDB_FunctionDef *)
		xmalloc (sizeof (MDB_FunctionDef));

	      while (!whitespace (page->buffer[i])) i++;
	      while (whitespace (page->buffer[i]))
		{
		  if (page->buffer[i] == '\n')
		    lineno++;
		  i++;
		}
	      start = i;
	      while ((!whitespace (page->buffer[i])) &&
		     (page->buffer[i] != '>')) i++;
	      end = i;

	      fname = (char *)xmalloc (1 + (end - start));
	      strncpy (fname, page->buffer + start, end - start);
	      fname[end - start] = '\0';

	      fdef->file = file;
	      fdef->lineno = thisline;
	      fdef->fname = fname;

	      /* Find out if we should replace an existing definition. */
	      {
		register int j;
		MDB_FunctionDef *installed = (MDB_FunctionDef *)NULL;

		for (j = 0; j < snarfed_index; j++)
		  if (strcasecmp (fname, snarfed_functions[j]->fname) == 0)
		    {
		      installed = snarfed_functions[j];
		      break;
		    }

		if (installed)
		  {
		    free (installed->fname);
		    free (installed);
		    snarfed_functions[j] = fdef;
		  }
		else
		  {
		    if ((snarfed_index + 2) > snarfed_slots)
		      snarfed_functions = (MDB_FunctionDef **)xrealloc
			(snarfed_functions,
			 (snarfed_slots += 20) * sizeof (MDB_FunctionDef *));

		    snarfed_functions[snarfed_index++] = fdef;
		    snarfed_functions[snarfed_index] = (MDB_FunctionDef *)NULL;
		  }
	      }
	    }
	  else
	    i++;
	}
    }
}

static MDB_File *
mdb_add_file (char *filename, PAGE *page)
{
  MDB_File *file = (MDB_File *)xmalloc (sizeof (MDB_File));

  file->filename = strdup (filename);

  file->nameonly = strrchr (file->filename, '/');
  if (file->nameonly)
    file->nameonly += 1;
  else
    file->nameonly = file->filename;

  file->contents = page;
  file->line_number = 0;
  stat (filename, &(file->finfo));

  if (mdb_files_index + 2 > mdb_files_slots)
    mdb_files = (MDB_File **)xrealloc
      (mdb_files, ((mdb_files_slots += 10) * sizeof (MDB_File *)));

  mdb_files[mdb_files_index++] = file;
  mdb_files[mdb_files_index] = (MDB_File *)NULL;

  mdb_scan_for_defuns (file, page);
  return (file);
}

static MDB_File *
mdb_find_file (char *filename)
{
  register int i;
  MDB_File *file = (MDB_File *)NULL;
  char *incpref = pagefunc_get_variable ("mhtml::include-prefix");
  char *relpref = pagefunc_get_variable ("mhtml::relative-prefix");
  char *wr = (char *)NULL;
  char *canon_name;

  if ((filename && incpref) &&
      (strncmp (filename, incpref, strlen (incpref)) == 0))
    canon_name = strdup (filename);
  else
    canon_name = mhtml_canonicalize_file_name(filename, incpref, relpref, &wr);

  if (canon_name != (char *)NULL)
    {
      for (i = 0; i < mdb_files_index; i++)
	if ((strcmp (canon_name, mdb_files[i]->filename) == 0) ||
	    (strcmp (filename, mdb_files[i]->filename) == 0) ||
	    (strcmp (filename, mdb_files[i]->nameonly) == 0))
	  {
	    file = mdb_files[i];
	    break;
	  }

      if (!file)
	{
	  PAGE *page = page_read_template (canon_name);

	  if (page)
	    file = mdb_add_file (canon_name, page);
	}
      else
	{
	  struct stat finfo;

	  if (stat (canon_name, &finfo) > -1)
	    {
	      if (finfo.st_mtime > file->finfo.st_mtime)
		{
		  page_free_page (file->contents);
		  file->contents = page_read_template (canon_name);
		  memmove (&(file->finfo), &finfo, sizeof (finfo));
		  fprintf (stderr, "Reloaded changed `%s'.\n", file->nameonly);
		}
	    }
	}
      free (canon_name);
    }

  return (file);
}

static void
mdb_set_current_file (char *filename)
{
  register int i;

  for (i = 0; i < mdb_files_index; i++)
    if (strcmp (filename, mdb_files[i]->filename) == 0)
      {
	mdb_files_offset = i;
	if (mdb_recent_file)
	  free (mdb_recent_file);
	mdb_recent_file = strdup (mdb_files[i]->filename);
	break;
      }
}

char *
mdb_command (char *line)
{
  register int start, i;
  char *name = (char *)NULL;
  char *result = (char *)NULL;
  MDBCommand *command = (MDBCommand *)NULL;
  static int initialized = 0;

  if (!initialized)
    {
      initialize_page_handlers ();
      initialized = 1;
    }

  /* Kill leading whitespace. */
  for (start = 0; whitespace (line[start]); start++);

  mdb_allow_redo = 1;
  if (line[start] == '<')
    {
      name = strdup ("eval");
      i = start;
    }
  else
    {
      for (i = start; line[i] && !whitespace (line[i]); i++);

      if (i == start)
	return ((char *)NULL);

      name = (char *)xmalloc (1 + (i - start));
      strncpy (name, line + start, i - start);
      name[i - start] = '\0';

      while (whitespace (line[i])) i++;
    }

  command = find_command (name);

  /* Trim trailing whitespace from LINE. */
  start = i;
  i = strlen (line);
  if (i)
    {
      i--;
      while ((i > start) && (whitespace (line[i]))) i--;

      if (!whitespace (line[i]))
	line[i + 1] = '\0';
      else
	line[i] = '\0';
    }

  if (!command)
    result =  make_error ("%s: Undefined MDB command.  Try `help'.", name);
  else
    {
      result = (*(command->handler)) (name, line + start);
      if (mdb_allow_redo)
	MDB_LastCommand = command->handler;
      else
	MDB_LastCommand = (MFunction *)NULL;
    }

  free (name);
  return (result);
}

char *
mdb_redo (void)
{
  char *result = (char *)NULL;

  if (MDB_LastCommand)
    result = (*MDB_LastCommand) ("redo", "");

  return (result);
}

static char **
mdb_split_into_words (char *line)
{
  char **words = (char **)NULL;
  int words_slots = 0;
  int words_index = 0;

  if (line != (char *)NULL)
    {
      register int done = 0;
      register int start = 0;

      while (!done)
	{
	  register int i;
	  char *word;

	  while (whitespace (line[start])) start++;

	  if (line[start] == '\0')
	    break;

	  for (i = start; (line[i] != '\0') && !whitespace (line[i]); i++);
	  word = (char *)xmalloc (1 + (i - start));
	  strncpy (word, line + start, i - start);
	  word[i - start] = '\0';

	  start = i;

	  if (words_index + 2 > words_slots)
	    words = (char **)xrealloc
	      (words, (words_slots += 5) * sizeof (char *));

	  words[words_index++] = word;
	  words[words_index] = (char *)NULL;
	}
    }
  return (words);
}

static char *
mdb_help (MDBArgs)
{
  register int i;
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *result;
  char **words = mdb_split_into_words (line);

  if (words != (char **)NULL)
    {
      register int windex;

      for (windex = 0; words[windex] != (char *)NULL; windex++)
	{
	  MDBCommand *command = find_command (words[windex]);

	  if (command != (MDBCommand *)NULL)
	    {
	      bprintf (buffer, "%25s    %s\n",
		       command->invocation, command->description);
	    }
	  else
	    {
	      BPRINTF_BUFFER *temp = bprintf_create_buffer ();
	      char *fundoc;

	      bprintf (temp, "<mdb::describe-function %s>", words[windex]);
	      fundoc = mhtml_evaluate_string (temp->buffer);

	      bprintf_free_buffer (temp);

	      if (!empty_string_p (fundoc))
		bprintf (buffer, "%s", fundoc);
	      else
		{
		  xfree (fundoc);
		  temp = bprintf_create_buffer ();
		  bprintf (temp, "<mdb::function-documentation %s>",
			   words[windex]);
		  fundoc = mhtml_evaluate_string (temp->buffer);
		  bprintf_free_buffer (temp);

		  if (!empty_string_p (fundoc))
		    bprintf (buffer, "VARIABLE: %s\n%s",
			     words[windex], fundoc);
		  else
		    bprintf (buffer,
			     "%s: No such command, function or variable.\n",
			     words[windex]);
		}

	      xfree (fundoc);
	    }
	  free (words[windex]);
	}
      free (words);
    }
  else
    {
      for (i = 0; mdb_command_table[i].name; i++)
	{
	  bprintf (buffer, "%25s    %s\n", mdb_command_table[i].invocation,
		   mdb_command_table[i].description);
	}

      bprintf (buffer, "  Or, type the name of a function or macro.\n");
    }

  result = buffer->buffer;
  free (buffer);

  return (result);
}

extern void add_history (char *line);
static void mdb_parser_interrupt_callback (void);
static void
parser_interrupt (int sig)
{
  mhtml_parser_interrupted++;
}

static char *
mdb_eval (MDBArgs)
{
  PAGE *page = page_create_page ();
  char *result = (char *)NULL;
  int syntax_complete = 0;

  CANNOT_REDO;

  page_debug_clear ();
  page_syserr_clear ();
  bprintf (page, "%s\n", line);
  syntax_complete = page_check_syntax (page);

  while (!syntax_complete)
    {
      line = readline ("   ");

      if (line == (char *)NULL)
	{
	  syntax_complete = 1;
	  page_free_page (page);
	  page = (PAGE *)NULL;
	}
      else
	{
	  bprintf (page, "%s\n", line);
	  add_history (line);
	  free (line);
	  syntax_complete = page_check_syntax (page);
	}
    }

  if ((page != (PAGE *)NULL) && (page->buffer != (char *)NULL))
    {
      page_debug_clear ();
      page_syserr_clear ();
      mhtml_parser_callback_function = mdb_parser_interrupt_callback;
      signal (SIGINT, parser_interrupt);
      page_process_page (page);
      signal (SIGINT, mdb_signal_restart);
      mhtml_parser_callback_function = NULL;

      if (page)
	{
	  result = page->buffer;
	  free (page);
	}
    }

  if (page_debug_buffer ())
    {
      register int i;
      char *x = page_debug_buffer ();

      for (i = strlen (x) - 1; i > 0; i--)
	if (x[i] == '\n')
	  x[i] = '\0';
	else
	  break;

      fprintf (stderr, "[DEBUGGING-OUTPUT\n%s]\n", x);
      page_debug_clear ();
    }

  if (page_syserr_buffer ())
    {
      register int i;
      char *x = page_syserr_buffer ();

      for (i = strlen (x) - 1; i > 0; i--)
	if (x[i] == '\n')
	  x[i] = '\0';
	else
	  break;

      fprintf (stderr, "[SYSTEM-ERROR-OUTPUT\n%s]\n", x);
      page_syserr_clear ();
    }

  return (result);
}

static char *
mdb_print (MDBArgs)
{
  PAGE *page = page_create_page ();
  char *result = (char *)NULL;
  register int i;

  CANNOT_REDO;

  page_debug_clear ();
  page_syserr_clear ();

  for (i = 0; line[i] && line[i] != '<'; i++);
  if (line[i])
    bprintf (page, "%s", line);
  else
    bprintf (page, "<get-var %s>\n", line);

  page_debug_clear ();
  page_syserr_clear ();
  page_process_page (page);

  if (page)
    {
      result = page->buffer;
      free (page);
    }

  if (page_debug_buffer ())
    {
      fprintf (stderr, "[DEBUGGING-OUTPUT]: %s\n", page_debug_buffer ());
      page_debug_clear ();
    }

  if (page_syserr_buffer ())
    {
      fprintf (stderr, "[SYSTEM-ERROR-OUTPUT]: %s\n", page_syserr_buffer ());
      page_syserr_clear ();
    }

  return (result);
}

static char *
mdb_quit (MDBArgs)
{
  char *answer = "y";

  CANNOT_REDO;

  if (mdb_loop_level > 1)
    {
      int done = 0;

      while (!done)
	{
	  answer =
	    readline ("The program is running.  Quit anyway? (y or n) ");

	  if (answer &&
	      (strcasecmp (answer, "yes") != 0) &&
	      (strcasecmp (answer, "no") != 0) &&
	      (strcasecmp (answer, "y") != 0) &&
	      (strcasecmp (answer, "n") != 0))
	    printf ("\rPlease answer \"yes\" or \"no\".\n");
	  else
	    done = 1;
	}
    }

  if (!answer || (strncasecmp (answer, "y", 1) == 0))
    {
      MDB_QuitFlag = 1;
      /* mdb_throw_to_top_level (); */
    }

  return ((char *)NULL);
}

static char *
mdb_exec (MDBArgs)
{
  char *filename;
  int line_number, defaulted;
  MDB_File *file;
  char *result = (char *)NULL;
  PAGE *page = (PAGE *)NULL;
  static char *last_executed_filename = (char *)NULL;

  CANNOT_REDO;

  parse_file_and_line (line, &filename, &line_number, &defaulted);

  if (defaulted)
    {
      if (!last_executed_filename)
	last_executed_filename = strdup (filename ? filename : "");
      else
	{
	  free (filename);
	  filename = strdup (last_executed_filename);
	}
    }
  else
    {
      if (last_executed_filename)
	free (last_executed_filename);
      last_executed_filename = strdup (filename ? filename : "");
    }

  file = mdb_find_file (filename);

  if (!file)
    result = make_error ("Couldn't find `%s' for `%s'", filename, name);
  else
    {
      printf ("\rRunning %s...\n", file->filename);
      mdb_set_current_file (file->filename);
      mdb_recent_line_number = 1;

      if (mdb_recent_file)
	free (mdb_recent_file);

      mdb_recent_file = strdup (file->filename);

      page = page_copy_page (file->contents);

      mdb_insert_breakpoints (file, page, mdb_breakpoint_list ());
      page_process_page (page);

      if (page)
	{
	  result = page->buffer;
	  free (page);
	}
    }

  return (result);
}

static char *
mdb_load (MDBArgs)
{
  char *filename;
  int line_number, defaulted;
  MDB_File *file;
  char *result = (char *)NULL;

  CANNOT_REDO;

  parse_file_and_line (line, &filename, &line_number, &defaulted);
  file = mdb_find_file (filename);
  free (filename);

  if (!file)
    result = make_error ("Couldn't find `%s' for `%s'", line, name);
  else
    {
      mdb_set_current_file (file->filename);
      mdb_recent_line_number = 1;
      result = make_message ("Loaded `%s'", file->filename);
    }

  return (result);
}

static void
parse_file_and_line (char *string, char **file, int *line, int *defaulted_p)
{
  register int i;
  int digits_only = 1;
  int defaulted = 1;

  *file = (char *)NULL;
  *line = -1;

  if (string && *string)
    {
      char *colon = strchr (string, ':');
      char *parseloc = colon ? colon + 1 : string;

      if ((colon != (char *)NULL) && (*parseloc == ':'))
	{
	  /* This isn't a filename, it's a variable name with a package
	     qualifier!  Don't pretend that it is something different. */
	  colon = (char *)NULL;
	  parseloc = string;
	}

      /* Check to see if there are only digits here. */
      for (i = 0; parseloc[i]; i++)
	{
	  if (!isdigit (parseloc[i]))
	    {
	      digits_only = 0;
	      break;
	    }
	}

      if (digits_only)
	*line = atoi (parseloc);

      if (colon || !digits_only)
	{
	  if (!colon)
	    colon = string + strlen (string);

	  *file = (char *)xmalloc (1 + (colon - string));
	  strncpy (*file, string, (colon - string));
	  (*file)[colon - string] = '\0';
	}
    }

  /* If no file supplied, then use the old one. */
  if (!*file)
    *file = mdb_recent_file ? strdup (mdb_recent_file) : (char *)NULL;
  else
    defaulted = 0;

  /* If no line number, then use the old one. */
  if (*line < 0)
    *line = mdb_recent_line_number;
  else if (defaulted)
    defaulted = 0;

  *defaulted_p = defaulted;
}

int
mdb_count_lines (char *string)
{
  register int i;
  int lines = 0;

  if (string && *string)
    {
      lines++;

      for (i = 0; string[i]; i++)
	if (string[i] == '\n')
	  lines++;
    }
  return (lines);
}

static char *
make_listing (MDB_File *file, int line_number, int arrow_line)
{
  register int i;
  int counter = 1, limit;
  char *content = file->contents->buffer;
  BPRINTF_BUFFER *listing = bprintf_create_buffer ();
  char *result;

  limit = mdb_count_lines (content);

  if (line_number > limit)
    line_number = limit - 10;
  else
    line_number -= 10;

  if (line_number < 1)
    line_number = 1;

  /* Okay, count newlines until we are at the right number. */
  for (i = 0; i < file->contents->bindex; i++)
    {
      if (counter == line_number)
	break;

      if (content[i] == '\n')
	counter++;
    }

  /* Okay, ready to produce the listing. */
  limit = counter + 20;

  for (; counter < limit; counter++)
    {
      /* If this is the arrow line, print it now. */
      bprintf (listing, "%s%4d     ",
	       counter == arrow_line ? "->" : "  ", counter);

      /* Print one line. */
      for (; i < file->contents->bindex && content[i] != '\n'; i++)
	bprintf (listing, "%c", content[i]);

      i++;
      bprintf (listing, "\n");
      if (i >= file->contents->bindex)
	break;
    }

  result = listing->buffer;
  free (listing);

  return (result);
}

static char *
mdb_list (MDBArgs)
{
  int line_number, defaulted;
  int arrow_line = -1;
  char *filename;
  MDB_File *file;

  parse_file_and_line (line, &filename, &line_number, &defaulted);

  /* If there was no recent file, and we defaulted everything,
     there is nothing to list. */
  if (!mdb_recent_file && defaulted)
    return (make_error ("No source file specified.  Use `load'"));

  /* If the file and line were defaulted, and the last command was
     a LIST, then increase the line number now. */
  if (defaulted && (MDB_LastCommand == mdb_list))
    {
      line_number += 20;
      mdb_recent_line_number = line_number;
    }

  /* Find the file that the user specified. */
  file = mdb_find_file (filename);

  if (!file)
    {
      /* Try to find the function that the user specified. */
      char *fname = filename;
      char *found = (char *)NULL;
      find_file_and_line_for_function (fname, &found, &line_number);

      if (found)
	{
	  file = mdb_find_file (found);
	  /* Make this function's definition line be at the top of the
	     listing.  This assumes that we know how many lines appear in
	     a listing. */
	  line_number += 9;
	}
    }

  if (!file)
    return (make_error ("Cannot find `%s' for `%s'", filename, name));

  if (mdb_recent_file != filename)
    {
      mdb_recent_line_number = line_number;
      mdb_set_current_file (file->filename);
    }

  if ((mdb_loop_level > 1) && (mdb_current_bp))
    arrow_line = mdb_current_bp->line_number;

  return (make_listing (file, line_number, arrow_line));
}


static char *
mdb_disasm (MDBArgs)
{
  /* Try to find the function that the user specified. */
  int line_number = 0, defaulted = 0;
  char *funcname = (char *)NULL;
  char *listing = (char *)NULL;
  BPRINTF_BUFFER *command = bprintf_create_buffer ();

  parse_file_and_line (line, &funcname, &line_number, &defaulted);

  bprintf (command, "<compiler::disassemble %s>", funcname ? funcname : "");
  listing = mhtml_evaluate_string (command->buffer);
  bprintf_free_buffer (command);

  return (listing);
}

static char *
mdb_compile (MDBArgs)
{
  /* Try to find the function that the user specified. */
  int line_number = 0, defaulted = 0;
  char *funcname = (char *)NULL;
  char *listing = (char *)NULL;
  BPRINTF_BUFFER *command = bprintf_create_buffer ();

  parse_file_and_line (line, &funcname, &line_number, &defaulted);

  bprintf (command, "<compiler::compile-function %s>",
	   funcname ? funcname : "");
  listing = mhtml_evaluate_string (command->buffer);
  bprintf_free_buffer (command);

  return (listing);
}

static char *
mdb_file_info (void)
{
  BPRINTF_BUFFER *listing = bprintf_create_buffer ();
  char *result;

  if (mdb_files_index == 0)
    {
      bprintf (listing, "There are no loaded files.");
    }
  else
    {
      register int i;

      bprintf (listing, " Lines Bpt's  File\n");

      for (i = 0; i < mdb_files_index; i++)
	{
	  MDB_File *file = mdb_files[i];
	  bprintf (listing, "%5d  %3d    %s\n",
		   mdb_count_lines (file->contents->buffer),
		   mdb_count_breakpoints (file),
		   file->filename);
	}
    }

  result = listing->buffer;
  free (listing);

  return (result);
}

static char *
strstrcase (char *haystack, char *needle)
{
  char *result = (char *)NULL;

  if ((haystack != (char *)NULL) && (needle != (char *)NULL))
    {
      register int i;
      int hl = strlen (haystack);
      int nl = strlen (needle);

      for (i = 0; i < (hl - nl); i++)
	if (strncasecmp (haystack + i, needle, nl) == 0)
	  {
	    result = haystack + i;
	    break;
	  }
    }

  return (result);
}

static char *
mdb_info_funs (char *text)
{
  register int i;
  char *result;
  BPRINTF_BUFFER *output = bprintf_create_buffer ();
  MDB_File *last_file = (MDB_File *)NULL;

  bprintf (output, "Functions loaded with `%s' in their names:\n", text);

  for (i = 0; i < snarfed_index; i++)
    {
      if (strstrcase (snarfed_functions[i]->fname, text) == 0)
	{
	  if (snarfed_functions[i]->file != last_file)
	    {
	      last_file = snarfed_functions[i]->file;
	      bprintf (output, "In file %s:\n",
		       last_file->filename ? last_file->filename : "(prim)");
	    }
	  bprintf (output, "    %s\n", snarfed_functions[i]->fname);
	}
    }

  result = output->buffer;
  free (output);
  return (result);
}

static char *
mdb_info (MDBArgs)
{
  char *result;

  if ((strncasecmp (line, "fi", 2) == 0) || (!*line))
    result = mdb_file_info ();
  else if (strncasecmp (line, "b", 1) == 0)
    result = mdb_breakpoint_info ();
  else if (strncasecmp (line, "fu", 2) == 0)
    {
      register int i = 0;
      while ((line[i] != '\0') && (!whitespace (line[i]))) i++;
      while (whitespace (line[i])) i++;
      result = mdb_info_funs (line + i);
    }
  else
    result = make_error ("Unrecognized argument to %s, `%s'.", name, line);

  return (result);
}

/* The honest to goodness workhorse function declaration. */
static void pf_break_handler (PFunArgs);
static void pf_include_handler (PFunArgs);
static void pf_rep_handler (PFunArgs);

static PFunDesc MDB_BreakPFunDesc = { "*MDB*::BREAK", 0, 0, pf_break_handler };
static PFunDesc MDB_IncludePFunDesc = { "INCLUDE", 0, 0, pf_include_handler };
static PFunDesc MDB_UserStopPFunDesc = { "MDB::REP", 0, 0, pf_rep_handler };
extern char *get_positional_arg (Package *package, int position);

/* The <MDB::REP> function simply gives you a loop to work with. */
static void
pf_rep_handler (PFunArgs)
{
  mdb_loop ();
}

/* The BREAK function looks like "<*mdb*::break which existing-statements>". */
static void
pf_break_handler (PFunArgs)
{
  int which = atoi (get_positional_arg (vars, 0));
  MDB_Breakpoint *bp = mdb_this_breakpoint (which);
  MDB_Breakpoint *previous = mdb_current_bp;

  mdb_current_bp = bp;

  /* Before we do anything else, put back the remainder of the expression to
     be evaluated.  */
  bp->position = start;
  bp->code = page;

  if (body && body->buffer)
    {
      register int i;

      /* Skip leading whitespace. */
      for (i = 0; whitespace (body->buffer[i]); i++);

      /* Skip the digits which identify this breakpoint. */
      if (body->buffer[i] == '-') i++;

      while (isdigit (body->buffer[i])) i++;

      /* Skip the single trailing space. */
      i++;

      /* Insert the remainder. */
      bprintf_insert (page, start, "%s", body->buffer + i);
    }

  if (MDB_ContFlag)
    {
      mdb_current_bp = previous;
      MDB_ContFlag--;
      return;
    }

  if (bp->type != break_DELETED)
    {
      char *listing;

      mdb_set_current_file (bp->file->filename);

      if (bp->type != break_INTERNAL)
	{
	  printf ("\n*Bpt %d in %s at line %d\n", which + 1,
		  bp->file->filename, bp->line_number);

	  listing =
	    make_listing (mdb_find_file (bp->file->filename),
			  bp->line_number, bp->line_number);
	  printf ("%s", listing);
	  free (listing);
	}
      /* else */
	{
	  /* Print the remainder of the breakpoint here. */
	  PAGE *source = bp->file->contents;
	  int pos = mdb_position_of_line (source->buffer, bp->line_number);

	  printf ("\r%5d  ", bp->line_number);

	  while ((pos > 0) && (pos < source->bindex))
	    {
	      if (source->buffer[pos] == '\n')
		break;
	      printf ("%c", source->buffer[pos++]);
	    }
	  printf ("\n");
	}

      mdb_loop ();

      mdb_current_bp = previous;

      /* If the user has quit, under NO CIRCUMSTANCES should we
	 continue to execute the code. */
      if (MDB_QuitFlag)
	mdb_throw_to_top_level ();
    }
}

/* Continue execution from a breakpoint. */
static char *
mdb_cont (MDBArgs)
{
  int count = atoi ((line && *line) ? line : "1");
  char *result = (char *)NULL;

  if (mdb_loop_level > 1)
    {
      MDB_ContFlag += count;
      result = strdup ("Continuing...");
    }

  return (result);
}

/* Do the next instruction. */
static char *
mdb_next (MDBArgs)
{
  int count = atoi ((line && *line) ? line : "1");
  char *result = (char *)NULL;

  if (mdb_loop_level > 1)
    {
      if (!mdb_current_bp)
	result = make_message ("Can't do NEXT from this type of breakpoint!");
      else
	{
	  mdb_set_next_breakpoint (mdb_current_bp);
	  MDB_ContFlag += count;
	}
    }
  return (result);
}

/* Step into the running expression. */
static char *
mdb_step (MDBArgs)
{
  int count = atoi ((line && *line) ? line : "1");
  char *result = (char *)NULL;

  if (mdb_loop_level > 1)
    {
      if (!mdb_current_bp)
	result = make_message ("Can't do STEP from this type of breakpoint!");
      else
	{
	  mdb_set_step_breakpoint (mdb_current_bp);
	  MDB_ContFlag += count;
	}
    }
  return (result);
}

/* Where are we in the stack? */
static char *
mdb_where (MDBArgs)
{
  char **stack = mdb_pushed_function_names ();
  char *result = (char *)NULL;

  if ((stack == (char **)NULL) || (stack[0] == (char *)NULL))
    result = make_message ("Not running any function!");
  else
    {
      register int i;
      BPRINTF_BUFFER *result_buff = bprintf_create_buffer ();

      for (i = 0; stack[i] != (char *)NULL; i++)
	{
	  /* We don't have to print the last element, since it is
	     always *MDB-Break*. */
	  if (stack[i + 1] != (char *)NULL)
	    bprintf (result_buff, "#%03d  %s\n", i, stack[i]);
	}

      result = result_buff->buffer;
      free (result_buff);
    }
  return (result);
}
      
/* Remove a breakpoint. */
static char *
mdb_delete (MDBArgs)
{
  register int i = 0;
  int which = atoi (line) - 1;
  MDB_Breakpoint **bps = mdb_breakpoint_list ();
  char *result = (char *)NULL;

  if (bps)
    for (i = 0; bps[i]; i++);

  if ((which < 0) || (which >= i) || (bps[which]->type == break_DELETED))
    result = make_error ("There is no breakpoint `%s'", line);
  else
    {
      if ((bps[which]->type == break_CALLBACK) && (bps[which]->fname))
	{
	  char *fname = bps[which]->fname;
	  UserFunction *uf = mhtml_find_user_function (fname);
	  PFunDesc *desc = pagefunc_get_descriptor (fname);

	  /* Turn off debugging on this function. */
	  if (uf)
	    uf->debug_level = 0;

	  if (desc)
	    desc->debug_level = 0;
	}
      bps[which]->type = break_DELETED;
      result = make_message ("Deleted breakpoint %d", which + 1);
    }

  return (result);
}

static void
initialize_page_handlers (void)
{
  Symbol *sym;
  PAGE *page = page_create_page ();

  bprintf (page, "<defsubst *mdb-prog*>%%body</defsubst>\n");
  page_process_page (page);
  page_free_page (page);

  sym = symbol_intern_in_package 
    (mhtml_function_package, MDB_BreakPFunDesc.tag);
  sym->values = (char **)&MDB_BreakPFunDesc;
  sym->type = symtype_FUNCTION;

  sym = symbol_intern_in_package
    (mhtml_function_package, MDB_IncludePFunDesc.tag);
  sym->values = (char **)&MDB_IncludePFunDesc;
  sym->type = symtype_FUNCTION;

  sym = symbol_intern_in_package
    (mhtml_function_package, MDB_UserStopPFunDesc.tag);
  sym->values = (char **)&MDB_UserStopPFunDesc;
  sym->type = symtype_FUNCTION;
}

/* Find the file and line-number of the function FUNCTION if possible. */
static void
find_file_and_line_for_function (char *function, char **filename, int *line)
{
  register int i;
  MDB_FunctionDef *fdef = (MDB_FunctionDef *)NULL;

  for (i = 0; i < snarfed_index; i++)
    {
      if (strcasecmp (snarfed_functions[i]->fname, function) == 0)
	{
	  fdef = snarfed_functions[i];
	  break;
	}
    }

  if (fdef)
    {
      *filename = fdef->file->filename;
      *line = fdef->lineno;
    }
}

static int
mdb_callback (PFunArgs, PFunDesc *desc, UserFunction *uf, char *open_body)
{
  char *ftype;
  char *arg_string;
  int which_breakpoint = -1;
  /* MDB_Breakpoint *bp = (MDB_Breakpoint *)NULL; */

  if (uf != (UserFunction *)NULL)
    {
      fname = uf->name;
      switch (uf->type)
	{
	case user_MACRO:
	  ftype = "defmacro";
	  if (uf->flags & user_WEAK_MACRO)
	    ftype = "defweakmacro";
	  break;
	case user_SUBST: ftype = "defsubst"; break;
	case user_DEFUN: ftype = "defun"; break;
	default: ftype = "BADCODE (1)";
	}
    }
  else
    {
      fname = desc->tag;
      switch (desc->complexp)
	{
	case 0:  ftype = "internal defun"; break;
	case 1:  ftype = "internal defmacro"; break;
	case -1: ftype = "internal defweakmacro"; break;
	default: ftype = "internal BADCODE (2)";
	}
    }

  which_breakpoint = mdb_find_breakpoint_function (fname);

  arg_string = mhtml_funargs (vars);
  printf ("\n*Bpt %d in %s %s\n", which_breakpoint + 1, ftype, fname);
  printf ("* <%s %s>\n", fname, arg_string);
  free (arg_string);

  mdb_loop ();

  /* If the user has quit, under NO CIRCUMSTANCES should we
     continue to execute the code. */
  if (MDB_QuitFlag)
    mdb_throw_to_top_level ();

  return (1);
}

static void
mdb_parser_interrupt_callback (void)
{
  printf ("Interrupt!\n");
  mdb_loop ();

  /* If the user has quit, under NO CIRCUMSTANCES should we
     continue to execute the code. */
  if (MDB_QuitFlag)
    mdb_throw_to_top_level ();
}

static char *
mdb_breakpoint (MDBArgs)
{
  int line_number, defaulted;
  char *filename = (char *)NULL;
  char *result = (char *)NULL;
  int callback_break = 0;
  char *fname = (char *)NULL;

  CANNOT_REDO;

  parse_file_and_line (line, &filename, &line_number, &defaulted);

  if (!*line || !defaulted || (line_number < 1))
    {
      if (filename != (char *)NULL)
	find_file_and_line_for_function (filename, &filename, &line_number);
    }

  if (!*line || !defaulted || (line_number < 1))
    {
      if (filename != (char *)NULL)
	{
	  /* Try to set a breakpoint on a function which isn't currently
	     loaded in any file.  Only can do this if the function is
	     defined already. */
	  UserFunction *uf = mhtml_find_user_function (filename);
	  PFunDesc *desc = pagefunc_get_descriptor (filename);

	  if ((uf != (UserFunction *)NULL) || (desc != (PFunDesc *)NULL))
	    {
	      /* We can set a callback breakpoint on this function. */
	      if (uf != (UserFunction *)NULL)
		uf->debug_level = -2;
	      else
		desc->debug_level = -1;

	      fname = strdup (filename);
	      free (filename);
	      filename = (char *)xmalloc (20 + strlen (filename));
	      sprintf (filename, "#<internal-file: %s>", fname);

	      line_number = 0;
	      callback_break = 1;
	    }
	}
      else
	return
	  (make_error
	   ("`%s' requires a file name, function name, or line number", name));
    }

  if (callback_break)
    {
      mdb_add_breakpoint ((MDB_File *)NULL, &line_number, break_CALLBACK,
			  fname);
      result = make_message ("Break in function %s.", fname);
      mhtml_debug_callback_function = mdb_callback;
    }
  else
    {
      MDB_File *file = mdb_find_file (filename);

      if (!file)
	result = make_error ("Cannot load `%s' for `%s'", name, filename);
      else
	{
	  mdb_add_breakpoint (file, &line_number, break_USER, (char *)NULL);
	  result = make_message ("Break at line %05d, in file %s.",
				 line_number, file->nameonly);
	}
    }
  return (result);
}

static char *
mdb_apropos (MDBArgs)
{
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *result;

  bprintf (buffer, "Symbols matching `%s'\n", line ? line : "");

  if (line != (char *)NULL)
    {
      regex_t re;
      regmatch_t offsets[2];

      regcomp (&re, line, REG_EXTENDED | REG_ICASE);

      if (AllPackages)
	{
	  register int pi;
	  Package *pack;

	  for (pi = 0; (pack = AllPackages[pi]) != (Package *)NULL; pi++)
	    {
	      /* Operate on global packages only. */
	      if (pack->name != (char *)NULL)
		{
		  register int i;
		  Symbol **syms = symbols_of_package (pack);

		  for (i = 0; (syms && syms[i]); i++)
		    {
		      char *symname = syms[i]->name;

		      if (regexec (&re, symname, 1, offsets, 0) == 0)
			{
			  if (pack == mhtml_function_package)
			    bprintf (buffer, "Primitive: <%s>\n", symname);
			  else if (pack == mhtml_user_keywords)
			    bprintf (buffer, " Function: <%s>\n", symname);
			  else
			    bprintf (buffer, " Variable: %s::%s\n",
				     pack->name, symname);
			}
		    }
		}
	    }
	}
      regfree (&re);
    }
  result = buffer->buffer;
  free (buffer);
  return (result);
}

#if defined (MDB_EDITOR_COMMAND)
static char *
mdb_edit (MDBArgs)
{
  int line_number, defaulted;
  char *filename;
  char *result = (char *)NULL;
}
#endif

static int include_recursive_calls = 0;

static void
pf_include_handler (PFunArgs)
{
  int verbatim_p = var_present_p (vars, "VERBATIM");

  include_recursive_calls++;
  if (include_recursive_calls < MHTML_INCLUDE_RECURSION_LIMIT)
    {
      char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
      char *alt = mhtml_evaluate_string
	(get_one_of (vars, "ALT", "ALTERNATE", (char *)0));
      char *canonicalized_name = (char *)NULL;
      char *incpref = pagefunc_get_variable ("%%::incpref");
      char *relpref = pagefunc_get_variable ("%%::relpref");
      char *wr = (char *)NULL;

      if (!incpref) incpref = pagefunc_get_variable ("mhtml::include-prefix");
      if (!relpref) relpref = pagefunc_get_variable ("mhtml::relative-prefix");

      if (arg != (char *)NULL)
	canonicalized_name =
	  mhtml_canonicalize_file_name (arg, incpref, relpref, &wr);

      if (canonicalized_name != (char *)NULL)
	{
	  PAGE *file_contents = (PAGE *)NULL;
	  MDB_File *file;

	  /* Instead of simply reading the page, we load it into
	     MDB if it isn't already loaded.  That way, breakpoints
	     already set in that file are not deleted. */
	  file = mdb_find_file (canonicalized_name);

	  /* Did the user specify some alternate HTML if the file
	     couldn't be found? */
	  if (!file)
	    {
	      if (alt != (char *)NULL)
		{
		  verbatim_p = 0;
		  file_contents = page_create_page ();
		  bprintf (file_contents, "%s", alt);
		}
	    }
	  else
	    {
	      file_contents = page_copy_page (file->contents);
	      mdb_insert_breakpoints
		(file, file_contents, mdb_breakpoint_list ());
	    }

	  if (file_contents)
	    {
	      if (!verbatim_p)
		{
		  bprintf_insert (file_contents, 0, "<*parser*::push-file %s>",
				  wr);
		  bprintf (file_contents, "<*parser*::pop-file>");
		}

	      bprintf_insert_binary (page, start, file_contents->buffer,
				     file_contents->bindex);

	      if (verbatim_p)
		*newstart += file_contents->bindex;

	      page_free_page (file_contents);
	    }

	  free (canonicalized_name);
	}
    }
  include_recursive_calls--;
}

#include "readline/history.h"

/* Accessors for HIST_ENTRY lists that are called HLIST. */
#define histline(i) (hlist[(i)]->line)
#define histdata(i) (hlist[(i)]->data)
static char *
mdb_history (MDBArgs)
{
  register int i;
  int limited, limit;
  HIST_ENTRY **hlist;
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *result = (char *)NULL;

  if (line && *line)
    {
      limited = 1;
      limit = atoi (line);
      if (limit == 0) limit = 20;
    }
  else
    limited = limit = 0;

  hlist = history_list ();

  if (hlist)
    {
      for (i = 0;  hlist[i]; i++)
	;

      if (limit < 0)
	limit = -limit;

      if ((limited == 0)  || ((i -= limit) < 0))
	i = 0;

      while (hlist[i])
	{
	  bprintf (buffer, "%5d%c %s\n", i, histdata(i) ? '*' : ' ',
		   histline(i));
	  i++;
	}
    }

  if (buffer->bindex)
    {
      result = buffer->buffer;
      free (buffer);
    }
  else
    {
      result = (strdup ("No History\n"));
      bprintf_free_buffer (buffer);
    }

  return (result);
}

static char *
mdb_cd (MDBArgs)
{
  register int i;
  int changed = 0;
  char *result = (char *)NULL;
  char b[1024];

  if (line && *line)
    {
      int len = strlen (line);
      if ((line[len - 1] == '/') && (len > 1))
	{
	  while ((len > 1) && (line[len - 1] == '/'))
	    line[--len] = '\0';
	}

      changed = (chdir (line) == 0);
    }

  if (changed)
    {
      /* Try to get smart here.  If the include-prefix contains "/docs/...",
	 then the relative prefix is "/...", and the include-prefix goes up
	 to "/docs". */
      for (i = 0; line[i] != '\0'; i++)
	if (strncasecmp (line + i, "/docs", 5) == 0)
	  {
	    if ((line[i + 5] == '\0') || (line[i + 5] == '/'))
	      {
		char *incpref = (char *)xmalloc (1 + (i + 5));
		strncpy (incpref, line, i + 5);
		incpref[i + 5] = '\0';
		pagefunc_set_variable ("mhtml::include-prefix", incpref);
		pagefunc_set_variable ("mhtml::relative-prefix", line + i +5);
		sprintf (b, "Changed directory: [%s, %s]", incpref, line+i+5);
		result = strdup (b);
		free (incpref);
		break;
	      }
	  }

      if (!result)
	{
	  pagefunc_set_variable ("mhtml::include-prefix", line);
	  pagefunc_set_variable ("mhtml::relative-prefix", "");
	  sprintf (b, "Changed directory: [%s, %s]", line, "");
	  result = strdup (b);
	}
    }

  if (!result)
    {
      sprintf (b, "Cannot change to directory `%s'", line);
      result = strdup (b);
    }

  return (result);
}

static char *
mdb_pwd (MDBArgs)
{
  char *dir, buffer[1024];
  BPRINTF_BUFFER *b = bprintf_create_buffer ();

  dir = getcwd (buffer, sizeof (buffer));

  bprintf (b, "   Physical Dir: %s\n", dir);
  bprintf (b, " Include Prefix: %s\n",
	   pagefunc_get_variable ("mhtml::include-prefix"));
  bprintf (b, "Relative Prefix: %s\n",
	   pagefunc_get_variable ("mhtml::relative-prefix"));

  dir = b->buffer;
  free (b);
  return (dir);
}

#if defined (__cplusplus)
}
#endif
