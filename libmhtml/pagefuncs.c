/* pagefuncs.c: HTML `functions' can be executed with page_process_page (). */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Tue Jul 18 17:50:42 1995.  */

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
/************************************************************/
/*							    */
/*		Language Manipulation Functions		    */
/*							    */
/************************************************************/

static void pf_randomize (PFunArgs);
static void pf_random (PFunArgs);
static void pf_make_identifier (PFunArgs);

/* <date> --> Mon Feb 12 04:28:15 PST 1996 */
static void pf_date (PFunArgs);
static void pf_time (PFunArgs);
static void pf_pid (PFunArgs);

/* <comment> ... </comment> */
static void pf_comment (PFunArgs);

/* <include filename alt="page value if FILENAME isn't found"> */
static void pf_include (PFunArgs);

/* <replace-page filename> Replace the current page with FILENAME. */
static void pf_replace_page (PFunArgs);

/* <redirect URL> returns an HTTP Location: directive.  The returned
   URL will include the SID if the SID is present and has a value. */
static void pf_redirect (PFunArgs);

/* <server-push>TEXT</server-push> makes TEXT go down the line immediately. */
static void pf_server_push (PFunArgs);

/* <subst-in-page this that> Replace occurrences of THIS with THAT. */
static void pf_subst_in_page (PFunArgs);

#if defined (NOT_AT_THIS_TIME)
/* <page-insert location string> */
static void pf_page_insert (PFunArgs);

/* <page-search start-offset regex> --> location */
static void pf_page_search (PFunArgs);
#endif /* NOT_AT_THIS_TIME */

/************************************************************/
/*							    */
/*			HTML "Helper" Functions		    */
/*							    */
/************************************************************/

static PFunDesc func_table[] =
{
  /* File manipulation functions. */
  { "INCLUDE",		0, 0, pf_include },
  { "REPLACE-PAGE",	0, 0, pf_replace_page },
  { "REDIRECT",		0, 0, pf_redirect },
  { "SERVER-PUSH",	1, 0, pf_server_push },

  /* Block manipulation. */
  { "COMMENT",		1, 0, pf_comment },

  /* Page manipulation.  The following functions operate on the page as it
     exists in its current state at the time the function was called. */
  { "SUBST-IN-PAGE",	0, 0, pf_subst_in_page },

#if defined (NOT_AT_THIS_TIME)
  { "PAGE-INSERT",	0, 0, pf_page_insert },
  { "PAGE-SEARCH",	0, 0, pf_page_search },
#endif /* NOT_AT_THIS_TIME */

  { "RANDOMIZE",	0, 0, pf_randomize },
  { "RANDOM",		0, 0, pf_random },
  { "MAKE-IDENTIFIER",	0, 0, pf_make_identifier },

  { "DATE",		0, 0, pf_date },
  { "TIME",		0, 0, pf_time },

  { "PID",		0, 0, pf_pid },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_pagefunc_functions)

DEFINE_SECTION (PAGE-VARIABLES, , "Several variables are predefined by \
<meta-html>, and made available to the page writer.  In addition to \
the contents of the Unix environment (which includes the server-side \
variables <varref ENV::SERVER_NAME>, <varref ENV::PATH_INFO>, etc.) \
variables describing the location of the current document in various \
ways are also defined.  ", "The following short list is a quick look \
at typical values of the most commonly referenced page variables. \
\n\
When the URL is ``<i><get-var mhtml::full-url></i>'', the following \
variables are set to: \
\n\
<defun showvar var>\n\
  <tr>\n\
    <td align=right> <varref <upcase <get-var var>>>: </td>\n\
    <td align=left>\n\
      <i><get-var <get-var var>></i><br>\n\
    </td>\n\
  </tr>\n\
</defun>\n\
\n\
<table>\n\
  <showvar mhtml::path-info>\n\
  <showvar mhtml::http-to-host>\n\
  <showvar mhtml::current-doc>\n\
  <showvar mhtml::include-prefix>\n\
  <showvar mhtml::relative-prefix>\n\
  <showvar mhtml::location>\n\
  <showvar mhtml::location-sans-sid>\n\
  <showvar mhtml::full-url>\n\
  <showvar mhtml::full-url-sans-sid>\n\
  <showvar mhtml::url-to-dir>\n\
  <showvar mhtml::url-to-dir-sans-sid>\n\
  <showvar mhtml::http-prefix>\n\
  <showvar mhtml::http-prefix-sans-sid>\n\
</table>")

