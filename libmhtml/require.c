/* require.c: -*- C -*-  Load a package if it isn't already loaded. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Fri Aug 30 11:25:54 1996.  */

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
#include "symdump.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* The name of the variable which holds the already found filenames. */
#define REQUIRE_LOADED "MHTML::REQUIRE-LOADED"

/* The name of the variable which holds the directories to search. */
#define REQUIRE_DIRECTORIES "MHTML::REQUIRE-DIRECTORIES"

/* The types of files that REQUIRE finds. */
#define require_TYPE_ANY     0
#define require_TYPE_INCLUDE 1
#define require_TYPE_LIB     2

/* Extension to filetype mapping. */
typedef struct
{
  char *ext;
  int filetype;
} EXT2FILETYPE;

static EXT2FILETYPE extensions[] =
{
  { ".mhtml",	require_TYPE_INCLUDE },
  { ".src",	require_TYPE_INCLUDE },
  { ".lib",	require_TYPE_LIB },
  { (char *)NULL, -1 }
};

static void pf_require (PFunArgs);

static PFunDesc func_table[] =
{
  { "require",			0, 0, pf_require },
  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_require_functions)
DOC_SECTION (FILE-OPERATORS)

/* Return the full pathname of the file found in webspace at ROOT and LEAF,
   side-effecting FILETYPE based on the extension of that file.  For LEAFs
   which do not specify an extension, the list of possible extensions is
   used to determine the most recently written file.
   A newly consed string (or NULL) is returned; you must free it yourself.*/
static char *
require_search (char *root, char *leaf, int *filetype,
		char **web_relative_name)
{
  register int i;
  static char *pathname = (char *)NULL;
  static int psize = 0;
  char *ext, *temp;
  int nsize = strlen (root) + strlen (leaf) + 1024;
  struct stat finfo;
  char *result = (char *)NULL;
  char *incpref = pagefunc_get_variable ("mhtml::include-prefix");
  char *relpref = pagefunc_get_variable ("mhtml::relative-prefix");
  char *wr = (char *)NULL;

  if (nsize >= psize)
    pathname = (char *)xrealloc (pathname, psize = nsize);

  if ((root[0] == '.') && (root[1] == '\0'))
    {
      strcpy (pathname, leaf);
    }
  else
    {
      strcpy (pathname, root);
      strcat (pathname, "/");
      strcat (pathname, leaf);
    }

  temp = mhtml_canonicalize_file_name (pathname, incpref, relpref, &wr);

  /* If found and not a directory, return it immediately. */
  if ((stat (temp, &finfo) != -1) && (!S_ISDIR (finfo.st_mode)))
    {
      /* Don't forget to set the filetype... */
      char *foundtype = strrchr (temp, '.');
      result = temp;

      if (foundtype)
	{
	  for (i = 0; extensions[i].ext != (char *)NULL; i++)
	    if (strcmp (foundtype, extensions[i].ext) == 0)
	      {
		*filetype = extensions[i].filetype;
		break;
	      }
	}
    }
  else
    {
      /* Not found under the specific name given.  If that name included
	 a file name extension, it is time to give up.  Otherwise, try all
	 of the extensions, and then take the one which was most recently
	 written. */
      char *slash = strrchr (temp, '/');

      if ((slash != (char *)NULL) &&
	  (ext = strrchr (slash, '.')) == (char *)NULL)
	{
	  time_t recent = 0;
	  int offset = strlen (temp);

	  temp = (char *)xrealloc (temp, 20 + offset);

	  for (i = 0; extensions[i].ext != (char *)NULL; i++)
	    {
	      strcpy (temp + offset, extensions[i].ext);

	      if (stat (temp, &finfo) != -1)
		{
		  if (finfo.st_mtime > recent)
		    {
		      if (result != (char *)NULL)
			free (result);

		      recent = finfo.st_mtime;
		      *filetype = extensions[i].filetype;
		      strcat (wr, extensions[i].ext);
		      result = strdup (temp);
		    }
		}
	    }
	}
      /* Get rid of unneeded temporary. */
      free (temp);
    }

  if ((result != (char *)NULL) &&
      ((unsigned long)finfo.st_mtime > page_most_recent_modification_time))
    {
      char digits[40];

      page_most_recent_modification_time = (unsigned long)finfo.st_mtime;
      sprintf (digits, "%ld", (unsigned long)finfo.st_mtime);
      pagefunc_set_variable ("mhtml::last-modification-time", digits);
    }

  *web_relative_name = wr;
  return (result);
}

static int
require_already_loaded_p (char *pathname)
{
  char **loaded = symbol_get_values (REQUIRE_LOADED);
  int result = 0;

  if (loaded != (char **)NULL)
    {
      register int i;

      for (i = 0; loaded[i] != (char *)NULL; i++)
	if (strcmp (loaded[i], pathname) == 0)
	  {
	    result = 1;
	    break;
	  }
    }

  return (result);
}

static void
require_remember_pathname (char *pathname)
{
  Symbol *sym = symbol_intern (REQUIRE_LOADED);
  symbol_add_value (sym, pathname);
}

