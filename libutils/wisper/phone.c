/*  phone.c: Read a pb.el style database, and report on it. */

/* Author: Brian J. Fox (bfox@ua.com) Sun Jun  4 14:05:26 1995.  */

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
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>
#include <ctype.h>
#include <regex.h>

#include <xmalloc/xmalloc.h>

#include "wisp.h"

static int list_length (WispObject *list);
static void print_entries (FILE *stream, WispObject *list, int long_p);
static void print_entry (FILE *stream, WispObject *entry, int long_p);
static void dump_vcards (FILE *stream, WispObject *database);
static void dump_vcard (FILE *stream, WispObject *entry);
static WispObject *read_database (char *filename);
static WispObject *find_matches (WispObject *database, char *key, char *string);

static char *rolodex_file = "/homes/UniAx/Admin/ROLODEX";
static char *user_file_template = "%s/.phones";


int
main (int argc, char *argv[])
{
  WispObject *database;
  int do_long_listing = 0;
  int do_vcard_dump = 0;
  int matches_found = 0;
  char *database_filename = (char *)NULL;
  FILE *output_stream = (FILE *)NULL;

  output_stream = stdout;

  if (strcmp (argv[0], "rolodex") == 0)
    {
      database_filename = getenv ("ROLODEX");
      if (database_filename == (char *)NULL)
	database_filename = rolodex_file;
    }
  else
    {
      struct passwd *entry;

      entry = getpwuid (getuid ());
      if (entry)
	{
	  database_filename = (char *) xmalloc
	    (strlen (user_file_template) + strlen (entry->pw_dir) + 4);

	  sprintf (database_filename, user_file_template, entry->pw_dir);
	}
    }

  if (!database_filename)
    {
      fprintf (stderr, "Who are you?\n");
      return (-1);
    }

  database = read_database (database_filename);

  while (--argc)
    {
      char *arg = *++argv;

      if (strcmp (arg, "-l") == 0)
	{
	  do_long_listing = 1;
	}
      else if (strcmp (arg, "--vcard") == 0)
	{
	  do_vcard_dump++;
	}
      else if (strcmp (arg, "--separate-files") == 0)
	{
	  output_stream = (FILE *)NULL;
	}
      else
	{
	  WispObject *matches;

	  matches = find_matches (database, "name:", arg);
	  if (matches != NIL)
	    {
	      matches_found += list_length (matches);
	      if (do_vcard_dump)
		dump_vcards (output_stream, matches);
	      else
		print_entries (output_stream, matches, do_long_listing);
	    }

	  if (!do_vcard_dump || matches_found == 0)
	    {
	      matches = find_matches (database, "email:", arg);
	      if (matches != NIL)
		{
		  matches_found += list_length (matches);

		  if (do_vcard_dump)
		    dump_vcards (output_stream, matches);
		  else
		    print_entries (output_stream, matches, do_long_listing);
		}
	    }

	  if (!do_vcard_dump || matches_found == 0)
	    {
	      matches = find_matches (database, "e-mail:", arg);
	      if (matches != NIL)
		{
		  matches_found += list_length (matches);

		  if (do_vcard_dump)
		    dump_vcards (output_stream, matches);
		  else
		    print_entries (output_stream, matches, do_long_listing);
		}
	    }
	}
    }

  return (matches_found != 0);
}

static int
list_length (WispObject *list)
{
  register int i = 0;

  while (list != NIL)
    {
      i++;
      list = CDR (list);
    }

  return (i);
}

static void
print_entries (FILE *stream, WispObject *list, int long_p)
{
  while (list != NIL)
    {
      print_entry (stream, CAR (list), long_p);
      if (long_p) fprintf (stream, "\n");
      list = CDR (list);
    }
}

