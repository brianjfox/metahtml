/* globals.c: Global configuration variables. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Nov 10 15:29:18 1995.  */

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
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <ctype.h>
#include <xmalloc/xmalloc.h>
#include <bprintf/bprintf.h>
#include "pages.h"
#include "symbols.h"
#include "forms.h"
#include "parser.h"
#include "http.h"
#include "path_resolve.h"
#include "globals.h"
#include "logging.h"

#if defined (HAVE_GETPWNAM)
#  include <pwd.h>
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

#if !defined (errno)
extern int errno;
#endif

#if defined (USE_SSL)
#  define PROTO
#  include "ssl.h"
#  undef PROTO
static SSL *mhttpd_SSL = (SSL *)NULL;
#endif

/* The current version of the Meta-HTML engine. */
char *sv_MHTML_VERSION = MHTML_VERSION_STRING;
char *sv_VersionString = SERVER_VERSION;

/* The document root directory. */
char *sv_DocumentRoot = "";

/* The session ID as gobbled from the URL, or from the HTTP Cookie.  This
 is the value of the variable "SID". */
char *gbl_passed_sid = (char *)NULL;

/* When non-zero, debugging messages are written to debug log. */
int mhttpd_debugging = 0;

/* When non-zero, performance messages are written to debug log. */
int mhttpd_log_performance = 0;

/* When non-zero, requests that have a referer are written to referer log. */
int mhttpd_log_referer = 0;

/* When non-zero, write the name of the connecting browser to the agent log. */
int mhttpd_log_agent = 0;

/* When non-null, this is the name of a Meta-HTML defsubst to run for
   each request the server receives. */
char *mhttpd_per_request_function = (char *)NULL;

/* When non-zero, this server communicates with the client using SSL
   security. */
int mhttpd_ssl_server = 0;

/* When non-zero, this server and client can handle http cookies.
   MHTML::COOKIE-COMPATIBLE. */
int mhtml_cookie_compatible = 0;

/* Return non-zero if this client is known to eat cookies. */
static int
client_eats_cookies (char *name)
{
  int result = 0;

  if (name != (char *)NULL)
    {
      register int i;
      char **clients = symbol_get_values ("mhtml::cookie-eating-clients");

      if (clients != (char **)NULL)
	{
	  for (i = 0; clients[i] != (char *)NULL; i++)
	    {
	      if (strncmp (name, clients[i], strlen (clients[i])) == 0)
		{
		  result = 1;
		  break;
		}
	    }
	}
    }
  return (result);
}

int
mhtml_check_cookie_compatible (HTTP_REQUEST *request)
{
  char *cookie = mhttpd_get_mime_header (request->headers, "Cookie");
  int compat_p = 0;

  if (cookie != (char *)NULL)
    compat_p = 1;
  else
    {
      char *client_name;

      client_name = mhttpd_get_mime_header (request->headers, "User-Agent");

      compat_p = client_eats_cookies (client_name);
    }

  return (compat_p);
}

static char *
substring (char *string, int start, int end)
{
  char *result = (char *)xmalloc (1 + end - start);

  strncpy (result, string + start, end - start);
  result[end - start] = '\0';

  return (result);
}

char *
mhtml_concat (int count, ...)
{
  BPRINTF_BUFFER *buffer;
  char *result = (char *)NULL;
  va_list args;

  va_start (args, count);

  buffer = bprintf_create_buffer ();

  while (count--)
    {
      char *arg = va_arg (args, char *);

      bprintf (buffer, "%s", arg ? arg : "");
    }

  result = buffer->buffer;
  free (buffer);

  return (result);
}

/* Get the filename extension of FILENAME. */
char *
mhtml_filename_extension (char *filename)
{
  char *extension = (char *)NULL;
  char *slash = strrchr (filename, '/');
  char *dot = strrchr (filename, '.');

  if ((dot != (char *)NULL) &&
      ((slash == (char *)NULL) || (slash < dot)))
    extension = substring (filename, dot - filename, strlen (filename));

  return (extension);
}

