/*  pages.c: Functions which aid in creating pages on the fly. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Tue May 30 19:32:35 1995.  */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
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
#if defined (HAVE_BSTRING_H)
#  include <bstring.h>
#endif
#include <sys/ioctl.h>
#if defined (Solaris) && defined (HAVE_SYS_TTOLD_H)
#  include <sys/ttold.h>
#endif
#if defined (TIME_WITH_SYS_TIME)
#  include <sys/time.h>
#  include <time.h>
#else
#  if defined (HAVE_SYS_TIME_H)
#    include <sys/time.h>
#  else
#    if defined (HAVE_TIME_H)
#      include <time.h>
#    endif
#  endif
#endif
#include <sys/stat.h>
#if !defined (HAVE_REGEX_H)
#  include <regex/regex.h>
#else
#  include <regex.h>
#endif /* HAVE_REGEX_H */

#include <xmalloc/xmalloc.h>
#include <bprintf/bprintf.h>
#include "pages.h"
#if defined (macintosh)
#include "mac_port.h"
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

extern char LEFT_BRACKET, RIGHT_BRACKET;

#undef whitespace
#define whitespace(c) \
     (((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n'))

#define form_tag_ender(c) (whitespace (c) || (c == '>') || (c == '"'))
#define tag_ender(c) (whitespace (c) || (c == '>') || (c == RIGHT_BRACKET))


#if defined (macintosh)
extern char *strdup (const char *string);
#  define os_open(name, flags, mode) open (name, flags)
#endif

#if defined (__CYGWIN32__)
#  if !defined (O_BINARY)
#    define O_BINARY 0
#  endif
#  define os_open(name, flags, mode) open (name, flags | O_BINARY, mode)
#endif
#if !defined (os_open)
#  define os_open(name, flags, mode) open (name, flags, mode)
#endif

#if !defined (errno)
extern int errno;
#endif

#if !defined (xfree)
#  define xfree(x) if (x) free (x)
#endif

/* The name of the file most recently loaded with page_read_template (). */
char *page_last_read_filename = (char *)NULL;
static int plrf_size = 0;

/* Local details about the page most recently loaded with page_read_template.*/
struct stat page_last_finfo;

/* The time of the most recent modification of all pages loaded through
   page_read_template (). */
unsigned long page_most_recent_modification_time = (unsigned long)0;

/* Make PAGE contain CONTENTS, and nothing else. */
void
page_set_contents (PAGE *page, char *contents)
{
  if (page->buffer)
    free (page->buffer);

  page->buffer = contents ? strdup (contents) : (char *)NULL;
  page->bindex = contents ? strlen (contents) : 0;
  page->bsize = page->bindex;
  if (page->attachment)
    {
      bprintf_free_buffer ((BPRINTF_BUFFER *)(page->attachment));
      page->attachment = (void *)NULL;
    }
}

/* Free the entire page, including any attachments. */
void
page_free_page (PAGE *page)
{
  if (page && page->attachment)
    bprintf_free_buffer ((BPRINTF_BUFFER *)(page->attachment));

  bprintf_free_buffer (page);
}

/* Return a new buffer containing the page template stored in FILENAME.
   If the file couldn't be found or read, then a NULL pointer is returned
   instead. */
PAGE *
page_read_template (char *passed_filename)
{
  PAGE *buffer = (PAGE *)NULL;
  int result;
  char *filename = passed_filename;
  
  if (passed_filename == (char *)NULL)
    return (buffer);

#if defined (__CYGWIN32__)
  if (((passed_filename[0] != '\0') && (passed_filename[1] == ':')) ||
      (strchr (passed_filename, '\\') != (char *)NULL))
    {
      register int i;
      filename = (char *)xmalloc (4 + strlen (passed_filename));

      if (passed_filename[1] == ':')
	{
	  filename[0] = '/';
	  filename[1] = '/';
	  filename[2] = passed_filename[0];
	  filename[3] = '\0';
	  i = 2;
	}
      else
	{
	  i = 0;
	  filename[0] = 0;
	}

      strcat (filename, passed_filename + i);

      for (i = 0; filename[i] != '\0'; i++)
	if (filename[i] == '\\')
	  filename[i] = '/';
    }
#endif

#if defined (macintosh)
  {
    register int i;

    filename = (char *)xmalloc (2 + strlen (passed_filename));

    if (*passed_filename != '/')
      sprintf (filename, ":%s", passed_filename);
    else
      sprintf (filename, "%s", passed_filename + 1);

    for (i = 0; filename[i] != '\0'; i++)
      if (filename[i] == '/')
	filename[i] = ':';
  }
#endif /* macintosh */

  result = stat (filename, &page_last_finfo);

  if ((result != -1) && (!S_ISDIR (page_last_finfo.st_mode)))
    {
      int fd;
      int filename_len = strlen (filename);

      if (page_most_recent_modification_time
	  < (unsigned long)page_last_finfo.st_mtime)
	page_most_recent_modification_time =
	  (unsigned long)page_last_finfo.st_mtime;

      if (filename_len + 2 > plrf_size)
	page_last_read_filename = 
	  (char *)xrealloc (page_last_read_filename,
			    (plrf_size = (20 + filename_len)));
      strcpy (page_last_read_filename, filename);

      fd = os_open (filename, O_RDONLY, 0666);
      if (fd > -1)
	{
	  int filesize = (int) page_last_finfo.st_size;
	  char *contents = (char *)xmalloc (1 + filesize);

	  if (read (fd, contents, filesize) != filesize)
	    {
	      free (contents);
	    }
	  else
	    {
	      contents[filesize] = '\0';
	      buffer = page_create_page ();
	      buffer->buffer = contents;
	      buffer->bsize = filesize;
	      buffer->bindex = filesize;
	    }
	  close (fd);
	}
    }

#if defined (macintosh) || defined (__CYGWIN32__)
  if (filename != passed_filename) free (filename);
#endif

  return (buffer);
}

/* Do some substitutions in PAGE; that is, substitute THIS WITH_THAT in PAGE.
   The substitutions happen in place, and the return value is the number of
   characters added or deleted before PIVOT.  THIS is a regular expression.
   PIVOT is a pointer to an integer which is adjusted depending on the number
   of characters added or deleted before it.  An NULL pointer means skip this
   calculation. */
int
page_subst_in_page_pivot (PAGE *page, char *ths, char *with_that, int *pivot)
{
  regex_t re;
  regmatch_t offsets[10];
  int matched, search_point, done;
  int len = 0;
  int point = (pivot ? *pivot : 0);
  int initial_characters = page->bindex;
  static int *replacement_offsets = (int *)NULL;
  static int ro_slots = 0;
  int ro_index = 0;
  int eflags = 0;

  regcomp (&re, ths, REG_EXTENDED);

  search_point = done = 0;

  if (with_that)
    {
      char *temp = with_that;
      char *pos = (char *)NULL;

      len = strlen (with_that);

      while ((pos = strchr (temp, '\\')) != (char *)NULL)
	{
	  if ((pos[1] != '\0') &&
	      (strchr ("0123456789", pos[1]) != (char *)NULL))
	    {
	      if (ro_index + 2 > ro_slots)
		replacement_offsets = (int *)xrealloc
		(replacement_offsets, (ro_slots += 10) * sizeof (int *));

	      replacement_offsets[ro_index++] = pos - with_that;
	      temp = pos + 2;
	    }
	  else if (pos[1] == '\\')
	    temp = pos + 2;
	  else
	    temp = pos + 1;
	}
    }

  while (!done)
    {
      matched =
	regexec (&re, page->buffer + search_point, 10, offsets, eflags) == 0;

      if (matched)
	{
	  int start = offsets[0].rm_so + search_point;
	  int end   = offsets[0].rm_eo + search_point;

	  /* Some searches will always succeed.  In that case, you will
	     never get back anything good, so give up right away.  For
	     example, <subst-in-string "foo" "" "a"> results in infinite
	     replacement. */
	  if (start == end)
	    {
	      done = 1;
	      continue;
	    }

	  /* Use REG_NOTBOL for subsequent calls, thus disallowing
	     beginning-of-line matches for those calls. */
	  eflags = REG_NOTBOL;

	  if (ro_index == 0)
	    {
	      if (end < point)
		point = point - (end - start) + len;
	      else if (start < point)
		point = start + len;

	      bprintf_delete_range (page, start, end);
	      bprintf_insert_text (page, start, with_that);

	      search_point = start + len;
	    }
	  else
	    {
	      register int i;
	      char *wt = strdup (with_that);
	      BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
	      char *text;

	      for (i = 0; i < ro_index; i++)
		{
		  int b = ((i == 0) ? 0 : replacement_offsets[i - 1] + 2);
		  int e = replacement_offsets[i];

		  wt[e] = '\0';
		  bprintf (buffer, "%s", wt + b);

		  e = wt[e + 1] - '0';

		  b = offsets[e].rm_so + search_point;
		  e = offsets[e].rm_eo + search_point;
		  text = (char *)xmalloc (1 + (e - b));

		  strncpy (text, page->buffer + b, e - b);
		  text[e - b] = '\0';
		  bprintf (buffer, "%s", text);
		  free (text);
		}

	      /* Insert the remainder of the string. */
	      bprintf (buffer, "%s", wt + replacement_offsets[i - 1] + 2);

	      free (wt);

	      if (end < point)
		point = point - (end - start) + buffer->bindex;
	      else if (start < point)
		point = start + buffer->bindex;

	      bprintf_delete_range (page, start, end);
	      bprintf_insert_text (page, start, buffer->buffer);
	      search_point = start + buffer->bindex;
	      bprintf_free_buffer (buffer);
	    }
	}
      else
	done = 1;
    }

  regfree (&re);

  if (pivot != (int *)NULL)
    *pivot = point;

  return (page->bindex - initial_characters);
}

/* Do some substitutions in PAGE; that is, substitute THIS WITH_THAT in PAGE.
   The substitutions happen in place, and the return value is the number of
   characters added or deleted.  THIS is a regular expression. */
int
page_subst_in_page (PAGE *page, char *ths, char *with_that)
{
  return (page_subst_in_page_pivot (page, ths, with_that, (int *)NULL));
}

/* Return the offset of STRING in PAGE.  Start the search at START.
   The absolute index is returned, not the offset from START, so
   the value is suitable for resubmission to this function as START value.
   A value of -1 indicates that the STRING couldn't be found. */
int
page_search (PAGE *page, char *string, int start)
{
  regex_t re;
  regmatch_t offsets[2];
  int result = -1;

  if ((page != (PAGE *)NULL) && (start < page->bindex) &&
      (string != (char *)NULL))
    {
      regcomp (&re, string, 0);
      if (regexec (&re, page->buffer + start, 1, offsets, 0) == 0)
	result = offsets[0].rm_so + start;
      regfree (&re);
    }
  return (result);
}

/* Get both the start and end points of STRING in PAGE.  The search is
   started at START, and the return value is the absolute offset of
   STRING in page.  END (if non-null) gets the absolute offset of
   the end of the match. The search is caseless by default. */
int
page_search_boundaries (PAGE *page, char *string, int start, int *end)
{
  regex_t re;
  regmatch_t offsets[2];
  int result = -1;

  if ((page != (PAGE *)NULL) && (start < page->bindex) &&
      (string != (char *)NULL))
    {
      regcomp (&re, string, REG_EXTENDED | REG_ICASE);
      if (regexec (&re, page->buffer + start, 1, offsets, 0) == 0)
	{
	  result = offsets[0].rm_so + start;
	  if (end != (int *)NULL)
	    *end = offsets[0].rm_eo + start;
	}
      regfree (&re);
    }
  return (result);
}

/* Grovel through PAGE deleting <INPUT ... name="TAG" ...> constructs, where
   TAG matches the value of the NAME field.
   Returns the number of fields that were deleted. */
int
page_delete_input (PAGE *page, char *tag)
{
  static char *input_finder = "<[Ii][Nn][Pp][Uu][Tt][^>]*";
  static char *name_finder = "[Nn][Aa][Mm][Ee]=";
  static regex_t re_input_finder, re_name_finder;
  static regmatch_t offsets[2];
  static int initialized = 0;
  int temp, start, changed, tag_len;

  if (!initialized)
    {
      regcomp (&re_input_finder, input_finder, 0);
      regcomp (&re_name_finder, name_finder, 0);
      initialized = 1;
    }

  start = changed = 0;
  tag_len = strlen (tag);

  while (regexec (&re_input_finder, page->buffer + start, 1, offsets, 0) == 0)
    {
      int beg = offsets[0].rm_so + start;
      int end = offsets[0].rm_eo + start;
      char *input_form;
      int c;

      input_form = (char *)xmalloc (1 + (end - beg));
      strncpy (input_form, page->buffer + beg, (end - beg));
      input_form[end - beg] = '\0';

      temp = regexec (&re_name_finder, input_form, 1, offsets, 0);
      if (temp != 0)
	goto continue_searching;

      /* Check to see if this tag matches the one passed in. */
      temp = offsets[0].rm_eo;
      if (input_form[temp] == '"')
	temp++;

      if (strlen (input_form + temp) < tag_len)
	goto continue_searching;

      c = input_form[temp + tag_len];

      if ((strncmp (input_form + temp, tag, tag_len) == 0) &&
	  (form_tag_ender (c)))
	{
	  bprintf_delete_range (page, beg, end);
	  changed++;
	}

    continue_searching:
      free (input_form);
      start = beg + 1;
    }

  return (changed);
}

/* Grovel through PAGE changing the RHS of "value=" statements found within
   `<input name="TAG" value="xxx"...>' structures.
   Returns the number of tags that were changed. */
int
page_set_form_input_value (PAGE *page, char *tag, char *value)
{
  char *input_finder = "<INPUT[ \t\r\n]+[^>]*";
  char *name_finder = "NAME[ \t\r\n]*=[ \r\n\t]*";
  char *value_finder = "VALUE[ \t\r\n]*=[ \t\r\n]*";
  int temp, start, changed, tag_len;
  static regex_t re_input_finder, re_name_finder, re_value_finder;
  static regmatch_t offsets[2];
  static int initialized = 0;

  if (!initialized)
    {
      /* Initialize our finders. */
      regcomp (&re_input_finder, input_finder, REG_EXTENDED | REG_ICASE);
      regcomp (&re_value_finder, value_finder, REG_EXTENDED | REG_ICASE);
      regcomp (&re_name_finder,  name_finder,  REG_EXTENDED | REG_ICASE);
      initialized = 1;
    }

  /* A VALUE of NULL means the empty string. */
  if (value == (char *)NULL)
    value = "";

  /* Search every input form. */
  start = changed = 0;
  tag_len = strlen (tag);

  while (regexec (&re_input_finder, page->buffer + start, 1, offsets, 0) == 0)
    {
      int exp_beg = offsets[0].rm_so + start;
      int exp_end = offsets[0].rm_eo + start;
      int form_length = (exp_end - exp_beg);
      char *input_form = (char *)xmalloc (1 + form_length);
      int c;

      strncpy (input_form, page->buffer + exp_beg, form_length);
      input_form[form_length] = '\0';

      /* Find the `name' tag within this input form. */
      temp = regexec (&re_name_finder, input_form, 1, offsets, 0);

      /* What to do when something neccessary isn't found. */
      if (temp != 0)
	goto continue_searching;

      /* Check to see if this tag matches the one passed in. */
      temp = offsets[0].rm_eo;
      if (input_form[temp] == '"')
	temp++;

      if (strlen (input_form + temp) < tag_len)
	goto continue_searching;

      c = input_form[temp + tag_len];

      if ((strncasecmp (input_form + temp, tag, tag_len) == 0) &&
	  (form_tag_ender (c)))
	{
	  int range_start, range_end;

	  /* This input form should have its value field changed. */
	  temp = regexec (&re_value_finder, input_form, 1, offsets, 0);

	  if (temp != 0)
	    goto continue_searching;

	  /* Point to right after the `VALUE=' string in the page. */
	  range_start = offsets[0].rm_eo + exp_beg;

	  /* If there is a quote here, skip until the next quote, otherwise
	     skip until form_tag_ender(). */
	  if (page->buffer[range_start] == '"')
	    {
	      range_start++;
	      for (range_end = range_start;
		   page->buffer[range_end] != '\0' &&
		   page->buffer[range_end] != '"';
		   range_end++);
	    }
	  else
	    {
	      for (range_end = range_start;
		   page->buffer[range_end] != '\0' &&
		   !form_tag_ender (page->buffer[range_end]);
		   range_end++);
	    }

	  /* Do the replacement. */
	  bprintf_delete_range (page, range_start, range_end);
	  bprintf_insert_text (page, range_start, value);
	  exp_end += (range_end - range_start) + strlen (value);
	  changed++;
	}

    continue_searching:
      free (input_form);
      start = exp_end;
      continue;
    }

  return (changed);
}

/* A few functions for helping in debugging. */

/* This variable is where the output of calling page_debug_xxx gets stored. */
static BPRINTF_BUFFER *debugger_output = (BPRINTF_BUFFER *)NULL;

/* This variable is where the output of calling page_syserr gets stored. */
static BPRINTF_BUFFER *syserr_output = (BPRINTF_BUFFER *)NULL;

/* Like printf, but stores the output in debugger_output. */
void
page_debug (char *format, ...)
{
  va_list args;

  if (!debugger_output)
    debugger_output = bprintf_create_buffer ();

  va_start (args, format);

  vbprintf (debugger_output, format, args);
  if (debugger_output->buffer[debugger_output->bindex - 1] != '\n')
    bprintf (debugger_output, "\n");
}

/* Like printf, but stores the output in syserr_output. */
void
page_syserr (char *format, ...)
{
  va_list args;

  if (!syserr_output)
    syserr_output = bprintf_create_buffer ();

  va_start (args, format);

  vbprintf (syserr_output, format, args);
  if (syserr_output->buffer[syserr_output->bindex - 1] != '\n')
    bprintf (syserr_output, "\n");
}

char *
page_debug_buffer (void)
{
  if (!debugger_output || !debugger_output->bindex)
    return ((char *)NULL);

  return ((char *)debugger_output->buffer);
}

char *
page_syserr_buffer (void)
{
  if (!syserr_output || !syserr_output->bindex)
    return ((char *)NULL);

  return ((char *)syserr_output->buffer);
}

void
page_debug_clear (void)
{
  if (debugger_output)
    bprintf_free_buffer (debugger_output);

  debugger_output = (BPRINTF_BUFFER *)NULL;
}

void
page_syserr_clear (void)
{
  if (syserr_output)
    bprintf_free_buffer (syserr_output);

  syserr_output = (BPRINTF_BUFFER *)NULL;
}

/* Given TICKS (the number of seconds since the epoch), return a string
   representing that date in the format the HTTP header mandate.  The
   string returned comes from a static buffer, the caller must manually
   save it away if it is not to be used immediately. */
char *
http_date_format (long ticks)
{
  struct tm *date = gmtime ((const time_t *)&ticks);
  static char result[100];

  static char *weekdays[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };

  static char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  sprintf (result, "%s, %02d %s %04d %02d:%02d:%02d GMT",
	   weekdays[date->tm_wday],
	   date->tm_mday, months[date->tm_mon], date->tm_year + 1900,
	   date->tm_hour, date->tm_min, date->tm_sec);

  return (result);
}

#if defined (PAGE_INSERT_HTTP_HEADER)
/* Insert a stnadard HTTP header at the start of page if it isn't already
   a redirection specification.  EXPIRATION is one of:

      page_NOT_EXPIRED:	To produce no special expiration date.
      page_IS_EXPIRED:	To inhibit server/browser caching.
      page_EXPIRES_NOW:	To give the page an expiration date of right now.
      integer > 0:	To cause the page to expire that many minutes in the
			future. */
void
page_insert_http_header (PAGE *page, int expiration)
{
  if (page)
    {
      time_t ticks = time ((time_t *)0);
      BPRINTF_BUFFER *http_header = bprintf_create_buffer ();
      int redirection = 0;
      int end_of_headers = -1;
      int page_length = page->bindex;

      /* Find out if this is a location redirector. */
      if (page->buffer && (strncasecmp (page->buffer, "Location:", 9) == 0))
	end_of_headers = 0;
      else
	end_of_headers = page_search (page, "[^\n]\nLocation:", 0);

      redirection = (end_of_headers != -1);

      if (redirection)
	{
	  while (end_of_headers < page_length - 1)
	    {
	      if ((page->buffer[end_of_headers] == '\n') &&
		  (page->buffer[end_of_headers + 1] == '\n'))
		{
		  end_of_headers += 2;
		  page_length = page->bindex - end_of_headers;
		  break;
		}
	      end_of_headers++;
	    }
	}

      /* Say that this page is using a MIME version of 1.0. */
      bprintf (http_header, "MIME-Version: 1.0\n");

      /* Say that this page contains text/html. */
      bprintf (http_header, "Content-Type: text/html\n");

      /* Say how long the page is. */
      bprintf (http_header, "Content-Length: %d\n", page_length);

      /* Say what time it is now. */
      bprintf (http_header, "Date: %s\n", http_date_format (ticks));

      /* Handle page expiration. */
      switch (expiration)
	{
	case page_NOT_EXPIRED:
	  break;

	case page_IS_EXPIRED:
	  bprintf (http_header, "Status: 500\n");
	  break;

	default:
	  {
	    /* Adjust the contents of NOW to reflect the new date. */
	    if (expiration > 0)
	      ticks += (60 * expiration);
	    else
	      ticks -= (60 * 60 * 24);	/* Expired one day ago. */

	    bprintf (http_header, "Expires: %s\n", http_date_format (ticks));
	  }
	}

      /* Does this page already contain an "HTTP/1.0" line?  If so, then
	 insert our header after it. */
      {
	register int i = 0;

	if (strncmp (page->buffer, "HTTP/", 5) == 0)
	  {
	    for (i = 5; page->buffer[i] != '\n'; i++);
	    i++;
	  }
	bprintf_insert (page, i, "%s%s",
			http_header->buffer, redirection ? "" : "\n");
      }
      bprintf_free_buffer (http_header);
    }
}
#endif /* PAGE_INSERT_HTTP_HEADER */

#if defined (PAGE_SET_COOKIE)
/* Set a mime header in PAGE giving it NAME and VALUE as a HTTP-COOKIE.
   PAGE should already have a full Mime header in it. */
void
page_set_cookie (PAGE *page, char *name, char *value, char *path)
{
  register int i;

  if (!page)
    return;

  i = page_search (page, "[Mm][Ii][Mm][Ee]-[Vv][Ee][Rr][Ss][Ii][Oo][Nn]: ", 0);

  if (i != -1)
    {
      BPRINTF_BUFFER *cookie_buffer = bprintf_create_buffer ();

      bprintf (cookie_buffer, "Set-Cookie: %s=%s; ", name, value ? value : "");

      if (!value || !*value)
	{
	  time_t ticks = time ((time_t *)0);
	  ticks -= 1000;
	  bprintf (cookie_buffer, " expires=%s; ", http_date_format (ticks));
	}

      bprintf (cookie_buffer, "path=%s;", path && *path ? path : "/");

      while (page->buffer[i++] != '\n');
      bprintf_insert (page, i, "%s\n", cookie_buffer->buffer);
    }
}
#endif /* PAGE_SET_COOKIE */

#if defined (HTTP_RETURN_IMAGE)
/* Given the pathname of a image file, return a page containing the image
   prefixed with the correct HTTP magic. */
PAGE *
http_return_image (char *name, char *type)
{
  PAGE *page = page_read_template (name);

  if (page != (PAGE *)NULL)
    {
      int image_len = page->bindex;
      time_t ticks = time ((time_t *)0);
      struct tm *now = gmtime (&ticks);

      bprintf_insert (page, 0, "\n");
      bprintf_insert (page, 0, "Content-Length: %ld\n", image_len);
      if (strchr (type, '/') != (char *)NULL)
	bprintf_insert (page, 0, "Content-Type: %s\n", type);
      else
	bprintf_insert (page, 0, "Content-Type: image/%s\n", type);
      bprintf_insert (page, 0, "Date: %s\n", http_date_format ((long) now));
    }
  return (page);
}
#endif /* HTTP_RETURN_IMAGE */

/* Create a generic error page with placeholders for <HEADER>, <ERROR-MESSAGE>,
   <DEBUGGING-OUTPUT>, <RETURN-TO-URL> and <FOOTER>. */
PAGE *
page_error_page (void)
{
  BPRINTF_BUFFER *page;

  page = bprintf_create_buffer ();

  bprintf (page, "<HEADER>\n<ERROR-MESSAGE>\n<RETURN-TO-URL>\n");
  bprintf (page, "<DEBUGGING-OUTPUT><SYSTEM-ERROR-OUTPUT><FOOTER>\n");
  return (page);
}

/* Clean up an error page if it needs it. */
void
page_clean_up (PAGE *page)
{
  BPRINTF_BUFFER *subber;

  subber = bprintf_create_buffer ();

  if (page_search (page, "<HEADER>", 0) != -1)
    {
      bprintf (subber, "<html><head><title>Error</title></head>\n");
      bprintf (subber, "<body>\n");
      page_subst_in_page (page, "<HEADER>", subber->buffer);
      subber->bindex = 0;
    }

  page_subst_in_page (page, "<ERROR-MESSAGE>", "");
  page_subst_in_page (page, "<RETURN-TO-URL>", "");

  if (debugger_output != (BPRINTF_BUFFER *)NULL)
    {
      bprintf (subber, "%s", debugger_output->buffer);
      page_subst_in_page (subber, "\n", "\n<br>");
      bprintf_insert (subber, 0,
		      "<h2>Output From Debugging Statements</h2><p>");
      page_subst_in_page (page, "<DEBUGGING-OUTPUT-HTML>", subber->buffer);
      subber->bindex = 0;
      bprintf (subber, "%s", debugger_output->buffer);
      page_subst_in_page (subber, "<", "&lt;");
      page_subst_in_page (subber, ">", "&gt;");
      page_subst_in_page (subber, "\n", "\n<br>");

      bprintf_insert (subber, 0,
		      "<h2>Output From Debugging Statements</h2><p>");

      page_subst_in_page (page, "<DEBUGGING-OUTPUT>", subber->buffer);
      subber->bindex = 0;
    }
  else
    page_subst_in_page (page, "<DEBUGGING-OUTPUT>", "");

  if (syserr_output != (BPRINTF_BUFFER *)NULL)
    {
      bprintf (subber, "%s", syserr_output->buffer);
      page_subst_in_page (subber, "<", "&lt;");
      page_subst_in_page (subber, ">", "&gt;");
      page_subst_in_page (subber, "\n", "\n<br>");
      bprintf_insert (subber, 0,
		      "<h2>System Error Messages</h2><p>");

      page_subst_in_page (page, "<SYSTEM-ERROR-OUTPUT>", subber->buffer);
      subber->bindex = 0;
    }
  else
    page_subst_in_page (page, "<SYSTEM-ERROR-OUTPUT>", "");

  if (page_search (page, "<FOOTER>", 0) != -1)
    page_subst_in_page (page, "<FOOTER>", "</body></html>\n");

  bprintf_free_buffer (subber);
}

/* Search PAGE for a tag beginning with TAG starting at START.
   Return the start of that tag, or -1 if the tag couldn't be
   found.  Case is insignificant in the search. */
int
page_find_tag_start (PAGE *page, char *tag, int start)
{
  int point = -1;
  register int i, c;
  int len = strlen (tag), limit = page->bindex - (len + 1);

  for (i = start; i < limit; i++)
    {
      c = page->buffer[i];
      if ((c == ';') && (i + 3 < limit) &&
	  ((page->buffer[i + 1] == ';') && (page->buffer[i + 2] == ';')))
	{
	  i += 3;
	  while ((i < limit) && (page->buffer[i] != '\n')) i++;
	  continue;
	}

      if ((c == '<') || (c == LEFT_BRACKET))
	{
	  c = page->buffer[i + 1 + len];

	  if (tag_ender (c) &&
	      (strncasecmp (page->buffer + i + 1, tag, len) == 0))
	    {
	      point = i;
	      break;
	    }
	}
    }
  return (point);
}

/* Given that PAGE->buffer + POINT is pointing at the start of a simple
   tag, find the location of the matching close bracket.  This counts
   double quotes, and brackets outside of double quotes.  Returns the
   offset in PAGE->buffer just after the matching close bracket, or -1
   if the matching end could not be found. */
int
page_find_tag_end (PAGE *page, int start)
{
  int end = -1;

  if ((start < page->bindex) &&
      ((page->buffer[start] == '<') || (page->buffer[start] == LEFT_BRACKET)))
    {
      register int i;
      int quote_depth = 0;
      int bracket_depth = 0;

      for (i = start; (end == -1) && (i < page->bindex); i++)
	{
	  int c = page->buffer[i];

	  /* Backslash quotes the character that follows. */
	  if (c == '\\')
	    {
	      i++;
	      continue;
	    }

	  if (c == LEFT_BRACKET) c = '<';
	  if (c == RIGHT_BRACKET) c = '>';

	  switch (c)
	    {
	    case '<':
	      if (quote_depth == 0)
		bracket_depth++;
	      break;

	    case '>':
	      if (quote_depth == 0)
		{
		  bracket_depth--;
		  if (bracket_depth == 0)
		    end = i + 1;
		}
	      break;

	    case '"':
	      quote_depth = !quote_depth;
	      break;

	    case ';':
	      if (((i + 2) < page->bindex) &&
		  (page->buffer[i + 1] == ';') &&
		  (page->buffer[i + 2] == ';'))
		{
		  /* Found a comment.  Skip it. */
		  i += 2;
		  while ((i < page->bindex) && (page->buffer[i] != '\n')) i++;

		  /* Our index is on the newline character that ends this line.
		     Just continue on. */
		}
	      break;

	    default:
	      break;
	    }
	}
    }
  return (end);
}

/* Find the inclusive boundaries in PAGE of a simple tag named TAG.
   Returns the boundaries in STARTP and ENDP, and a non-zero value
   to the caller if the simple tag TAG was found, else -1 is
   returned.  STARTP is both input and output; it controls the starting
   location in PAGE of the search. If TAG was "FOO", then the bounds of
   "<FOO ....>" are returned.  Case is insignificant in the search. */
int
page_simple_tag_bounds (PAGE *page, char *tag, int *startp, int *endp)
{
  int result = 0;
  int start = -1, end = -1;

  start = page_find_tag_start (page, tag, *startp);

  if (start != -1)
    end = page_find_tag_end (page, start);

  if (end != -1)
    {
      *startp = start;
      *endp = end;
      result = 1;
    }

  return (result);
}

/* Find the inclusive boundaries in PAGE of a complex tag named TAG.
   Returns the boundaries in STARTP and ENDP, and a non-zero value
   to the caller if the simple tag TAG was found, else -1 is
   returned.  STARTP is both input and output; it controls the starting
   location in PAGE of the search.  If TAG was "FOO", then the bounds
   of "<FOO ...> .... </FOO>" are returned.  Case is insignificant in
   the search. */
int
page_complex_tag_bounds (PAGE *page, char *tag, int *startp, int *endp)
{
  int result = 0;
  int open_start = *startp, open_end;
  int tlen = strlen (tag);
  int tag_found = 0;

  /* Find the opening tag. */

  /* Try to optimize... */
  if (((open_start + tlen + 1) < page->bindex) &&
      ((page->buffer[open_start] == '<') ||
       (page->buffer[open_start] == LEFT_BRACKET)) &&
      (strncasecmp (tag, page->buffer + open_start + 1, tlen) == 0) &&
      ((whitespace (page->buffer[open_start + tlen + 1]) ||
	((page->buffer[open_start + tlen + 1] == '>') ||
	 (page->buffer[open_start + tlen + 1] == RIGHT_BRACKET)))))
    {
      if ((page->buffer[open_start + tlen + 1] == '>') ||
	  (page->buffer[open_start + tlen + 1] == RIGHT_BRACKET))
	open_end = open_start + tlen + 1;
      else
	open_end = page_find_tag_end (page, open_start);

      if (open_end != -1)
	tag_found = 1;
    }
  else
    tag_found = page_simple_tag_bounds (page, tag, &open_start, &open_end);

  if (tag_found)
    {
      char *closer = (char *)xmalloc (2 + tlen);
      int close_start, close_end;
      int temp_start;
      int done = 0;
      int opens_found = 0;

      closer[0] = '/';
      strcpy (closer + 1, tag);
      close_start = open_end;
      temp_start = open_end;

      while (!done)
	{
	  /* Find the matching closer. */
	  if (page_simple_tag_bounds (page, closer, &close_start, &close_end))
	    {
	      /* We found a closer.  Count the number of openers in between
		 the end of our opener and the start of this closer. */
	      int offset;
	      int loop_start = temp_start;

	      while (1)
		{
		  offset = page_find_tag_start (page, tag, loop_start);

		  if (offset == -1)
		    break;

		  if (offset > close_start)
		    break;

		  loop_start = offset + tlen + 2;
		  opens_found++;
		}

	      /* If any opens were found, move past this ender, and search
		 again.  If not, then this is the one that matches. */
	      if (opens_found)
		{
		  opens_found--;
		  close_start = close_end;
		  temp_start = close_start;
		  continue;
		}

	      /* The number of opens is exactly zero.  This closer is the
		 one that matches our opener, so return it. */
	      *startp = open_start;
	      *endp = close_end;
	      result = 1;
	      done = 1;
	    }
	  else
	    done = 1;
	}
      free (closer);
    }

  return (result);
}

/* Find out if <indicator> belongs to the complex <tag>, and return 3 values:
      1) The output of the function is non-zero if INDICATOR was found,
         It is 1 if the indicator was found and it was ours, -1 if the
	 indicator was found and it wasn't ours, and 0 if the indicator
	 wasn't found at all.
      2) The value of STARTP gets the offset of INDICATOR,
      3) the value of ENDP gets the offset of the end of INDICATOR. */
int
page_indicator_owned_by (PAGE *page, char *indicator, char *tag,
			 int *startp, int *endp, int search_start)
{
  int result = 0;
  int is = search_start, ie;
  int indicator_present = page_simple_tag_bounds (page, indicator, &is, &ie);

  /* If there is an indicator here, then it *might* belong to TAG.
     Find out by finding TAG boundaries in the page.  For each
     boundary found, if the start is past the indicator, then the
     indicator belongs to us.  Stop when there are no more TAGs found
     within range, or if the indicator position falls within the start
     and end of a TAG. */
  if (indicator_present)
    {
      int ts = *startp, te;

      *startp = is;
      *endp = ie;

      while (1)
	{
	  int found = page_complex_tag_bounds (page, tag, &ts, &te);

	  /* If there isn't another complex tag matching TAG, then this
	     indicator is ours. */
	  if (found == 0)
	    {
	      result = 1;
	      break;
	    }

	  /* Found a tag pair.  If the tag start is past the indicator start,
	     then this indicator is ours. */
	  if (ts > is)
	    {
	      result = 1;
	      break;
	    }

	  /* Well, we found a tag pair in the page.  The beginning of
	     the tag pair is before the indicator.  If the end is before
	     the indicator as well, then we don't have an answer yet.
	     Otherwise, the tag doesn't belong to us. */
	  if (te > ie)
	    {
	      result = -1;
	      break;
	    }

	  ts = te;
	}
    }
  return (result);
}

#if defined (NEVER_USED)
/* Extract the value side of a randomly named variable in PAGE.
   For example, if PAGE contains `<DOCTITLE="this">', then the
   call: page_assigned_label (page, "DOCTITLE") returns "this". */
char *
page_assigned_label (PAGE *page, char *tag)
{
  char *value = (char *)NULL;
  int start = 0, end;
  BPRINTF_BUFFER *search;

  search = bprintf_create_buffer ();
  bprintf (search, "%s[\t\r\n ]*=", tag);

  if (page_simple_tag_bounds (page, search->buffer, &start, &end))
    {
      register int i;
      int delimited = 0;

      start += strlen (tag) + 1;

      /* Skip whitespace after name, equals sign, and whitespace
	 following name. */
      for (; whitespace (page->buffer[start]); start++);
      if (page->buffer[start] == '=') start++;
      for (; whitespace (page->buffer[start]); start++);

      /* Check for delimiter, and then gobble contents. */
      if (page->buffer[start] == '"')
	{
	  delimited = '"';
	  start++;
	}

      for (i = start; i < end; i++)
	{
	  if ((page->buffer[i] == delimited) ||
	      (!delimited &&
	       ((page->buffer[i] == '>')  ||
		(page->buffer[i] == RIGHT_BRACKET) ||
		(whitespace (page->buffer[i])))))
	    break;
	}

      value = (char *)xmalloc (1 + (i - start));
      strncpy (value, page->buffer + start, (i - start));
      value[i - start] = '\0';
    }

  bprintf_free_buffer (search);
  return (value);
}
#endif /* NEVER_USED */

/* Find and extract the inclusive boundaries in PAGE of a complex tag
   named TAG.  Returns the boundaries in STARTP and ENDP, and a string
   which is the extracted complex tag in its entirety, or a NULL
   pointer if TAG wasn't found.  STARTP is both input and output; it
   controls the starting location in PAGE of the search.  If TAG was
   "FOO", then the bounds of "<FOO ...> .... </FOO>" are returned.
   Case is insignificant in the search. */
char *
page_complex_tag_extract (PAGE *page, char *tag, int *startp, int *endp)
{
  char *result = (char *)NULL;

  if (page_complex_tag_bounds (page, tag, startp, endp))
    {
      int len = (*endp - *startp);

      result = (char *)xmalloc (1 + len);
      strncpy (result, page->buffer + *startp, len);
      result[len] = '\0';
    }

  return (result);
}

/* Find and extract the inclusive boundaries in PAGE of a simple tag
   named TAG.  Returns the boundaries in STARTP and ENDP, and a string
   which is the extract simple tag in its entirety, or a NULL pointer
   if TAG wasn't found.  STARTP is both input and output; it controls
   the starting location in PAGE of the search. If TAG was "FOO", then
   the bounds of "<FOO ....>" are returned.  Case is insignificant in
   the search. */
char *
page_simple_tag_extract (PAGE *page, char *tag, int *startp, int *endp)
{
  char *result = (char *)NULL;

  if (page_simple_tag_bounds (page, tag, startp, endp))
    {
      int len = (*endp - *startp);

      result = (char *)xmalloc (1 + len);
      strncpy (result, page->buffer + *startp, len);
      result[len] = '\0';
    }

  return (result);
}

static PagePDL *page_pdl = (PagePDL *)NULL;
int page_pdl_index = 0;
int page_pdl_size = 0;

void
page_push_page (PAGE *page, int start, int *search_start_modified)
{
  if (page_pdl_index + 2 > page_pdl_size)
    page_pdl = (PagePDL *)
      xrealloc (page_pdl, (page_pdl_size += 10) * sizeof (PagePDL));
  page_pdl[page_pdl_index].page = page;
  page_pdl[page_pdl_index].start = start;
  page_pdl[page_pdl_index].search_start_modified = search_start_modified;
  page_pdl_index++;
}

void
page_pop_page (void)
{
  page_pdl_index--;

  if (page_pdl_index < 0)
    abort ();
}

PagePDL *
page_pdl_tos (void)
{
  if (page_pdl_index)
    return (&page_pdl[page_pdl_index - 1]);
  else
    return ((PagePDL *)NULL);
}

PagePDL *
page_pdl_page (int offset)
{
  return (&page_pdl[offset]);
}

#if defined (__cplusplus)
}
#endif