static char *
prettify (char *string)
{
  register int i, c;
  char *result;
  int cap_next = 1;

  result = strdup (string);

  for (i = 0; (c = string[i]) != '\0'; i++)
    {
      if (!isalpha (c))
	{
	  cap_next = 1;
	}
      else if (cap_next)
	{
	  cap_next = 0;
	  if (islower (c))
	    c = toupper (c);
	}
      else
	{
	  if (isupper (c))
	    c = tolower (c);
	}
      result[i] = c;
    }
  result[i] = '\0';

  return (result);
}

static void
print_entry (FILE *stream, WispObject *entry, int long_p)
{
  if (!long_p)
    {
      char *name = (char *)NULL;
      char *home_phone = (char *)NULL;
      char *work_phone = (char *)NULL;

      name = sassoc ("name:", entry);
      if (!home_phone) home_phone = sassoc ("home:", entry);
      if (!home_phone) home_phone = sassoc ("phone:", entry);
      if (!work_phone) work_phone = sassoc ("work:", entry);
      if (!work_phone) work_phone = sassoc ("office:", entry);
      if (!work_phone) work_phone = sassoc ("pager:", entry);

      fprintf (stream, "%20s: (home: %14s) --- (work: %14s)\n", name,
	       home_phone ? home_phone : "",
	       work_phone ? work_phone : "");
    }
  else
    {
      while (entry != NIL)
	{
	  char *tag, *value;
	  WispObject *pair;

	  pair = CAR (entry);
	  entry = CDR (entry);

	  tag = prettify (STRING_VALUE (CAR (pair)));
	  value = "";

	  if (CDR (pair) != NIL)
	    value = STRING_VALUE (CADR (pair));

	  fprintf (stream, "%14s  %s\n", tag, value);
	  free (tag);
	}
    }
}

static WispObject *
read_database (char *filename)
{
  char *buffer;
  int fd, len;
  struct stat finfo;
  WispObject *obj;

  if ((stat (filename, &finfo) == -1) ||
      ((fd = open (filename, O_RDONLY, 0666)) < 0))
    return (NIL);

  len = (int)finfo.st_size;

  buffer = (char *)xmalloc (1 + len);
  read (fd, buffer, len);
  buffer[len] = '\0';
  close (fd);

  /* Read and discard the first object. */
  wisp_push_input_string (buffer);
  obj = wisp_read ();
  obj = wisp_read ();

  free (buffer);
  return (obj);
}
  
static char *
strcasestr (char *big, char *little)
{
  register int i;
  int big_len, little_len;

  if (!little || !big)
    return (big);

  big_len = strlen (big);
  little_len = strlen (little);

  for (i = 0; i <= big_len - little_len; i++)
    if (strncasecmp (little, big + i, little_len) == 0)
      return (big + i);

  return ((char *)NULL);
}

static WispObject *
find_matches (WispObject *database, char *key, char *string)
{
  WispObject *result = NIL;

  while (database != NIL)
    {
      WispObject *entry;
      char *contents;

      entry = CAR (database);
      database = CDR (database);

      contents = sassoc (key, entry);

      if ((contents != (char *)NULL) &&
	  (strcasestr (contents, string) != (char *)NULL))
	result = make_cons (entry, result);
    }

  return (result);
}

