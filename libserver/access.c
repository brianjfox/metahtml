/* access.c: -*- C -*-  Access handlers for mhttpd. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jan 24 08:18:47 1996.  */

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
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include <regex.h>
#include "pages.h"
#include "forms.h"
#include "session.h"
#include "parser.h"
#include "http.h"
#include "globals.h"
#include "logging.h"
#if defined (HAVE_CRYPT_H)
#include <crypt.h>
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

#define HAVE_CRYPT 1

static int access_match (Package *package, HTTP_RESULT *result);
static int document_protected_p (HTTP_RESULT *result);
static char *access_realm (char *filename);
static int access_allowed_p (HTTP_RESULT *result, char *filename);
static int password_match (char *clear, char *password);

/* Returns 1 if access is allowed for the request of RESULT, zero
   otherwise.  This can also complain and deny access using WWW Basic
   authorization protocols. */
int
mhttpd_check_access (HTTP_RESULT *result)
{
  Package *package;
  int some_allowed = 0;

  /* You are allowed access to non-existant documents. */
  if (!result->request || !result->spec)
    return (1);

  /* First, check to see if the requested document is in a protected
     area.  If it is, this function fills in the page with the correct
     HTTP magic to force authorization. */
  if (document_protected_p (result))
    return (0);

  package = symbol_lookup_package ("mhttpd-allow");

  if (package != (Package *)NULL)
    {
      some_allowed = 1;
      mhttpd_log (log_DEBUG, "Checking allowed hosts: ");

      if (access_match (package, result) == 1)
	return (1);
    }

  package = symbol_lookup_package ("mhttpd-deny");
  if (package != (Package *)NULL)
    {
      mhttpd_log (log_DEBUG, "Checking denied hosts: ");

      if (access_match (package, result) == 1)
	return (0);
    }
  else if (some_allowed)
    return (0);

  return (1);
}


static int
access_match (Package *package, HTTP_RESULT *result)
{
  Symbol **syms = symbols_of_package (package);

  if (syms != (Symbol **)NULL)
    {
      register int i;
      Symbol *sym;
      char *url;
      char *method;
      int mlen;

      if ((url = result->spec->logical_path) == (char *)NULL)
	url = result->spec->requested_path;

      if (!url)
	return (0);

      method = result->request->method;
      mlen = strlen (method);

      for (i = 0; (sym = syms[i]) != (Symbol *)NULL; i++)
	{
	  regex_t re;
	  regmatch_t offsets[2];
	  int matched;
	  int start = 0;
	  char *name;

	  if ((sym->name == (char *)NULL) ||
	      (sym->values == (char **)NULL) ||
	      (sym->values[0] == (char *)NULL))
	    continue;

	  name = sym->name;

	  if (name[0] == '*')
	    start = 1;
	  else if ((name[0] == '.') && (name[1] == '*'))
	    start = 2;
	  else if ((strncasecmp (name, method, mlen) == 0) &&
		   (name[mlen] == '|'))
	    start = mlen + 1;

	  if (start == 0)
	    continue;

	  regcomp (&re, name + start, REG_EXTENDED | REG_ICASE);
	  matched = (regexec (&re, url, 1, offsets, 0) == 0);
	  mhttpd_log (log_DEBUG, "   Match Var: %s against %s: %d",
		      url, name + start, matched);

	  regfree (&re);

	  if (matched)
	    {
	      regcomp (&re, sym->values[0], REG_EXTENDED);
	      matched =
		(regexec (&re, result->request->requester, 1, offsets, 0)
		 == 0);
	      mhttpd_log (log_DEBUG, "  Match Host: %s against %s: %d",
			  result->request->requester, sym->values[0], matched);

	      regfree (&re);

	      if (matched)
		return (1);
	    }
	}
    }

  return (0);
}

/* Return non-zero if the document referenced in RESULT->spec is within a
   protected area, and the user is not authorized to be in that area.
   Side-effect RESULT by placing the appropriate HTTP magic in RESULT->PAGE,
   and by changing the result code. */
static int
document_protected_p (HTTP_RESULT *result)
{
  char *location = result->spec ? result->spec->physical_path : (char *)NULL;
  int answer = 0;

  /* See if this directory contains a .mhttpd_access file.  If so,
     then the documents in this directory require authorization. */
  if (location != (char *)NULL)
    {
      static char access_filename[1024];
      char *temp;

      if (strlen (location) < 1000)
	{
	  strcpy (access_filename, location);
	  temp = strrchr (access_filename, '/');

	  if (temp != (char *)NULL)
	    {
	      char *realm = (char *)NULL;

	      temp++;
	      strcpy (temp, ".mhttpd_access");

	      realm = access_realm (access_filename);

#if defined (HTACCESS_COMPATIBLE)
	      /* If no .mhttpd_access file here, check for .htaccess. */
	      if (realm == (char *)NULL)
		{
		  char *htaccess_user_file;

		  strcpy (temp, ".htaccess");
		  htaccess_user_file = parse_htaccess_file (access_filename);

		  if (htaccess_user_file)
		    {
		      temp = strrchr (access_filename, '/');
		      if (temp)
			{
			  *temp = 0;
			  realm = strrchr (access_filename, '/');
			  if (realm) realm = strdup (1 + realm);
			  strcpy (access_filename, htaccess_user_file);
			}
		    }
		}
#endif /* HTACCESS_COMPATIBLE */
			  
	      /* If there is a realm, and the user is not authenticated
		 yet, arrange to have that done. */
	      if ((realm != (char *)NULL) &&
		  !access_allowed_p (result, access_filename))
		{
		  /* Not authorized.  Ask for it. */
		  result->result_code = res_UNAUTHORIZED;
		  result->page = page_create_page ();
		  bprintf (result->page,
			   "WWW-Authenticate: Basic realm=\"%s\"\n\n", realm);

		  answer = 1;
		}
	    }
	}
    }

  return (answer);
}

