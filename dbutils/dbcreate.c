/* dbcreate.c: -*- C -*-  Create a GDB database from input file. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Thu Dec  7 13:41:15 1995.  */

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
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include <sys/types.h>
#include <time.h>

/* The name of this program, as taken from argv[0]. */
char *progname = (char *)NULL;

/* The name of the description file. */
static char *description_filename = (char *)NULL;

/* The name of the output database. */
static char *database_filename = (char *)NULL;

/* The list of fields that we expect to find in the file. */
static char **record_template = (char **)NULL;

/* The name of the field to use as key field. */
static char **key_fields = (char **)NULL;

/* Must this key have something appended to it which makes it unique? */
int key_unique_p = 0;

/* Does this key have :<pid> appended to it? */
int key_has_pid_p = 0;

/* The field separator character. */
static char FieldSep = ',';

/* The list of filenames to process. */
static char **files = (char **)NULL;
static int files_index = 0;
static int files_slots = 0;

typedef struct
{
  char *field;
  char *value;
} FORCED;

static FORCED **forced_fields = (FORCED **)NULL;
static int forced_fields_index = 0;
static int forced_fields_slots = 0;

static void parse_program_arguments (int argc, char *argv[]);
static void process_datafile (char *file);
static void usage (int exit_code);
static void process_description_file (char *filename);

#undef whitespace
#define whitespace(c) ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))

int
main (int argc, char *argv[])
{
  register int i;

  parse_program_arguments (argc, argv);
  process_description_file (description_filename);

  /* Now process the files. */
  for (i = 0; i < files_index; i++)
    process_datafile (files[i]);

  return (0);
}

static char *current_filename = (char *)NULL;
static int current_line_number = 0;

static void
line_error (char *format, ...)
{
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  va_list args;

  va_start (args, format);

  bprintf (buffer, "%s: %d: ",
	   current_filename ? current_filename : "BOGUS-FILE",
	   current_line_number);

  vbprintf (buffer, format, args);
  fprintf (stderr, "%s", buffer->buffer);
  bprintf_free_buffer (buffer);
}

static void
free_array (char **array)
{
  register int i;

  if (array)
    {
      for (i = 0; array[i]; i++)
	free (array[i]);

      free (array);
    }
}

static int
array_len (char **array)
{
  register int i = 0;

  if (array)
    while (array[i]) i++;

  return (i);
}

static char **
split_line (char *line, int at_character)
{
  register int start, end, i;
  char **fields = (char **)NULL;
  int fields_slots = 0;
  int fields_index = 0;

  /* Loop until done. */
  start = 0;

  while (line[start])
    {
      int quoted = 0;
      char *field;

      /* Skip leading whitespace. */
      for (i = start; whitespace (line[i]); i++);
      start = i;

      /* Get a single field, or finish. */
      if (!line[start])
	continue;

      if (line[start] == '"')
	{
	  quoted = 1;
	  start++;
	}

      end = 0;
      for (i = start; line[i]; i++)
	{
	  if (quoted && (line[i] == '\\'))
	    {
	      i++;
	    }
	  else if (quoted && (line[i] == '"'))
	    {
	      end = i;
	      i++;
	      while (whitespace (line[i])) i++;
	      if (line[i] == at_character)
		i++;
	      break;
	    }
	  else if (!quoted && (line[i] == at_character))
	    {
	      end = i;
	      i++;
	      break;
	    }
	}

      if (!end)
	end = i;

      field = (char *)xmalloc (1 + (end - start));
      strncpy (field, line + start, end - start);
      field[end - start] = '\0';
      start = i;

      if (fields_index + 2 > fields_slots)
	fields = (char **)
	  xrealloc (fields, (fields_slots += 10) * sizeof (char *));

      fields[fields_index++] = field;
      fields[fields_index] = (char *)NULL;
    }

  return (fields);
}

