/* sessionfuncs.c: -*- C -*-  Functions for manipulating session data. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Thu Sep 12 05:42:28 1996.  */

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
#include "session_data.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/************************************************************/
/*							    */
/*		Session Manipulation Functions		    */
/*							    */
/************************************************************/

static void pf_create_session (PFunArgs);
static void pf_delete_session (PFunArgs);
static void pf_set_session_var (PFunArgs);
static void pf_get_session_var (PFunArgs);
static void pf_unset_session_var (PFunArgs);
static void pf_set_session_timeout (PFunArgs);
static void pf_require_session (PFunArgs);
static void pf_session_export (PFunArgs);
static void pf_session_import (PFunArgs);
static void pf_with_locked_session (PFunArgs);
static void pf_sessions_of_key (PFunArgs);
static void pf_get_session_data (PFunArgs);
static void pf_set_session_data (PFunArgs);
static void pf_set_session_db (PFunArgs);
#if defined (MHTML_CRYPTOGRAPHY)
static void pf_set_session_encryption_key (PFunArgs);
#endif /* MHTML_CRYPTOGRAPHY */

static PFunDesc func_table[] =
{
  /* The following functions deal with the session database. */
  { "CREATE-SESSION",	0, 0, pf_create_session },
  { "DELETE-SESSION",	0, 0, pf_delete_session },
  { "SET-SESSION-VAR",	0, 0, pf_set_session_var },
  { "GET-SESSION-VAR",	0, 0, pf_get_session_var },
  { "UNSET-SESSION-VAR",0, 0, pf_unset_session_var },
  { "SET-SESSION-TIMEOUT", 0, 0, pf_set_session_timeout },
  { "REQUIRE-SESSION",	0, 0, pf_require_session },
  { "SESSION-EXPORT",	0, 0, pf_session_export },
  { "SESSION-IMPORT",	0, 0, pf_session_import },
  { "WITH-LOCKED-SESSION", 1, 0, pf_with_locked_session },
  { "SESSIONS-OF-KEY",	0, 0, pf_sessions_of_key },
  { "GET-SESSION-DATA", 0, 0, pf_get_session_data },
  { "SET-SESSION-DATA", 0, 0, pf_set_session_data },
  { "SET-SESSION-DB",	0, 0, pf_set_session_db },

#if defined (MHTML_CRYPTOGRAPHY)
  { "SET-SESSION-ENCRYPTION-KEY", 0, 0, pf_set_session_encryption_key },
#endif /* MHTML_CRYPTOGRAPHY */

#if defined (DEPRECATED)
  /* Deprecate this as soon as possible. */
  { "SESSION-DATA-ALIST", 0, 0, pf_get_session_data },
#endif

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_session_functions)
DEFINE_SECTION (SESSION-OPERATORS, ,
"<Meta-HTML> solves the problem of keeping state for a particular
session, and it does it in a server and browser independent
manner. Our method requires no additional CGI programs to be written,
and you don't have to be a programmer to take advantage of it.

The basic essence of sessions is the <i>session database</i>.  An
entry in the session database can have exactly one of three possible
states: <i>present</i>, <i>missing</i>, or <i>timed out</i>.",

"Using the session commands, you can:

<ul>
<li>
<b>Create a session.</b><br> This generates a unique entry
in the session database, and returns that information as a session
identifier (<strong>SID</strong>).  You can associate a special
<i>key</i> with the session at creation time, and you can decide
whether the key must be unique or not.  Session information can be
located by either the SID or the key.

<p><li> <b>Delete a session.</b><br>
This removes the entry in the database associated with a specific SID,
thus placing the session in the \"missing\" state.

<p><li> <b>Change a session's timeout value.</b><br>
With each session there is an associated <i>timeout</i> value, which
is the number of minutes the session can live without any activity on
the part of the user.  When the session is created, a default timeout
is installed; currently this is five minutes.  Using
<funref SESSION-OPERATORS >set-session-timeout>, you can change this
value. A special signal value of \"<code>0</code>\" causes the session
to live forever (or until it is deleted with <funref SESSION-OPERATORS
delete-session>).

<p><li> <b>Require a session to be present.</b><br>
Using the <funref session-operators require-session> form allows you
to check the state of the session, and optionally execute code for the
cases where it is missing or timed out.

<p><li> <b>Manipulate persistent variables.</b><br>
Commands exist to associate a name with a value in the session
database, to retrieve the value associated with a name in the session
database, and to store and retrieve a group of associations at once
from the session database.
</ul>")

DEFVAR (SID, "The current session ID, as set by the function
<funref session-operators session::initialize>.

The session ID is passed to and from the browser by either the use of
HTTP Cookies, or in the URL, as the first element of the path.

Normally, you never need to reference this variable directly, since
the various internal functions which deal with sessions know how to
set, get, and/or modify it without your help.

Related functions: <funref Session-Operators create-session>,
<funref Session-Operators delete-session>, <funref Session-Operators
set-session-var>.")

DEFVAR (mhttpd::session-database-file, "The fully qualified pathname to the
current session database.  Setting this variable only has meaning in the
<code>mhttpd.conf</code> or <code>engine.conf</code> files.

If this variable is not set, then the default value of
<code>\"/tmp/sessions.db\"</code> is used.

If the server or engine cannot access this file, no session state
information may be saved or retrieved.")

DEFVAR (MHTML::SID-PREFIX, "When this variable is present, the
<code>PATH</code> component of a returned <code>Set-Cookie</code> MIME
header is set to its value.  Normally, the value of <varref
MHTML::RELATIVE-PREFIX> is used.

If you need to use <code>MHTML::SID-PREFIX</code>, you probably have a
very complex directory structure, and the need for every page access
to have a cookie associated with it.  In this case, setting
<code>MHTML::SID-PREFIX</code> will have the effect of flattening your
directory structure in the browser's view.

Generally, correctly written <meta-html> applications do not need to
use this variable, and it might be used in only the most complicated
of Web sites, with multiple sessions per connectee, etc.

The explanation of HTTP Cookies is beyond the scope of this document.
Reference information on Cookies can be found through <a
href=\"http://home.netscape.com/newsref/std/cookie_spec.html\"> this
link.</a>")

/* Get the values of VAR1 .. VARn and concatenate them to make a key.
   Create the session with that key.
   Special tag "allow-multiple" allows more than one sesson with same key.
   If VAR1 doesn't have a posted value, and there is a value supplied
   for VAR1 in the function call, that value is returned as the final
   page.  Example:
   <create-session
     USERNAME="<set-var ERROR-MESSAGE=\"You must supply a Username\">
		<include registration.html>"
     PASSWORD="<set-var ERROR-MESSAGE=\"You must supply a Password\">
		<include registration.html>"
     ALT="<set-var ERROR-MESSAGE=\"You are already signed on!\">
		<include begin.html>"
   >  */
DEFUN (pf_create_session,
       &optional var=missing-code... &key alt=altcode allow-multiple,
"Create a session, and place the session identifier in the variable
<var default::sid>. The key for the session is created by
concatenating the values of the <var VAR>s together, separated by
hyphens (-). If a <var VAR> doesn't have a value currently assigned in
the default package, but does have an optional <var MISSING-CODE>
assignment, then the session is not created, and the <var
MISSING-CODE> for that variable is evaluated. If all of the variables
have values (or have no missing action), and the session could not be
created, then <var ALTCODE> is evaluated if it is present.

Finally, if the keyword <var ALLOW-MULTIPLE> is present, it indicates
that the key does not have to be unique; it is enough that the session
is created and a session ID returned.

Example:
<example>
<create-session
 USERNAME=<group <set-var ERROR-MESSAGE=\"You must supply a Username\">
                 <include registration.html>>
 PASSWORD=<group <set-var ERROR-MESSAGE=\"You must supply a Password\">
                 <include registration.html>>
      ALT=<group <set-var ERROR-MESSAGE=\"You are already signed on!\">
                 <include begin.html>>>
</example>")
{
  register int i;
  BPRINTF_BUFFER *key_buffer = (BPRINTF_BUFFER *)NULL;
  char *alternate;
  int allow_multiple_p = var_present_p (vars, "ALLOW-MULTIPLE");
  session_id_t sid;
  char **names = get_vars_names (vars);
  char **vals = get_vars_vals (vars);

  alternate = get_value (vars, "ALT");

  /* If there is already a session ID, then run the ALTernate code. */
  if (get_value (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID"))
    {
      if (alternate != (char *)NULL)
	bprintf_insert (page, start, "%s", alternate);
      return;
    }

  for (i = 0; names && names[i]; i++)
    {
      char *value;

      if ((strcasecmp (names[i], "ALT") == 0) ||
	  (strcasecmp (names[i], "ALLOW-MULTIPLE") == 0))
	continue;

      value = forms_get_tag_value (names[i]);

      if (value != (char *)NULL)
	{
	  if (!key_buffer)
	    key_buffer = bprintf_create_buffer ();

	  bprintf (key_buffer, "%s%s", key_buffer->bindex ? "-" : "", value);
	}
      else
	{
	  value = vals[i];

	  if (value != (char *)NULL)
	    {
	      bprintf_insert (page, start, "%s", value);
	      if (key_buffer) bprintf_free_buffer (key_buffer);
	      return;
	    }
	}
    }

  /* session_reap (); */
  if (key_buffer)
    {
      sid = session_begin (key_buffer->buffer, allow_multiple_p);
      bprintf_free_buffer (key_buffer);
    }
  else
    sid = session_begin ("Meta-HTML-Anonymous", allow_multiple_p);

  if (sid != (session_id_t)0)
    {
      char symbuff[1024];
      sprintf (symbuff, "%s::SID", DEFAULT_PACKAGE_NAME);
      pagefunc_set_variable (symbuff, (char *)sid);
    }
  else
    {
      if (alternate != (char *)NULL)
	bprintf_insert (page, start, "%s", alternate);
      else
	page_debug ("Couldn't create a session!");
    }
}

DEFUN (pf_delete_session, ,
"Immediately end the current session. This never returns anything. The
current session is indicated by the value of the variable <var
default::sid>, so you can delete sessions other than your own by
setting that variable. Also see <funref SESSION-OPERATORS sessions-of-key>.")
{
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid)
    session_end (sid);
}

static int
session_lock (session_id_t sid)
{
  char lockname[100];
  int fd;

  sprintf (lockname, "/tmp/meta-html-%s", (char *)sid);
  fd = os_open (lockname, O_CREAT | O_WRONLY | O_APPEND, 0666);

  if ((fd < 0) || (LOCKFILE (fd) == -1))
    {
      if (fd > -1)
	{
	  char pid_name[100];
	  sprintf (pid_name, "%ld\n", (long)getpid ());
	  write (fd, (void *)pid_name, (size_t) strlen (pid_name));
	  close (fd);
	  fd = -1;
	}
    }

  return (fd);
}

static void
session_unlock (int lock, session_id_t sid)
{
  if (lock > -1)
    {
      char lockname[100];

      sprintf (lockname, "/tmp/meta-html-%s", (char *)sid);
      unlink (lockname);
      UNLOCKFILE (lock);
      close (lock);
    }
}

DEFMACRO (pf_with_locked_session, ,
"Executes <var body> in an environment where access to the current
session is blocked. This allows concurrency (such as might be required
when multiple <code>GET</code>s are being processed for a single user)
to take place without introducing any race conditions for access to
the current session record of the session database.")
{
  int jump_again = 0;
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid != (session_id_t)0)
    {
      int lock = session_lock (sid);
      PAGE *body_code = page_copy_page (body);
      PageEnv *page_environ = pagefunc_save_environment ();

      if ((jump_again = setjmp (page_jmp_buffer)) == 0)
	page_process_page_internal (body_code);

      pagefunc_restore_environment (page_environ);

      if (body_code != (PAGE *)NULL)
	{
	  if (!jump_again && (body_code->buffer != (char *)NULL))
	    {
	      bprintf_insert (page, start, "%s", body_code->buffer);
	      *newstart = start + (body_code->bindex);
	    }

	  page_free_page (body_code);
	}

      session_unlock (lock, sid);
    }
  if (jump_again) longjmp (page_jmp_buffer, 1);
}

DEFUN (pf_sessions_of_key, key,
"Returns a newline separated list of the session ID's whose key
matches <var key>.

If <var key> is not specified, or if it contains only whitespace, all
session ID's in the session database are returned. Since the list is
newline separated, it is suitable for assignment to an array, as in:

<example>
<set-var sids[]=<sessions-of-key \"On-Line Shopping\">>
</example>")
{
  register int i;
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  SESSION_INFO **info_list = session_all_sessions ();
  SESSION_INFO *info;

  for (i = 0;
       (info_list != (SESSION_INFO **)NULL) &&
       ((info = info_list[i]) != (SESSION_INFO *)NULL);
       i++)
    {
      if (empty_string_p (arg) ||
	  ((info->key != (char *)NULL) && strcasecmp (info->key, arg) == 0))
	{
	  bprintf_insert (page, start, "%s\n", (char *)info->sid);
	  start += 1 + strlen ((char *)info->sid);
	}

      session_free (info);
    }

  if (arg) free (arg);

  if (info_list) free (info_list);
  *newstart = start;
}
      
DEFUN (pf_set_session_var, name=value...,
"Give the <var name>s the associated <var value>s in the session
database record associated with the current session.")
{
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid != (session_id_t)0)
    {
      SESSION_INFO *info = session_get_info (sid);

      if ((info != (SESSION_INFO *)NULL) && (vars != (Package *)NULL))
	{
	  register int i;
	  Package *session_data = symbol_get_package ((char *)NULL);
	  char **names = get_vars_names (vars);
	  char **vals = get_vars_vals (vars);

	  sd_info_to_package (info, session_data);

	  for (i = 0; names[i] != (char *)NULL; i++)
	    {
	      char *name = names[i], *value = vals[i];

	      name = mhtml_evaluate_string (name);

	      if (value != (char *)NULL)
		value = mhtml_evaluate_string (value);

	      forms_set_tag_value_in_package (session_data, name, value);
	      if (value) free (value);
	      if (name) free (name);
	    }

	  sd_package_to_info (info, session_data);
	  session_put_info (info);
	  symbol_destroy_package (session_data);
	}

      if (info) session_free (info);
    }
}

DEFUN (pf_get_session_var, &optional name...,
"Return the value of the <var name>s given from the session data
associated with the current session.  Each <var name> is a
variable name which has had a value assigned to it with <funref
SESSION-OPERATORS set-session-var>, or was created implicity via
<funref SESSION-OPERATORS session-export>.

The values are returned in the order in which the <var name>s appear.

Examples:
<set-session-db /tmp/<make-identifier>>
<session::initialize>
<complete-example>
<set-session-var foo=Var-1 bar=Var-2>
<get-session-var foo>, <get-session-var bar>
</complete-example>

Example of retrieving a single session variable from a session
package:
<example>
<set-var record::name=\"Brian Fox\">
<set-var record::job=\"Programmer\">
<session-export record record>
<get-session-var record::name> --> Brian Fox
</example>")
{
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid != (session_id_t)0)
    {
      SESSION_INFO *info = session_get_info (sid);

      if ((info != (SESSION_INFO *)NULL) && (vars != (Package *)NULL))
	{
	  register int i;
	  Package *session_data = symbol_get_package ((char *)NULL);
	  char **names = get_vars_names (vars);

	  sd_info_to_package (info, session_data);

	  for (i = 0; names[i] != (char *)NULL; i++)
	    {
	      char *name = names[i], *value = (char *)NULL;

	      name = mhtml_evaluate_string (name);

	      value = forms_get_tag_value_in_package (session_data, name);

	      if (value != (char *)NULL)
		{
		  bprintf_insert (page, start, "%s", value);
		  start += strlen (value);
		}

	      if (name) free (name);
	    }
	  symbol_destroy_package (session_data);
	}

      if (info) session_free (info);
    }
}

/* <get-session-data [sid]> --> Alist of the session data for SID. */
DEFUN (pf_get_session_data, &optional sid,
"Returns an association list representing all of the data in the
session database associated with <var sid>.

If <var sid> is not supplied, it defaults to the value of <var
default::sid> (i.e., the current session ID).")
{
  char *passed = mhtml_evaluate_string (get_positional_arg (vars, 0));
  session_id_t sid;

  if (!empty_string_p (passed))
    sid = (session_id_t)passed;
  else
    sid = (session_id_t)get_value
      (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid != (session_id_t)0)
    {
      SESSION_INFO *info = session_get_info (sid);

      if ((info != (SESSION_INFO *)NULL) &&
	  (info->data != (unsigned char *)NULL))
	{
	  bprintf_insert (page, start, "%s", (char *)info->data);
	  *newstart += strlen ((char *)info->data);
	}

      if (info) session_free (info);
    }
  if (passed != (char *)NULL) free (passed);
}

/* Punt.  Not because we can't do it, but because it doesn't make good
   sense to do it in set_session_data(). */
#define valid_alist_p(string) 1
  
/* <set-session-data [alist] [sid]> --> Set session info from ALIST. */
DEFUN (pf_set_session_data, alist &optional sid,
"Sets all of the session data associated with the session ID <var sid>
to the contents of the association list <var alist>.  If <var sid> is
not supplied, it defaults to the value of <var default::sid>.

This construct is really of use only for those programs which deal
directly with the session database, such as a backend tool for viewing
and manipulating session data.  For all other applications, the use of
<funref SESSION-OPERATORS set-session-var> and <funref
SESSION-OPERATORS session-export> should be more than sufficient.")
{
  char *alist = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *passed_sid = mhtml_evaluate_string (get_positional_arg (vars, 1));
  session_id_t sid;

  if (!empty_string_p (passed_sid))
    sid = (session_id_t)passed_sid;
  else
    sid = (session_id_t)get_value
      (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if ((sid != (session_id_t)0) && (!empty_string_p (alist)))
    {
      SESSION_INFO *info = session_get_info (sid);

      if (info == (SESSION_INFO *)NULL)
	{
	  time_t now = (time_t)time ((time_t *)NULL);
	  info = (SESSION_INFO *)xmalloc (sizeof (SESSION_INFO));
	  memset (info, 0, sizeof (SESSION_INFO));
	  info->sid = (session_id_t)strdup ((char *)sid);
	  info->key = strdup ("Anonymous Restarted");
	  info->start = now;
	  info->access = now;
	  info->timeout = 200;
	}

      if (info->data != (unsigned char *)NULL)
	free (info->data);

      if (valid_alist_p (alist))
	{
	  info->data = (unsigned char *)strdup (alist);
	  info->length = strlen (alist);
	}
      else
	{
	  info->data = (unsigned char *)NULL;
	  info->length = 0;
	}
      session_put_info_internal (info);
      session_free (info);
    }

  xfree (alist);
  xfree (passed_sid);
}

DEFUN (pf_unset_session_var, &optional name...,
"Analogous to <funref VARIABLES unset-var>, <code>unset-session-var</code>
removes the variables named by the given <var name>s from the session
data associated with the current session.
<example>
<set-session-var foo=bar>
<get-session-var foo>     --> bar
<unset-session-var foo>
<get-session-var foo>     -->
</example>")
{
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid != (session_id_t)0)
    {
      SESSION_INFO *info = session_get_info (sid);

      if ((info != (SESSION_INFO *)NULL) && (vars != (Package *)NULL))
	{
	  register int i;
	  Package *session_data = symbol_get_package ((char *)NULL);
	  char **names = get_vars_names (vars);

	  sd_info_to_package (info, session_data);

	  for (i = 0; names[i] != (char *)NULL; i++)
	    {
	      char *name = names[i];

	      name = mhtml_evaluate_string (name);

	      if (name != (char *)NULL)
		symbol_remove_in_package (session_data, name);

	      if (name && name != names[i]) free (name);
	    }

	  sd_package_to_info (info, session_data);
	  session_put_info (info);
	  symbol_destroy_package (session_data);
	}
      if (info) session_free (info);
    }
}

DEFUN (pf_session_export, package &optional to-package,
"Save the package variables of <var package> in the current session.
If <var to-package> is supplied, it indicates a new  package name to
assign to each variable from <var package> that is saved.

This function is quite useful when you wish to save all of the
variables that were <b>posted</b> from the previous  URL (presumably,
that URL contained a form whose <b>action</b> pointed to the current
URL) in the session database record for the current session.  For
example, if the previous URL posted the variables <code>FOO</code> and
<code>BAR</code>, then executing:

<example code><session-export posted blue></example>

results in the session database containing values for
<code>blue::FOO</code> and <code>blue::BAR</code>.  These values can
either be looked up directly, or retrieved as a group 
(see <funref session-operators session-import> for more information.)")
{
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid != (session_id_t)0)
    {
      SESSION_INFO *info = session_get_info (sid);

      if (info != (SESSION_INFO *)NULL)
	{
	  char *internal_name, *external_name;
	  Package *package, *session_data;
	  Symbol *sym, **symbols = (Symbol **)NULL;

	  internal_name = mhtml_evaluate_string (get_positional_arg (vars, 0));
	  external_name = mhtml_evaluate_string (get_positional_arg (vars, 1));
	  session_data = symbol_get_package ((char *)NULL);
	  sd_info_to_package (info, session_data);

	  if (empty_string_p (internal_name))
	    package = CurrentPackage;
	  else
	    package = symbol_get_package (internal_name);

	  if (package != (Package *)NULL)
	    symbols = symbols_of_package (package);

	  if (symbols != (Symbol **)NULL)
	    {
	      register int i;
	      char *prefix = (char *)NULL;
	      int prefix_len = 0;

	      if (external_name == (char *)NULL)
		external_name = package->name;

	      if (!empty_string_p (external_name))
		prefix = external_name;
	      else if (!empty_string_p (internal_name))
		prefix = internal_name;

	      if (prefix)
		prefix_len = strlen (prefix);

	      for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
		{
		  char *save_name = sym->name;
		  Symbol *newsym;

		  if (prefix_len)
		    {
		      save_name = (char *)xmalloc
			(3 + prefix_len + sym->name_len);
		      sprintf (save_name, "%s::%s", prefix, sym->name);
		    }

		  newsym = symbol_copy (sym, session_data);
		  symbol_rename (newsym, save_name);

		  if (save_name != sym->name)
		    free (save_name);
		}
	      free (symbols);
	    }
	  sd_package_to_info (info, session_data);
	  session_put_info (info);
	  symbol_destroy_package (session_data);
	  session_free (info);
	}
    }
}

DEFUN (pf_session_import, package &optional to-package,
"Get the values of the session variables which belong to <var package>
and set their values in the current package. If <var to-package> is
supplied, it is the name of the package which will receive the
variable values. Thus, if the session data for the current session
contains:

<example code>((BLUE::FOO . bar))</example>

and then at a later time in the same session the command:

<example code><session-import blue></example>

is given, then the variable <code>FOO</code> in the current package
will be set to <code>bar</code>.  This is extremely useful when
bouncing back and forth between forms, see <funref sessions
session-export>.")
{
  SESSION_INFO *info = (SESSION_INFO *)NULL;
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (sid != (session_id_t)0)
    info = session_get_info (sid);

  /* If there is session info, snarf the vars.. */
  if (info != (SESSION_INFO *)NULL)
    {
      Package *session_data = symbol_get_package ((char *)NULL);
      Symbol **symbols;

      sd_info_to_package (info, session_data);
      symbols = symbols_of_package (session_data);

      if (symbols != (Symbol **)NULL)
	{
	  register int i;
	  char *external = mhtml_evaluate_string(get_positional_arg (vars, 0));
	  char *internal = mhtml_evaluate_string(get_positional_arg (vars, 1));
	  int external_len = (external ? strlen (external) : 0);
	  int internal_len = (internal ? strlen (internal) : 0);
	  Symbol *sym;

	  /* Import variables skipping those that don't start with EXTERNAL.
	     Strip EXTERNAL from imported variable names, perhaps placing
	     them in the package specified by INTERNAL. */
	  for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	    {
	      int import_it = 1;

	      if (external_len)
		{
		  if ((strncasecmp (sym->name, external, external_len) == 0) &&
		      (sym->name[external_len] == ':' &&
		       sym->name[external_len + 1] == ':'))
		    import_it = 1;
		  else
		    import_it = 0;
		}

	      if (import_it)
		{
		  char *sym_name = sym->name;
		  char *new_sym_name;
		  Symbol *killer;

		  if (external_len)
		    sym_name += external_len + 2;

		  new_sym_name = sym_name;

		  /* If there is no internal or external package prefix,
		     then import this symbol into the package that is
		     named in the symbol name. */
		  if ((internal_len + external_len) == 0)
		    {
		      char *temp_name;
		      killer = symbol_intern (sym_name);
		      temp_name = strdup (killer->name);
		      sym = symbol_copy (sym, (Package *)killer->package);
		      symbol_rename (sym, temp_name);
		      free (temp_name);
		    }
		  else
		    {
		      char *intern_name = sym_name;
		      Package *import_package;

		      /* Get the package that we are importing to. */
		      if (!internal_len)
			import_package = CurrentPackage;
		      else
			import_package = symbol_get_package (internal);

		      /* If no external name was mentioned, but an internal
			 package exists, then strip off the package name
			 of this symbol before interning it. */
		      if (external == (char *)NULL)
			{
			  intern_name = strstr (sym_name, "::");
			  if (!intern_name)
			    intern_name = sym_name;
			}

		      intern_name = strdup (intern_name);

		      /* Delete this symbol from the import package. */
		      killer =
			symbol_remove_in_package (import_package, intern_name);
		      symbol_free (killer);

		      /* Place this symbol in the import package. */
		      sym = symbol_copy (sym, import_package);
		      symbol_rename (sym, intern_name);
		      free (intern_name);
		    }
		}
	    }
	  free (symbols);
	}
      symbol_destroy_package (session_data);
      session_free (info);
    }
}


DEFUN (pf_set_session_timeout, minutes,
"Make the current session time out after <var minutes> minutes have
transpired since the last access to that session.

You cannot supply the value of (<code>0</code>); that is accomplished
with <funref sessions delete-session>.")
{
  char *timeout_ascii = mhtml_evaluate_string (get_positional_arg (vars, 0));
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if ((sid != (session_id_t)0) && (timeout_ascii != (char *)NULL))
    {
      int timeout_val = atoi (timeout_ascii);

      if (timeout_val > 0)
	{
	  SESSION_INFO *info = session_get_info (sid);

	  if (info != (SESSION_INFO *)NULL)
	    {
	      info->timeout = timeout_val;
	      session_put_info (info);
	      session_free (info);
	    }
	}
    }
}

DEFUN (pf_require_session, &key missing=missing-code timeout=timeout-code,
"Check that the current session is valid and not timed out.  If either
of those things are true, the respective code is evaluated.  Usually,
the targets of <var TIMEOUT> and <var MISSING> are a <funref
FILE-OPERATORS redirect> command which points to a page explaining the
problem.

You could force an anonymous session with the following code:

<example>
  <set-var anonymous=\"Anonymous Session\">
  <require-session
    missing=\"<create-session anonymous allow-multiple>
             <redirect <get-var SID>/<get-var mhtml::current-doc>\"
    timeout=<redirect /timed-out.mhtml>>
</example>

In the real world, additional care should be taken when returning new
URLs.  The variable <var mhtml::cookie-compatible> is
<code>true</code> when the attached browser/server combination is
capable of handling HTTP Cookies.  In this case, it is preferable to
use the HTTP Cookie handling facilities instead of returning the
session ID in the URL.  Here is how one might write the above example
to handle both cases:

<example>
<set-var anonymous=\"Anonymous Session\" session-timeout=60>
<require-session
 missing = <prog
             <create-session anonymous allow-multiple>
             <set-session-timeout <get-var session-timeout>>
             <when <not <get-var mhtml::cookie-compatible>>>
             <redirect <concat
                        <get-var mhtml::http-to-host>/
                        <get-var SID><get-var mhtml::relative-prefix>/
                        <get-var mhtml::current-doc>>>
             </when>>
 timeout = <prog
             <set-var SID=\"\">
             <if <get-var mhtml::cookie-compatible>
                 <replace-page
                  <get-var mhtml::relative-prefix>/timedout.mhtml>
               <redirect
                <get-var mhtml::url-to-dir-sans-sid>/timedout.mhtml>>>>
</example>

For more detail on the built-in variables provided by the <meta-html>
Server and CGI Engine, see the section <secref server-variables>.")
{
  char *missing_code = get_one_of (vars, "MISSING", "ALT", (char *)0);
  char *timeout_code = get_one_of (vars, "TIMEOUT", "TIMED-OUT", (char *)0);
  char *code = (char *)NULL;
  session_id_t sid = (session_id_t)get_value
    (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID");

  if (timeout_code == (char *)NULL)
    timeout_code = missing_code;

  if (empty_string_p ((char *)sid))
    code = missing_code;
  else if (session_access (sid) != 0)
    {
      symbol_free (symbol_remove_in_package
		   (symbol_lookup_package (DEFAULT_PACKAGE_NAME), "SID"));
      code = timeout_code;
    }

  if (code)
    bprintf_insert (page, start, "%s", code);
}

DEFUN (pf_set_session_db, pathname,
"Dynamically change the location of the file used to store session information.
You probably don't need this in your programs -- it is used within the server
and engine configuration files to specify the location of your session
database.")
{
  char *session_db_file = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (empty_string_p (session_db_file))
    {
      xfree (session_db_file);
      session_db_file = (char *)NULL;
    }

  set_session_database_location (session_db_file);
  pagefunc_set_variable ("mhttpd::session-database-file", session_db_file);

  xfree (session_db_file);
}

#if defined (MHTML_CRYPTOGRAPHY)
extern char *session_encryption_key;

DEFUN (pf_set_session_encryption_key, keystring,
"With non-empty argument <var keystring>, turns on the triple-DES
encryption of data saved to the session database.  The data is
encrypted and decrypted using <var key>.

This could usefully be placed in the \"mhttpd.conf\" or \"engine.conf\"
file with a statement similar to the following:

<example>
  <set-session-encryption-key <make-identifier 128>>
</example>

Don't turn this on unless you really have to, as it (obviously) affects
the performance of the session database.  (Each access to the database
results in a full call to triple ECB DES encryption, using the entire
contents of the session, and encrypting/decryption with a random 128-bit
key.)")
{
  char *key = mhtml_evaluate_string (get_positional_arg (vars, 0));

  xfree (session_encryption_key);
  session_encryption_key = (char *)NULL;

  if (!empty_string_p (key))
    session_encryption_key = strdup (key);

  xfree (key);
}
#endif /* MHTML_CRYPTOGRAPHY */


#if defined (__cplusplus)
}
#endif