/* Get the access realm from FILENAME, the name of an mhttpd_access file. */
static char *recent_access_realm = (char *)NULL;
static int recent_access_realm_size = 0;

static char *
access_realm (char *filename)
{
  FILE *stream = fopen (filename, "r");
  int found = 0;
  char line[512];

  if (stream != (FILE *)NULL)
    {
      while (fgets (line, 511, stream) != (char *)NULL)
	{
	  if (*line == '#')
	    continue;

	  if (strncasecmp (line, "Realm:", 6) == 0)
	    {
	      char *start = line + 6;
	      char *end;
	      int quoted = 0;

	      while (whitespace (*start)) start++;
	      if (*start == '"')
		{
		  start++;
		  quoted++;
		}
	      end = start;
	      while (*end)
		{
		  if (quoted)
		    {
		      if (*end == '"')
			break;
		    }
		  else
		    {
		      if (whitespace (*end))
			break;
		    }

		  end++;
		}

	      if (((end - start) + 1) > recent_access_realm_size)
		recent_access_realm =
		  (char *)xrealloc (recent_access_realm,
				    (recent_access_realm_size =
				     1 + (end - start)));
  
	      strncpy (recent_access_realm, start, end - start);
	      recent_access_realm[end - start] = '\0';
	      found = 1;
	      break;
	    }
	}

      fclose (stream);
    }

  if (found)
    return (recent_access_realm);
  else
    return ((char *)NULL);
}

/* Return non-zero if access is allowed for the caller in RESULT based
   on the contents of FILENAME. */
static int
access_allowed_p (HTTP_RESULT *result, char *filename)
{
  char *auth;
  int answer = 0;

  auth = mhttpd_get_mime_header (result->request->headers, "Authorization");

  if (auth != (char *)NULL)
    {
      register int i;
      FILE *stream = (FILE *)NULL;
      char buffer[256];
      char *decoded;
      char *file_password;
      char *username, *password;

      /* Skip past "Basic", and any following whitespace. */
      for (i = 5; whitespace (auth[i]); i++);

      /* Get the decoded string. */
      decoded = mhtml_base64decode (auth + i, (int *)NULL);

      mhttpd_log (log_DEBUG, "Decoded authorization string: `%s'", decoded);

      /* Check the authorization string here. */
      username = decoded;
      password = strchr (username, ':');
      if (password != (char *)NULL)
	{
	  *password = '\0';
	  password++;
	}

      pagefunc_set_variable_readonly ("env::remote_user", username);
      pagefunc_set_variable_readonly ("mhtml::remote-user", username);

      /* Open the file and find the username entry. */
      stream = fopen (filename, "r");

      if (stream)
	{
	  i = strlen (username);
	  while (fgets (buffer, 255, stream) != (char *)NULL)
	    {
	      if ((strncmp (username, buffer, i) == 0) &&
		  ((buffer[i] == ':') || whitespace (buffer[i])))
		{
		  file_password = buffer + i + 1;
		  for (i = strlen (file_password) - 1; i > -1; i--)
		    if (!whitespace (file_password[i]))
		      break;

		  file_password[++i] = '\0';

		  /* If no password in file, none required. */
		  if (!*file_password)
		    {
		      answer = 1;
		    }
		  else
		    {
		      if (password_match (password, file_password) == 0)
			answer = 1;
		    }
		  break;
		}
	    }
	  fclose (stream);
	}

      free (decoded);
    }

  return (answer);
}

#if defined (HAVE_CRYPT)
/* Passed a cleartext string and an encrypted password, encrypt the
   cleartext string with the salt of the encrypted password, and
   compare the results.  Returns what strcmp returns. */
static int
password_match (char *clear, char *password)
{
  int length = (13 * ((strlen (clear) + 7) / 8));
  char *encrypted = (char *)xmalloc (1 + length);
  char *clear_p = clear;
  char *pass_p = password;
  int result = -1;

  encrypted[0] = '\0';

  while (length > 0)
    {
      char salt[3] = { '\0', '\0', '\0' };
      char chunk[9];
      char *temp;

      salt[0] = pass_p[0];
      salt[1] = pass_p[1];

      strncpy (chunk, clear_p, 8);
      chunk[8] = (char)0;

      temp = crypt (chunk, salt);
      strcat (encrypted, temp);

      clear_p += 8;
      pass_p += 13;
      length -= 13;
    }

  mhttpd_log (log_DEBUG, "Clear: `%s', encrypted: `%s'", clear, encrypted);
  mhttpd_log (log_DEBUG, "Password from file: `%s'", password);

  result = strcmp (encrypted, password);
  free (encrypted);
  return (result);
}
#else /* !HAVE_CRYPT */
static int
password_match (char *clear, char *password) { return (0); }
#endif /* !HAVE_CRYPT */


#if defined (__cplusplus)
}
#endif
