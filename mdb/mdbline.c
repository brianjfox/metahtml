/* mdbline.c: MDB's interface to the readline library. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct  2 08:07:30 1995.  */

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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include "pages.h"
#include "session_data.h"
#include "parser.h"
#include "mdb.h"
#include "commands.h"
#include <readline/rlconf.h>
#include <readline/readline.h>
#include <readline/history.h>

#if defined (__cplusplus)
extern "C"
{
#endif


/* Helper functions for Readline. */
static char **attempt_mdb_completion (char *text, int start, int end);
static char *mdb_command_completion_function (char *text, int state);
static char *pagefunc_completion_function (char *text, int state);
static char *symbol_completion_function (char *text, int state);
static void mdb_function_args (void);
static int mdb_forward_sexp (int count, int key);
static int mdb_backward_sexp (int count, int key);
static int mdb_kill_sexp (int count, int key);
static int mdb_backward_kill_sexp (int count, int key);
static int mdb_document_fun (int count, int key);

/* Variables used here but defined in other files. */
extern int rl_filename_completion_desired;

/* Non-zero once mdb_initalize_readline () has been called. */
int mdb_readline_initialized = 0;


/* Called once from mdb.c. */
void
mdb_initialize_readline (void)
{
  if (mdb_readline_initialized)
    return;

  rl_terminal_name = getenv ("TERM");
  rl_instream = stdin;
  rl_outstream = stderr;
  rl_special_prefixes = "<";

  /* Allow conditional parsing of `~/.inputrc'. */
  rl_readline_name = "MDB";

  /* Tell the (completer that we want a crack first. */
  rl_attempted_completion_function = (CPPFunction *)attempt_mdb_completion;

  rl_completer_quote_characters = "'\"";

  /* Need to modify this from the default; `$', `{', `\', and ``' are not
     word break characters. */
  rl_completer_word_break_characters = " \t\n\"'@><=;|&(";

  rl_bind_key ('>', (Function *)rl_insert_close);
  rl_add_defun ("mdb-function-args", (Function *)mdb_function_args, -1);
  rl_bind_key_in_map (CTRL('A'), (Function *)mdb_function_args,
		      emacs_meta_keymap);
  rl_add_defun ("mdb-document-fun", (Function *)mdb_document_fun, -1);
  rl_bind_key_in_map (CTRL('D'), (Function *)mdb_document_fun,
		      emacs_meta_keymap);
  rl_add_defun ("mdb-kill-sexp", (Function *)mdb_kill_sexp, -1);
  rl_bind_key_in_map (CTRL('K'), (Function *)mdb_kill_sexp,
		      emacs_meta_keymap);
  rl_add_defun ("mdb-backward-kill-sexp", (Function *)mdb_backward_kill_sexp,
		-1);
  rl_add_defun ("mdb-foward-sexp", (Function *)mdb_forward_sexp, -1);
  rl_add_defun ("mdb-backward-sexp", (Function *)mdb_backward_sexp, -1);
  rl_bind_key_in_map (CTRL('F'), (Function *)mdb_forward_sexp,
		      emacs_meta_keymap);
  rl_bind_key_in_map (CTRL('B'), (Function *)mdb_backward_sexp,
		      emacs_meta_keymap);

  mdb_readline_initialized = 1;
}

static int
mdb_forward_sexp (int count, int key)
{
  if (count < 0)
    mdb_backward_sexp (-count, key);

  while (count > 0)
    {
      register int i = rl_point;
      int quoted = 0, depth = 0, done = 0;

      while (whitespace (rl_line_buffer[i])) i++;

      if (rl_line_buffer[i] == '"')
	{
	  quoted = 1;
	  i++;
	}

      while (!done)
	{
	  int c = rl_line_buffer[i];

	  switch (c)
	    {
	    case '\0':
	      done = 1;
	      continue;

	    case '"':
	      quoted = !quoted;
	      if (!quoted && !depth)
		done = 1;

	      break;

	    case '<':
	      if (!quoted) depth++;
	      break;

	    case '>':
	      if (!quoted) depth--;
	      if (depth < 0)
		{
		  done = 1;
		  continue;
		}
	      else if (depth == 0)
		done = 1;
	      break;

	    case '\t':
	    case ' ':
	    case '\r':
	    case '\n':
	      if (!quoted && !depth) { done = 1; continue; }
	      break;
	    }

	  i++;
	}

      rl_point = i;
      
      --count;
    }

  return (0);
}

static int
mdb_backward_sexp (int count, int key)
{
  if (count < 0)
    mdb_forward_sexp (-count, key);

  while (count > 0)
    {
      register int i = (rl_point - 1);
      int quoted = 0, depth = 0, done = 0;

      if (i < 0) break;

      while ((i != 0) && (whitespace (rl_line_buffer[i]))) i--;

      if (rl_line_buffer[i] == '"')
	{
	  quoted = 1;
	  i--;
	}

      while ((i > -1) && (!done))
	{
	  int c = rl_line_buffer[i];

	  switch (c)
	    {
	    case '"':
	      quoted = !quoted;
	      if (!quoted && !depth)
		{ done = 1; continue; }
	      break;

	    case '>':
	      if (!quoted) depth++;
	      break;

	    case '<':
	      if (!quoted)
		{
		  depth--;
		  if (depth < 1)
		    {
		      done = 1;
		      continue;
		    }
		}
	      break;

	    case '\t':
	    case ' ':
	    case '\r':
	    case '\n':
	      if (!quoted && !depth) { done = 1; continue; }
	      break;
	    }

	  i--;
	}

      if (i > -1)
	rl_point = i;
      
      --count;
    }

  return (0);
}

int
mdb_kill_sexp (int count, int key)
{
  int orig_point = rl_point;

  if (count < 0)
    return (mdb_backward_kill_sexp (-count, key));
  else
    {
      mdb_forward_sexp (count, key);

      if (rl_point != orig_point)
	rl_kill_text (orig_point, rl_point);

      rl_point = orig_point;
    }
  return 0;
}

int
mdb_backward_kill_sexp (int count, int ignore)
{
  int orig_point = rl_point;

  if (count < 0)
    return (mdb_kill_sexp (-count, ignore));
  else
    {
      mdb_backward_sexp (count, ignore);

      if (rl_point != orig_point)
	rl_kill_text (orig_point, rl_point);
    }
  return (0);
}

static char *containing_defun = (char *)NULL;
static int cd_size = 0;

static char *
mdb_containing_defun (char *buffer, int point)
{
  register int i;
  int start = 0;
  int quoted = 0;
  char *result = (char *)NULL;

  for (i = point; i > -1; i--)
    {
      char c = buffer[i];

      switch (c)
	{
	case '"':
	  quoted = !quoted;
	  continue;

	case '<':
	  if (!quoted && (buffer[i + 1] != '\0'))
	    {
	      i++;
	      start = i;
	      while (!whitespace (buffer[i])) i++;
	      if ((i - start) >= cd_size)
		containing_defun = (char *)xrealloc
		  (containing_defun, (cd_size += (2 * (i - start))));

	      strncpy (containing_defun, buffer + start, i - start);
	      containing_defun[i - start] = '\0';
	      result = containing_defun;
	      i = 0;
	      continue;
	    }
	  break;
	}
    }

  return (result);
}

static void
mdb_function_args (void)
{
  char *defun = mdb_containing_defun (rl_line_buffer, rl_point);
  BPRINTF_BUFFER *buffer = (BPRINTF_BUFFER *)NULL;

  if (defun != (char *)NULL)
    {
      char *result;

      buffer = bprintf_create_buffer ();
      bprintf (buffer, "<mdb::usage-string %s>", defun);

      result = mhtml_evaluate_string (buffer->buffer);
      bprintf_free_buffer (buffer);

      if (!empty_string_p (result))
	{
	  fprintf (stderr, "\r\n");
	  fprintf (stderr, "%s\n", result);
	  rl_on_new_line ();
	}
      else
	ding (0, 0);

      free (result);
    }
  else
    ding (0, 0);
}

static int
mdb_document_fun (int count, int key)
{
  char *defun = mdb_containing_defun (rl_line_buffer, rl_point);
  BPRINTF_BUFFER *buffer = (BPRINTF_BUFFER *)NULL;

  if (defun != (char *)NULL)
    {
      char *result;

      buffer = bprintf_create_buffer ();
      bprintf (buffer, "<mdb::describe-function %s>", defun);
      result = mhtml_evaluate_string (buffer->buffer);
      bprintf_free_buffer (buffer);

      if (empty_string_p (result))
	{
	  if (result) free (result);
	  result = strdup ("No documentation found.");
	  ding (0, 0);
	}

      fprintf (stderr, "\r\n");
      fprintf (stderr, "%s\n", result);
      free (result);

      rl_on_new_line ();
    }
  else
    ding (0, 0);

  return (0);
}

static void **
concat_arrays (void **first, void **second)
{
  if (second)
    {
      register int i1, i2;

      for (i1 = 0; first[i1]; i1++);
      for (i2 = 0; second[i2]; i2++);

      first = (void **)xrealloc (first, ((2 + i1 + i2) * sizeof (void *)));

      for (i2 = 0; second[i2]; i2++)
	first[i1++] = second[i2];

      first[i1] = (void *)NULL;
    }

  return (first);
}

/* Do some completion on TEXT.  The indices of TEXT in RL_LINE_BUFFER are
   at START and END.  Return an array of matches, or NULL if none. */
static char **
attempt_mdb_completion (char *text, int start, int end)
{
  char **matches = (char **)NULL;
  int in_command_position = 0;
  int tindex;

  /* Determine if this could be a command word.  It is if it appears at
     the start of the line disregarding preceding whitespace. */
  tindex = start - 1;

  while ((tindex > -1) && (whitespace (rl_line_buffer[tindex])))
    tindex--;

  if (tindex < 0)
    in_command_position++;

  /* If the word starts with '<' then we want to complete over the
     possible function calls in Meta-HTML. */
  if (*text == '<')
    matches = completion_matches (text, pagefunc_completion_function);
  else
    {
      /* If this word is in a command position, then complete over possible
	 MDB command names. */
      if (in_command_position)
	matches = completion_matches (text, mdb_command_completion_function);
      else
	{
	  /* Complete over both symbols and function names. */
	  char **symbol_matches;

	  symbol_matches = completion_matches
	    (text, symbol_completion_function);
	  matches = completion_matches (text, pagefunc_completion_function);
	  if (matches)
	    {
	      matches = (char **)concat_arrays
		((void **)matches, (void **)symbol_matches);
	      free (symbol_matches);
	    }
	  else
	    matches = symbol_matches;
	}
    }

  return (matches);
}

/* Find possible matches out of our command table. */
static char *
mdb_command_completion_function (char *hint_text, int state)
{
  static int local_index = 0;
  static char *hint = (char *)NULL;
  static int hint_len = 0;

  /* We have to map over the possibilities for command words.  If we have
     no state, then make one just for that purpose. */
  if (!state)
    {
      if (hint)
	free (hint);

      hint = strdup (hint_text);
      hint_len = strlen (hint);
      local_index = 0;
    }

  /* Try all of the command names. */
  while (mdb_command_table[local_index].name)
    {
      register char *command_name;

      command_name = mdb_command_table[local_index++].name;

      if (strncasecmp (command_name, hint, hint_len) == 0)
	return (strdup (command_name));
    }

  return ((char *)NULL);
}



/* Find possible matches for a Meta-HTML function call. */
static char *
pagefunc_completion_function (char *hint_text, int state)
{
  static int local_index = 0;
  static char *hint = (char *)NULL;
  static int hint_len = 0;
  static char *prefix = (char *)NULL;
  static Symbol **symbols = (Symbol **)NULL;

  if (!state)
    {
      if (symbols) { free (symbols); symbols = (Symbol **)NULL; }
      if (prefix)  { free (prefix); prefix = (char *)NULL; }
      if (hint) free (hint);
      hint = strdup (hint_text);
      hint_len = strlen (hint);
      local_index = 0;

      /* Skip any number of open braces and slash characters. */
      {
	register int i;

	for (i = 0; hint[i]; i++)
	  if ((hint[i] != '<') && (hint[i] != '/'))
	    break;

	if (i)
	  {
	    prefix = (char *)xmalloc (1 + i);
	    strncpy (prefix, hint, i);
	    prefix[i] = '\0';

	    memmove (hint, hint + i, hint_len - (i - 1));
	    hint_len -= i;
	  }
      }

      symbols = symbols_of_package (mhtml_function_package);
      if (mhtml_user_keywords)
	{
	  Symbol **uks = symbols_of_package (mhtml_user_keywords);
	  symbols = (Symbol **)
	    concat_arrays ((void **)symbols, (void **)uks);
	  if (uks) free (uks);
	}
    }

  while (symbols && symbols[local_index])
    {
      Symbol *sym = symbols[local_index++];

      if (sym && sym->name && (strncasecmp (sym->name, hint, hint_len) == 0))
	{
	  if (prefix)
	    {
	      char *result;

	      result = (char *)xmalloc (1 + strlen (prefix) + sym->name_len);
	      sprintf (result, "%s%s", prefix, sym->name);
	      return (result);
	    }
	  else
	    return (strdup (sym->name));
	}
    }

  return ((char *)NULL);
}

/* Find possible matches for a Meta-HTML symbol. */
static char *
symbol_completion_function (char *hint_text, int state)
{
  static int local_index = 0;
  static char *hint = (char *)NULL;
  static int hint_len = 0;
  static char *packname = (char *)NULL;
  static char **names = (char **)NULL;

  if (!state)
    {
      /* Check to see if we have seen a "::" yet.  If so, limit the
	 search to those symbols in that package.  Otherwise, the
	 possibilites are the symbols in the default package, and all
	 of the package names, including "default". */
      register int i;
      Symbol **symbols;
      char *dots = strstr (hint_text, "::");

      if (packname) { free (packname); packname = (char *)NULL; }

      if (dots)
	{
	  if (dots == hint_text)
	    packname = strdup ("default");
	  else
	    {
	      packname = (char *)xmalloc (dots - hint_text);
	      strncpy (packname, hint_text, (dots - hint_text));
	      packname[dots - hint_text] = '\0';
	    }
	}

      if (hint) { free (hint); hint = (char *)NULL; }
      if (names)
	{
	  for (i = 0; names[i]; i++)
	    free (names[i]);
	  free (names);
	  names = (char **)NULL;
	}
	
      hint = strdup (hint_text);
      hint_len = strlen (hint);
      local_index = 0;

      /* Get the symbol names that we need.  If there is a specific
	 package name, then get all of those names now. */
      if (packname)
	{
	  Package *package = symbol_lookup_package (packname);

	  /* If the package couldn't be found, there are no possible
	     completions here for that package... but there might be
	     completions for the *user-functions* package or for the
	     *meta-html* package.  Give up immediately. */
	  if (!package)
	    {
	      free (packname);
	      packname = (char *)NULL;
	      return (char *)NULL;
	    }

	  /* The package could be found.  Add all of these symbols to
	     our array. */
	  symbols = symbols_of_package (package);

	  for (i = 0; symbols && symbols[i]; i++);

	  names = (char **)xmalloc ((i + 2) * sizeof (char *));
	  for (i = 0; symbols && symbols[i]; i++)
	    {
	      char *addname = (char *)
		xmalloc (4 + package->name_len + symbols[i]->name_len);
	      sprintf (addname, "%s::%s", packname, symbols[i]->name);
	      names[i] = addname;
	    }

	  names[i] = (char *)NULL;
	  free (symbols);
	}
      else
	{
	  /* No particular package name was specified, so get all of the
	     packages, and all of the symbols in those packages. */
	  register int pi, count = 0;
	  char **temp;

	  for (pi = 0; AllPackages && AllPackages[pi]; pi++)
	    if (AllPackages[pi]->name_len)
	      count++;

	  names = (char **)xmalloc ((1 + count) * sizeof (char *));
	  count = 0;

	  for (pi = 0; AllPackages && AllPackages[pi]; pi++)
	    if (AllPackages[pi]->name_len)
	      names[count++] = strdup (AllPackages[pi]->name);

	  names[count] = (char *)NULL;

	  /* Now, loop over each package, adding their symbols to the
	     existing NAMES. */
	  for (pi = 0; AllPackages && AllPackages[pi]; pi++)
	    {
	      char *addname;
	      char *pname = AllPackages[pi]->name_len ?
		AllPackages[pi]->name : (char *)NULL;

	      symbols = symbols_of_package (AllPackages[pi]);
	      for (i = 0; symbols && symbols[i]; i++);
	      temp = (char **)xmalloc ((1 + i) * sizeof (char *));

	      for (i = 0; symbols && symbols[i]; i++)
		{
		  addname = (char *)xmalloc
		    (4 + AllPackages[pi]->name_len + symbols[i]->name_len);

		  if (pname)
		    sprintf (addname, "%s::%s", pname, symbols[i]->name);
		  else
		    sprintf (addname, "%s", symbols[i]->name);

		  temp[i] = addname;
		}
	      temp[i] = (char *)NULL;

	      names = (char **)concat_arrays ((void **)names, (void **)temp);
	    }
	}
    }

  while (names && names[local_index])
    {
      char *result = names[local_index++];

      if (strncasecmp (result, hint, hint_len) == 0)
	return (strdup (result));
    }

  return ((char *)NULL);
}


#if defined (__cplusplus)
}
#endif
