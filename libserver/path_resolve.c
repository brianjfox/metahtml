/* path_resolve.c: -*- C -*-  DESCRIPTIVE TEXT. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Nov  5 22:05:53 1995.  */

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
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include "pages.h"
#include "session_data.h"
#include "parser.h"
#include "http.h"
#include "path_resolve.h"
#include "globals.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static DOC_SPEC *
make_doc_spec (void)
{
  DOC_SPEC *spec = (DOC_SPEC *)xmalloc (sizeof (DOC_SPEC));

  memset (spec, 0, sizeof (DOC_SPEC));
  return (spec);
}

void
mhttpd_free_doc_spec (DOC_SPEC *spec)
{
  register int i;

  if (spec)
    {
      if (spec->requested_path) free (spec->requested_path);
      if (spec->physical_path) free (spec->physical_path);
      if (spec->logical_path) free (spec->logical_path);
      if (spec->query_string) free (spec->query_string);
      if (spec->argv)
	{
	  for (i = 0; spec->argv[i] != (char *)NULL; i++)
	    free (spec->argv[i]);
	  free (spec->argv);
	}

      if (spec->content)
	free (spec->content);

      free (spec);
    }
}

static char *
strappend (char *string, char *appendage)
{
  string = (char *)xrealloc (string, 1 + strlen (string) + strlen (appendage));
  strcat (string, appendage);
  return (string);
}

#define STRAPPEND(x,y) x = strappend (x, y)

#define PROXIES_ARE_BROKEN
#if defined (PROXIES_ARE_BROKEN)
/* Do the `%FF' and `+' hacking on string.  We can do this hacking in
   place, since the resultant string cannot be longer than the input
   string. */
static void
cleanup (char *string)
{
  register int i, j, len;
  char *dest;

  len = strlen (string);
  dest = (char *)alloca (1 + len);

  for (i = 0, j = 0; i < len; i++)
    {
      switch (string[i])
	{
	case '%':
	  dest[j++] = parse_hex_pair (string + i + 1);
	  i += 2;
	  break;

	case '+':
	  dest[j++] = ' ';
	  break;

	default:
	  dest[j++] = string[i];
	}
    }

  dest[j] = '\0';
  strcpy (string, dest);
}

static int
parse_hex_pair (char *pair_start)
{
  int value = 0;
  int char1, char2;

  char1 = char2 = 0;

  char1 = *pair_start;

  if (char1)
    char2 = (pair_start[1]);

  if (isupper (char1))
    char1 = tolower (char1);

  if (isupper (char2))
    char2 = tolower (char2);

  if (isdigit (char1))
    value = char1 - '0';
  else if ((char1 <= 'f') && (char1 >= 'a'))
    value = 10 + (char1 - 'a');

  if (isdigit (char2))
    value = (value * 16) + (char2 - '0');
  else if ((char2 <= 'f') && (char2 >= 'a'))
    value = (value * 16) + (10 + (char2 - 'a'));

  return (value);
}
#endif /* PROXIES_ARE_BROKEN */