DEFUN (pf_require, stem,
"<code>require</code> tries hard to locate the source or library file
specified by <var stem>, and then loads that file if it hasn't already
been loaded.

Both `<var stem>.lib' and `<var stem>.mhtml' are searched for -- <tag
require> loads the one which is newer.  In either case, if a function
named `<var .libinit>' is defined after the load, then that function is
executed with 0 arguments, and the function definition is then removed.

If the variable <varref mhtml::require-directories> is present, then
it is an array of directory names (without trailing slashes) relative
to Web space that should be searched through, in the order that they
appear in the array.

<code>require</code> understands the following extensions:

<ul>
<li> <b>.mhtml</b>, or <b>.src</b>: A <meta-html> source file.

<li> <b>.lib</b>: A <meta-html> library file.
</ul>

<code>require</code> loads the newest version of the file that it
finds, and records the complete pathname of the loaded file in the
array variable <varref mhtml::require-loaded>.")
{
  register int i;
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char **directories = symbol_get_values (REQUIRE_DIRECTORIES);
  char *default_dirs[2] = { ".", (char *)NULL };
  char *pathname = (char *)NULL;
  char *prefix = (char *)NULL;
  char *wr = (char *)NULL;
  int filetype = require_TYPE_ANY;

  if (directories == (char **)NULL)
    directories = &default_dirs[0];

  if (arg == (char *)NULL)
    return;

  if (*arg == '/')
    {
      pathname = require_search ("", arg, &filetype, &wr);
    }
  else
    {
      for (i = 0; (prefix = directories[i]) != (char *)NULL; i++)
	{
	  pathname = require_search (prefix, arg, &filetype, &wr);
	  if (pathname != (char *)NULL)
	    break;
	}
    }

  if (pathname == (char *)NULL)
    {
      page_debug ("<REQUIRE %s>: No file found in the search path", arg);
    }
  else if (!require_already_loaded_p (pathname))
    {
      switch (filetype)
	{
	case require_TYPE_ANY:
	case require_TYPE_INCLUDE:
	  {
	    PAGE *file_contents = page_read_template (pathname);

	    if (debug_level > 5)
	      page_debug ("<require %s> --> Found %s", arg, pathname);

	    if (file_contents != (PAGE *)NULL)
	      {
		require_remember_pathname (pathname);

#if defined (NOT_BINARY_COMPATIBLE)
		bprintf_insert (page, start, "%s", file_contents->buffer);
#else
		bprintf_insert (file_contents, 0, "<*parser*::push-file %s>",
				wr);
		bprintf (file_contents, "<*parser*::pop-file>");
		bprintf (file_contents, "<if <function-def .libinit> ");
		bprintf (file_contents, "\"<.libinit><undef .libinit>\">");
		bprintf_insert_binary (page, start, file_contents->buffer,
				       file_contents->bindex);

#endif /* BINARY_COMPATIBLE */
		page_free_page (file_contents);
	      }
	    else
	      page_debug ("<REQUIRE %s> Failed in read of %s", arg, pathname);
	  }
	  break;

	case require_TYPE_LIB:
	  {
	    int fd = os_open (pathname, O_RDONLY, 0666);
	    Package *pack;

	    if (fd > -1)
	      {
		Symbol *sym;
		UserFunction *uf;

		require_remember_pathname (pathname);
		while ((pack = symbol_load_package (fd)) != (Package *)NULL);

		close (fd);

		if (mhtml_user_keywords == (Package *)NULL)
		  mhtml_user_keywords = 
		    symbol_get_package_hash ("*user-functions*", 577);

		sym = mhtml_find_user_function_symbol (".libinit");
		if ((sym != (Symbol *)NULL) &&
		    (uf = (UserFunction *) sym->values)
		    != (UserFunction *)NULL)
		  {
		    char *open_body = strdup ("");
		    PAGE *p = page_create_page ();
		    Package *v = symbol_get_package ((char *)NULL);
		    int mystart=0, mynewstart = 0;

		    bprintf (p, " ");
		    p->bindex = 0; p->buffer[0] = '\0';

		    mhtml_execute_function
		      (sym, uf, p, p, v, mystart, 0, &mynewstart,
		       uf->debug_level, FNAME_COMMA open_body);

		    symbol_destroy_package (v);

		    if ((p != (BPRINTF_BUFFER *)NULL) && (p->bindex > 0))
		      {
			bprintf_insert (page, start, "%s", p->buffer);
			*newstart += p->bindex;
		      }
		    page_free_page (p);
		    free (open_body);
		    open_body = mhtml_evaluate_string ("<undef .libinit>");
		    xfree (open_body);
		  }
	      }
	    else
	      page_debug ("<REQUIRE %s> Failed in read of %s", arg, pathname);
	  }
	  break;

	default:
	  page_debug ("<REQUIRE %s> (%s) Don't understand files of type %d",
		      arg, pathname, filetype);
	  break;
	}
    }
  xfree (pathname);
  xfree (arg);
}

#if defined (__cplusplus)
}
#endif
