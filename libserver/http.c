/* http.c: Guts of a HTTP server. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Nov 10 18:14:08 1995.  */

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

#if defined (sgi)
#  include <bstring.h>
#  define vfork fork
#endif

#if defined (__CYGWIN32__)
/* Needed for select(). */
#  include <sys/socket.h>
#  define vfork fork
#endif /* __CYGWIN32__ */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include "signal_defs.h"
#include <ctype.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include "pages.h"
#include "forms.h"
#include "session.h"
#include "parser.h"
#include "http.h"
#include "access.h"
#include "path_resolve.h"
#include "globals.h"
#include "logging.h"
#include "streamfuncs.h"

#if defined (__cplusplus)
extern "C"
{
#endif

#if defined (__CYGWIN32__)
typedef RETSIGTYPE (*sig_t) (int);
#endif /* __CYGWIN32__ */

#if !defined (errno)
extern int errno;
#endif

extern char **environ;

/* Yechh!  This is solely for making the Open Market Fast CGI implementation
   work without having to recompile this library separately for the engine.
   This makes me feel a little sick. */
char *fast_cgi_content = (char *)NULL;
int fast_cgi_content_length = 0;

static void mhttpd_fill_in_missing_headers (HTTP_RESULT *result);
static PAGE *mhttpd_find_page_for_result (HTTP_RESULT *result);
static void mhttpd_handle_empty_page (HTTP_RESULT *result);

#define word_separator(x) whitespace(x)

static HTTP_RESULT *http_get_handler (ReqFunArgs);
static HTTP_RESULT *http_head_handler (ReqFunArgs);
static HTTP_RESULT *http_post_handler (ReqFunArgs);
static HTTP_RESULT *http_put_handler (ReqFunArgs);
static HTTP_RESULT *http_delete_handler (ReqFunArgs);
static HTTP_RESULT *http_link_handler (ReqFunArgs);
static HTTP_RESULT *http_unlink_handler (ReqFunArgs);
static HTTP_RESULT *http_unimplemented_handler (ReqFunArgs);

static METHOD_HANDLER method_handlers[] = {
  { "GET",	http_get_handler },
  { "HEAD",	http_head_handler },
  { "POST",	http_post_handler },
  { "PUT",	http_put_handler },
  { "DELETE",	http_delete_handler },
  { "LINK",	http_link_handler },
  { "UNLINK",	http_unlink_handler },
  { (char *)NULL, (ReqFun *)NULL }
};

void
http_log_error (char *format, ...)
{
}

char *
http_readline (int fd)
{
  static char *buffer = (char *)NULL;
  static int buffer_size = 0;
  char *line;

  if (buffer == (char *)NULL)
    buffer = (char *)xmalloc (buffer_size = 2048);

  line = mhttpd_gets (buffer, buffer_size, fd);

  if (line != (char *)NULL)
    {
      int l = strlen (line) - 1;

      while ((l > -1) && ((line[l] == '\n') || (line[l] == '\r')))
	{
	  line[l] = '\0';
	  l--;
	}
    }
  return (line);
}

MIME_HEADER **
mime_headers_from_string (char *string, int *last_char)
{
  register int start, i;
  MIME_HEADER **result = (MIME_HEADER **)NULL;
  int result_index = 0;
  int result_slots = 0;
  static char *line = (char *)NULL;
  static int line_size = 0;
  int done = 0;

  if (last_char != (int *)NULL)
    *last_char = 0;

  if (!string)
    return ((MIME_HEADER **)NULL);

  start = 0;

  while (!done)
    {
      int length = 0;
      int colon_seen = 0;

      for (i = start; string[i]; i++)
	{
	  if (string[i] == ':')
	    colon_seen++;
	  else if (!colon_seen && (!isalpha (string[i]) && (string[i] !='-')))
	    {
	      done = 1;
	      break;
	    }
	  else if (string[i] == '\n')
	    {
	      int c = string[i + 1];

	      /* Only check for continuation lines if we're in a header. */
	      if (!colon_seen)
		{
		  done = 1;
		  break;
		}

	      /* Handle continuation lines. */
	      if ((c == ' ') || (c == '\t'))
		{
		  while (whitespace (string[i])) i++;
		  i--;
		}
	      else
		{
		  length = i - start;
		  i++;

		  if ((c == '\n') || (c == '\0'))
		    done = 1;

		  break;
		}
	    }
	}

      if (length == 0)
	{
	  done = 1;
	}
      else
	{
	  char *tag, *value;

	  if (line_size < (1 + length))
	    line = (char *)xrealloc (line, (line_size += (256 + length)));

	  strncpy (line, string + start, length);
	  line[length] = '\0';
	  tag = line;
	  value = strchr (line, ':');

	  if (!value)
	    {
	      done = 1;
	    }
	  else
	    {
	      MIME_HEADER *header;

	      start = i;
	      header = (MIME_HEADER *)xmalloc (sizeof (MIME_HEADER));

	      *value = '\0';
	      value++;
	      while (whitespace (*value)) value++;

	      header->tag = strdup (tag);
	      header->value = strdup (value);

	      {
		register int j = strlen (value) - 1;

		while (j > -1 && whitespace (value[j])) j--;
		j++;
		value[j] = '\0';
	      }
	      
	      if (result_index + 2 > result_slots)
		result = (MIME_HEADER **)xrealloc
		  (result, (result_slots += 10) * sizeof (MIME_HEADER *));

	      result[result_index++] = header;
	      result[result_index] = (MIME_HEADER *)NULL;
	    }
	}

      if (last_char != (int *)NULL)
	*last_char = start;
    }

  return (result);
}

MIME_HEADER **
mime_headers_from_fd (int fd)
{
  char *line;
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  MIME_HEADER **result = (MIME_HEADER **)NULL;
  int ignore;

  while ((line = http_readline (fd)) != (char )NULL)
    {
      if (!*line)
	{
	  break;
	}
      else
	{
	  bprintf (buffer, "%s\n", line);
	}
    }

  result = mime_headers_from_string (buffer->buffer, &ignore);
  bprintf_free_buffer (buffer);
  return (result);
}

char *
mhttpd_get_mime_header (MIME_HEADER **headers, char *which)
{
  char *result = (char *)NULL;

  if (headers != (MIME_HEADER **)NULL)
    {
      register int i;

      for (i = 0; headers[i] != (MIME_HEADER *)NULL; i++)
	{
	  if (strcasecmp (which, headers[i]->tag) == 0)
	    {
	      result = headers[i]->value;
	      break;
	    }
	}
    }

  return (result);
}

HTTP_REQUEST *
http_read_request (int fd)
{
  char *line = http_readline (fd);
  HTTP_REQUEST *req = (HTTP_REQUEST *)NULL;

  if (line != (char *)NULL)
    {
      req = http_parse_request (line);
      req->headers = mime_headers_from_fd (fd);
    }
  return (req);
}

static char *
gobble_word (char *line, int *start)
{
  register int i;
  char *result = (char *)NULL;

  /* Skip word separators. */
  for (i = *start; line[i] && word_separator (line[i]); i++);

  *start = i;

  while (line[i] && !word_separator (line[i])) i++;

  result = (char *)xmalloc (1 + (i - *start));
  strncpy (result, line + *start, (i - *start));
  result[i - *start] = '\0';

  *start = i;
  return (result);
}

static HTTP_REQUEST *
http_make_request (void)
{
  HTTP_REQUEST *request = (HTTP_REQUEST *)xmalloc (sizeof (HTTP_REQUEST));

  memset (request, 0, sizeof (HTTP_REQUEST));

  return (request);
}

void
mhttpd_free_request (HTTP_REQUEST *request)
{
  if (request)
    {
      if (request->method) free (request->method);
      if (request->location) free (request->location);
      if (request->protocol) free (request->protocol);
      if (request->protocol_version) free (request->protocol_version);
      if (request->requester_addr) free (request->requester_addr);
      if (request->requester) free (request->requester);

      free (request);
    }
}

HTTP_REQUEST *
http_parse_request (char *line)
{
  int start = 0;
  HTTP_REQUEST *req = (HTTP_REQUEST *)NULL;

  if (line != (char *)NULL)
    {
      char *version;

      req = http_make_request ();

      /* Get method. */
      req->method = gobble_word (line, &start);

      /* Get location. */
      req->location = gobble_word (line, &start);

      /* Get protocol. */
      req->protocol = gobble_word (line, &start);
      if ((req->protocol == (char *)NULL) || (req->protocol[0] == '\0'))
	{
	  if (req->protocol != (char *)NULL)
	    free (req->protocol);

	  req->protocol = strdup ("HTTP/1.0");
	}

      version =  strchr (req->protocol, '/');

      if (version)
	{
	  *version = '\0';
	  version++;
	  req->protocol_version = strdup (version);
	}
      else
	req->protocol_version = strdup ("1.0");
    }

  return (req);
}

static METHOD_HANDLER *
http_find_method_handler (HTTP_REQUEST *request)
{
  METHOD_HANDLER *result = (METHOD_HANDLER *)NULL;

  if (request)
    {
      register int i;

      for (i = 0; method_handlers[i].name; i++)
	{
	  if (strcasecmp (method_handlers[i].name, request->method) == 0)
	    {
	      result = &(method_handlers[i]);
	      break;
	    }
	}
    }
  return (result);
}

HTTP_RESULT *
http_handle_request (HTTP_REQUEST *request, int fd)
{
  METHOD_HANDLER *mh = http_find_method_handler (request);
  HTTP_RESULT *result = (HTTP_RESULT *)NULL;
  char *remote_ident = pagefunc_get_variable ("mhtml::remote-ident");
  long when = (long) time ((time_t *)0);

  int refered_to = 0;

  if (mh)
    result = (*mh->handler) (request, fd);
  else
    result = http_unimplemented_handler (request, fd);

  /* Log the access. */
  {
    BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
    char *authuser = pagefunc_get_variable ("env::authuser");

    bprintf (buffer, "%s %s %s [%s] ",
	     request->requester,
	     remote_ident ? remote_ident : "-",
	     authuser ? authuser : "-",
	     mhttpd_date_format (when));

    bprintf (buffer, "\"%s %s %s/%s\" ",
	     request->method ? request->method : "?",
	     request->location ? request->location : "?",
	     request->protocol ? request->protocol : "?",
	     request->protocol_version ? request->protocol_version : "?");

    bprintf (buffer, "%d %ld", result->result_code,
	     result->page ? result->page->bindex : 0);

    mhttpd_log_verbatim (log_ACCESS, "%s\n", buffer->buffer);
    bprintf_free_buffer (buffer);
  }

  /* If we are logging the referer from the server, and the request
     has a MIME header of "Referer" in it, then log that now. */
  if (mhttpd_log_referer)
  {
    char *referer = mhttpd_get_mime_header (request->headers, "Referer");

    if (referer)
      {
	/* Carefully ignore those references which just don't matter.
	   This includes references to images, self-pointing references,
	   and the like. */
	int should_log = 1;

	if (result->spec && result->spec->mime_type &&
	    (strncasecmp (result->spec->mime_type, "image", 5) == 0))
	  should_log = 0;

	if (should_log)
	  {
	    char *temp = strchr (referer, ':');

	    if (temp && temp[1] == '/' && temp[2] == '/')
	      {
		char *sname = pagefunc_get_variable ("mhtml::server-name");

		if ((sname != (char *)NULL) &&
		    (strncasecmp (temp + 3, sname, strlen (sname)) == 0))
		  should_log = 0;
	      }
	  }

	if (should_log)
	  {
	    refered_to = 1;
	    mhttpd_log_verbatim
	      (log_REFERER, "%s -> %s\n", referer, request->location);
	  }
      }
  }

  if (mhttpd_log_agent)
    {
      char *agent = mhttpd_get_mime_header (request->headers, "User-Agent");

      if (agent)
	mhttpd_log_verbatim (log_AGENT, "%s\n", agent);
    }

  if (!result->page)
    mhttpd_handle_empty_page (result);

  if (pagefunc_get_variable ("mhtml::server-pushed") == (char *)NULL)
    mhttpd_fill_in_missing_headers (result);

  return (result);
}

static PAGE *
mhttpd_find_page_for_result (HTTP_RESULT *result)
{
  PAGE *page = (PAGE *)NULL;
  char *value = (char *)NULL;
  char temp[20];

  sprintf (temp, "%d", result->result_code);
  pagefunc_set_variable ("mhttpd::result-code", temp);
  pagefunc_set_variable ("mhttpd::requested-url",
			 result->spec->logical_path ?
			 result->spec->logical_path :
			 result->spec->requested_path);
  pagefunc_set_variable ("mhtml::version", sv_VersionString);
  pagefunc_set_variable
    ("mhttpd::result-reason",
     mhttpd_result_reason (result->result_code));
  pagefunc_set_variable
    ("mhttpd::request-method",
     result->request ? result->request->method : "UNKNOWN");

  /* Try to find the default document for this host. */
  {
    UserFunction *uf = (UserFunction *)NULL;
    char *host = mhttpd_get_mime_header (result->request->headers, "Host");

    if (host != (char *)NULL)
      {
	BPRINTF_BUFFER *funname = bprintf_create_buffer ();
	bprintf (funname, "%s::default-document", host);
	if ((uf = mhtml_find_user_function (funname->buffer)) != NULL)
	  {
	    bprintf_insert (funname, 0, "<");
	    bprintf (funname, ">");
	    value = mhtml_evaluate_string (funname->buffer);
	  }
	bprintf_free_buffer (funname);
      }

    if ((uf == (UserFunction *)NULL) &&
	((uf = mhtml_find_user_function ("mhttpd::default-document")) != NULL))
      value = mhtml_evaluate_string ("<mhttpd::default-document>");
  }

  if (!empty_string_p (value))
    {
      page = page_create_page ();
      page_set_contents (page, value);
    }

  if (value != (char *)NULL) free (value);

  if (!page)
    {
      char *dir = pagefunc_get_variable ("mhtml::mhttpd-pages");
      BPRINTF_BUFFER *path = bprintf_create_buffer ();

      if (!dir)
	dir = sv_DocumentRoot;

      bprintf (path, "%s/Results/result-%d.mhtml", dir, result->result_code);

      page = page_read_template (path->buffer);
      free (path->buffer);
    }

  if (page)
    page_process_page (page);

  return (page);
}

static char *
downcase (char *string)
{
  register int i = 0;
  static char *buffer = (char *)NULL;
  static int size = 0;
  int len = string ? strlen (string) : 0;

  if ((2 + len) > size)
    buffer = (char *)xrealloc (buffer, (size += (2 + len)));

  if (string)
    for (i = 0; string[i] != '\0'; i++)
      {
	if (isupper (string[i]))
	  buffer[i] = tolower (string[i]);
	else
	  buffer[i] = string[i];
      }

  buffer[i] = '\0';

  return (buffer);
}

static void
mhttpd_handle_empty_page (HTTP_RESULT *result)
{
  char *result_string;

  if (!result->result_code)
    result->result_code = res_INTERNAL_SERVER_ERROR;

  result_string = mhttpd_result_string (result->result_code);

  /* Try hard to get a page which is used to describe this error.
     If the user has supplied one, use that, otherwise, use our
     internal ones. */
  if (!result->page)
    result->page = mhttpd_find_page_for_result (result);

  if (!result->page)
    {
      result->page = page_create_page ();

      bprintf (result->page, "<html><head><title>%s</title></head>\n",
	       result_string);

      bprintf (result->page, "<body>\n<p><h2>%s</h2><hr><br>\n",
	       result_string);
      bprintf (result->page, "This message generated by Mhttpd.\n");
      bprintf (result->page, "</body>\n</html>");
    }

  if (!mhttpd_page_redirect_p (result->page))
    {
      bprintf_insert (result->page, 0, "%s\nContent-type: text/html\n\n",
		      result_string);
      page_clean_up (result->page);
    }
}

#if defined (NOT_USED)
static char *
mr_get_variable (HTTP_RESULT *result, char *name)
{
  return (pagefunc_get_variable (name));
}
#endif /* NOT_USED */

static char *
mr_mime_version (HTTP_RESULT *result, char *ignore)
{
  /* I don't think we need to return the MIME version style that
     we are using, so I'm not going to. */
#if 0
  return ("1.0");
#else
  return ((char *)NULL);
#endif
}

static char *
mr_server (HTTP_RESULT *result, char *ignore)
{
  if (!ignore)
    return (sv_VersionString);
  else
    return ((char *)NULL);
}

static char *
mr_date (HTTP_RESULT *result, char *mr_offset)
{
  long ticks = (long) time ((time_t *)0);

  if (mr_offset != (char *)NULL)
    {
      long offset = (long) mr_offset;
      ticks += offset;
    }

  return (http_date_format (ticks));
}

static char *
mr_set_cookie (HTTP_RESULT *result, char *ignore)
{
  char *answer = (char *)NULL;
  char *temp;
  char *permanent_p = pagefunc_get_variable ("mhtml::permanent-cookies");
  char *no_cookies_p = pagefunc_get_variable ("mhtml::never-send-cookies");

  if (!empty_string_p (no_cookies_p))
    return ((char *)NULL);

  /* Set the SID cookie if that makes sense.
     I think it is important to pass the set-cookie command to each
     client in any case, since that client may be able to eat them.
     Then, when they talk back to us, they can pass the cookie back
     in, and we'll detect that they are using cookies, and thus won't
     have to put the SID in the URL.  Cool, huh? */

  /* First off, only do this if the browser didn't pass the cookie in,
     or, if the browser did pass the cookie in, if the variable
     mhtml::permanent-cookies is not set. */
  temp = (char *)NULL;
  {
    char **browser_cookies = symbol_get_values ("mhtml::cookies");

    if (browser_cookies != (char **)NULL)
      {
	register int i;

	for (i = 0; browser_cookies[i] != (char *)NULL; i++)
	  if (strncmp (browser_cookies[i], "SID=", 4) == 0)
	    {
	      temp = browser_cookies[i];
	      break;
	    }
      }
  }

  if ((temp == (char *)NULL) || (permanent_p == (char *)NULL))
    {
      if ( /* mhtml_cookie_compatible && */
	  ((temp = pagefunc_get_variable ("SID")) != (char *)NULL))
	{
	  BPRINTF_BUFFER *cookie_buffer = bprintf_create_buffer ();
	  char *sid_path = pagefunc_get_variable ("mhtml::sid-prefix");
	  long ticks;

	  if (!sid_path)
	    {
	      sid_path = pagefunc_get_variable ("mhtml::relative-prefix");

	      if (!sid_path)
		sid_path = "/";
	    }

	  bprintf (cookie_buffer, "SID=%s; path=%s; expires=", temp, sid_path);

	  ticks = (long) time ((time_t *)0);

	  if (*temp)
	    {
	      int timeout = 60 * session_get_timeout ((session_id_t) temp);

	      if (timeout > 0)
		{
		  if (permanent_p)
		    ticks = 1293825599;	/* Fri Dec 31 11:59:59 2010 */
		  else
		    ticks += timeout;
		}
	      else
		{
		  ticks += 1000;
		}
	    }
	  else
	    ticks -= 1000;

	  bprintf (cookie_buffer, "%s", http_date_format (ticks));
	  answer = cookie_buffer->buffer;
	  free (cookie_buffer);
	}
    }

  return (answer);
}

static char *
mr_content_length (HTTP_RESULT *result, char *ignore)
{
  static char num[40];

  if ((result->page->bindex == 1) &&
      (result->result_code == res_MOVED_TEMPORARILY))
    {
      num[0] = '0';
      num[1] = '\0';
    }
  else
    sprintf (num, "%d", result->page->bindex);

  return (num);
}

static char *
mr_content_type (HTTP_RESULT *result, char *existing_type)
{
  if (result->spec->mime_type && (existing_type == (char *)NULL) &&
      (strcmp (result->spec->mime_type, "metahtml/interpreted") == 0))
    return ("text/html");

  if (!existing_type)
    return (result->spec->mime_type);

  return (existing_type);
}

static char *
mr_expires (HTTP_RESULT *result, char *ignore)
{
  char *answer = (char *)NULL;
  int expire_page_p = 0;

  if ((result->result_code == res_SUCCESS) &&
      ((result->spec->doc_type == doc_EXTERNAL_CGI) ||
       ((result->spec->doc_type == doc_PARSEABLE_MHTML) &&
	(pagefunc_get_variable ("mhtml::cacheable") == (char *)NULL))))
    expire_page_p = 1;

  if (expire_page_p)
    {
      /* Boy, would I like to make the thing expire one second ago, or what?
	 Of course I would.  But that slimy, stinking, load of pig manure
	 that is called Netscape/1.1N doesn't pay any attention to the
	 seconds part of the time... after all, what's a second or two
	 amongst friends?  So, I thought to make it expire one minute ago,
	 but naturally, that didn't work on 1.1N either.  Losers.  So,
	 I made it expire 24 hours ago, which, if you ask me, is most
	 heinous, considering the Last-Modified date could be later
	 than that.  I fucking hate you, you monopolizing piece of
	 Netscape feces. */
      /* answer = mr_date (result, (char *)-1); Pscyhe!! */
      answer = mr_date (result, (char *)- (60 * 60 * 24));
    }
  else
    {
      char *ttlstr = pagefunc_get_variable ("mhtml::cacheable");
      long offset_seconds = ttlstr ? atol (ttlstr) : 864000; /* 10 days. */

      if (offset_seconds == 0)
	offset_seconds = 864000;

      answer = mr_date (result, (char *)offset_seconds);
    }

  return (answer);
}

static char *
mr_last_modified (HTTP_RESULT *result, char *ignore)
{
  char *answer = (char *)NULL;

  if (result->result_code == res_SUCCESS)
    {
      if (result->spec->doc_type == doc_BASIC)
	{
	  struct stat finfo;

	  if (stat (result->spec->physical_path, &finfo) != -1)
	    {
	      long ticks = (long)finfo.st_mtime;
	      answer = http_date_format (ticks);
	    }
	}
      else
	answer = mr_date (result, (char *)-60); /* Changed one minute ago. */
    }

  return (answer);
}

/* This does two things.  First, it forces an existing location string to
   appear before the content length or type.  Second, it inserts the 
   protocol specification if it isn't present. */
static char *
mr_location (HTTP_RESULT *result, char *existing_location)
{
  if ((existing_location != (char *)NULL) &&
      !url_has_protocol_p (existing_location))
    {
      char *protocol = downcase (result->request->protocol);
      char *fproto = pagefunc_get_variable ("mhtml::forced-protocol");
      char *portvar = pagefunc_get_variable ("mhtml::server-port");
      int port = portvar ? atoi (portvar) : 80;
      char *server_name = pagefunc_get_variable ("mhtml::server-name");
      char *extra = pagefunc_get_variable ("rewriter::engine-webpath");
      char *newloc;

      if (empty_string_p (fproto))
	{
	  char *env_https = (char *)getenv ("HTTPS");

	  if (env_https == (char *)NULL)
	    env_https = (char *)getenv ("REDIRECT_HTTPS");
	  if ((env_https != (char *)NULL) &&
	      (strcasecmp (env_https, "on") == 0))
	    {
	      fproto = "https";
	      port = 443;
	    }
	}

      if (extra == (char *)NULL) extra = "";

      newloc = (char *)xmalloc
	(100 + strlen (extra) + strlen (existing_location));

      if (!empty_string_p (fproto))
	protocol = fproto;

      sprintf (newloc, "%s://%s", protocol, server_name);

      /* This is on purpose.  Don't add ":80" to the url, even if the 
	 protocol isn't "http".  It makes various cases of virtual servers
	 work correctly. */
      if (port != 80)
	{
	  if (((strcasecmp (protocol, "http") == 0) && (port != 80)) ||
	      ((strcasecmp (protocol, "https") == 0) && (port != 443)))
	    sprintf (newloc + strlen (newloc), ":%d", port);
	}
      strcat (newloc, extra);
      strcat (newloc, existing_location);
      existing_location = newloc;
    }
  return (existing_location);
}

/* We always say something about the fact that this is the Meta-HTML engine.
   Otherwise, it isn't directly obvious when connecting to a user's site. */
static char *
mr_engine (HTTP_RESULT *result, char *ignore)
{
  return (sv_VersionString);
}

/* If the user has supplied a status, organize things so that this is the
   status returned. */
static char *
mr_status (HTTP_RESULT *result, char *status_text)
{
  if (status_text != (char *)NULL)
    {
      int status_value = atoi (status_text);

      if (status_value != 0)
	result->result_code = status_value;
    }

  return (status_text);
}

#if defined (USE_KEEP_ALIVE)
/* This is in conjunction with CONNECTION. */
static char *
mr_keep_alive (HTTP_RESULT *result, char *existing_value)
{
  if ((result->request->flags & flag_KEEP_ALIVE) &&
      (result->result_code == 200))
    {
      if (!existing_value)
	{
	  char *temp =
	    mhttpd_get_mime_header (result->request->headers, "Connection");

	  if ((temp != (char *)NULL) && (strcasecmp (temp, "Keep-Alive") == 0))
	    existing_value = "max=10 timeout=5";
	}
    }
  else
    {
      existing_value = (char *)NULL;
      result->request->flags &= ~flag_KEEP_ALIVE;
    }

  return (existing_value);
}

/* If the client requested that the connection be kept alive, and the
   request was successfully served, add the Keep-Alive flag. */
static char *
mr_connection (HTTP_RESULT *result, char *existing_value)
{
  char *answer = (char *)NULL;

  if ((result->request->flags & flag_KEEP_ALIVE) &&
      (result->result_code == res_SUCCESS))
      answer = "Keep-Alive";
  else
    result->request->flags &= ~flag_KEEP_ALIVE;

  return (answer);
}

#endif /* USE_KEEP_ALIVE */

typedef char *GFunc (HTTP_RESULT *, char *);
typedef struct
{
  char *mime_name;		/* The name of the header. */
  GFunc *value_generator;	/* Function to get this value. */
  int call_anyway;		/* Non-zero means always call this function. */
} MIME_RESOLVER;

/* For CALL_ANYWAY, a value of zero means only call the resolver function if
   this header was already present in the outgoing headers, a non-zero value
   means always call this function, and a value of -1 means keep original
   value as well as modified value. */

static MIME_RESOLVER mime_resolvers[] = {
  { "MIME-Version",	mr_mime_version, 0 },
  { "Server",		mr_server, 0 },
  { "Date",		mr_date, 0 },
  { "Set-cookie",	mr_set_cookie, -1 },
  { "Expires",		mr_expires, 0 },
  { "Last-modified",	mr_last_modified, 0 },
  { "Location",		mr_location, 1 },
  { "Content-length",	mr_content_length, 0 },
  { "Meta-HTML-Engine",	mr_engine, 1 },
  { "Content-type",	mr_content_type, 1 },
  { "Status",		mr_status, 1 },

#if defined (USE_KEEP_ALIVE)
  { "Connection",	mr_connection, 1 },
  { "Keep-Alive",	mr_keep_alive, 1 },
#endif /* USE_KEEP_ALIVE */

  { (char *)NULL,	(GFunc *)NULL, 0 }
};

static void
mhttpd_fill_in_missing_headers (HTTP_RESULT *result)
{
  register int i;
  MIME_HEADER **present = (MIME_HEADER **)NULL;
  BPRINTF_BUFFER *response = bprintf_create_buffer ();
  int end_of_headers;

  /* If this page already has an HTTP result line, snarf the result
     code. */
  if (result->page && result->page->buffer &&
      strncasecmp (result->page->buffer, "HTTP", 4) == 0)
    {
      char *buffer = result->page->buffer;
      for (i = 0; !whitespace (buffer[i]); i++);
      result->result_code = atoi (buffer + i);
      for (; buffer[i] != '\0' && buffer[i] != '\n'; i++);
      if (buffer[i] == '\n') i++;
      bprintf_delete_range (result->page, 0, i);
    }

  if (!result->page)
    result->page = page_create_page ();

  present = mime_headers_from_string (result->page->buffer, &end_of_headers);

  if (end_of_headers)
    bprintf_delete_range (result->page, 0, end_of_headers);

  /* Next, make sure that the required headers are there. */
  for (i = 0; mime_resolvers[i].mime_name != (char *)NULL; i++)
    {
      int val_initially_present = 1;
      char *orig_value =
	mhttpd_get_mime_header (present, mime_resolvers[i].mime_name);
      char *value = orig_value;

      if (value == (char *)NULL)
	{
	  val_initially_present = 0;
	  value = (*mime_resolvers[i].value_generator) (result, (char *)NULL);
	}
      else if (mime_resolvers[i].call_anyway)
	{
	  value = (*mime_resolvers[i].value_generator) (result, value);
	}
	
      if (value)
	bprintf (response, "%s: %s\n", mime_resolvers[i].mime_name, value);

      if (val_initially_present && (mime_resolvers[i].call_anyway > -1))
	*orig_value = '\0';
    }

  /* Okay, put the response code back. */
  bprintf_insert (response, 0, "%s\n",
		  mhttpd_result_string (result->result_code));

  /* Now, insert all of the headers that were present, but that were not
     required. */
  for (i = 0; present && present[i]; i++)
    if ((present[i]->value != (char *)NULL) && (present[i]->value[0] != '\0'))
      bprintf (response, "%s: %s\n", present[i]->tag, present[i]->value);

  /* If the logical location differs from the physical location,
     tell the client what the name should really be. */
  if ((result->result_code < 300) &&
      (result->spec && result->spec->logical_path) &&
      (!mhttpd_get_mime_header (present, "Location")))
    {
      char *protocol = downcase (result->request->protocol);
      char *fproto = pagefunc_get_variable ("mhtml::forced-protocol");
      char *portvar = pagefunc_get_variable ("mhtml::server-port");
      int port = portvar ? atoi (portvar) : 80;
      char *server_name = pagefunc_get_variable ("mhtml::server-name");
      char *rewrite_prefix=pagefunc_get_variable ("rewriter::engine-webpath");
      char *env_https = (char *)getenv ("HTTPS");

      if (env_https == (char *)NULL)
	env_https = (char *)getenv ("REDIRECT_HTTPS");

      if ((empty_string_p (fproto)) && (env_https != (char *)NULL))
	{
	  fproto = "https";
	  port = 443;
	}

      if (fproto) protocol = fproto;

      bprintf (response, "Location: %s://%s", protocol, server_name);

      /* This is on purpose.  Don't add ":80" to the url, even if the 
	 protocol isn't "http".  It makes various cases of virtual servers
	 work correctly. */
      if (port != 80)
	{
	  if (((strcasecmp (protocol, "http") == 0) && (port != 80)) ||
	      ((strcasecmp (protocol, "https") == 0) && (port != 443)))
	    bprintf (response, ":%d", port);
	}

      bprintf (response, "%s%s\n", rewrite_prefix ? rewrite_prefix : "",
	       result->spec->logical_path);
    }

  if (mhttpd_debugging)
    mhttpd_log (log_DEBUG, "\nHeaders Returned:\n%s", response->buffer);

  bprintf_insert (result->page, 0, "%s\n", response->buffer);
  bprintf_free_buffer (response);

#if 0
  if (present)
    free_mime_headers (present);
#endif
}

void
mhttpd_debug_request (HTTP_REQUEST *req)
{
  register int i;
  BPRINTF_BUFFER *b = bprintf_create_buffer ();

  bprintf (b, "Request:\n");

  bprintf (b, "struct HTTP_REQUEST *%0xl = {\n", (unsigned long) req);
  bprintf (b, "           char *method = %s\n", req->method);
  bprintf (b, "         char *location = %s\n", req->location);
  bprintf (b, "         char *protocol = %s\n", req->protocol);
  bprintf (b, " char *protocol_version = %s\n", req->protocol_version);
  bprintf (b, "        char *requester = %s\n", req->requester);
  bprintf (b, "   char *requester_addr = %s\n", req->requester_addr);
  bprintf (b, "              int flags = (");
  if (req->flags == 0) bprintf (b, "NONE");
  if (req->flags & flag_KEEP_ALIVE) bprintf (b, "KEEP-ALIVE");
  bprintf (b, ")\n");

  bprintf (b, "  MIME_HEADER **headers = {\n");

  for (i = 0; req->headers && req->headers[i]; i++)
    bprintf (b, "     %s: %s\n", req->headers[i]->tag, req->headers[i]->value);

  bprintf (b, "  }\n}\n");

  mhttpd_log (log_DEBUG, "%s", b->buffer);
  bprintf_free_buffer (b);
}

void
mhttpd_debug_doc_spec (DOC_SPEC *spec)
{
  BPRINTF_BUFFER *b = bprintf_create_buffer ();

  bprintf (b, "DOC_SPEC:\n");

  bprintf (b, "struct DOC_SPEC *%0xl = {\n", (unsigned long) spec);
  bprintf (b, "    char *requested_path = %s\n", spec->requested_path);
  bprintf (b, "     char *physical_path = %s\n", spec->physical_path);
  bprintf (b, "      char *logical_path = %s\n", spec->logical_path);
  bprintf (b, "         char *path_info = %s\n", spec->path_info);
  bprintf (b, "      char *query_string = %s\n", spec->query_string);
  bprintf (b, "         char *mime_type = %s\n", spec->mime_type);
  bprintf (b, "            int doc_type = ");

  {
    char *dtype = (char *)NULL;

    switch (spec->doc_type)
      {
      case doc_NONE: dtype = "NONE"; break;
      case doc_PARSEABLE_MHTML: dtype = "PARSEABLE_MHTML"; break;
      case doc_EXTERNAL_CGI: dtype = "EXTERNAL_CGI"; break;
      case doc_BASIC: dtype = "BASIC"; break;
      case doc_SERVER_REDIRECT: dtype = "SERVER_REDIRECT"; break;
      }

    bprintf (b, "%s\n", dtype);
  }
  bprintf (b, "}\n");
  mhttpd_log (log_DEBUG, "%s", b->buffer);
  bprintf_free_buffer (b);
}

static HTTP_RESULT *
mhtml_make_result (void)
{
  HTTP_RESULT *result = (HTTP_RESULT *)xmalloc (sizeof (HTTP_RESULT));

  memset (result, 0, sizeof (HTTP_RESULT));
  return (result);
}

#define add_to_env(name, value) \
  mhttpd_add_to_env (&new_env, &new_env_slots, &new_env_index, name, value)
  
static char **new_env = (char **)NULL;
static int new_env_slots = 0;
static int new_env_index = 0;

static void
mhttpd_add_to_env (char ***env, int *slots, int *ind, char *name, char *value)
{
  char *env_string;
  int s = *slots;
  int i = *ind;
  char **e = *env;

  env_string = (char *)xmalloc
    (2 + strlen (name) + (value ? strlen (value) : 1));
  strcpy (env_string, name);
  strcat (env_string, "=");
  if (value)
    strcat (env_string, value);

  if (i + 2 > s)
    e = (char **)xrealloc (e, (s += 10) * sizeof (char *));

  e[i++] = env_string;
  e[i] = (char *)NULL;
  *ind = i;
  *slots = s;
  *env = e;
}

static void
mhttpd_build_environment (HTTP_RESULT *result)
{
  MIME_HEADER **headers = result->request->headers;
  DOC_SPEC *spec = result->spec;
  char *temp;

  /* Build an environment for this executable. */
  add_to_env ("PATH", pagefunc_get_variable ("mhtml::exec-path"));
  add_to_env ("SERVER_NAME", pagefunc_get_variable ("mhtml::server-name"));
  add_to_env ("SERVER_PORT", pagefunc_get_variable ("mhtml::server-port"));
  if (((char *)getenv ("SERVER_SOFTWARE")) == (char *)NULL)
    add_to_env ("SERVER_SOFTWARE", sv_VersionString);
  else
    {
      add_to_env ("SERVER_SOFTWARE", (char *)getenv ("SERVER_SOFTWARE"));
      add_to_env ("ENGINE_SOFTWARE", sv_VersionString);
    }
  add_to_env ("REMOTE_HOST", result->request->requester);
  add_to_env ("REMOTE_ADDR", result->request->requester_addr);
  add_to_env ("REMOTE_USER", pagefunc_get_variable ("mhtml::remote-user"));
  add_to_env ("REMOTE_IDENT", pagefunc_get_variable ("mhtml::remote-ident"));
  temp = mhtml_concat
     (3, result->request->protocol, "/", result->request->protocol_version);
  add_to_env ("SERVER_PROTOCOL", temp);
  free (temp);

  if ((char *)getenv ("LD_LIBRARY_PATH") != (char *)NULL)
    add_to_env ("LD_LIBRARY_PATH", (char *)getenv ("LD_LIBRARY_PATH"));

  add_to_env ("HTTP_AUTHORIZATION",
	      mhttpd_get_mime_header (headers, "Authorization"));

  if ((char *)getenv ("AUTH_TYPE") != (char *)NULL)
    add_to_env ("AUTH_TYPE", (char *)getenv ("AUTH_TYPE"));
  else if (mhttpd_get_mime_header (headers, "Authorization"))
    add_to_env ("AUTH_TYPE", "Basic");

  add_to_env ("PATH_INFO", spec->path_info);
  add_to_env ("REQUEST_METHOD", result->request->method);

  add_to_env ("HTTP_USER_AGENT",
	      mhttpd_get_mime_header (headers, "User-Agent"));

  if (!spec->argc)
    {
      spec->argv = (char **)xmalloc (2 * sizeof (char *));
      spec->argv[1] = (char *)NULL;
      spec->argv[0] = strrchr (spec->physical_path, '/');
      if (spec->argv[0])
	(spec->argv[0])++;
      else
	spec->argv[0] = spec->physical_path;
    }

  if (spec->content_length)
    {
      char lbuff[40];

      sprintf (lbuff, "%d", spec->content_length);
      add_to_env ("CONTENT_LENGTH", lbuff);
      if (mhttpd_get_mime_header (headers, "Content-type"))
	add_to_env ("CONTENT_TYPE",
		    mhttpd_get_mime_header (headers, "Content-type"));
    }

  add_to_env ("SCRIPT_NAME", spec->argv[0]);
  add_to_env ("PATH_TRANSLATED", spec->physical_path);
  add_to_env ("QUERY_STRING", spec->query_string);

  if (mhttpd_get_mime_header (headers, "Cookie"))
    add_to_env ("HTTP_COOKIE", mhttpd_get_mime_header (headers, "Cookie"));

  if (mhttpd_get_mime_header (headers, "Referer"))
    add_to_env ("HTTP_REFERER", mhttpd_get_mime_header (headers, "Referer"));

  if (gbl_passed_sid)
    add_to_env ("SID", gbl_passed_sid);

  /* Add additional HTTP variables if present. */
  if (environ != (char **)NULL)
    {
      register int i;
      char var_and_val[256];

      for (i = 0; environ[i] != (char *)NULL; i++)
	{
	  if (((strncmp (environ[i], "HTTP", 4) == 0) ||
	       (strncmp (environ[i], "REDIRECT", 8) == 0)) &&
	      (strlen (environ[i]) < 255))
	    {
	      strcpy (var_and_val, environ[i]);
	      temp = strchr (var_and_val, '=');

	      if (temp != (char *)NULL)
		{
		  *temp = '\0';
		  temp++;
		  add_to_env (var_and_val, temp);
		}
	    }
	}
    }

  add_to_env ("WEBBASEDIR", sv_DocumentRoot);
}

static void
mhtml_read_content_from_fd (HTTP_RESULT *result, int fd)
{
  char *content_length =
    mhttpd_get_mime_header (result->request->headers, "Content-Length");

  if (content_length)
    result->spec->content_length = atoi (content_length);

  if (result->spec->content_length)
    {
#define GUESS_TRANSFER_RATE 1
#if defined (GUESS_TRANSFER_RATE)
      /* Here is a nice chance to find out how fast the connection is.
	 Simply get the time before reading the content, compare that
	 with the time when we have finished, and we will know the
	 transmission speed. We use a millisecond timer when that is
	 available. */
      struct timeval before, after;
      unsigned long diff_mills;

      gettimeofday (&before, (void *)NULL);
#endif

      /* I just hate the whole world.  Why O Why is there an extra
	 CR/LF at the end of this fucking post that doesn't show up
	 in the Content-Length? */
      result->spec->content = (char *)xmalloc
	(3 + result->spec->content_length);

      /* Handle Fast CGI cruft.  Ugh. */
#define CRLF_BUG 0
      if (fast_cgi_content_length > 0)
	memmove (result->spec->content, fast_cgi_content,
		 fast_cgi_content_length);
      else
	mhttpd_read (fd, result->spec->content,
		     result->spec->content_length + CRLF_BUG);

      result->spec->content[result->spec->content_length] = '\0';

#if defined (GUESS_TRANSFER_RATE)
      gettimeofday (&after, (void *)NULL);
      diff_mills = ((1000 * (after.tv_sec - before.tv_sec)) +
		    (after.tv_usec - before.tv_usec));

      /* Now we know how many milliseconds it takes to transfer
	 CONTENT_LENGTH bytes from the client to this server.
	 Guess we should set a variable? */
      {
	char dbuf[100];

	sprintf (dbuf, "<div %d.0 %lu>",
		 result->spec->content_length, diff_mills);
	pagefunc_set_variable ("mhtml::bytes-per-ms", dbuf);
      }
#endif
    }
}

static void
mhtml_post_to_document (char *string)
{
  if (string)
    {
      register int i;
      Package *package = symbol_get_package ("POSTED");

      forms_parse_data_string (string, package);

      /* Copy the parsed symbols into the default package. */
      {
	Package *default_pack = symbol_get_package (DEFAULT_PACKAGE_NAME);
	Symbol **symbols = symbol_package_symbols ("POSTED");
	    
	if (symbols != (Symbol **)NULL)
	  {
	    /* If maxed out on server debugging, show the contents of the
	       posted package here. */
	    char *debug_p = pagefunc_get_variable ("mhtml::debug-post");
	    int debug = 0;

	    if (!empty_string_p (debug_p)) debug = 1;

	    for (i = 0; symbols[i] != (Symbol *)NULL; i++)
	      {
		if (debug)
		  mhttpd_log
		    (log_DEBUG, "POSTED::%s = %s", symbols[i]->name,
		     symbols[i]->values[0] ? symbols[i]->values[0] : "");

		symbol_copy (symbols[i], default_pack);
	      }
	  }
      }
    }
}

static void
mhttpd_set_mhtml_variables (HTTP_RESULT *result)
{
  char *protocol = downcase (result->request->protocol);
  char *fproto = pagefunc_get_variable ("mhtml::forced-protocol");
  char *portvar = pagefunc_get_variable ("mhtml::server-port");
  int port = portvar ? atoi (portvar) : 80;
  char *server_name = pagefunc_get_variable ("mhtml::server-name");
  char *http_prefix, *temp, *physical_path, *logical_path;
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  int root_len = strlen (sv_DocumentRoot);
  int lpath_len;

  pagefunc_set_variable ("mhtml::version", sv_MHTML_VERSION);
  if (fproto != (char *)NULL) protocol = fproto;

  if (port == 80)
    bprintf (buffer, "%s://%s", protocol, server_name);
  else if ((port == 443) || ((char *)getenv ("HTTPS") != (char *)NULL))
    bprintf (buffer, "https://%s", server_name);
  else
    bprintf (buffer, "%s://%s:%d", protocol, server_name, port);

  http_prefix = buffer->buffer;
  buffer->buffer = (char *)NULL;
  buffer->bindex = buffer->bsize = 0;

  physical_path = strdup (result->spec->physical_path);
  logical_path = 
    result->spec->logical_path ?
      result->spec->logical_path : result->spec->requested_path;

  logical_path = strdup (logical_path);
  lpath_len = strlen (logical_path);

  /* If the document requested ended in a slash, and there was a suitable
     document present to handle this case, and the client is not cookie
     compatible, the cookie could appear in LOGICAL_PATH.  This is
     undesirable for the purposes of building the variable set, so we
     remove it here. */
  if (gbl_passed_sid && !mhtml_cookie_compatible)
    {
      int sidlen = strlen (gbl_passed_sid);

      if ((sidlen < lpath_len) &&
	  (strncmp (gbl_passed_sid, logical_path + 1, sidlen) == 0))
	{
	  memmove (logical_path, logical_path + 1 + sidlen,
		   strlen (logical_path + sidlen));

	  if (logical_path[0] == '\0')
	    {
	      logical_path[0] = '/';
	      logical_path[1] = '\0';
	    }
	}
    }

  /* MHTML::HTTP-TO-HOST */
  pagefunc_set_variable ("mhtml::http-to-host", http_prefix);

  /* MHTML::CURRENT-DOC */
  temp = strrchr (physical_path, '/');
  if (temp) temp++; else temp = physical_path;
  pagefunc_set_variable ("mhtml::current-doc", temp);

  /* MHTML::RELATIVE-PREFIX */
  if (strncmp (sv_DocumentRoot, physical_path, root_len) == 0)
    {
      temp = strrchr (physical_path, '/');
      if (temp)
	{
	  *temp = '\0';
	  pagefunc_set_variable
	    ("mhtml::relative-prefix", physical_path + root_len);
	}
    }
  else if (logical_path[1] != '\0')
    {
      char *x = strdup (logical_path);
      temp = strrchr (x, '/');
      if (temp)
	{
	  *temp = '\0';
	  pagefunc_set_variable ("mhtml::relative-prefix", x);
	}

      free (x);
    }

  /* MHTML::PHYSICAL-FILENAME */
  pagefunc_set_variable ("mhtml::physical-filename", physical_path);

  /* MHTML::PHYSICAL-DIRECTORY */
  pagefunc_set_variable ("mhtml::physical-directory", physical_path);
  temp = strrchr (pagefunc_get_variable ("mhtml::physical-directory"), '/');
  if (temp != (char *)NULL) *temp = '\0';

  /* MHTML::LOCATION */
  if (gbl_passed_sid && !mhtml_cookie_compatible)
    {
      bprintf (buffer, "/%s%s", gbl_passed_sid, logical_path);
      pagefunc_set_variable ("mhtml::location", buffer->buffer);

      /* mhtml::current-url is deprecated... */
      pagefunc_set_variable ("mhtml::current-url", buffer->buffer);
      bprintf_free_buffer (buffer);
      buffer = bprintf_create_buffer ();
    }
  else
    {
      pagefunc_set_variable ("mhtml::location", logical_path);

      /* mhtml::current-url is deprecated... */
      pagefunc_set_variable ("mhtml::current-url", logical_path);
    }

  /* MHTML::LOCATION-SANS-SID */
  pagefunc_set_variable ("mhtml::location-sans-sid", logical_path);

  /* mhtml::current-url-sans-sid is deprecated... */
  pagefunc_set_variable ("mhtml::current-url-sans-sid", logical_path);


  /* MHTML::FULL-URL */
  if (gbl_passed_sid && !mhtml_cookie_compatible)
    bprintf (buffer, "%s/%s%s", http_prefix, gbl_passed_sid, logical_path);
  else
    bprintf (buffer, "%s%s", http_prefix, logical_path);

  pagefunc_set_variable ("mhtml::full-url", buffer->buffer);
  
  /* MHTML::URL-TO-DIR */
  if (result->spec->path_info)
    {
      /* MHTML::PATH-INFO */
      pagefunc_set_variable ("mhtml::path-info", result->spec->path_info);
      temp = strstr (buffer->buffer, result->spec->path_info);
      if (temp) *temp = '\0';
    }
  temp = strrchr (buffer->buffer, '/');
  if (temp) *temp = '\0';
  pagefunc_set_variable ("mhtml::url-to-dir", buffer->buffer);
  bprintf_free_buffer (buffer);
  buffer = bprintf_create_buffer ();

  /* MHTML::FULL-URL-SANS-SID */
  bprintf (buffer, "%s%s", http_prefix, logical_path);
  pagefunc_set_variable ("mhtml::full-url-sans-sid", buffer->buffer);

  /* MHTML::URL-TO-DIR-SANS-SID */
  if (result->spec->path_info)
    {
      temp = strstr (buffer->buffer, result->spec->path_info);
      if (temp) *temp = '\0';
    }
  temp = strrchr (buffer->buffer, '/');
  if (temp) *temp = '\0';
  pagefunc_set_variable ("mhtml::url-to-dir-sans-sid", buffer->buffer);
  bprintf_free_buffer (buffer);
  buffer = bprintf_create_buffer ();

  /* MHTML::HTTP-PREFIX */
  if (gbl_passed_sid && !mhtml_cookie_compatible)
    {
      bprintf (buffer, "%s/%s", http_prefix, gbl_passed_sid);
      pagefunc_set_variable ("mhtml::http-prefix", buffer->buffer);
      bprintf_free_buffer (buffer);
      buffer = bprintf_create_buffer ();
    }
  else
    {
      pagefunc_set_variable ("mhtml::http-prefix", http_prefix);
    }

  /* MHTML::HTTP-PREFIX */
  pagefunc_set_variable ("mhtml::http-prefix-sans-sid", http_prefix);

  /* SID */
  if (gbl_passed_sid)
    pagefunc_set_variable ("SID", gbl_passed_sid);

  /* MHTML::COOKIE-COMPATIBLE */
  if (mhtml_cookie_compatible)
    pagefunc_set_variable ("mhtml::cookie-compatible", "true");

  pagefunc_set_variable ("mhtml::unparsed-headers", "true");

  /* MHTML::REFERER */
  if ((temp = mhttpd_get_mime_header
       (result->request->headers, "Referer")) != (char *)NULL)
    pagefunc_set_variable ("mhtml::referer", temp);

  free (http_prefix);
  free (logical_path);

  if (buffer)
    bprintf_free_buffer (buffer);

  /* Add the ENV package. */
  if (new_env == (char **)NULL)
    mhttpd_build_environment (result);

  if (new_env)
    {
      register int i;
      Package *pack = symbol_get_package ("ENV");

      for (i = 0; new_env[i] != (char *)NULL; i++)
	{
	  char *name = strdup (new_env[i]);
	  char *value = strchr (name, '=');

	  if (value)
	    {
	      *value = '\0';
	      value++;

	      forms_set_tag_value_in_package (pack, name, value);
	    }
	  free (name);
	}
    }
}

void
mhttpd_reset_server_variables (void)
{
  char *temp = (char *)NULL;
  int val = 0;

  temp = pagefunc_get_variable ("mhtml::server-port");

  if (temp != (char *)NULL)
    val = atoi (temp);

  if (val < 1)
    val = 80;

  temp = pagefunc_get_variable ("mhtml::document-root");

  if (temp != (char *)NULL)
    {
      sv_DocumentRoot = temp;
      pagefunc_set_variable ("mhtml::include-prefix", temp);

      chdir (sv_DocumentRoot);
    }

  /* Create a reasonable default PATH variable if the user didn't do so. */
  if (pagefunc_get_variable ("mhtml::exec-path") == (char *)NULL)
    {
      PAGE *page = page_create_page ();
      bprintf (page, "<set-var mhtml::exec-path =");
      bprintf (page, "<get-var mhtml::server-root>/bin:/bin:");
      bprintf (page, "<get-var mhtml::document-root>/cgi-bin:/usr/bin:");
      bprintf (page, "/usr/local/bin:/usr/ucb:/www/bin:/opt/metahtml/bin");
      bprintf (page, ">");
      page_process_page (page);

      if (page)
	page_free_page (page);
    }

  /* Now to the logfiles. */
  mhttpd_set_logfile (log_ACCESS,  "");
  mhttpd_set_logfile (log_ERROR,   "");
  mhttpd_set_logfile (log_DEBUG,   "");
  mhttpd_set_logfile (log_REFERER, "");
  mhttpd_set_logfile (log_AGENT, "");

  temp = pagefunc_get_variable ("mhtml::access-log");
  if (temp != (char *)NULL)
    mhttpd_set_logfile (log_ACCESS, temp);

  temp = pagefunc_get_variable ("mhtml::error-log");
  if (temp != (char *)NULL)
    mhttpd_set_logfile (log_ERROR, temp);

  mhttpd_debugging = 0;
  temp = pagefunc_get_variable ("mhtml::debug-log");
  if (temp != (char *)NULL)
    {
      mhttpd_set_logfile (log_DEBUG, temp);
      mhttpd_debugging = 1;
    }

  mhttpd_log_performance = 0;
  temp = pagefunc_get_variable ("mhtml::log-performance");
  if (temp != (char *)NULL)
    mhttpd_log_performance = 1;

  mhttpd_log_referer = 0;
  temp = pagefunc_get_variable ("mhtml::referer-log");
  if (temp != (char *)NULL)
    {
      mhttpd_set_logfile (log_REFERER, temp);
      mhttpd_log_referer = 1;
    }

  mhttpd_log_agent = 0;
  temp = pagefunc_get_variable ("mhtml::agent-log");
  if (temp != (char *)NULL)
    {
      mhttpd_set_logfile (log_AGENT, temp);
      mhttpd_log_agent = 1;
    }

  mhttpd_per_request_function =
    pagefunc_get_variable ("mhttpd::per-request-function");

  set_session_database_location
    (pagefunc_get_variable ("mhttpd::session-database-file"));

#if defined (MHTTPD_DNS_CACHE)
  temp = pagefunc_get_variable ("mhttpd::cache-dns");
  if (empty_string_p (temp))
    mhttpd_cache_dns = 0;
  else
    mhttpd_cache_dns = 1;
#endif
}

static void
decompose_and_post (char *enctype, char *data, int length)
{
  register int i;
  char *boundary = (char *)NULL;
  Package *package;
      
  if (!length)
    return;

  package = symbol_get_package ("POSTED");

  /* Find Boundary string.  Start by skipping past the encoding type. */
  i = 0;
  while (1)
    {
      for (; enctype[i] != '\0' && enctype[i] != ';'; i++);

      /* Now skip any whitespace. */
      if (enctype[i]) i++;
      for (; whitespace (enctype[i]); i++);

      /* Is this the "boundary" string? */
      if (strncasecmp (enctype + i, "Boundary", 8) == 0)
	{
	  boundary = enctype + i + 9;
	  for (; whitespace (*boundary); boundary++);
	  if (*boundary == '=') boundary++;
	  for (; whitespace (*boundary); boundary++);
	  boundary = strdup (boundary);

	  /* Find the end. */
	  for (i = 0;
	       ((boundary[i] != '\0') &&
		(boundary[i] != ';') && 
		(!whitespace (boundary[i])));
	       i++);
	  boundary[i] = '\0';
	  break;
	}
      else
	{
	  if (enctype[i] == '\0')
	    break;
	}
    }

  /* Okay, now find the boundary strings in the content data. */
  if (boundary != (char *)NULL)
    {
      int blen = strlen (boundary);
      int limit = (length - blen) + 1;

      i = 0;

      while (i < limit)
	{
	  if (strncmp (data + i, boundary, blen) == 0)
	    {
	      int start, end;
	      Package *temp_pack = symbol_get_package ((char *)NULL);
	      char *posted_data = (char *)NULL;
	      int posted_data_len = 0;

	      /* Found one. If it is the last one, quit now.  Otherwise,
		 skip whitespace and decode the MIME data that follows. */
	      i += blen; if (i >= limit) break;
	      while ((i < limit) && (word_separator (data[i]))) i++;

	      /* Next come a set of headers.  Bind them to their values
		 in a local package. */
	      while (1)
		{
		  char header_title[1024];
		  char header_value[1024];

		  header_title[0] = '\0';
		  header_value[0] = '\0';

		  /* At end of header information? */
		  if (((data[i] == '\r') && (data[i + 1] == '\n')) ||
		      (data[i] == '\n'))
		    {
		      if (data[i] == '\r') i++;
		      if (data[i] == '\n') i++;
		      break;
		    }

		  /* Find a header. */
		  start = i;
		  while ((data[i] != ':') && (!word_separator (data[i]))) i++;
		  end = i;
		  if ((end - start) < 1023)
		    {
		      strncpy (header_title, data + start, end - start);
		      header_title[end - start] = '\0';

		      /* Now find the value for this header. */
		      /* Skip the ":" which ended this title and any
			 following whitespace characters. */
		      if (data[i]) i++;
		      while ((i < limit) && (word_separator (data[i]))) i++;
		      start = i;
		      while ((i < limit) && (data[i] != '\n')) i++;
		      end = i;
		      if (data[end - 1] == '\r') end--;

		      if ((end - start) < 1023)
			{
			  int l = end - start;
			  strncpy (header_value, data + start, l);
			  header_value[l] = '\0';
			  l--;
			  while (whitespace (header_value[l])) l--;
			  header_value[++l] = '\0';
			}
		    }

		  /* At any rate, skip to the end of this line. */
		  while ((i < limit) && (data[i] != '\n')) i++;
		  if (data[i] == '\n') i++;

		  /* If we have both a header and a value, add them to this
		     package. */
		  if ((header_title[0] != '\0') && (header_value[0] != '\0'))
		    {
		      Symbol *sym = symbol_intern_in_package
			(temp_pack, header_title);
		      symbol_add_value (sym, header_value);
		    }
		}

	      /* We have gobbled up all of the headers into our local
		 package, and the data pointer is at the start of the
		 data for this MIME entry.  Advance to the end of that
		 data, and find out what to do with it. */
	      start = i;
	      while ((i < limit) && (strncmp (data + i, boundary, blen) != 0))
		i++;
	      end = i;

	      /* Now, it might be me, but it appears that the fucking
		 Netscape Browser has a different view of the boundary
		 string in the Content-Encoding header than it places in
		 the body.  This is really heinous, but we try to
		 handle it here. */
	      while (data[end - 1] == '-') end--;

	      if (data[end - 1] == '\n')
		{
		  --end;
		  if (data[end - 1] == '\r')
		    --end;
		}

	      /* The data corresponding to this item is delimited by the
		 indices in START and END.  The package TEMP_PACK contains
		 symbols whose names are MIME headers, and whose values
		 are the corresponding value strings.  We can only
		 handle the item if it contains a
		 Content-Disposition header. */
	      {
		Symbol *sym = symbol_lookup_in_package
		  (temp_pack, "Content-Disposition");
		char *value = (char *)NULL;

		if ((sym != (Symbol *)NULL) && (sym->values != (char **)NULL))
		  value = sym->values[0];

		if (value == (char *)NULL)
		  {
		    mhttpd_log (log_DEBUG, "Missing Content-Disposition!");
		    continue;
		  }

		/* We only know how to handle FORM-DATA types. */
		if (strncasecmp (value, "form-data;", 10) != 0)
		  {
		    mhttpd_log (log_DEBUG, "Missing `form-data;'");
		    continue;
		  }

		/* Now, find the name. */
		value += 10;
		while (whitespace (*value)) value++;

		/* Now we are expecting the name of the variable to bind. */
		if (strncasecmp (value, "name=", 5) != 0)
		  {
		    mhttpd_log (log_DEBUG,
				"Missing `name=' in Content-Disposition");
		    continue;
		  }

		value += 5;
		while (whitespace (*value)) value++;

		/* Okay, read the name of this variable. */
		{
		  register int j = 0;
		  char *name = (char *)NULL;
		  char *filename = (char *)NULL;
		  int nstart = 0;
		  int nend = 0;
		  int quoted = 0;

		  if (value[j] == '"')
		    {
		      quoted = 1;
		      j++;
		    }

		  nstart = nend = j;
		  while (value[j])
		    {
		      if (quoted && value[j] == '"')
			{
			  nend = j;
			  j++;
			  quoted = 0;
			  break;
			}

		      if (!quoted &&
			  (whitespace (value[j]) || value[j] == ';'))
			{
			  nend = j;
			  break;
			}

		      j++;
		    }

		  name = (char *)xmalloc (1 + (nend - nstart));
		  strncpy (name, value + nstart, (nend - nstart));
		  name[nend - nstart] = '\0';

		  /* We have the name of the variable.  Optionally, this
		     is a file, so save the filename if present. */
		  if (value[j] == ';')
		    {
		      j++;
		      while (whitespace (value[j])) j++;

		      if (strncasecmp (value + j, "filename=", 9) == 0)
			{
			  j += 9;

			  /* Okay, read the name of this file. */
			  if (value[j] == '"')
			    {
			      quoted = 1;
			      j++;
			    }

			  nstart = nend = j;
			  while (value[j])
			    {
			      if (quoted && value[j] == '"')
				{
				  nend = j;
				  j++;
				  quoted = 0;
				  break;
				}

			      if (!quoted && whitespace (value[j]))
				{
				  nend = j;
				  break;
				}
			      j++;
			    }

			  filename = (char *)xmalloc (1 + (nend - nstart));
			  strncpy (filename, value + nstart, (nend - nstart));
			  filename[nend - nstart] = '\0';
			}
		    }

		  posted_data_len = (end - start);
		  posted_data = (char *)xmalloc (1 + posted_data_len);
		  memcpy (posted_data, data + start, posted_data_len);
		  posted_data[posted_data_len] = '\0';

		  /* Save value and name in the posted package. */
		  if (filename != (char *)NULL)
		    {
		      Datablock *block =
			datablock_create (posted_data, posted_data_len);

		      /* Kill old definition if there is one. */
		      sym = symbol_remove_in_package (package, name);
		      symbol_free (sym);

		      /* Create new definition. */
		      sym = symbol_intern_in_package (package, name);
		      sym->type = symtype_BINARY;
		      sym->values = (char **)block;

		      /* Create a new variable called NAME-FILENAME. */
		      {
			char *newname = (char *)xmalloc (40 + strlen (name));
			sprintf (newname, "POSTED::%s-FILENAME", name);
			pagefunc_set_variable (newname, filename);

			/* Create a new variable called NAME-TYPE. */
			sprintf (newname, "POSTED::%s-TYPE", name);
			pagefunc_set_variable
			  (newname, forms_get_tag_value_in_package
			   (temp_pack, "Content-Type"));
			free (newname);
		      }
		    }
		  else
		    {
		      /* Just a regular variable.  Add this name/value to the
			 POSTED package. */
		      sym = symbol_intern_in_package (package, name);
		      symbol_add_value (sym, posted_data);
		    }

		  if (name) free (name);
		  if (filename) free (filename);
		  if (posted_data) free (posted_data);
		}
		symbol_destroy_package (temp_pack);
	      }
	    }
	  else
	    i++;
	}
      free (boundary);
    }

  /* Cause the material in the POSTED package to appear in the DEFAULT
     package as well.  We pass an empty string because there isn't anything
     new to add, we just want to copy the data that we just made. */
  mhtml_post_to_document ("");
}

static void
mhttpd_metahtml_engine (HTTP_RESULT *result)
{
  char *prologue_doc = pagefunc_get_variable ("mhtml::prologue-document");
  char *page_function = pagefunc_get_variable ("mhtml::per-page-function");
  int dont_process = 0;

  result->page = page_read_template (result->spec->physical_path);

  if (!result->page)
    result->result_code = res_NOT_FOUND;
  else
    {
      char digits[40];
      char *enctype;

      sprintf (digits, "%ld", page_most_recent_modification_time);
      pagefunc_set_variable ("mhtml::last-modification-time", digits);
      
      {
	PAGE *p = page_create_page ();
	bprintf (p, "<*parser*::push-file %s>", result->spec->requested_path);
	page_process_page (p);
	page_free_page (p);
      }

      enctype = mhttpd_get_mime_header
	(result->request->headers, "Content-Type");

      mhtml_post_to_document (result->spec->query_string);
      if (result->spec->path_info)
	{
	  if (result->spec->path_info[0] == '/')
	    mhtml_post_to_document (result->spec->path_info + 1);
	  mhtml_post_to_document (result->spec->path_info);
	}

      /* It is now possible to post a multipart/form-data document.
	 This means that the contents of a binary file is sent over
	 the net without any encoding at all.  Don't ask me why, I
	 just work here. */
      {
	Datablock *block = datablock_create
	  (result->spec->content, result->spec->content_length);
	Symbol *sym = symbol_remove ("MHTML::RAW-CONTENT-DATA");

	symbol_free (sym);
	sym = symbol_intern ("MHTML::RAW-CONTENT-DATA");
	sym->type = symtype_BINARY;
	sym->values = (char **)block;
      }

      if (!enctype ||
	  (strncasecmp (enctype, "multipart/form-data;", 20) != 0))
	{
	  mhtml_post_to_document (result->spec->content);
	}
      else
	{
	  /* Decompose the content string, and post the resultant
	     variables to the document.  */
	  decompose_and_post
	    (enctype, result->spec->content, result->spec->content_length);
	}
      
      mhttpd_set_mhtml_variables (result);

      /* Not last, and certainly not least.
	 If the user has specified a prologue document, then
	 read and execute it now. */
      if (prologue_doc)
	{
	  PAGE *page = page_read_template (prologue_doc);

	  if (page != (PAGE *)NULL)
	    {
	      page_process_page (page);

	      if (page != (PAGE *)NULL)
		{
		  if (mhttpd_page_redirect_p (page))
		    {
		      page_free_page (result->page);
		      result->page = page;
		      dont_process++;
		    }
		  else
		    page_free_page (page);
		}
	    }
	}

      /* If there is a per-page-function call, then do it now. */
      if (!dont_process && (page_function != (char *)NULL))
	{
	  PAGE *per_page = page_create_page ();
	  bprintf (per_page, "<%s> </%s>", page_function, page_function);
	  page_process_page (per_page);

	  if (per_page != (PAGE *)NULL)
	    {
	      if (mhttpd_page_redirect_p (per_page))
		{
		  page_free_page (result->page);
		  result->page = per_page;
		  dont_process++;
		}
	      else
		page_free_page (per_page);
	    }
	}

      if (!dont_process)
	{
	  char *epilogue_doc =
	    pagefunc_get_variable ("mhtml::epilogue-document");

	  if (epilogue_doc)
	    {
	      PAGE *page = page_read_template (epilogue_doc);
	      if (page != (PAGE *)NULL)
		{
		  bprintf (result->page, "%s", page->buffer);
		  page_free_page (page);
		}
	    }

	  page_process_page (result->page);
	}

      if (result->page && result->page->buffer)
	{
	  if (mhttpd_page_redirect_p (result->page))
	    {
	      result->result_code = res_MOVED_TEMPORARILY;
	    }
	  else
	    {
	      page_clean_up (result->page);
	      result->result_code = res_SUCCESS;
	    }
	}
    }
}

static int child_status = 0;

/* Release a child that has died in the normal way. */
static void
release_child (void)
{
  wait (&child_status);
}

static PAGE *
read_from_fd (int fd)
{
  PAGE *buffer = bprintf_create_buffer ();
  int select_result;
#if defined (FD_SET)
  struct timeval timeout;
  fd_set read_fds;
  int intr = 0;

  timeout.tv_sec = 90;
  timeout.tv_usec = 0;

  while (intr < 2)
    {
      FD_ZERO (&read_fds);
      FD_SET (fd, &read_fds);
      select_result = select (fd + 1, (fd_set *)(&read_fds), 0, 0, &timeout);
      if ((select_result == -1) && (errno == EINTR))
	intr++;
      else
	break;
    }
#else /* !FD_SET */
  select_result = 1;
#endif /* !FD_SET */

  switch (select_result)
    {
    case 0:
    case -1:
      break;

    default:
      {
        int amount_read;
        int done = 0;

        while (!done)
          {
            while ((buffer->bindex + 1024) > (buffer->bsize))
              buffer->buffer = (char *)xrealloc
		(buffer->buffer, (buffer->bsize += 1024));
            buffer->buffer[buffer->bindex] = '\0';

            amount_read = read (fd, buffer->buffer + buffer->bindex, 1023);

            if ((amount_read < 0) && (errno != EINTR))
              {
                done = 1;
              }
            else
              {
		if (amount_read > -1)
		  {
		    buffer->bindex += amount_read;
		    buffer->buffer[buffer->bindex] = '\0';
		  }
                if (amount_read == 0)
                  done = 1;
              }
          }
      }
    }

  return (buffer);
}

static PAGE *
mhttpd_execute_cgi (HTTP_RESULT *result, int fd)
{
  DOC_SPEC *spec = result->spec;
  PAGE *page = (PAGE *)NULL;

  /* Change the current directory to that where the program resides. */
  {
    char *new_dir = strdup (spec->physical_path);
    char *slash = strrchr (new_dir, '/');

    if (slash)
      *slash = '\0';

    chdir (new_dir);
    free (new_dir);
  }

  mhttpd_build_environment (result);

  /* Execute the program. */
  {
    pid_t child;
    int parent_to_child[2];
    int child_to_parent[2];
    int deliver_verbatim = 0;

    /* If the string "nph-" appears anywhere in the program path, then
       this CGI has "Non-Parsed Headers", and doesn't want me mucking
       about with it.  In that case, don't collect the output data at
       all, just pass it through verbatim. */
    if (strstr (result->spec->physical_path, "nph-") != (char *)NULL)
      deliver_verbatim = 1;
    
    pipe (parent_to_child);
    pipe (child_to_parent);

    child = vfork ();

    if (child != (pid_t)0)
      {
	int read_fd  = child_to_parent[0];
	int write_fd = parent_to_child[1];

	if (deliver_verbatim)
	  _exit (0);

	/* In the parent, setup pipes for communication. */
	close (parent_to_child[0]);
	close (child_to_parent[1]);

	/* Say what to do when a child dies. */
	signal (SIGCHLD, (sig_t)release_child);

	/* Say what to do if the pipe is broken. */
	signal (SIGPIPE, SIG_IGN);

	if (spec->content_length)
	  write (write_fd, spec->content, spec->content_length);
	close (write_fd);

	page = read_from_fd (read_fd);
	close (read_fd);

	if (!page->bindex)
	  {
	    page_free_page (page);
	    page = (PAGE *)NULL;

	    if (WIFEXITED (child_status))
	      {
		if (WEXITSTATUS (child_status) == 127)
		  result->result_code = res_NOT_FOUND;
	      }
	    else
	      result->result_code = res_INTERNAL_SERVER_ERROR;
	  }
      }
    else
      {
	if (!deliver_verbatim)
	  {
	    /* In the child, make stdin and stdout be our pipes. */
	    close (child_to_parent[0]);
	    close (parent_to_child[1]);
	    dup2 (parent_to_child[0], 0);
	    dup2 (child_to_parent[1], 1);
	  }
	else
	  {
	    if (mhtml_stdin_fileno != 0)  dup2 (mhtml_stdin_fileno, 0);
	    if (mhtml_stdout_fileno != 1) dup2 (mhtml_stdout_fileno, 1);
	    if (mhtml_stderr_fileno != 2) dup2 (mhtml_stderr_fileno, 2);
	  }

	execve (result->spec->physical_path, result->spec->argv, new_env);
	_exit (127);
      }
  }
  return (page);
}

/* GMT formatted dates in HTTP headers look like one of the following:

   1) Sun, 06 Nov 1994 08:49:37 GMT    ; RFC 822, updated by RFC 1123
   2) Sunday, 06-Nov-94 08:49:37 GMT   ; RFC 850, obsoleted by RFC 1036 */
static void
mhttpd_gmt_canonicalize (char *time_string)
{
  register int i;
  int dashes_found = 0;

  /* Format 1 always has day names that are longer than 3 characters.
     If this day name is longer, then truncate it. */
  for (i = 0; (time_string[i] != ','); i++);

  if (i > 3)
    memmove (time_string + 3, time_string + i, 1 + strlen (time_string + i));

  /* Now remove all `-' characters. */
  for (i = 0; time_string[i] != '\0'; i++)
    {
      if (time_string[i] == '-')
	{
	  dashes_found++;
	  time_string[i] = ' ';

	  if (dashes_found == 2)
	    {
	      /* The next two characters depict the year of the document.
		 If the year is less than 100, add 1900 to it. */
	      register int j;

	      for (j = i + 1; isdigit (time_string[j]); j++);

	      if ((j - i) < 4)
		{
		  memmove (time_string + j, time_string + i + 1,
			   strlen (time_string + i));
		  time_string[i + 1] = '1';
		  time_string[i + 2] = '9';
		}
	    }
	}
    }
}

/* Given a complete, basically ready to return document in RESULT, determine
   if we should actually return it or not.  We do this by checking for the
   presence of the If-Modified-Since header in the request, and actually
   comparing dates.  The date comparison can be done on a string-wise level,
   since the message passed to us is in GMT as taken from our message to
   the client. */
void
mhttpd_handle_if_modified_since (HTTP_RESULT *result)
{
  struct stat finfo;
  char *ims;

  /* Only do this for documents that would succeed for a simple get. */
  if (!result || !result->spec  || !result->spec->physical_path ||
      (result->result_code > 200) || !result->spec->mime_type)
    return;

  ims = mhttpd_get_mime_header (result->request->headers, "If-Modified-Since");

  if (ims == (char *)NULL)
    return;
  else
    {
      char *temp;

      /* GMT formatted dates in HTTP headers look like one of the following:

	 1) Sun, 06 Nov 1994 08:49:37 GMT    ; RFC 822, updated by RFC 1123
	 2) Sunday, 06-Nov-94 08:49:37 GMT   ; RFC 850, obsoleted by RFC 1036

	 So, canonicalize the date of the if-modified header and the
	 date of the file into the same format, and then do direct string
	 comparisons between them. */

      ims = strdup (ims);
      temp = strchr (ims, ';');
      if (temp != (char *)NULL)
	{
	  temp--;

	  while ((temp != ims) && (whitespace (*temp))) --temp;
	  temp++;
	  *temp = '\0';

	  mhttpd_gmt_canonicalize (ims);
	}
    }

  if (stat (result->spec->physical_path, &finfo) == 0)
    {
      char *filetime = http_date_format ((long) finfo.st_mtime);

      mhttpd_gmt_canonicalize (filetime);

#if 0
      mhttpd_log (log_DEBUG, "\nIf Modified Since:\n\t`%s'\n\t`%s'\n\n",
		  ims, filetime);
#endif

      if (strcasecmp (ims, filetime) == 0)
	{
	  /* The document hasn't been modified since the client last received
	     it.  Modify the return information. */
	  PAGE *discard = result->page;
	  PAGE *page = page_create_page ();

	  result->result_code = res_NOT_MODIFIED;
	  bprintf (page, "%s\n", mhttpd_result_string (res_NOT_MODIFIED));
	  bprintf (page, "Content-Type: %s\n", result->spec->mime_type);
	  bprintf (page, "Content-Length: %d\n", discard->bindex);
	  page_free_page (discard);
	  result->page = page;
	}
    }
  free (ims);
}

static HTTP_RESULT *
http_get_handler (ReqFunArgs)
{
  HTTP_RESULT *result = mhtml_make_result ();

  result->request = request;
  result->spec = mhttpd_resolve_location (request);

  if (mhttpd_debugging)
    mhttpd_debug_doc_spec (result->spec);

  mhtml_read_content_from_fd (result, mhtml_stdin_fileno);

  if (result->spec != (DOC_SPEC *)NULL)
    {
      /* If this host is allowed access for this request, do it now. */
      if (mhttpd_check_access (result))
	{
	  switch (result->spec->doc_type)
	    {
	    case doc_BASIC:
	      result->page = page_read_template (result->spec->physical_path);
	      mhttpd_handle_if_modified_since (result);
	      break;

	    case doc_PARSEABLE_MHTML:
	      mhttpd_metahtml_engine (result);
	      break;

	    case doc_EXTERNAL_CGI:
	      result->page = mhttpd_execute_cgi (result, fd);
	      break;

	    case doc_SERVER_REDIRECT:
	      {
		result->page = page_create_page ();
		bprintf (result->page, "Location: %s\n\n",
			 result->spec->logical_path);
		result->result_code = res_MOVED_TEMPORARILY;
		break;
	      }
	    }
	}
      else
	{
	  if (!result->result_code)
	    result->result_code = res_SERVICE_UNAVAILABLE;
	}

      if (!result->result_code)
	{
	  if (result->page)
	    {
	      if (mhttpd_page_redirect_p (result->page))
		result->result_code = res_MOVED_TEMPORARILY;
	      else
		result->result_code = res_SUCCESS;
	    }
	  else
	    result->result_code = res_NOT_FOUND;
	}
    }
  else
    {
      result->result_code = res_BAD_REQUEST;
    }

  return (result);
}

static HTTP_RESULT *
http_head_handler (ReqFunArgs)
{
  HTTP_RESULT *result = http_get_handler (request, fd);

  /* Okay, this is for Art Medlar, that little persnickity know-it-all. */
  if ((result->result_code != res_BAD_REQUEST) &&
      (result->page != (PAGE *)NULL) && (result->page->buffer != (char *)NULL))
    {
      register int i;
      char *temp = result->page->buffer;
      int limit = result->page->bindex;
      int found = 0;

      for (i = 0; i < (limit - 1); i++)
	if ((temp[i] == '\n') &&
	    ((temp[i + 1] == '\n')  ||
	     ((temp[i + 1] == '\r') && (temp[i + 2] == '\n'))))
	  {
	    found++;

	    if (temp[i + 1] == '\r') i++;

	    result->page->bindex = i + 2;
	    temp[i + 2] = '\0';
	    break;
	  }

      if (!found)
	{
	  result->page->bindex = 0;
	  result->page->buffer[0] = '\0';
	}
    }
  return (result);
}

static HTTP_RESULT *
http_post_handler (ReqFunArgs)
{
  return (http_get_handler (request, fd));
}
  
static HTTP_RESULT *
http_put_handler (ReqFunArgs)
{
  return (http_unimplemented_handler (request, fd));
}

static HTTP_RESULT *
http_delete_handler (ReqFunArgs)
{
  return (http_unimplemented_handler (request, fd));
}

static HTTP_RESULT *
http_link_handler (ReqFunArgs)
{
  return (http_unimplemented_handler (request, fd));
}

static HTTP_RESULT *
http_unlink_handler (ReqFunArgs)
{
  return (http_unimplemented_handler (request, fd));
}

static HTTP_RESULT *
http_unimplemented_handler (ReqFunArgs)
{
  HTTP_RESULT *result = mhtml_make_result ();
  result->spec = mhttpd_resolve_location (request);
  result->result_code = res_NOT_IMPLEMENTED;
  return (result);
}

#if defined (__cplusplus)
}
#endif