char *
mhtml_path_translate (char *path)
{
  char *result = (char *)NULL;

  if (symbol_lookup_in_package (mhtml_user_keywords, "mhtml::path-translate"))
    {
      PAGE *page = page_create_page ();
      bprintf (page, "<mhtml::path-translate %s>", path);
      page_process_page (page);

      if (page)
	{
	  result = page->buffer;
	  free (page);
	}
    }

  if (result == (char *)NULL)
    {
      char *pt = pagefunc_get_variable ("MHTML::PATH-TRANSLATIONS");

      result = strdup (path);

      if (pt != (char *)NULL)
	{
	  register int i, start = 0;
	  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();

	  bprintf (buffer, "%s", path);

	  /* PATH_TRANSLATIONS is a comma separated list of strings.
	     Even strings are the thing to look for, and odd strings
	     are the thing to replace it with.  We don't let the page
	     engine do the work because we don't want to deal with
	     gnarly regular expressions. */
	  while (pt[start])
	    {
	      char *search;
	      char *replace;
	      char *loc;

	      /* Find the extent of the SEARCH component. */
	      for (i = start; pt[i] != '\0' && pt[i] != ','; i++);

	      /* Nothing to search for? */
	      if (pt[i] == '\0')
		break;

	      search = substring (pt, start, i);
	      start = i + 1;

	      /* Find the extent of the REPLACE component. */
	      for (i = start; pt[i] != '\0' && pt[i] != ','; i++);

	      replace = substring (pt, start, i);
	      start = i;
	      if (pt[i] != '\0')
		start++;

	      /* If the search component is empty, then try the next one. */
	      if (*search == '\0')
		{
		  free (search);
		  free (replace);
		  continue;
		}

	      /* We have the search and replace strings.  Is the search string
		 in the path? */
	      {
		int offset = 0;
		int search_len = strlen (search);
		int replace_len = strlen (replace);

		while ((loc = strstr (buffer->buffer + offset, search))
		       != (char *)NULL)
		  {
		    int beg = loc - buffer->buffer;
		    int end = beg + search_len;

		    /* Okay, do the replacement. */
		    bprintf_delete_range (buffer, beg, end);
		    if (replace[0])
		      bprintf_insert (buffer, beg, replace);

		    offset = beg + replace_len;
		  }
	      }

	      free (search);
	      free (replace);
	    }
	  if (strcmp (buffer->buffer, result) != 0)
	    {
	      result = buffer->buffer;
	      free (buffer);
	    }
	  else
	    bprintf_free_buffer (buffer);
	}
    }

  return (result);
}

/* Return a newly consed string which is the result of expanding ~user in
   PATH, or NULL if there is no expansion. */
char *
mhtml_user_translate (char *path)
{
  char *result = (char *)NULL;
#if defined (HAVE_GETPWNAM)
  char *homedir = pagefunc_get_variable ("mhtml::~directory");

  if ((homedir != (char *)NULL) && ((path[0] == '/') && (path[1] == '~')))
    {
      register int i;
      char *username;
      struct passwd *entry;

      for (i = 2; (path[i] != '\0') && (path[i] != '/'); i++);

      username = (char *)xmalloc (i);
      strncpy (username, path + 2, i - 2);
      username[i - 2] = '\0';
      entry = (struct passwd *)getpwnam (username);
      free (username);

      if ((entry != (struct passwd *)NULL) && (entry->pw_dir != (char *)NULL))
	{
	  result = mhtml_concat (4, entry->pw_dir, "/", homedir, path + i);
	}
    }
#endif /* HAVE_GETPWENT */

  return (result);
}

static void
mhtml_process_incoming_cookie (char *cookie)    
{
  if ((cookie != (char *)NULL) && (*cookie != '\0'))
    {
      register int i, j, done = 0;
      Symbol *sym = symbol_intern ("mhtml::cookies");

      /* The cookie line consists of label="maybe quoted value"; label=value
	 pairs.  Split them into individual assignments, and remember each
	 assignment in the array MHTML::COOKIES.  In addition, if any
	 cookie assignment has "SID" as the label, set GBL_PASSED_SID to its
	 value. */
      j = 0;
      while (!done)
	{
	  int start, len;
	  char *value = (char *)NULL;

	  while (whitespace (cookie[j]) || (cookie[j] == ';')) j++;

	  /* Now pointing at start of label=value pair. */
	  start = j;

	  /* Advance to the equals sign. */
	  while ((cookie[j] != '\0') && (cookie[j] != '=')) j++;

	  /* If no equals sign, then no cookie. */
	  if (cookie[j] != '=')
	    break;

	  /* Skip the equals sign, and read the value which follows. */
	  j++;
	  while (whitespace (cookie[j])) j++;

	  /* Now at start of value. If quoted, skip to matching quote. */
	  if (cookie[j] == '\"')
	    {
	      int c, skip_next = 0;
	      j++;

	      while ((c = cookie[j]) != '\0')
		{
		  if (skip_next)
		    skip_next = 0;
		  else
		    {
		      if (c == '\\')
			skip_next++;
		      else if (c == '"')
			break;
		    }
		  j++;
		}
	    }

	  /* Either after quoted string, or before cookie value.
	     In either case, read until we find a semi or EOL. */
	  while ((cookie[j] != '\0') && (cookie[j] != ';')) j++;

	  /* Text from START to J comprises the entire cookie assignment.
	     Save it verbatim in MHTML::COOKIES, and check to see if it
	     is SID. */
	  len = j - start;
	  value = (char *)xmalloc (1 + len);
	  strncpy (value, cookie + start, len);
	  value[len] = '\0';
	
	  symbol_add_value (sym, value);

	  if (strncmp (value, "SID=", 4) == 0)
	    {
	      if (gbl_passed_sid) free (gbl_passed_sid);
	      gbl_passed_sid = strdup (value + 4);

	      for (i = 0; isdigit (gbl_passed_sid[i]); i++);
	      gbl_passed_sid[i] = '\0';
	    }
	  free (value);
	}
    }
}

