/* filefuncs.c: Implementation of file-based functions in Meta-HTML. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Mar 22 13:54:13 1996.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1995, 2001, Brian J. Fox (bfox@ai.mit.edu).

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

#if defined (__cplusplus)
extern "C"
{
#endif

#if !defined (S_IFMT)
#  if defined (__S_IFMT)
#    define S_IFMT __SIFMT
#  else
#    define S_IFMT 0170000
#  endif
#endif

static void pf_get_file_properties (PFunArgs);
static void pf_set_file_properties (PFunArgs);
static void pf_directory_contents (PFunArgs);
static void pf_file_exists (PFunArgs);
#if defined (HAVE_MKFIFO)
static void pf_mkfifo (PFunArgs);
#endif
static char *unix_username_from_uid (uid_t uid);
static void unix_add_date (Symbol *sym, long ticks);

static PFunDesc func_table[] =
{
  { "GET-FILE-PROPERTIES",	0, 0, pf_get_file_properties },
  { "SET-FILE-PROPERTIES",	0, 0, pf_set_file_properties },
  { "DIRECTORY-CONTENTS",	0, 0, pf_directory_contents },
  { "FILE-EXISTS",		0, 0, pf_file_exists },
#if defined (HAVE_MKFIFO)
  { "UNIX::MKFIFO",		0, 0, pf_mkfifo },
#endif
  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_file_functions)
DEFINE_SECTION (FILE-OPERATORS, files; disk; system, 
"There are several types of commands in <meta-html> for dealing with files.

Some of these commands operate directly on an open files, while others
operate on <i>streams</i> (see <secref stream-operators>), which may be
connected to files or network data.

The functions allow you to <funref FILE-OPERATORS include> the
contents of, open (<funref STREAM-OPERATORS with-open-stream>), create,
read, or write various data sources, or to replace the contents of a page
(<funref PAGE-OPERATORS replace-page>) with another file.",
"Commands such as <funref FILE-OPERATORS get-file-properties> may
return additional information on some systems; this is documented with
each function where that is the case.")

/* <get-file-properties /www/nirvana/docs/welcome.mhtml>

     returns:
       ((name . "welcome.mhtml")
        (full-name . "/www/nirvana/docs/welcome.mhtml")
	(size . 2188)
	(type . FILE)
	(created 1 21 95 01 01 00 298398348)
	(written 1 21 95 01 01 00 298349349)
	(read    3 21 96 01 01 00 928948938)
	(mode . 0644)
	(creator . bfox))

     Values for TYPE: FILE, DIRECTORY, EXECUTABLE. */
static char *
get_file_properties (char *filename)
{
  char *result = (char *)NULL;

  if (filename != (char *)NULL)
    {
      struct stat finfo;
      int error;

      error = stat (filename, &finfo);

      if (error != -1)
	{
	  Package *package = symbol_get_package ((char *)NULL);
	  Symbol *sym;
	  char buffer[128];

	  /* FULL-NAME... */
	  sym = symbol_intern_in_package (package, "FULL-NAME");
	  symbol_add_value (sym, filename);

	  /* NAME... */
	  {
	    char *temp = strrchr (filename, '/');

	    if (temp != (char *)NULL)
	      temp++;
	    else
	      temp = filename;

	    sym = symbol_intern_in_package (package, "NAME");
	    symbol_add_value (sym, temp);
	  }

	  /* SIZE... */
	  sprintf (buffer, "%ld", (long)finfo.st_size);
	  sym = symbol_intern_in_package (package, "SIZE");
	  symbol_add_value (sym, buffer);

	  /* CREATED... */
	  sym = symbol_intern_in_package (package, "CREATED");
	  unix_add_date (sym, finfo.st_ctime);

	  /* WRITTEN... */
	  sym = symbol_intern_in_package (package, "WRITTEN");
	  unix_add_date (sym, finfo.st_mtime);

	  /* READ... */
	  sym = symbol_intern_in_package (package, "READ");
	  unix_add_date (sym, finfo.st_atime);

	  /* TYPE... */
	  sym = symbol_intern_in_package (package, "TYPE");
	  if (S_ISDIR (finfo.st_mode))
	    symbol_add_value (sym, "DIRECTORY");
	  else if ((S_IXOTH & finfo.st_mode))
	    symbol_add_value (sym, "EXECUTABLE");
	  else if (S_ISREG (finfo.st_mode))
	    symbol_add_value (sym, "FILE");

	  /* CREATOR... */
	  sym = symbol_intern_in_package (package, "CREATOR");
	  symbol_add_value (sym, unix_username_from_uid (finfo.st_uid));

	  /* MODE... (in octal) */
	  {
	    char mode_string[40];
	    unsigned int mode = (finfo.st_mode & ~S_IFMT);
	    sprintf (mode_string, "%0o", mode);
	    sym = symbol_intern_in_package (package, "MODE");
	    symbol_add_value (sym, mode_string);
	  }

	  result = package_to_alist (package, 0);
	  symbol_destroy_package (package);
	}
    }

  return (result);
}