static void
dump_data (char **fields, FILE *stream)
{
  register int i;
  int num_fields = array_len (fields);
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  static int times_called = 0;
  static int offsets_calculated = 0;
  static int *key_indices = (int *)NULL;
  static int num_key_fields = 0;

  /* Get the key for this data. */
  if (!offsets_calculated)
    {
      register int j;

      offsets_calculated++;

      if (key_fields != (char **)NULL)
	{
	  int keys_matched = 0;

	  num_key_fields = array_len (key_fields);
	  key_indices = (int *)xmalloc
	    ((1 + array_len (key_fields)) * sizeof (int));

	  for (j = 0; key_fields[j] != (char *)NULL; j++)
	    {
	      for (i = 0; record_template[i] != (char *)NULL; i++)
		if (strcasecmp (record_template[i], key_fields[j]) == 0)
		  {
		    key_indices[j] = i;
		    keys_matched++;
		    break;
		  }
	      if (!record_template[i])
		break;
	    }

	  if (keys_matched != num_key_fields)
	    {
	      fprintf
		(stderr, "Warning: KeyField value (%s) not found in Template.",
		 key_fields[j]);
	      exit (2);
	    }
	}
      else
	{
	  fprintf (stderr,
		   "Warning: KeyField value not specified.  Using `%s'.\n",
		   record_template[0]);
	  num_key_fields = 1;
	  key_indices = (int *)xmalloc (2 * sizeof (int));
	  key_indices[0] = 0;
	  key_fields = (char **)xmalloc (2 * sizeof (char *));
	}
    }

  bprintf (buffer, "(\"");
  for (i = 0; i < num_key_fields; i++)
    {
      if (key_indices[i] >= num_fields)
	{
	  line_error ("Record missing required key field (%s) offset (%d)",
		      key_fields[i], key_indices[i]);
	  bprintf_free_buffer (buffer);
	  return;
	}
      else
	{
	  bprintf (buffer, "%s", fields[key_indices[i]]);
	}
    }

  times_called++;

  if (key_unique_p)
    {
      time_t ticks = (time_t)time ((time_t *)0);
      char *time_string = strdup ((char *)ctime (&ticks));

      time_string[24] = '\0'; 
      bprintf (buffer, "-%s-%d", time_string, times_called);
    }

  if (key_has_pid_p)
    {
      pid_t pid = getpid ();
      bprintf (buffer, ":%d", (int) pid);
    }

  bprintf (buffer, "\" (");

  for (i = 0; record_template[i]; i++)
    bprintf (buffer, "(\"%s\" . \"%s\")",
	     record_template[i], (i < num_fields) ? fields[i] : "");

  /* Now add any fields which were forced. */
  for (i = 0; i < forced_fields_index; i++)
    bprintf (buffer, "(\"%s\" . \"%s\")",
	     forced_fields[i]->field, forced_fields[i]->value);

  fprintf (stream, "%s))", buffer->buffer);
  bprintf_free_buffer (buffer);
}

static void
process_datafile (char *filename)
{
  register int i, counter = 0;
  char buffer[2048];
  char *line;
  FILE *input_stream, *output_stream;

  current_filename = filename;
  current_line_number = 0;
  input_stream = fopen (filename, "r");

  if (!input_stream)
    {
      fprintf (stderr, "Can't open the data input file `%s'.\n", filename);
      return;
    }

  output_stream = fopen (database_filename, "a+");

  if (!output_stream)
    {
      fprintf (stderr, "Can't open the data output file `%s'.\n",
	       database_filename);
      fclose (input_stream);
      return;
    }

  while ((line = fgets (buffer, sizeof (buffer) - 1, input_stream))
	 != (char *)NULL)
    {
      char **fields;

      current_line_number++;
      /* Okay, clean up the line. */
      for (i = strlen (line) - 1; i > -1; i--)
	{
	  if ((line[i] == '\n') || (line[i] == '\r'))
	    line[i] = '\0';
	  else
	    break;
	}

      fields = split_line (line, FieldSep);
      counter++;
      dump_data (fields, output_stream);
      free_array (fields);
    }

  fclose (input_stream);
  fclose (output_stream);
  fprintf (stderr, "Dumped %d records to %s\n", counter, database_filename);
}

static void
parse_program_arguments (int argc, char *argv[])
{
  register int i;
  char *arg;

  progname = argv[0];

  for (i = 1; (arg = argv[i]) != (char *)NULL; i++)
    {
      if (*arg != '-')
	{
	  if (files_index + 2 > files_slots)
	    files = (char **)
	      xrealloc (files, (files_slots += 10) * sizeof (char *));

	  files[files_index++] = strdup (arg);
	  files[files_index] = (char *)NULL;
	}
      else if (strcasecmp (arg, "--description") == 0)
	{
	  if (i + 1 < argc && !description_filename)
	    description_filename = argv[++i];
	  else
	    usage (1);
	}
      else
	usage (1);
    }
}

static void
usage (int exit_code)
{
  fprintf (stderr, "Usage: %s --description DESCFILE DATAFILE ...\n",
	   progname);

  if (exit_code)
    exit (exit_code);
}

typedef void COMMAND_FUNC (char *name, char **args);