DOC_SECTION (ARITHMETIC-OPERATORS)
static int randomize_called = 0;

static unsigned long
random_seed (void)
{
  unsigned long seed = 0;
  struct timeval tv;
  struct timezone tz;

  gettimeofday (&tv, &tz);

  seed = tv.tv_sec + tv.tv_usec;
  return (seed);
}

DEFUN (pf_randomize, &optional seed,
"Sets the pseudo-random number generator seed to <var seed>.  The next "
"call to <funref arithmetic-operators random> uses this seed value to "
"find the next pseudo-random number.  There is no return value. "
"\n\
Examples:\n\
<example>\n\
  <randomize 10>\n\
  <random 100> --> 28\n\
  <random 100> --> 15\n\
  <randomize 10>\n\
  <random 100> --> 28\n\
</example>")
{
  unsigned long seed = random_seed ();
  char *user_seed = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (user_seed)
    {
      seed = (unsigned long)atol (user_seed);
      free (user_seed);
    }

  if (seed == 0) seed = 1;
  srandom ((unsigned int)seed);
  randomize_called = 1;
}


static int
pf_random_internal (int max_value)
{
  int result;

  if (!randomize_called)
    {
      unsigned int seed = (unsigned int) random_seed ();
      srandom (seed);
      randomize_called++;
    }

  if (max_value)
    result = (int) (((double)max_value * random ()) / RAND_MAX );
  else
    result = random ();

  return (result);
}

DEFUN (pf_random, arg,
"Returns a pseudo-random number between 0 and <var arg> -1.  The "
"distribution is pretty good; calling <example code><random "
"2></example> returns 0 50% of the time, and 1 the other 50%. "
"\n\
Examples:\n\
<example>\n\
  <random 100> --> 87\n\
  <random 100> --> 44\n\
\n\
  <b>I'm thinking of a number between 1 and 10:</b>\n\
  <set-var guess = <add 1 <random 10>>>\n\
</example>\n\
\n\
Also see <funref arithmetic-operators randomize>.")
{
  char *max_arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int max_value = max_arg ? atoi (max_arg) : 0;
  int result = pf_random_internal (max_value);

  if (max_arg) free (max_arg);
  bprintf_insert (page, start, "%d", result);
}

DEFINE_SECTION (LANGUAGE-OPERATORS, self-modif; language;
self-referential, "<Meta-HTML> provides various functions and "
"variables which are used to operate on or within the language "
"environment itself, rather than within the application environment. "
"\n"
"Such constructs make it possible to create self-modifying code, to "
"change the way in which a builtin function operates, and other such "
"language dependent features.", "")

DEFVAR (MHTML::INHIBIT-COMMENT-PARSING, "When this variable is set to "
"a non-empty value, the triple semi-colon comment feature is disabled. "
"\n"
"You may need this for complex applications where you are deliberately "
"shipping the source of a <meta-html> document to the client.")

DEFVAR (MHTML::VERSION, "Contains the version identification of the "
"currently running interpreter.  This variable is available whereever "
"the interpreter is running, including the <meta-html> server, Engine, "
"and in <code>mdb</code>."
"\n\
<complete-example>\n\
<get-var mhtml::version>\n\
</complete-example>")


DEFUN (pf_make_identifier, &optional limit &key alphabet,
"Create an identifier that is <var limit> characters in length. "
"<var limit> defaults to 16 if not specified. "
"The identifier characters are taken from the string variable "
"<var mi::alphabet>, unless passed in from keyword argument "
"<var alphabet>.  If there is neither a passed in alphabet, nor "
"a value for <var mi::alphabet>, then the alphabet defaults to "
uppercase letters and numbers excluding \"I\", \"O\", zero, and one.")
{
  char *limit_arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *alphabet = mhtml_evaluate_string (get_value (vars, "alphabet"));
  char *identifier = (char *)NULL;
  int limit = 0;

  if (!empty_string_p (limit_arg))
    limit = atoi (limit_arg);

  identifier = mhtml_make_identifier (alphabet, limit);
  bprintf_insert (page, start, "%s", identifier);
  *newstart += strlen (identifier);
  free (identifier);
  xfree (limit_arg);
  xfree (alphabet);
}

