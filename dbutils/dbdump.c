/* dbdump.c: Dump the contents of the database argument human readably. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Sep 17 14:00:45 1995.  */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif

#if defined (HAVE_FCNTL_H)
#  include <fcntl.h>
#else
#  if defined (HAVE_SYS_FCNTL_H)
#    include <sys/fcntl.h>
#  endif
#endif
#if defined (HAVE_BSTRING_H)
#  include <bstring.h>
#endif
#include <sys/stat.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include "database.h"

static void fprintf_alist (FILE *stream, char *string, int starting_column);
static void dbdump (char *dbfile, FILE *stream);

int
main (int argc, char *argv[])
{
  int arg_index = 1;

  while (arg_index < argc)
    {
      char *dbfile = argv[arg_index++];

      dbdump (dbfile, stdout);
      fprintf (stdout, "\n");
    }
  return (0);
}

static void
dbdump (char *dbfile, FILE *stream)
{
  DBFILE db = database_open (dbfile, DB_READER);

  if (!db)
    {
      fprintf (stderr, "Cannot open database `%s'\n", dbfile);
    }
  else
    {
      DBOBJ *key, *nextkey, *content;

      fprintf (stream, "DUMP OF DATABASE: `%s'\n", dbfile);

      key = database_firstkey (db);

      while (key && key->data)
	{
	  fprintf (stream, "\n     Key: `%s'\n", (char *)key->data);

	  content = database_fetch (db, key);

	  if (content && content->data)
	    {
	      content->data = (unsigned char *)xrealloc
		(content->data, (1 + content->length));
	      content->data[content->length] = 0;
	      fprintf (stream, "    Data: ");
	      fprintf_alist (stream, (char *)content->data, 10);
	      free (content->data);
	      free (content);
	    }

	  nextkey = database_nextkey (db, key);
	  free (key->data);
	  free (key);
	  key = nextkey;
	}

      database_close (db);
    }
}

static int
current_indent (BPRINTF_BUFFER *buffer, int point)
{
  register int i = point;

  if (i > 0)
    {
      while (--i)
	{
	  if (buffer->buffer[i] == '\n')
	    {
	      i++;
	      break;
	    }
	}

      i = point - i;
    }
  return (i);
}

static int
indent_to (BPRINTF_BUFFER *buffer, int point, int desired_column)
{
  int num_spaces, current_column;

  current_column = current_indent (buffer, point);
  num_spaces = desired_column - current_column;

  if (num_spaces <= 0)
    num_spaces = 1;

  while (num_spaces--)
    bprintf_insert (buffer, point++, " ");

  return (point);
}

static char *
format_alist (char *string, int starting_column)
{
  register int i = 0;
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  int list_depth = 0, done = 0;
  int quoted = 0;
  int quote_point = 0;
  char *result;

  bprintf (buffer, "%s", string);

  while (i < buffer->bindex && !done)
    {
      int c = buffer->buffer[i++];

      switch (c)
	{
	case '"':
	  quoted = !quoted;
	  if (quoted)
	    quote_point = current_indent (buffer, i);
	  else
	    quote_point = 0;
	  break;

	case '(':
	  if (!quoted)
	    list_depth++;
	  break;

	case ')':
	  if (!quoted)
	    {
	      list_depth--;
	      if (i < buffer->bindex && buffer->buffer[i] != ')')
		{
		  bprintf_insert (buffer, i, "\n"); i++;
		  i = indent_to (buffer, i, starting_column + list_depth);
		}
	      if (list_depth == 0)
		done = 1;
	    }
	  break;

	case '\n':
	  if (quoted)
	    i = indent_to (buffer, i, quote_point);
	  break;
	}
    }

  result = buffer->buffer;
  free (buffer);
  return (result);
}

static void
fprintf_alist (FILE *stream, char *string, int starting_column)
{
  char *printed = format_alist (string, starting_column);
  fprintf (stream, "%s\n", printed);
  free (printed);
}



