/* strip-tags.c: -*- C -*-  Remove HTML/Meta-HTML tags from file. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Dec  8 13:47:06 1995. */

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
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>

static void process_data (FILE *input, FILE *output);

int
main (int argc, char *argv[])
{
  FILE *input_stream = stdin;
  FILE *output_stream = stdout;

  if (argc < 2)
    process_data (input_stream, output_stream);
  else
    {
      register int i;
      char *filename;

      for (i = 1; i < argc; i++)
	{
	  input_stream = fopen (argv[i], "r");

	  if (!input_stream)
	    {
	      fprintf (stderr, "Cannot open input stream: %s\n", argv[i]);
	    }
	  else
	    {
	      filename = (char *)xmalloc (11 + strlen (argv[i]));
	      sprintf (filename, "%s-stripped", argv[i]);
	      output_stream = fopen (filename, "w");
	      if (!output_stream)
		{
		  fprintf (stderr, "Cannot open output stream: %s\n",
			   filename);
		}
	      else
		{
		  process_data (input_stream, output_stream);
		  fclose (output_stream);
		}
	      free (filename);
	      fclose (input_stream);
	    }
	}
    }

  return (0);
}

static void
process_data (FILE *input, FILE *output)
{
  register int i, c, cache_index = 0;
  int quoted = 0, depth = 0, skip_processing_next = 0;
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *cache;
  char fgets_buffer[1024];
  char *line;

  /* Read all of the input data. */
  while ((line = fgets (fgets_buffer, sizeof (fgets_buffer) - 1, input))
	 != (char *)NULL)
    bprintf (buffer, "%s", line);


  /* Okay, we have all of the input.  Grovel it. */
  cache = (char *)xmalloc (1 + buffer->bindex);
  cache_index = 0;

  for (i = 0; i < buffer->bindex; i++)
    {
      c = buffer->buffer[i];

      if (skip_processing_next)
	{
	  skip_processing_next = 0;
	}
      else
	{
	  switch (c)
	    {
	    case '\\':
	      skip_processing_next = 1;
	      break;

	    case '"':
	      quoted = !quoted;
	      break;
	  
	    case '<':
	      /* if (!quoted) */
		depth++;
	      break;

	    case '>':
	      if (/* !quoted && */ depth)
		{
		  depth--;
		  if (depth == 0)
		    c = 0;
		}
	      break;

	    case ';':
	      if (((i + 2) < buffer->bindex) &&
		  (buffer->buffer[i + 1] == ';') &&
		  (buffer->buffer[i + 2] == ';'))
		{
		  while (buffer->buffer[i] != '\n') i++;
		  c = 0;
		}
	    }

	  if (!depth && c)
	    cache[cache_index++] = c;
	}
    }

  cache[cache_index] = '\0';

  fprintf (output, "%s", cache);
  free (cache);
  bprintf_free_buffer (buffer);
}

  

	      
      
