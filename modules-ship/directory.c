/* directory.c: -*- C -*-  Generate a directory listing page for DIRECTORY. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jan 18 09:51:57 1997. 

    This file is part of <Meta-HTML>(tm), a system for the rapid
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

#include "modules.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* 1) Declare the functions which implement the Meta-HTML functionality. */
static void pf_dir_table (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag	     complex? debug_level	   code    */
  { "DIR::DIR-TABLE",	0,	 0,		pf_dir_table },
  { (char *)NULL,	0,	 0,		(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("directory", ftab)

/* 4) Write the actual code which implements your functionality. */

typedef struct
{
  char *full_path;		/* The full pathname of the file. */
  char *stem;			/* The name of the file. */
  struct stat finfo;		/* The information about this file. */
} dir_info;

static dir_info **
directory_info (char *dirname, PAGE *page, int start)
{
  dir_info **result = (dir_info **)NULL;
  int dirname_len = strlen (dirname);
  int result_index = 0, result_size = 0;
  DIR *dir = opendir (dirname);

  if (dir != (DIR *)NULL)
    {
      struct dirent *entry;

      while ((entry = readdir (dir)) != (struct dirent *)NULL)
	{
	  dir_info *element = (dir_info *)xmalloc (sizeof (dir_info));
	  int len = D_NAMELEN (entry);

	  element->stem = (char *)xmalloc (1 + len);
	  strncpy (element->stem, entry->d_name, len);
	  element->stem[len] = '\0';
	  element->full_path = (char *)xmalloc (2 + dirname_len + len);
	  strcpy (element->full_path, dirname);
	  strcat (element->full_path, "/");
	  strcat (element->full_path, element->stem);

	  stat (element->full_path, &(element->finfo));

	  if (result_index + 2 > result_size)
	    result = (dir_info **)
	      xrealloc (result, (result_size += 10) * sizeof (dir_info *));
	  result[result_index++] = element;
	  result[result_index] = (dir_info *)NULL;
	}

      closedir (dir);
    }

  return (result);
}

typedef enum
{ sort_name,
  sort_size,
  sort_type,
  sort_created,
  sort_accessed,
  sort_modified
} SortType;

static SortType
sort_type_of_key (char *key)
{
  SortType result = sort_name;

  if (key != (char *)NULL)
    {
      if (strcasecmp (key, "name") == 0)
	result = sort_name;
      else if (strcasecmp (key, "size") == 0)
	result = sort_size;
      else if (strcasecmp (key, "type") == 0)
	result = sort_type;
      else if (strcasecmp (key, "created") == 0)
	result = sort_created;
      else if (strcasecmp (key, "accessed") == 0)
	result = sort_accessed;
      else if (strcasecmp (key, "modified") == 0)
	result = sort_modified;
    }

  return (result);
}

static SortType the_sort_type = sort_name;

static int
directory_sorter (const void *item1, const void *item2)
{
  dir_info *first = *((dir_info **)item1);
  dir_info *second = *((dir_info **)item2);

  /* Entries are both files, or both directories.  Sort on criteria. */
  switch (the_sort_type)
    {
    case sort_name:
      /* Always place directories before files. */
      if (S_ISDIR (first->finfo.st_mode) && !S_ISDIR (second->finfo.st_mode))
	return (-1);

      if (S_ISDIR (second->finfo.st_mode) && !S_ISDIR (first->finfo.st_mode))
	return (1);
      return (strcasecmp (first->stem, second->stem));

    case sort_size:
      return (first->finfo.st_size > second->finfo.st_size);

    case sort_type:
      {
	char *ext1 = strrchr (first->stem, '.');
	char *ext2 = strrchr (second->stem, '.');

	/* Always place directories before files. */
	if (S_ISDIR (first->finfo.st_mode) && !S_ISDIR (second->finfo.st_mode))
	  return (-1);

	if (S_ISDIR (second->finfo.st_mode) && !S_ISDIR (first->finfo.st_mode))
	  return (1);

	if ((ext1 == (char *)NULL) && (ext2 != (char *)NULL))
	  return (1);

	if ((ext1 != (char *)NULL) && (ext2 == (char *)NULL))
	  return (-1);

	/* Both null? */
	if (ext2 == ext1)
	  return (strcasecmp (first->stem, second->stem));

	/* Identical extension? */
	if (strcasecmp (ext1, ext2) == 0)
	  return (strcasecmp (first->stem, second->stem));

	return (strcasecmp (ext1, ext2));
      }

    case sort_created:
      return (first->finfo.st_ctime < second->finfo.st_ctime);

    case sort_accessed:
      return (first->finfo.st_atime < second->finfo.st_atime);

    case sort_modified:
      return (first->finfo.st_mtime < second->finfo.st_mtime);
    }

  /* NOT REACHED */
  return (-1);
}

static char *
directory_date (long ticks)
{
  struct tm *ti = localtime ((const time_t *)&ticks);
  static char buffer[100];
  static char *month_names[12] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

  sprintf (buffer, "%02d-%s-%d %02d:%02d:%02d",
	   ti->tm_mday,
	   month_names[ti->tm_mon],
	   ti->tm_year,
	   ti->tm_hour, ti->tm_min, ti->tm_sec);
  return (buffer);
}

#define ONE_MEG (1024 * 1024)

DOC_SECTION (FILE-OPERATORS)
DEFUNX (pf_dir::dir-table, &optional webpath regexp
	&key show-icons=true icon-dir=pathname
	sort=sort-op
	link=URL,
"Produces a clickable listing of the files in <var webpath> limited to
those which match <var regexp>, displaying icons found in <var icon-dir>.

<var webpath> defaults to the directory containing this page.

<var regexp>  defaults to <code>\"(^[^.].*[^~]$)|(^\\.\\.$)|(^\\.$)\"</code>,
i.e., files which do not begin with a dot, and files which do not end with
a tilde, and the two special directories, <code>`.'</code> and <code>`..'</code>.

<var icon-dir> defaults to /images/icons -- each file is associated with an
icon file named by file extension (e.g, mhtml.gif, html.gif, etc.)
Icons are only shown if <var show-icons> is set.

Possible values for <var sort-op> are:
<ul>
<li> <code>size</code>: Order by size of file,
<li> <code>name</code>: Order alphabetically by name,
<li> <code>type</code>: Order by directory .vs. file, and filename extension,
<li> <code>created</code>: Order by creation date,
<li> <code>accessed</code>: Order by last access time, and
<li> <code>modified</code>: Order by last modification time.
</ul>

<var sort-op> defaults to <code>name</code>.

<example>
<dir::dir-table>
</example>")

static void
pf_dir_table (PFunArgs)
{
  char *webpath = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *regexp = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *sortkey = mhtml_evaluate_string (get_value (vars, "sort"));
  char *linkurl = mhtml_evaluate_string (get_value (vars, "link"));
  char *icondir = mhtml_evaluate_string
    (get_one_of (vars, "icon-dir", "icon-directory", (char *)NULL));
  char *show_icons_arg = mhtml_evaluate_string
    (get_value (vars, "show-icons"));
  int show_icons_p = !empty_string_p (show_icons_arg);

  xfree (show_icons_arg);

  if (webpath == (char *)NULL)
    {
      webpath = pagefunc_get_variable ("mhtml::relative-prefix");
      if (empty_string_p (webpath))
	webpath = "";
      webpath = strdup (webpath);
    }
  else if (strcmp (webpath, "/") == 0)
    {
      free (webpath);
      webpath = strdup ("");
    }

  if (regexp == (char *)NULL)
    regexp = strdup ("(^[^.].*[^~]$)|(^\\.\\.$)|(^\\.$)");

  if (empty_string_p (icondir) && (show_icons_p != 0))
    {
      xfree (icondir);
      icondir = strdup ("/images/icons");
    }

  /* Okay, start building the directory. */
  {
    dir_info **contents = (dir_info **)NULL;
    char *dirname;
    regex_t re;
    regmatch_t offsets[2];
    BPRINTF_BUFFER *temp = bprintf_create_buffer ();

    bprintf (temp, "%s%s", pagefunc_get_variable ("mhtml::include-prefix"),
	     webpath);
    dirname = strdup (temp->buffer);
    bprintf_free_buffer (temp);

    contents = directory_info (dirname, page, start);

    if (contents != (dir_info **)NULL)
      {
	register int i, x;
	int stem_len = 0;
	dir_info *entry;

	the_sort_type = sort_type_of_key (sortkey);

	for (i = 0; contents[i] != (dir_info *)NULL; i++)
	  {
	    x = strlen (contents[i]->stem);
	    if (x > stem_len) stem_len = x;
	  }

	qsort ((void *)contents, (size_t)i, sizeof (dir_info *),
	       directory_sorter);

	temp = bprintf_create_buffer ();
	bprintf (temp, "\n<pre>\n");

	/* Produce the header. */
	if (show_icons_p)
	  bprintf (temp, "   ");

	bprintf (temp, "   Size ");
	bprintf (temp, "File/Directory Name            ");
	/*             "14-Jan-97 01:36:37" */
	bprintf (temp, "    Created On    "); bprintf (temp, "    ");
	bprintf (temp, "   Last Accessed  "); bprintf (temp, "    ");
	bprintf (temp, "   Last Modified  "); bprintf (temp, "    ");
	bprintf (temp, "\n");
	bprintf (temp, "----------------------------------------");
	bprintf (temp, "----------------------------------------");
	bprintf (temp, "-------------------------\n");

	regcomp (&re, regexp, REG_EXTENDED | REG_ICASE);

	for (i = 0; (entry = contents[i]) != (dir_info *)NULL; i++)
	  {
	    if (regexec (&re, entry->stem, 1, offsets, 0) == 0)
	      {
		if (show_icons_p)
		  {
		    char *tail = strrchr (entry->stem, '.');

		    if (tail)
		      tail++;
		    else
		      {
			if (S_ISDIR (entry->finfo.st_mode))
			  tail = "folder";
			else
			  tail = "generic";
		      }

		    if ((strcmp (entry->stem, ".") == 0) ||
			(strcmp (entry->stem, "..") == 0))
		      tail = "folder";

		    /* Check that the desired icon actually exists. */
		    {
		      BPRINTF_BUFFER *checker = bprintf_create_buffer ();
		      char *answer = (char *)NULL;

		      bprintf (checker, "<webpath-exists %s/%s.gif>",
			       icondir, tail);
		      answer = mhtml_evaluate_string (checker->buffer);
		      bprintf_free_buffer (checker);
		      if (empty_string_p (answer))
			tail = "generic";
		      xfree (answer);
		    }

		    bprintf (temp, "<img src=\"%s/%s.gif\">", icondir, tail);
		  }

		if (entry->finfo.st_size > ONE_MEG)
		  bprintf (temp, " %4dM  ", entry->finfo.st_size / ONE_MEG);
		else
		  bprintf (temp, " %4dK  ", entry->finfo.st_size / 1024);

		{
		  static char buffer[1024];
		  char *slash = S_ISDIR (entry->finfo.st_mode) ? "/" : "";
		  int wlen = strlen (webpath);
		  char *add_slash = "";

		  if ((*slash != '\0') &&
		      ((wlen < 1) || (webpath[wlen - 1] != '/')))
		    add_slash = "/";

		  sprintf (buffer, "%s%s", entry->stem, slash);
		  if (!empty_string_p (linkurl))
		    {
		      char newbuff[1024];
		      char *cgi_encoded;
		      Package *p = symbol_get_package ((char *)NULL);
		      Symbol **syms;

		      sprintf (newbuff, "%s%s%s", webpath, add_slash, buffer);
		      forms_set_tag_value_in_package (p, "doc", newbuff);
		      syms = symbols_of_package (p);
		      cgi_encoded = forms_unparse_items (syms);
		      free (syms);
		      symbol_destroy_package (p);
		      bprintf (temp, "<a href=\"%s?%s\">",
			       linkurl, cgi_encoded);
		      free (cgi_encoded);
		    }
		  else
		    {
		      bprintf (temp, "<a href=\"%s%s%s\">",
			       webpath, add_slash, buffer);
		    }

		  stem_len = strlen (buffer);

		  if (stem_len > 31)
		    {
		      buffer[28] = '.';
		      buffer[29] = '.';
		      buffer[30] = '.';
		      buffer[31] = '\0';
		    }
		  bprintf (temp, "%s</a>", buffer);

		  if (stem_len < 31)
		    {
		      int pad = 31 - stem_len;

		      for (x = 0; x < pad; x++)
			buffer[x] = ' ';

		      buffer[x] = '\0';
		      bprintf (temp, "%s", buffer);
		    }
		}

		bprintf (temp, "%s    %s    %s",
			 directory_date (entry->finfo.st_ctime),
			 directory_date (entry->finfo.st_atime),
			 directory_date (entry->finfo.st_mtime));
		bprintf (temp, "\n");
	      }
	  }
	bprintf (temp, "</pre>");
	regfree (&re);
	bprintf_insert (page, start, "%s", temp->buffer);
	*newstart += temp->bindex - 1;
	bprintf_free_buffer (temp);
      }
  }

  xfree (webpath);
  xfree (icondir);
  xfree (regexp);
  xfree (sortkey);
  xfree (linkurl);
}

#if defined (__cplusplus)
}
#endif