DEFMACRO (pf_comment, ,
"Simply discard the <var body>.  No processing is done, and no output "
"is produced."
"\n\
A shorthand for commenting source on a single line exists; when the\n\
sequence of 3 semicolon character is seen\n\
(<code>;</code><code>;;</code>), then the text from this sequence to\n\
the end of the line inclusive is discarded.\n\
\n\
Example:<set-var mhtml::inhibit-comment-parsing=1>\n\
<example>\n\
<comment>\n\
  This text is simply ignored.\n\
\n\
  All of it.\n\
</comment>\n\
;;; So is this text, up to the end of the line.\n\
;; But NOT this line -- it only has two semi-colons on it.\n\
</example><unset-var mhtml::inhibit-comment-parsing>")
{
  /* Contents already deleted by caller. */
}

DEFINE_SECTION (PAGE-OPERATORS, page commands; page search; search;
insert; modify page,
"<meta-html> contains a few commands which\n\
operate on the entire <i>page</i> as it currently exists.\n\
\n\
You may find, modify, or delete text which has already been looked at by\n\
the interpreter, as well as text which hasn't yet been looked at. \n\
\n\
Most of the time, you won't need to use such commands.  They can make\n\
it hard for other people to understand the sequence of code in your\n\
page, and they can sometimes produce unexpected results which are\n\
difficult to debug because much of the information has been modified.\n\
", "")

static int include_recursive_calls = 0;

DEFUN (pf_include, webpath &key alt=altcode verbatim=true,
"Insert the contents of the file named by <var webpath> into the\n\
document at the point where the <code>include</code> form was read.\n\
If the keyword argument <var verbatim=true> is given, then the contents\n\
of the file are not executed, only inserted.  Otherwise, execution\n\
resumes at the point where the file was inserted.\n\
\n\
<var webpath> can be given as an absolute pathname, or a relative\n\
pathname.  If the path is relative, it is considered to be relative to\n\
the location of the document which contains the <code>include</code>\n\
form.  If the path given is not relative, it is appended to the\n\
directory found in <b>MHTML::INCLUDE-PREFIX</b>.\n\
\n\
If the named file could not be found on the server, and an <var alt=altcode>\n\
value is given, then that value is placed into the page instead.")
{
  int verbatim_p = var_present_p (vars, "VERBATIM");
  char *pathname = (char *)NULL;
  int found = 0;

  include_recursive_calls++;
  if (include_recursive_calls < MHTML_INCLUDE_RECURSION_LIMIT)
    {
      char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
      char *incpref = pagefunc_get_variable ("%%::incpref");
      char *relpref = pagefunc_get_variable ("%%::relpref");

      if (debug_level > 10)
	page_debug ("<include %s> [relpref: %s]",
		    arg ? arg : "\"\"",
		    relpref ? relpref : "\"\"");
		    
      if (!incpref) incpref = pagefunc_get_variable ("mhtml::include-prefix");
      if (!relpref) relpref = pagefunc_get_variable ("mhtml::relative-prefix");

      if (arg != (char *)NULL)
	{
	  PAGE *file_contents = (PAGE *)NULL;
	  char *wr = (char *)NULL;

	  pathname = mhtml_canonicalize_file_name (arg, incpref, relpref, &wr);

	  if (!empty_string_p (pathname))
	    file_contents = page_read_template (pathname);

	  /* Did the user specify some alternate HTML if the file
	     couldn't be found? */
	  if (!file_contents)
	    {
	      char *alt = mhtml_evaluate_string
		(get_one_of (vars, "ALT", "ALTERNATE", (char *)0));

	      found = 0;
	      if (alt != (char *)NULL)
		{
		  verbatim_p = 0;
		  file_contents = page_create_page ();
		  bprintf (file_contents, "%s", alt);
		  free (alt);
		}
	      else if (debug_level)
		page_debug ("include %s: Unable to read %s", arg, pathname);
	    }
	  else
	    {
	      char digits[40];
	      found = 1;
	      sprintf (digits, "%ld", page_most_recent_modification_time);
	      pagefunc_set_variable ("mhtml::last-modification-time", digits);
	    }

	  if (file_contents)
	    {
	      if (found && !verbatim_p)
		{
		  bprintf_insert (file_contents, 0, "<*parser*::push-file %s>",
				  wr);
		  bprintf (file_contents, "<*parser*::pop-file>");
		}

	      bprintf_insert_binary
		(page, start, file_contents->buffer, file_contents->bindex);

	      if (verbatim_p)
		*newstart += file_contents->bindex;

	      page_free_page (file_contents);
	    }
	  free (arg);
	}
    }
  xfree (pathname);
  include_recursive_calls--;
}