static void
dump_vcard (FILE *stream, WispObject *entry)
{
  char *contents;
  char buffer[1024];

  /* Build and print a vcard reference. */
  fprintf (stream, "BEGIN:VCARD\nVERSION:2.1\n");
  contents = sassoc ("name:", entry);

  if (contents != (char *)NULL)
    {
      char *comma = strchr (contents, ',');

      if (comma == (char *)NULL)
	comma = strchr (contents, ' ');

      if (comma != (char *)NULL)
	{
	  strncpy (buffer, contents, (comma - contents));
	  buffer[comma - contents] = '\0';
	  comma++;
	  while (whitespace (*comma)) comma++;
	  fprintf (stream, "N:%s;%s\r\n", buffer, comma);
	  fprintf (stream, "FN:%s %s\r\n", comma, buffer);
	}
      else
	{
	  fprintf (stream, "N:%s\r\nFN:%s\r\n", contents, contents);
	}

      /* Get Company? */
      contents = sassoc ("company:", entry);
      if (contents != (char *)NULL)
	fprintf (stream, "ORG:%s\r\n", contents);

      /* Get Title? */
      contents = sassoc ("title:", entry);
      if (contents != (char *)NULL)
	fprintf (stream, "TITLE:%s\r\n", contents);

      /* Get work phone. */
      contents = sassoc ("work:", entry);
      if (contents == (char *)NULL)
	contents = sassoc ("office:", entry);

      if (contents != (char *)NULL)
	fprintf (stream, "TEL;WORK;VOICE:%s\r\n", contents);

      /* Get Fax. */
      contents = sassoc ("fax:", entry);

      if (contents != (char *)NULL)
	fprintf (stream, "TEL;WORK;FAX:%s\r\n", contents);

      /* Get cell phone. */
      contents = sassoc ("cell:", entry);

      if (contents == (char *)NULL)
	contents = sassoc ("mobile:", entry);

      if (contents != (char *)NULL)
	fprintf (stream, "TEL;CELL;VOICE:%s\r\n", contents);

      /* Get home phone. */
      contents = sassoc ("home:", entry);

      if (contents != (char *)NULL)
	fprintf (stream, "TEL;HOME;VOICE:%s\r\n", contents);

      /* Get full address. */
      fprintf (stream, "ADR;HOME:;;");

      /* Get Street. */
      contents = sassoc ("street:", entry);
      if (contents != (char *)NULL)
	fprintf (stream, "%s", contents);

      fprintf (stream, ";");

      /* Get City. */
      contents = sassoc ("city:", entry);
      if (contents != (char *)NULL)
	fprintf (stream, "%s", contents);

      fprintf (stream, ";");

      /* Get State. */
      contents = sassoc ("state:", entry);
      if (contents != (char *)NULL)
	fprintf (stream, "%s", contents);

      fprintf (stream, ";");

      /* Get Zip. */
      contents = sassoc ("zip:", entry);
      if (contents != (char *)NULL)
	fprintf (stream, "%s", contents);

      fprintf (stream, ";");
      fprintf (stream, "\r\n");


      /* Get E-Mail address. */
      contents = sassoc ("e-mail:", entry);

      if (contents == (char *)NULL)
	contents = sassoc ("email:", entry);

      if (contents != (char *)NULL)
	fprintf (stream, "EMAIL;PREF;INTERNET:%s\r\n", contents);
    }

  fprintf (stream, "END:VCARD\r\n");
}

void
dump_vcards (FILE *stream, WispObject *list)
{
  while (list != NIL)
    {
      if (stream == (FILE *)NULL)
	{
	  char *value = sassoc ("name:", CAR (list));

	  if (value != (char *)NULL)
	    {
	      register int i, j;
	      char filename[1024];
	      FILE *file = (FILE *)NULL;

	      for (i = 0, j = 0; value[i]; i++)
		{
		  if (((value[i] >= 'a') && (value[i] <= 'z')) ||
		      ((value[i] >= 'A') && (value[i] <= 'Z')) ||
		      ((value[i] >= '0') && (value[i] <= '9')))
		    {
		      filename[j++] = value[i];
		    }
		  else
		    {
		      if ((j > 0) && (filename[j - 1] != '_'))
			filename[j++] = '_';
		    }
		}

	      if (j > 0)
		while (j && (filename[j - 1] == '_')) j--;

	      filename[j] = '\0';

	      file = fopen (filename, "w");

	      if (file != (FILE *)NULL)
		{
		  dump_vcard (file, CAR (list));
		  fclose (file);
		}
	    }
	}
      else
	{
	  dump_vcard (stream, CAR (list));
	  if (list != NIL)
	    fprintf (stream, "\r\n");
	}

      list = CDR (list);
    }
}
