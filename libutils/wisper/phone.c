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
static WispObject *read_database (char *filename);
static WispObject *find_matches (WispObject *database, char *key, char *string);

static char *rolodex_file = "/homes/UniAx/Admin/ROLODEX";
static char *user_file_template = "%s/.phones";

int
main (int argc, char *argv[])
{
  WispObject *database;
  int do_long_listing = 0;
  int matches_found = 0;
  char *database_filename = (char *)NULL;

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
      else
	{
	  WispObject *matches;

	  matches = find_matches (database, "name:", arg);
	  if (matches != NIL)
	    {
	      matches_found += list_length (matches);
	      print_entries (stdout, matches, do_long_listing);
	    }

	  matches = find_matches (database, "email:", arg);
	  if (matches != NIL)
	    {
	      matches_found += list_length (matches);
	      print_entries (stdout, matches, do_long_listing);
	    }

	  matches = find_matches (database, "e-mail:", arg);
	  if (matches != NIL)
	    {
	      matches_found += list_length (matches);
	      print_entries (stdout, matches, do_long_listing);
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