DEFUN (pf_replace_page, webpath &key alt=altcode contents,
"Replace the entire contents of the current page with the contents of\n\
the page named by <var webpath>.  You probably don't want to use this\n\
command, use <funref page-operator redirect> instead.  In that way, the\n\
URLs give a consistent view to the user.  The rules for which <var\n\
webpath> is chosen are identical to those for <funref page-operators\n\
include>.")
{
  PAGE *newpage = page_create_page ();
  char *content_var = mhtml_evaluate_string (get_value (vars, "CONTENTS"));

  if (!empty_string_p (content_var))
    {
      Symbol *sym = symbol_lookup (content_var);

      if (sym != (Symbol *)NULL)
	{
	  switch (sym->type)
	    {
	    case symtype_BINARY:
	      {
		Datablock *b = (Datablock *)sym->values;
		bprintf_insert_binary (newpage, 0, b->data, b->length);
	      }
	      break;

	    case symtype_STRING:
	      bprintf (newpage, "%s", sym->values[0]);
	      break;

	    default:
	      break;
	    }
	}
    }
  else
    {
      int newpage_start = 0;
      pf_include (newpage, body, vars, 0, 0, &newpage_start, debug_level
		  COMMA_FNAME);
      page_process_page_internal (newpage);
    }

  xfree (content_var);
  page_return_this_page (newpage);
}

DOC_SECTION (PAGE-OPERATORS)

DEFUN (pf_subst_in_page, this-string with-that,
"Replaces all occurrences of <var this-string> with <var with-that> in the\n\
current page.  Both <var this-string> and <var with-that> are evaluated\n\
before the replacement is done.  <var this-string> can be any regular\n\
expression allowed by the POSIX extended regular expression matching.\n\
\n\
This command can be useful when included as part of a footer\n\
<code>include</code> file.  For example, you could replace all\n\
occurrences of <code>%name%</code> with a the value of the variable\n\
<code>FirstName</code> by writing:\n\
<example>\n\
  <subst-in-page %name% <get-var forms::FirstName>>\n\
</example>")
{
  int arg = 0, done = 0;
  PagePDL *top_page = page_pdl_page (0);

  while (!done)
    {
      char *this_string, *with_that;

      this_string = get_positional_arg (vars, arg++);
      with_that = get_positional_arg (vars, arg++);

      if (this_string == (char *)NULL)
	done = 1;
      else
	{
	  this_string = mhtml_evaluate_string (this_string);
	  with_that = mhtml_evaluate_string (with_that);

	  if (this_string)
	    page_subst_in_page_pivot
	      (top_page->page, this_string, with_that, &(top_page->start));

	  xfree (this_string);
	  xfree (with_that);
	}
    }

  *top_page->search_start_modified = top_page->start;
  if (top_page->page == page) *newstart = top_page->start;
}

#if defined (NOT_AT_THIS_TIME)
static void
pf_page_search (PFunArgs)
{
  char *search_start = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *search_string = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((!empty_string_p (search_start)) && (!empty_string_p (search_string)) &&
      (ThePage != (PAGE *)NULL))
    {
      int loc = atoi (search_start);

      if (loc < ThePage->bindex)
	{
	  int end_point, beg_point;

	  beg_point =
	    page_search_boundaries (ThePage, search_string, loc, &end_point);

	  if (beg_point != -1)
	    {
	      int use_end_p = var_present_p (vars, "end");

	      bprintf_insert (page, start, "%d",
			      use_end_p ? end_point : beg_point);
	    }
	}
    }
}

static void
pf_page_insert (PFunArgs)
{
  char *insert_loc = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *insertion = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((!empty_string_p (insert_loc)) && (!empty_string_p (insertion)))
    {
      int loc = atoi (insert_loc);

      if ((loc > -1) && (loc < ThePage->bindex))
	{
	  bprintf_insert (ThePage, loc, "%s", insertion);
	  if ((loc < start) && (ThePage == page))
	    *newstart += strlen (insertion);
	}
    }
}
#endif /* NOT_AT_THIS_TIME */