static void
unix_add_date (Symbol *sym, long ticks)
{
  struct tm *ti = localtime ((const time_t *)&ticks);
  char buffer[40];

  sprintf (buffer, "%02d", ti->tm_mon + 1);
  symbol_add_value (sym, buffer);

  sprintf (buffer, "%02d", ti->tm_mday);
  symbol_add_value (sym, buffer);

  sprintf (buffer, "%02d", ti->tm_year);
  symbol_add_value (sym, buffer);

  sprintf (buffer, "%02d", ti->tm_hour);
  symbol_add_value (sym, buffer);

  sprintf (buffer, "%02d", ti->tm_min);
  symbol_add_value (sym, buffer);

  sprintf (buffer, "%02d", ti->tm_sec);
  symbol_add_value (sym, buffer);

  sprintf (buffer, "%ld", (unsigned long) ticks);
  symbol_add_value (sym, buffer);
}

static char *
unix_username_from_uid (uid_t uid)
{
  static uid_t last_uid = (uid_t)-1;
  static char last_name[50];

  if (uid != last_uid)
    {
      struct passwd *entry = getpwuid (uid);

      last_name[0] = '\0';
      last_uid = uid;

      if (entry != (struct passwd *)NULL)
	strcpy (last_name, entry->pw_name);

    }
  return (last_name);
}

/* File properties are returned in an alist representing operating system
   information about the named path.  Pathnames must be given fully; they
   are *not* relative to web space in any way.  This is probably a huge
   security hole, but it can't be any worse than CGI-EXEC.  Yeeesh. */
DEFUN (pf_get_file_properties, pathname,
"Return an association-list containing operating system information
about the file or directory named by <var path>. <var pathname> must be
given fully; it is <i>not</i> relative to Web space in any way.

If the file exists and is accessible, the members of the returned
association-list which are guaranteed to be present are:

<ul>
<li> <b>NAME</b>: <i>welcome.mhtml</i><br>
The name of the file or directory, without any of the path information.

<li> <b>FULL-NAME</b>: <i>/www/site/docs/welcome.mhtml</i><br>
The name of the file or directory, with full path
information.  This should be identical to <var PATH> as
received by <tag get-file-properties>.

<li> <b>SIZE</b>: <i> 2188 </i> <br>
The size of the file in bytes.

<li> <b>TYPE</b>: <i>FILE</i><br>
The <i>type</i> of the file.  This will either be
<code>FILE</code> or <code>DIRECTORY</code>.
</ul>

In addition to the above fields, the following fields appear on Unix
based systems:

<ul>
<li> <b>CREATED</b>: <i>6 29 96 10 3 24 897595648</i><br>
The date on which this file was created.  The value is an
array, with ordered values being: month, day, year, hours,
minutes, and seconds, and then the number of seconds since
Jan 1st, 1970.

<li> <b>WRITTEN</b>: <i>6 29 96 10 3 24 897595648</i><br>
The date on which this file was last written.  The value is an
array, with ordered values being: month, day, year, hours,
minutes, and seconds, and then the number of seconds since
Jan 1st, 1970.

<li> <b>READ</b>: <i>6 30 96 19 27 51 897595648</i><br>
The date on which this file was last read.  The value is an
array, with ordered values being: month, day, year, hours,
minutes, and seconds, and then the number of seconds since
Jan 1st, 1970.

<li> <b>CREATOR</b>: <i>bfox</i> <br>
The system identifier of the user who created this file.
</ul>

<html-complete-example>
<dump-alist <get-file-properties />>
</html-complete-example>")
{
  char *filename = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = get_file_properties (filename);

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }

  if (filename != (char *)NULL)
    free (filename);
}