typedef struct
{
  char *name;
  int required_args;
  COMMAND_FUNC *handler;
} COMMAND;

static void
handle_Database (char *name, char **args)
{
  database_filename = strdup (args[0]);
}

static void
handle_Template (char *name, char **args)
{
  register int i;
  int offset = array_len (record_template);
  int extra = array_len (args);

  record_template = (char **)
    xrealloc (record_template, (2 + extra + offset) * sizeof (char *));

  for (i = 0; args[i] != (char *)NULL; i++)
    {
      record_template[i + offset] = strdup (args[i]);
    }

  record_template[i + offset] = (char *)NULL;
}

static void
handle_KeyField (char *name, char **args)
{
  register int i;
  int offset = array_len (key_fields);
  int extra = array_len (args);

  key_fields = (char **)
    xrealloc (key_fields, (2 + extra + offset) * sizeof (char *));

  for (i = 0; args[i] != (char *)NULL; i++)
    key_fields[i + offset] = strdup (args[i]);

  key_fields[i + offset] = (char *)NULL;
}

static void
handle_KeyUnique (char *name, char **args)
{
  if (strcasecmp (args[0], "true") == 0)
    key_unique_p = 1;
  else
    key_unique_p = 0;
}

static void
handle_KeyHasPid (char *name, char **args)
{
  if (strcasecmp (args[0], "true") == 0)
    key_has_pid_p = 1;
  else
    key_has_pid_p = 0;
}

static void
handle_AddField (char *name, char **args)
{
  FORCED *new_force = (FORCED *)xmalloc (sizeof (FORCED));

  new_force->field = args[0];
  new_force->value = args[1];

  if (forced_fields_index + 2 > forced_fields_slots)
    forced_fields = (FORCED **) xrealloc
      (forced_fields, (forced_fields_slots += 10) * sizeof (FORCED *));

  forced_fields[forced_fields_index++] = new_force;
  forced_fields[forced_fields_index] = (FORCED *)NULL;
}
  
static void
handle_FieldSep (char *name, char **args)
{
  FieldSep = *args[0];
}

static COMMAND commands[] = {
  { "Database", 1, handle_Database },
  { "Template", 1, handle_Template },
  { "KeyField", 1, handle_KeyField },
  { "KeyUnique", 1, handle_KeyUnique },
  { "KeyHasPid", 1, handle_KeyHasPid },
  { "AddField", 2, handle_AddField },
  { "FieldSep", 1, handle_FieldSep },

  { (char *)NULL, 0, (COMMAND_FUNC *)NULL }
};

static void
handle_description_command (char *command, char **args)
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcasecmp (command, commands[i].name) == 0)
      {
	int args_len = array_len (args);

	if (commands[i].required_args > args_len)
	  {
	    fprintf (stderr, "Error: `%s' Requires at least %d args, got %d\n",
		     command, commands[i].required_args, args_len);
	    exit (1);
	  }

	(*commands[i].handler) (command, args);
	return;
      }

  fprintf (stderr, "Error: Unrecognized command (%s) in description file\n",
	   command);
  exit (1);
}

static void
process_description_file (char *filename)
{
  FILE *stream;
  char buffer [1024];
  char *line;

  if (!filename)
    usage (1);

  stream = fopen (filename, "r");

  if (!stream)
    {
      fprintf (stderr, "Can't open description file `%s'\n", filename);
      exit (1);
    }

  while ((line = fgets (buffer, sizeof (buffer) - 1, stream)) != (char *)NULL)
    {
      register int start, i;
      char *command = (char *)NULL;
      char **args = (char **)NULL;

      /* Kill trailing newline crap. */
      /* Okay, clean up the line. */
      for (i = strlen (line) - 1; i > -1; i--)
	{
	  if ((line[i] == '\n') || (line[i] == '\r'))
	    line[i] = '\0';
	  else
	    break;
	}

      /* Skip leading whitespace. */
      for (i = 0; whitespace (line[i]); i++);
      start = i;

      /* Check for comment. */
      if (line[i] == '#' || line[i] == ';')
	continue;

      /* Get command name. */
      for (i = start; line[i] != '\0' && line[i] != ':'; i++);

      command = (char *)xmalloc (1 + i - start);
      strncpy (command, line + start, i - start);
      command[i - start] = '\0';
      i++;

      args = split_line (line + i, ',');

      handle_description_command (command, args);
    }

  fclose (stream);
  if (!record_template)
    {
      fprintf (stderr, "You must supply a Template in `%s'!\n", filename);
      usage (1);
    }
}
