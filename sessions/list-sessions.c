/* list-session.c: Pretty print sessions in session database. */

/* Author: Brian J. Fox (bfox@ua.com) Sat Jul  1 16:34:18 1995.  */

/* This file is part of <Meta-HTML>(tm), a system for the rapid
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

	http://www.metahtml.com/COPYING
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "session.h"
#include "parseargs.h"
#include <bprintf/bprintf.h>

static void dump_session_info (SESSION_INFO *info, int short_p, FILE *stream);
static void fprintf_alist (FILE *stream, char *string, int starting_column);

int
main (int argc, char *argv[])
{
  register int i, count, short_p = 0;
  SESSION_INFO **list;
  int arg_index = 1;
  int sids_supplied = 0;

  parse_session_arguments (&argc, argv);

  if ((argc > 1) &&
      ((strcasecmp (argv[1], "-s") == 0) ||
       (strcasecmp (argv[1], "--short") == 0)))
    {
      arg_index += 1;
      short_p = 1;
    }

  list = session_all_sessions ();
  for (count = 0;
       (list != (SESSION_INFO **)NULL) &&
       (list[count] != (SESSION_INFO *)NULL);
       count++);

  printf ("Sessions in database: %d:\n", count);

  if (arg_index < argc) sids_supplied++;

  for (i = 0; i < count; i++)
    {
      int dump_this = 1;

      if (arg_index < argc)
	{
	  if (strcmp (list[i]->sid, argv[arg_index]) != 0)
	    dump_this = 0;
	  else
	    arg_index++;
	}

      if (dump_this)
	{
	  dump_session_info (list[i], short_p, stdout);
	  printf ("\n");
	}

      if (sids_supplied && arg_index == argc)
	break;
    }

  return (0);
}

static void
dump_session_info (SESSION_INFO *info, int short_p, FILE *stream)
{
  long seconds_remaining, seconds_elapsed;
  time_t now;

  now = time ((time_t *)0);
  seconds_elapsed = now - info->access;
  seconds_remaining = (info->timeout * 60) - seconds_elapsed;

  fprintf (stream, "  SESSION [%s/%s]", info->key, info->sid);

  if (short_p)
    {
      if (info->timeout == 0)
	fprintf (stream, "  (Forever)");
      else
	fprintf (stream, "  (%ld seconds remaining)", seconds_remaining);

      return;
    }
  else
    fprintf (stream, "\n");

  fprintf (stream, "      sid: %s\n", info->sid);
  fprintf (stream, "      key: %s\n", info->key);
  fprintf (stream, "    start: %s", ctime (&(info->start)));
  fprintf (stream, "   access: %s", ctime (&(info->access)));

  fprintf (stream, "  timeout: ");

  if (info->timeout == 0)
    fprintf (stream, "Forever\n");
  else
    {
      int minutes = 0;
      int seconds = 0;
      char *ender = "remaining";

      fprintf (stream, "%d minute%s ", info->timeout,
	       (info->timeout == 1) ? "" : "s");

      if (seconds_remaining < 0)
	{
	  seconds_remaining = -seconds_remaining;
	  ender = "overdue";
	}

      minutes = seconds_remaining / 60;
      seconds = seconds_remaining % 60;

      fprintf (stream, "(");

      if (minutes)
	fprintf (stream, "%d minute%s, ", minutes, (minutes == 1) ? "" : "s");

      fprintf (stream, "%d second%s %s)\n",
	       seconds, (seconds == 1) ? "" : "s", ender);
    }
  fprintf (stream, "   length: %ld\n", (long)info->length);
  fprintf (stream, "     data: ");
  if (strncmp ((char *)info->data, "((", 2) == 0)
    fprintf_alist (stream, (char *)info->data, 11);
  else
    fprintf (stream, "%s\n", (char *)info->data);
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
  int list_depth = 0;
  int quoted = 0;
  int quote_point = 0;
  char *result;

  bprintf (buffer, "%s", string);
  /* i = indent_to (buffer, i, starting_column); */

  while (i < buffer->bindex)
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