DEFUN (pf_redirect, url &key target,
"Immediately return the HTTP <b>Location</b> directive with <var url>.\n\
This command is extremely useful in conjunction with <funref variables\n\
var-case>; in effect, that usage creates a dispatch table.\n\
\n\
When a <code>redirect</code> tag is seen, the current page is\n\
discarded, and the appropriate HTTP code is returned to the browser to\n\
cause an immediate redirection to the specified page.\n\
\n\
<var url> may be expressed in relative terms, in absolute terms, or as\n\
a complete URI specification.  For example, assume the current\n\
document is located at\n\
<example code>http://www.metahtml.com/subdir/doc.mhtml</example>, and\n\
the following redirections are seen:\n\
<example>\n\
  <redirect ../foo.mhtml>     --> http://www.metahtml.com/foo.mhtml\n\
  <redirect /bar/foo.mhtml>   --> http://www.metahtml.com/bar/foo.mhtml\n\
  <redirect http://www.bsdi.com> --> http://www.bsdi.com\n\
</example>\n\
\n\
If the keyword argument <var target=XXX> is supplied, it is the name of\n\
the browser window in which to display the new page.  If one is working\n\
with frames, this can be very useful: a target of \"_top\" removes all\n\
frames, a target of \"_new\" creates a new window, etc.")
{
  PAGE *new_page;
  char *arg = (char *)NULL;
  char *protocol = pagefunc_get_variable ("env::server_protocol");
  char *protover = pagefunc_get_variable ("env::protocol_version");
  char *target = mhtml_evaluate_string (get_value (vars, "target"));

  if (!protocol) protocol = "HTTP";
  if (!protover) protover = "1.0";

  if ((body != (PAGE *)NULL) && (body->buffer != (char *)NULL))
    {
      int offset = 0;
      char *sexp = read_sexp (body->buffer, &offset, 0);

      if (!empty_string_p (sexp))
	arg =  mhtml_evaluate_string (sexp);

      xfree (sexp);
    }

  new_page = page_create_page ();

  /* If there is something to redirect to, then do it now.
     Otherwise, return a null response code, indicating that the
     browser should retain the old view. */
  if (!empty_string_p (arg))
    {
      register int i;

      for (i = 0; whitespace (arg[i]); i++);

      /* Fully qualify ARG if it isn't already. */
      if ((strncasecmp (arg + i, "http://", 7) != 0) &&
	  (strncasecmp (arg + i, "https://", 8) != 0) &&
	  (strncasecmp (arg + i, "ftp://", 6) != 0) &&
	  (strncasecmp (arg + i, "gopher://", 9) != 0))
	{
	  BPRINTF_BUFFER *newarg = bprintf_create_buffer ();
	  char *temp = pagefunc_get_variable ("mhtml::relative-prefix");

	  /* We can't do a complete canonicalization of the name here,
	     because there isn't any way to tell where the PATH_INFO
	     part of the name begins, if any.  But, we can strip double
	     dots which appear at the start of the name. */
	  if (temp != (char *)NULL)
	    {
	      temp = strdup (temp);

	      while (strncmp (arg + i, "../", 3) == 0)
		{
		  char *slash = strrchr (temp, '/');

		  if (slash != (char *)NULL)
		    {
		      *slash = '\0';
		      i += 3;
		    }
		}
	    }

	  bprintf (newarg, "%s", pagefunc_get_variable ("mhtml::http-prefix"));
	  if (arg[i] != '/')
	    bprintf (newarg, "%s/%s", temp ? temp : "", arg + i);
	  else
	    bprintf (newarg, "%s", arg + i);

	  xfree (temp);
	  free (arg);
	  arg = newarg->buffer;
	  free (newarg);
	  i = 0;
	}

#if defined (macintosh)
      bprintf (new_page,  "%s/%s 302 Found\nLocation: %s\n",
	       protocol, protover, arg + i);
#else
      if (pagefunc_get_variable ("mhtml::unparsed-headers"))
	bprintf (new_page, "%s/%s 302 Found\nLocation: %s\n",
		 protocol, protover, arg + i);
      else
	bprintf (new_page, "Location: %s\n", arg + i);

      if (!empty_string_p (target))
	bprintf (new_page, "Window-target: %s\n", target);

      bprintf (new_page, "\n");
#endif /* !macintosh */
    }
  else
    bprintf (new_page, "%s/%s.0 204 No Response\n\n", protocol, protover);

  xfree (arg);
  xfree (target);
  page_return_this_page (new_page);
}