/* Modifies PATH_INFO in place. */
void
mhtml_get_sid (HTTP_REQUEST *request, char *path_info)
{
  register int i, start = 0;

  /* Perhaps this is a client which supports HTTP Cookies.  In that
     case, get the SID from the cookie.  Gobble up any SID out of
     path_info in either case. */
  if (*path_info == '/') start = 1;
  for (i = start; path_info[i] != '\0'; i++)
    {
      if (path_info[i] == '/')
	{
	  if (gbl_passed_sid) free (gbl_passed_sid);
	  gbl_passed_sid = substring (path_info, start, i);
	  memmove (path_info, path_info + i, (1 + strlen (path_info)) - i);
	  break;
	}

      if (!isdigit (path_info[i]))
	break;
    }

  /* Check for HTTP Cookie and URL-Cookie. */
  {
    char *cookie;

    cookie = mhttpd_get_mime_header (request->headers, "URL-Cookie");
    mhtml_process_incoming_cookie (cookie);
    cookie = mhttpd_get_mime_header (request->headers, "Cookie");
    mhtml_process_incoming_cookie (cookie);
  }
}

int
mhttpd_page_redirect_p (PAGE *page)
{
  int result = 0;

  if ((page != (PAGE *)NULL) && (page->buffer != (char *)NULL))
    {
      register int i;
      register char *temp;

      for (i = 0, temp = page-> buffer; i < 25; i ++, temp ++)
	if (*temp == 'L' && strncasecmp (temp + 1, "ocation: ", 9) == 0)
	  {
	    result = 1;
	    break;
	  }
    }

  return (result);
}

void
mhtml_gobble_argv (DOC_SPEC *spec, char *string)
{
  char **argv = (char **)xmalloc (2 * sizeof (char *));
  int argc = 1;
  char *arg;

  arg = strrchr (spec->physical_path, '/');
  if (arg) arg++; else arg = spec->physical_path;
  argv[0] = strdup (arg);
  argv[1] = (char *)NULL;

#if defined (MY_SERVER_IS_SLICKER)
  if (string != (char *)NULL)
    {
      register int i, start = 0;
      int slots = 2;
      int done = 0;

      while (!done)
	{
	  for (i = start; string[i] != '\0' && string[i] !=','; i++);

	  if (i != start)
	    {
	      arg = (char *)xmalloc (1 + (i - start));
	      strncpy (arg, string + start, i - start);
	      arg[i - start] = '\0';
	      start = i;

	      if (string[start]) start++;

	      if (argc + 2 > slots)
		argv = (char **)xrealloc
		  (argv, (slots += 5) * sizeof (char *));

	      argv[argc++] = arg;
	      argv[argc] = (char *)NULL;
	    }
	  else
	    done = 1;
	}
    }
#endif /* MY_SERVER_IS_SLICKER */

  spec->argv = argv;
  spec->argc = argc;
}

Package *
mhttpd_mime_headers_to_package (MIME_HEADER **headers, char *packname)
{
  Package *pack = symbol_get_package (packname);

  if (headers)
    {
      register int i;
      MIME_HEADER *h;

      for (i = 0; (h = headers[i]) != (MIME_HEADER *)NULL; i++)
	forms_set_tag_value_in_package (pack, h->tag, h->value);
    }

  return (pack);
}

