/* dbpack.c: Pack/unpack a database for cross platform movement . */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Sep 17 11:27:15 1995.  */

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
#include <sys/stat.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include <wisper/wisp.h>
#include "database.h"

#undef whitespace
#define whitespace(x) ((x == ' ') || (x == '\t') || (x == '\n') || (x == '\r'))

static char *filename_extension (char *filename);
static char *filename_sans_extension (char *filename);
static void package_db (char *dbname);
static void unpackage_db (char *packname);

/* For every database file mentioned on the command line, either
   package or unpackage it depending on filename extension. */
int
main (int argc, char *argv[])
{
  int arg_index = 1;

  while (arg_index < argc)
    {
      char *filename = argv[arg_index++];

      if (strcmp (filename_extension (filename), "packed") == 0)
	unpackage_db (filename);
      else
	package_db (filename);
    }

  return (0);
}

static char *
filename_extension (char *filename)
{
  char *extension = strrchr (filename, '.');

  if (!extension)
    extension = "";
  else
    extension++;

  return (extension);
}

static char *
filename_sans_extension (char *filename)
{
  char *result = strdup (filename);
  char *extension = strrchr (result, '.');

  if (extension) 
    *extension = '\0';

  return (result);
}

static void
package_db (char *dbname)
{
  DBFILE db;

  db = database_open (dbname, DB_READER);

  if (db != (DBFILE)0)
    {
      BPRINTF_BUFFER *packbuff = bprintf_create_buffer ();
      char *packname = (char *)xmalloc (8 + (strlen (dbname)));
      DBOBJ *key = database_firstkey (db);

      sprintf (packname, "%s.packed", dbname);

      while (key != (DBOBJ *)NULL)
	{
	  DBOBJ *content = database_fetch (db, key);
	  DBOBJ *nextkey;
	  char *keyval = (char *)xmalloc (1 + key->length);
	  char *contentval = (char *)NULL;

	  strncpy (keyval, (char *)key->data, key->length);
	  keyval[key->length] = '\0';

	  if (content && content->length)
	    {
	      contentval = (char *)xmalloc (1 + content->length);
	      strncpy (contentval, (char *)content->data, content->length);
	      contentval[content->length] = '\0';
	    }

	  bprintf (packbuff, "(%s %s)",
		   wisp_readable (keyval), contentval ? contentval : "()");

	  free (keyval);
	  if (contentval) free (contentval);

	  nextkey = database_nextkey (db, key);
	  free (key->data);
	  free (key);
	  key = nextkey;
	  if (content)
	    {
	      if (content->data) free (content->data);
	      free (content);
	    }
	}

      /* Close the open database. */
      database_close (db);

      /* Now write the buffer to PACKNAME. */
      {
	FILE *stream = fopen (packname, "w");
	if (stream)
	  {
	    fwrite (packbuff->buffer, 1, packbuff->bindex, stream);
	    fclose (stream);
	  }
      }
      bprintf_free_buffer (packbuff);
    }
  else
    {
      fprintf (stderr, "Couldn't open database `%s'\n", dbname);
    }
}

static char *
read_sexp (char *string, int *start)
{
  char *result = (char *)NULL;

  if (string != (char *)NULL)
    {
      register int i = *start;
      int gobbled, quoted, depth;

      gobbled = quoted = depth = 0;

      /* Skip leading whitespace. */
      while (whitespace (string[i])) i++;
      *start = i;

      gobbled = 0;
      while (!gobbled)
	{
	  register int c = string[i++];

	  switch (c)
	    {
	    case '\\':
	      i++;
	      break;

	    case '(':
	      if (!quoted)
		depth++;
	      break;

	    case ')':
	      if (!quoted)
		{
		  depth--;
		  if (depth == 0)
		    {
		      gobbled++;
		      continue;
		    }
		}
	      break;

	    case '"':
	      quoted = !quoted;
	      break;

	    case ' ':
	    case '\t':
	    case '\n':
	    case '\r':
	      if (!quoted && depth <= 0)
		{
		  gobbled++;
		  continue;
		}
	      break;

	    case '\0':
	      gobbled++;
	      break;
	    }
	}

      result = (char *)xmalloc (1 + (i - *start));
      strncpy (result, string + *start, i - *start);
      result[i - *start] = '\0';
      *start = i;
    }

  return (result);
}

static void
unpackage_db (char *packname)
{
  struct stat finfo;
  int filesize, fd;
  char *buffer;
  DBFILE db;
  char *db_filename = filename_sans_extension (packname);

  if (stat (packname, &finfo) == -1)
    {
      fprintf (stderr, "Cannot open packed database `%s'\n", packname);
      return;
    }

  filesize = (int)finfo.st_size;

  buffer = (char *)xmalloc (1 + filesize);

  fd = open (packname, O_RDONLY, 0666);

  if (fd != -1)
    {
      read (fd, buffer, filesize);
      close (fd);
    }
  else
    {
      fprintf (stderr, "Couldn't read packed database `%s'\n", packname);
      return;
    }

  /* Process the set of lists in the file. */
  db = database_open (db_filename, DB_WRCREAT);

  if (db == (DBFILE)0)
    {
      fprintf (stderr, "Couldn't create database file `%s'\n", db_filename);
    }
  else
    {
      register int i;
      int start = 0;
      int done = 0;

      /* Get the key and content from the string. */
      while (!done)
	{
	  char *keydata;
	  char *contentdata;

	  /* Skip leading whitespace. */
	  for (i = start; whitespace (buffer[i]); i++);

	  /* Next character is an open paren, or we are done. */
	  if (buffer[i] != '(')
	    {
	      done = 1;
	      continue;
	    }

	  start = ++i;
	  keydata = read_sexp (buffer, &start);
	  contentdata = read_sexp (buffer, &start);
	  i = start;

	  if (buffer[i] != ')')
	    {
	      fprintf (stderr, "Malformed list for key `%s'?\n",
		       keydata ? keydata : "(Null Key)");
	    }
	  else
	    start++;

	  /* Now add this key and value to the database. */
	  {
	    WispObject *wispkey = wisp_from_string (keydata);
	    DBOBJ key, content;

	    key.data = (unsigned char *)STRING_VALUE (wispkey);
	    key.length = 1 + strlen ((char *)key.data);

	    content.data = (unsigned char *)contentdata;
	    content.length = 1 + strlen ((char *)content.data);

	    database_store (db, &key, &content);
	    gc_wisp_objects ();
	  }

	  /* Give up the storage space. */
	  free (contentdata);
	  free (keydata);
	}

      database_close (db);
    }
}