DEFUN (pf_file_exists, pathname,
"Returns \"true\" if <var pathname> is an existing file or directory.

<var pathname> is absolute, that is to say that it is looked up in
absolute file system space, not in Web space.  To find out if a file
<code>foo</code> in the current directory exists, use:
<example>
<file-exists <thisdir>/foo>
</example>")
{
  char *filename = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (filename))
    {
      struct stat finfo;

      if ((stat (filename, &finfo)) != -1)
	{
	  bprintf_insert (page, start, "true");
	  *newstart += 4;
	  if (debug_level > 5)
	    page_debug ("<file-exists %s> --> true", filename);
	}
      else if (debug_level > 5)
	page_debug ("<file-exists %s> -->", filename);
    }
}

/* Not yet implemented. */
static void
pf_set_file_properties (PFunArgs)
{
}

DEFUN (pf_directory_contents,
       pathname &optional package-name &key matching=pattern,
"Returns a newline separated list of association lists for
the files matching <var pattern>.

When <var package-name> is supplied, each variable in
<var package-name> is the name of a file in <var pathname>, and
the value of each variable is the association list for that
file.")
{
  char *dirname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *matching = mhtml_evaluate_string (get_value (vars, "matching"));
  DIR *dir = (DIR *)NULL;

  if (!empty_string_p (dirname))
    {
      dir = opendir (dirname);
  
      if (dir != (DIR *)NULL)
	{
	  char *packname;
	  Package *package;
	  Symbol *sym;
	  struct dirent *entry;
	  char buffer[1024];
	  int offset = 0;
	  regex_t re;
	  regmatch_t where[2];
	  int check_re = 0;

	  if (!empty_string_p (matching))
	    {
	      regcomp (&re, matching, REG_EXTENDED);
	      check_re = 1;
	    }

	  packname = mhtml_evaluate_string (get_positional_arg (vars, 1));
	  if ((packname != (char *)NULL) && empty_string_p (packname))
	    package = CurrentPackage;
	  else
	    package = symbol_get_package (packname);

	  sprintf (buffer, "%s", dirname);
	  offset = strlen (buffer);
	  if (buffer[offset - 1] != '/')
	    buffer[offset++] = '/';

	  while ((entry = readdir (dir)) != (struct dirent *)NULL)
	    {
	      char *info;

	      strncpy (&buffer[offset], entry->d_name, D_NAMELEN (entry));
	      buffer[offset + D_NAMELEN (entry)] = '\0';

	      if ((check_re == 0) ||
		  (regexec (&re, buffer + offset, 1, where, 0) == 0))
		{
		  info = get_file_properties (buffer);
		  sym = symbol_intern_in_package (package, buffer + offset);
		  symbol_add_value (sym, info);
		  if (info != (char *)NULL)
		    free (info);
		}
	    }

	  if (packname != (char *)NULL)
	    {
	      free (packname);
	    }
	  else
	    {
	      char *result = package_to_alist (package, 0);
	      if (!empty_string_p (result))
		{
		  bprintf_insert (page, start, "%s\n", result);
		  *newstart += 1 + strlen (result);
		  start = *newstart;
		}

	      xfree (result);
	    }

	  if (check_re)
	    regfree (&re);
	}
    }

  if (dir != (DIR *)NULL)
    closedir (dir);

  if (matching != (char *)NULL) free (matching);
  if (dirname != (char *)NULL) free (dirname);
}

#if defined (HAVE_MKFIFO)
DEFUNX (pf_unix::mkfifo, filename &optional mode-bits,
"Create a named pipe on unix systems that support named pipes.

The pipe is named <var filename> and is created with the access mode
specified by <var mode-bits>, an octal permissions mask.

<var mode-bits> defaults to 0666 if not specified.

After a named pipe is created, it can be written to and read from
in the normal manner for writing and reading file streams.

UNIX::MKFIFO returns \"true\" if the named pipe could be created, or
the empty string if not.  In the case of an error, the human readable
text of the error message appears in SYSTEM-ERROR-OUTPUT.")

static void
pf_mkfifo (PFunArgs)
{
  char *filename = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *modebits_arg = mhtml_evaluate_string (get_positional_arg (vars, 1));
  int created_p = -1;

  if (!empty_string_p (filename))
    {
      unsigned int modebits = 0666;

      if (!empty_string_p (modebits_arg))
	sscanf (modebits_arg, "%o", &modebits);

      created_p = mkfifo (filename, modebits);

      if (created_p == -1)
	page_syserr ("unix::mkfifo: %s", strerror (errno));
    }

  if (created_p == 0)
    {
      bprintf_insert (page, start, "true");
      *newstart += 4;
    }

  xfree (filename);
  xfree (modebits_arg);
}
#endif /* HAVE_MKFIFO */

#if defined (__cplusplus)
}
#endif