int
mhttpd_read (int fd, void *buf, size_t nbytes)
{
  int result = 0;

  if (nbytes < 1)
    return (0);

#if defined (USE_SLL)
  if (mhttpd_ssl_server)
    result = SSL_read (mhttpd_SSL, (char *)buf, (unsigned int)nbytes);
  else
#endif
    {
      int done = 0, offset = 0, times_zero = 0;

      while (!done)
	{
	  int count;

	  count = read (fd, buf + offset, nbytes);

	  if (count < 0)
	    {
	      if (errno == EINTR)
		continue;
	      else
		{
		  done = 1;
		  break;
		}
	    }

	  if (count == 0)
	    {
	      times_zero++;
	      if (times_zero > 4) /* How fucking arbitrary! */
		{
		  result = offset;
		  done = 1;
		}
	    }
	  else
	    {
	      offset += count;
	      nbytes -= count;

	      if (nbytes < 1)
		{
		  result = offset;
		  break;
		}
	    }
	}
    }

  return (result);
}

int
mhttpd_write (int fd, const void *buf, size_t nbytes)
{
  int result = 0;

#if defined (USE_SLL)
  if (mhttpd_ssl_server)
    result = SSL_write (mhttpd_SSL, (char *)buf, (unsigned int)nbytes);
  else
#endif
    {
      int select_result = 0;
#if defined (FD_SET)
      struct timeval timeout = { 0, 0 };
      fd_set except_fds;
      int intr = 0;

      FD_ZERO (&except_fds);
      FD_SET (fd, &except_fds);

      while (intr < 2)
	{
	  select_result =
	    select (fd + 1, (fd_set *)0, (fd_set *)0, &except_fds, &timeout);

	  if ((select_result == -1) && (errno == EINTR))
	    intr++;
	  else
	    break;
	}
#endif /* FD_SET */

      if (select_result == 0)
	result = write (fd, buf, nbytes);
    }
  return (result);
}

char *
mhttpd_gets (char *str, size_t size, int fd)
{
  char *result = (char *)NULL;
  register int i, amount_read;
  char c;
  int limit = size - 1;

#if defined (USE_SSL)
  if (mhttpd_ssl_server && mhttpd_SSL && (mhttpd_SSL->fd == fd))
    {
      for (i = 0; i < limit; i++)
	{
	  amount_read = SSL_read (mhttpd_SSL, &c, 1);

	  if (amount_read < 1)
	    break;
	  else if (c == EOF)
	    break;
	  else
	    {
	      str[i] = c;
	      if (c == '\n')
		{
		  i++;
		  break;
		}
	    }
	}

      if (i != 0)
	{
	  str[i] = '\0';
	  result = str;
	}
    }
  else
#endif
    {
      for (i = 0; i < limit; i++)
	{
	  int select_result = 1;
#if defined (FD_SET)
	  struct timeval timeout = { 60, 0 };
	  fd_set read_fds;

	  FD_ZERO (&read_fds);
	  FD_SET (fd, &read_fds);

	  select_result =
	    select (fd + 1, &read_fds, (fd_set *)0, (fd_set *)0, &timeout);
#endif

	  if (select_result == 1)
	    amount_read = read (fd, &c, 1);
	  else
	    amount_read = 0;

	  if (amount_read < 1)
	    break;
	  else
	    {
	      str[i] = c & 0x7f;
	      if (c == '\n')
		{
		  i++;
		  break;
		}
	    }
	}

      if (i != 0)
	{
	  str[i] = '\0';
	  result = str;
	}
    }

  return (result);
}

void
mhttpd_negotiate_ssl (int connection)
{
#if defined (USE_SSL)

  SSL_set_fd (mhttpd_SSL, connection);

  /* Accept (i.e., negotiate) the connection. */
  SSL_accept (mhttpd_SSL);

#else
  /* Not compiled to use SSL, so just barf. */
  exit (3);
#endif
}

void
mhttpd_initialize_ssl (void)
{
#if defined (USE_SSL)
  /* Create a SSL struct for the library to use, and specify the file
     descriptor to communicate over. */
  mhttpd_SSL = (SSL *)SSL_new ();

  /* Specify the location of the file containing the  private key. */
  SSL_use_RSAPrivateKey_file
    (mhttpd_SSL, "server.rsa", SSL_CT_X509_CERTIFICATE);

  /* Specify the file containing our certificate. */
  SSL_use_certificate_file
    (mhttpd_SSL, "server.cert", SSL_CT_X509_CERTIFICATE);
#else
  mhttpd_log (log_ERROR, "Server not compiled with SSL: cannot make secure connections");
#endif
}

#if defined (__cplusplus)
}
#endif