/* Here's a wild one.

   <server-push>
     <html>
     <head><title>Just a Moment, please</title></head>
     <body>
     <h3>Please wait a moment, we are searching the entire Web...</h3>
     </body>
     </html>
   </server-push>

   Immediately sends this stuff down the line, but doesn't affect
   processesing or the current page. */
DEFMACRO (pf_server_push, &key type=mime-type,
"Writes the appropriate headers and HTTP information to the connected\n\
browser, along with the data that you specify.\n\
\n\
The functionality provided by this convenience function is available\n\
other ways, specifically through the use of <funref stream-operators\n\
with-open-stream> and the variable <varref *standard-output*>.\n\
\n\
<example>\n\
<server-push>\n\
  <html>\n\
  <head><title>Just a Moment, please</title></head>\n\
  <body>\n\
  <h3>Please wait a moment, we are searching the entire Web...</h3>\n\
  </body>\n\
  </html>\n\
</server-push>\n\
</example>")
{
  PAGE *text = (PAGE *)NULL;
  static int called_yet = 0;
  static int output_fd = 0;
  char *type = mhtml_evaluate_string (get_value (vars, "type"));

  if (body && body->buffer)
    {
      text = page_create_page ();
      page_set_contents (text, body->buffer);
      page_process_page_internal (text);

      if (text && text->buffer)
	{
	  PAGE *pushed = page_create_page ();
	  char *boundary = "Meta-HTML-server-push-boundary";

	  /* Only do this for the first time through. */
	  if (!called_yet)
	    {
	      char *nph = pagefunc_get_variable ("mhtml::unparsed-headers");
	      Symbol *sym = symbol_remove ("mhtml::unparsed-headers");

	      called_yet++;

	      output_fd = mhtml_stdout_fileno;

	      if (nph)
		bprintf (pushed, "HTTP/1.0 200\n");

	      symbol_free (sym);
	      pagefunc_set_variable ("mhtml::server-pushed", "true");
	      bprintf (pushed, "Content-type: multipart/x-mixed-replace;");
	      bprintf (pushed, "boundary=%s\n\n", boundary);
	      bprintf (pushed, "%s\n", boundary);
	    }

	  if (type == (char *)NULL)
	    type = strdup ("text/html");

	  bprintf (pushed, "Content-type: %s\n", type);
	  bprintf (pushed, "Content-length: %d\n\n", text->bindex);
	  write (output_fd, pushed->buffer, pushed->bindex);
	  write (output_fd, text->buffer, text->bindex);
	  page_free_page (text);
	  write (output_fd, "\n", 1);
	  write (output_fd, boundary, strlen (boundary));
	  write (output_fd, "\n", 1);
	  free (type);
	  page_free_page (pushed);
	}
    }
}

char *
html_quote_string (char *string)
{
  BPRINTF_BUFFER *newstring = bprintf_create_buffer ();
  char *result;

  if (string != (char *)NULL)
    {
      register int i;

      for (i = 0; string[i] != '\0'; i++)
	{
	  if (string[i] == '<')
	    bprintf (newstring, "&lt;");
	  else if (string[i] == '>')
	    bprintf (newstring, "&gt;");
	  else if (string[i] == '&')
	    bprintf (newstring, "&amp;");
	  else
	    bprintf (newstring, "%c", string[i]);
	}
    }

  result = newstring->buffer;
  free (newstring);

  return (result);
}

DEFINE_SECTION (DATES-AND-TIMES,
		canonical date; time functions; clock; zone,
"<Meta-HTML> provides a number of pre-defined functions for manipulating\n\
date and time objects.  Along with the primitive builtins of <tag date>\n\
and <tag time>, there are several functions defined with the prefix of\n\
<code>date</code> which can be used to parse dates, convert between time\n\
and date formats, format time strings in a variety of ways, and even\n\
produce HTML-based calendars and the like.", "")