DOC_SPEC *
mhttpd_resolve_location (HTTP_REQUEST *request)
{
  register int i = 0;		/* GCC is confused, and this makes it happy. */
  DOC_SPEC *spec = make_doc_spec ();
  char *path_info = request->location;
  char *temp;
  char *final_request_name = (char *)NULL;
  struct stat finfo;
  int force_redirection = 0;
  char *the_filename = (char *)NULL;

  /* Find out if this is a virtual host call.  If so, use the variables
     in that structure instead of the sv_DocumentRoot. */
  {
    char *passed_host = mhttpd_get_mime_header (request->headers, "Host");

    if (passed_host != (char *)NULL)
      {
	char *hostinfo = (char *)NULL;
	char *colon = (char *)NULL;
	int is_default = 0;

	temp = (char *)xmalloc (20 + strlen ("mhttpd-virtual-host::")
				+ strlen (passed_host));
	sprintf (temp, "mhttpd-virtual-host::%s", passed_host);
	hostinfo = pagefunc_get_variable (temp);

	if (hostinfo == (char *)NULL)
	  {
	    colon = strrchr (passed_host, ':');
	    sprintf (temp, "mhttpd-virtual-host::_default_%s",
		     colon ? colon : "");
	    hostinfo = pagefunc_get_variable (temp);
	    is_default = 1;
	  }

	if (hostinfo != (char *)NULL)
	  {
	    Package *p = alist_to_package (hostinfo);
	    Package *mhttpd = symbol_get_package ("mhttpd");
	    Symbol *sym;

	    symbol_copy_package (p, symbol_get_package ("mhtml"));

	    sym = symbol_lookup_in_package (p, "SessionDB");
	    if ((sym != (Symbol *)NULL) && (sym->values != (char **)NULL))
	      forms_set_tag_value_in_package
		(mhttpd, "session-database-file", sym->values[0]);
	    
	    sym = symbol_lookup_in_package (p, "PerRequestFun");
	    if ((sym != (Symbol *)NULL) && (sym->values != (char **)NULL))
	      forms_set_tag_value_in_package
		(mhttpd, "per-request-function", sym->values[0]);
	  }

	if (is_default)
	  {
	    char *env_port = (char *)getenv ("SERVER_PORT");
	    if (colon == (char *)NULL)
	      pagefunc_set_variable ("mhtml::server-port",
				     env_port ? env_port : "80");
	    else
	      {
		pagefunc_set_variable ("mhtml::server-port", colon + 1);
		*colon = '\0';
	      }

	    pagefunc_set_variable ("mhtml::server-name", passed_host);
	  }

	mhttpd_reset_server_variables ();
	free (temp);
	temp = (char *)NULL;
      }
  }

  if ((path_info == (char *)NULL) || (path_info[0] == '\0'))
    path_info = "/";

  spec->requested_path = strdup (path_info);
  path_info = strdup (path_info);

  /* Remove client only info from the end of PATH_INFO. */
  if ((temp = strchr (path_info, '#')) != (char *)NULL)
    *temp = '\0';

  /* If there is a query string, remove it from the path info, and save the
     information for later. */
  if ((temp = strchr (path_info, '?')) != (char *)NULL)
    {
      *temp = '\0';
      temp++;
      spec->query_string = strdup (temp);
    }

#if defined (PROXIES_ARE_BROKEN)
  cleanup (path_info);
#endif

  /* Gobble the session ID. */
  {
    int pre_len = strlen (path_info);

    mhtml_get_sid (request, path_info);
    mhtml_cookie_compatible = mhtml_check_cookie_compatible (request);

    /* If we ate the SID out of the URL, pretend that this client
       supports cookies.  The function mhttpd_set_mhtml_variables
       will do the right thing for those clients which do not support
       cookies. */
    if (strlen (path_info) != pre_len)
      final_request_name = strdup (path_info);
  }

  /* Modify path_info according to any local rules. */
  temp = path_info;
  path_info = mhtml_path_translate (path_info);
  free (temp);
  temp = path_info;

  /* The following magic hack makes badly munged URLs work.  I don't like
     it, but it is the path of least resistance. */
  if ((temp[0] == '/') && (temp[1] == '%') && (temp[2] == '7') &&
      ((temp[3] == 'e') || (temp[3] == 'E')))
    {
      temp[1] = '~';
      memmove (temp + 2, temp + 4, strlen (temp + 3));
    }

  /* Now do username lookup. */
  temp = mhtml_user_translate (path_info);
  if (temp != (char *)NULL)
    the_filename = temp;
  else
    {
      char *discard = (char *)NULL;
      /* the_filename = mhtml_concat (2, sv_DocumentRoot, path_info); */
      the_filename = mhtml_canonicalize_file_name
	(path_info, sv_DocumentRoot, "", &discard);
    }

#if defined (HAVE_INTERNAL_URLS)
  /* If this is an internal URL, look it up right away.
     Internal URLs have an asterisk as the first character following the
     leading slash. */
  if (path_info[0] == '/' && path_info[1] == '*')
    {
      temp = url_internal_find_url (path_info);
      if (temp != (char *)NULL)
	{
	  free (path_info);
	  path_info = temp;
	  spec->mime_type = "metahtml/internal";
	  spec->doc_type = doc_PARSEABLE_MHTML;
	}
    }
#endif /* HAVE_INTERNAL_URLS */
      
  /* If PATH_INFO doesn't end in a slash, and the last component of
     it is a directory, make it end in a slash. */
  if ((path_info[0] != '\0') && (path_info[strlen (path_info) - 1] != '/'))
    {
      if ((stat (the_filename, &finfo) != -1) && (S_ISDIR (finfo.st_mode)))
	{
	  STRAPPEND (path_info, "/");
	  STRAPPEND (the_filename, "/");

	  /* We have to force a redirection here.  Without it, we cannot
	     move correctly within subdirectories. */
	  force_redirection++;
	}
    }

  /* If no PATH_INFO, or if is a directory, then redirect to the first
     file in that directory which satisfies our criteria. */
  if (path_info[strlen (path_info) - 1] == '/')
    {
      int the_filename_len = strlen (the_filename);

      if (the_filename[the_filename_len - 1] == '/')
	{
	  /* the_filename[the_filename_len - 1] = '\0'; */

	  if ((stat (the_filename, &finfo) != -1) && (S_ISDIR (finfo.st_mode)))
	    {
	      int use_this_name = 0;
	      char *stat_path = (char *)NULL;
	      char **default_filenames =
		symbol_get_values ("mhtml::default-filenames");

	      if (default_filenames != (char **)NULL)
		{
		  for (i = 0; default_filenames[i] != (char *)NULL; i++)
		    {
		      stat_path =
			mhtml_concat (2, the_filename, default_filenames[i]);

		      if (stat (stat_path, &finfo) != -1)
			{
			  use_this_name++;
			  break;
			}
		      else
			free (stat_path);
		    }
		}

	      if (use_this_name)
		{
		  /* The name under which this document was found differs
		     from the requested name.  Set the logical name to
		     indicate the difference. */
		  if (!spec->logical_path)
		    spec->logical_path = strdup (spec->requested_path);

		  /* Remove the query string from the logical path first. */
		  temp = strchr (spec->logical_path, '?');
		  if (temp != (char *)NULL)
		    *temp = '\0';

		  if (spec->logical_path[strlen (spec->logical_path) - 1]
		      != '/')
		    {
		      temp = mhtml_concat
			(3, spec->logical_path, "/", default_filenames[i]);
		    }
		  else
		    {
		      temp = mhtml_concat
			(2, spec->logical_path, default_filenames[i]);
		    }

		  /* If there is a query string, put it back. */
		  if (spec->query_string != (char *)NULL)
		    {
		      char *temp1;
		      temp1 = mhtml_concat (3, temp, "?", spec->query_string);
		      free (temp);
		      temp = temp1;
		    }

		  /* Now update the logical URL. */
		  free (spec->logical_path);
		  spec->logical_path = temp;
		  free (path_info);
		  path_info = strdup (temp);
		  free (the_filename);
		  the_filename = stat_path;
		}
	      else
		{
		  /* We cannot allow a redirection command to take place.
		     The URL simply could not be resolved, so it should
		     result in Not Found. */
		  force_redirection = 0;
		}
	    }
	}
    }

  /* Find out if this pathname has an extension.  Without one, we need
     to select one from the list of values in the ACCEPT header. */
  temp = strrchr (the_filename, '/');

  if (temp != (char *)NULL)
    {
      temp = strchr (temp, '.');

      /* If no extension, start trying to find the file by mime-type. */
      if (temp == (char *)NULL)
	{
	  char *extensions[] =
	  { ".html", ".mhtml", ".jpeg", ".jpg", ".gif", (char *)NULL };

	  for (i = 0; extensions[i] != (char *)NULL; i++)
	    {
	      char *doc = mhtml_concat (2, the_filename, extensions[i]);

	      if (stat (doc, &finfo) != -1)
		{
		  free (the_filename);
		  the_filename = doc;
		  break;
		}
	      else
		free (doc);
	    }
	}
    }

  /* Try hard to find out if this URL maps to a CGI program.
     The checks that appear below should take place in the order:
       1) Exact filename match
       2) Extension match.
       3) CGI directory match.
       4) Meta-HTML extension match. */
  {
    char **cgi_paths;
    char *the_template;
    int template_len;
    int executable_found = 0;

    /* Exact filename? */
    cgi_paths = symbol_get_values ("mhtml::cgi-urls");

    for (i = 0; cgi_paths && cgi_paths[i]; i++)
      {
	the_template = cgi_paths[i];
	template_len = strlen (the_template);

	if ((strncmp (the_template, path_info, template_len) == 0) &&
	    ((path_info[template_len] == '\0') ||
	     (path_info[template_len] == '/')))
	  {
	    /* This URL is specified as an executable.  Indicate that
	       in the specification, and set spec->path_info accordingly. */
	    register int j = template_len;

	    if (path_info[j] != '\0')
	      spec->path_info = strdup (path_info + j);
	    path_info[j] = '\0';
	    spec->doc_type = doc_EXTERNAL_CGI;
	    executable_found++;
	    break;
	  }
      }

    /* If this filename does not exactly map to a CGI executable, it
       still might be the case that the filename extension allows it
       to be executed.  Check for that case. */
    if (!executable_found)
      {
	char **cgi_exts = symbol_get_values ("mhtml::cgi-extensions");

	for (i = 0; cgi_exts && cgi_exts[i]; i++)
	  {
	    char *loc = strstr (path_info, cgi_exts[i]);

	    if (loc != (char *)NULL)
	      {
		/* Gotcha!  Indicate that this URL is executable, and
		   set spec->path_info accordingly. */

		loc += strlen (cgi_exts[i]);

		while ((*loc != '\0') && (*loc != '/')) loc++;

		if (*loc != '\0')
		  {
		    spec->path_info = strdup (loc);
		    *loc = '\0';
		  }

		spec->doc_type = doc_EXTERNAL_CGI;
		executable_found++;
		break;
	      }
	  }
      }

    /* Finally, check for a directory which contains CGI programs. */
    if (!executable_found)
      {
	cgi_paths = symbol_get_values ("mhtml::cgi-directories");

	for (i = 0; cgi_paths && cgi_paths[i]; i++)
	  {
	    the_template = cgi_paths[i];
	    template_len = strlen (the_template);

	    if (strncmp (the_template, path_info, template_len) == 0)
	      {
		/* This prefix indicates that the next pathname component
		   is an executable file.  Indicate that in the specification
		   and set spec->path_info accordingly. */
		register int j;

		for (j = template_len;
		     (path_info[j] != '\0' && path_info[j] != '/');
		     j++);

		if (path_info[j] == '/')
		  {
		    spec->path_info = strdup (path_info + j);
		    path_info[j] = '\0';
		  }

		spec->doc_type = doc_EXTERNAL_CGI;
		executable_found++;
		break;
	      }
	  }
      }

    /* If not identified as an executable, it may be that this document
       is to be parsed by Meta-HTML.  When that is the case, we still must
       strip off the path-info and arguments. */
    if (!executable_found)
      {
	char **cgi_exts = symbol_get_values ("mhtml::metahtml-extensions");

	for (i = 0; cgi_exts && cgi_exts[i]; i++)
	  {
	    char *loc = strstr (path_info, cgi_exts[i]);

	    if (loc != (char *)NULL)
	      {
		/* Gotcha!  Indicate that this URL is parseable by Meta-HTML,
		   and set spec->path_info accordingly. */

		loc += strlen (cgi_exts[i]);

		while ((*loc != '\0') && (*loc != '/')) loc++;

		if (*loc == '/')
		  {
		    spec->path_info = strdup (loc);
		    *loc = '\0';
		  }

		spec->doc_type = doc_PARSEABLE_MHTML;
		spec->mime_type = "metahtml/interpreted";
		break;
	      }
	  }
      }
  }

  /* If the document type has not been set, then this is a basic
     document.  Set the spec accordingly. */
  if (spec->doc_type == doc_NONE)
    spec->doc_type = doc_BASIC;

  /* If there is PATH_INFO, then find it in the filename, and remove it. */
  if (spec->path_info != (char *)NULL)
    {
      int l = strlen (spec->path_info);
      int y = strlen (the_filename) - l;

      if (strcmp (the_filename + y, spec->path_info) == 0)
	the_filename[y] = '\0';
    }

  /* From here on out, the physical path will not change its location.
     We store it permanently into the spec. */
  spec->physical_path = strdup (the_filename);

  /* Set the return mime-type of this document if not already set. */
  if (spec->mime_type == (char *)NULL)
    {
      char *ext = strrchr (spec->physical_path, '.');
      char *lookup = mhtml_concat (2, "mime-type::", ext ? ext : ".default");
      char *value = pagefunc_get_variable (lookup);

      free (lookup);
      spec->mime_type = value;
      if ((spec->mime_type != (char *)NULL) &&
	  (strcasecmp (spec->mime_type, "metahtml/interpreted") == 0))
	{
	  spec->doc_type = doc_PARSEABLE_MHTML;
	}
    }

  /* If the file is executable, gobble up the argument info that may
     appear after the executable image's name. */
  if ((spec->doc_type == doc_EXTERNAL_CGI) ||
       (spec->doc_type == doc_PARSEABLE_MHTML))
    {
      char *arguments = spec->query_string;

      if (arguments == (char *)NULL)
	arguments = spec->path_info;

      if (arguments != (char *)NULL)
	mhtml_gobble_argv (spec, arguments);
    }

  free (path_info);

  /* Force a redirection to take place if that is required. */
  if (force_redirection)
    spec->doc_type = doc_SERVER_REDIRECT;

  /* Lastly, change the name of the request if we have determined that
     it could be defined more elegantly. */
  if (final_request_name)
    {
      free (spec->requested_path);
      spec->requested_path = final_request_name;
    }

  return (spec);
}

#if defined (__cplusplus)
}
#endif