DEFUN (pf_time, &optional date-string &key zone,
"Returns the current local time, as measured in seconds since the\n\
epoch (the change from 23:59:59, December 31, 1969 to 00:00:00 Jan 1, 1970).\n\
\n\
If passed <var date-string>, <tag time> calls\n\
<funref DATES-AND-TIMES date::date-to-time> on that value instead.\n\
\n\
This is often useful as input to the <funref dates-and-times date>\n\
operator.  Here is how one might find out the printable date for the\n\
time 10 hours from now.\n\
\n\
<example>\n\
   <date> --> Wed Jul  3 17:14:53 1996\n\
   <date <add <time> <mul 60 60 10>>>\n\
          --> Thu Jul  4 03:14:57 1996\n\
</example>")
{
  char *date_string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *zone_arg = mhtml_evaluate_string (get_value (vars, "zone"));
  unsigned long ticks = (unsigned long)time ((time_t *)0);

  if (empty_string_p (date_string))
    bprintf_insert (page, start, "%ld", ticks);
  else
    {
      BPRINTF_BUFFER *b = bprintf_create_buffer ();
      char *result = (char *)NULL;

      bprintf (b, "<date::date-to-time %s", quote_for_setvar (date_string));
      if (!empty_string_p (zone_arg))
	bprintf (b, " zone=%s", quote_for_setvar (zone_arg));
      bprintf (b, ">");
      result = mhtml_evaluate_string (b->buffer);
      bprintf_free_buffer (b);

      if (result != (char *)NULL)
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart += strlen (result);
	  free (result);
	}
    }
  xfree (date_string);
  xfree (zone_arg);
}

DEFUN (pf_date, &optional seconds-since-epoch &key gmt=true zone=ZONE,
"Returns the date as a string of 24 characters.\n\
\n\
The output looks like <code><date></code>.\n\
\n\
Given the optional argument <var epoch-seconds>, this number is\n\
treated as the number of seconds since 12 midnight, December 31st,\n\
1969, and is converted into the current date.\n\
\n\
When the keyword argument <var gmt=true> is supplied, returns the date at\n\
the meridian (Greenwich Mean Time) in the same format as used by HTTP\n\
headers, such as the <b>Expires</b> header.\n\
\n\
Examples:\n\
<example>\n\
<date>                  --> Thu Nov 13 10:52:07 1997\n\
<date gmt=true>         --> Thu, 13 Nov 1997 18:52:07 GMT\n\
<date <add <time> 60>>  --> Thu Nov 13 10:53:07 1997\n\
</example>\n\
\n\
Also see the <funref dates-and-times time> function.")
{
  char *tstring = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *zone_arg = mhtml_evaluate_string (get_value (vars, "zone"));
  char *use_zone = zone_arg;
  char *gmt = mhtml_evaluate_string (get_value (vars, "gmt"));
  time_t ticks = tstring ? (time_t)atol (tstring) : (time_t)time ((time_t *)0);
  char *time_string = (char *)NULL;
  char *temp = (char *)NULL;

  if (empty_string_p (use_zone))
    use_zone = pagefunc_get_variable ("*date*::timezone");

  if (empty_string_p (use_zone))
    use_zone = pagefunc_get_variable ("env::tz");

  if (empty_string_p (use_zone))
    use_zone = (char *)getenv ("TZ");

  if (!empty_string_p (use_zone))
    {
      extern char **environ;
      char **saved_environ = environ;
      char *fake_environ[2];
      BPRINTF_BUFFER *b = bprintf_create_buffer ();
      char *zone_used;

      bprintf (b, "timezone-translations::%s", use_zone);
      zone_used = pagefunc_get_variable (b->buffer);
      b->bindex = 0;

      if (empty_string_p (zone_used))
	zone_used = use_zone;

      bprintf (b, "TZ=%s", zone_used);
      fake_environ[0] = b->buffer;
      fake_environ[1] = (char *)NULL;
      environ = fake_environ;

      if (empty_string_p (tstring))
	ticks = (time_t)time ((time_t *)0);

      {
	struct tm *temp_time;

	temp_time = (struct tm *)localtime (&ticks);
	ticks = mktime (temp_time);
      }

      time_string = ctime (&ticks);

      environ = saved_environ;
      bprintf_free_buffer (b);
    }

  if (gmt != (char *)NULL)
    time_string = http_date_format ((long) ticks);
  else if (time_string == (char *)NULL)
    time_string = ctime (&ticks);

  temp = strchr (time_string, '\n');
  if (temp) *temp = '\0';

  bprintf_insert (page, start, "%s", time_string);
  *newstart += strlen (time_string);
  xfree (tstring);
  xfree (zone_arg);
  xfree (gmt);
}

DOC_SECTION (LANGUAGE-OPERATORS)
DEFUN (pf_pid, ,
"Returns the process ID of the currently executing Meta-HTML\n\
interpreter, on those systems which have process IDs.")
{
  pid_t pid = getpid ();

  bprintf_insert (page, start, "%ld", (unsigned long)pid);
}

#if defined (__cplusplus)
}
#endif
