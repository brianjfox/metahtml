/* prims.c: -*- C -*-  Primitive internal functions. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jul 20 17:22:47 1996.  */

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

static void pf_eval (PFunArgs);
static void pf_read_sexp (PFunArgs);
static void pf_write_package_file (PFunArgs);
static void pf_read_package_file (PFunArgs);
static void pf_switch_user (PFunArgs);
static void pf_the_page (PFunArgs);
static void pf_point (PFunArgs);
static void pf_function_documentation (PFunArgs);
static void pf_function_arguments (PFunArgs);
static void pf_function_alist (PFunArgs);
static void pf_function_def (PFunArgs);
static void pf_quote_for_setvar (PFunArgs);
static void pf_after_page_return (PFunArgs);
static void pf_apply (PFunArgs);
static void pf_bootstrap_metahtml (PFunArgs);

static PFunDesc func_table[] =
{
  { "%%EVAL",			0, 0, pf_eval },
  { "%%READ-SEXP",		0, 0, pf_read_sexp },
  { "%%WRITE-PACKAGE-FILE",	0, 0, pf_write_package_file },
  { "%%READ-PACKAGE-FILE",	0, 0, pf_read_package_file },
  { "%%SWITCH-USER",		0, 0, pf_switch_user },
  { "%%THE-PAGE",		0, 0, pf_the_page },
  { "%%POINT",			0, 0, pf_point },
  { "%%FUNCTION-DOCUMENTATION",	0, 0, pf_function_documentation },
  { "%%FUNCTION-ARGUMENTS",	0, 0, pf_function_arguments },
  { "%%FUNCTION-ALIST",		0, 0, pf_function_alist },
  { "%%FUNCTION-DEF",		0, 0, pf_function_def },
  { "%%QUOTE-FOR-SET-VAR",	0, 0, pf_quote_for_setvar },
  { "%%AFTER-PAGE-RETURN",	1, 0, pf_after_page_return },
  { "APPLY",			-1, 0, pf_apply },
  { "%%BOOTSTRAP-METAHTML",	0, 0, pf_bootstrap_metahtml },

  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_primitive_functions)
DEFINE_SECTION (PRIMITIVE-OPERATORS, builtins; primitives; low-level,
"While most functions in <Meta-HTML> are used to create or manipulate Web\n\
pages, there may be times when you will wish to manipulate the <Meta-HTML>\n\
language itself, the low-level operation of the server, or have direct\n\
access to the current page that is executing, and information pertaining\n\
to the <Meta-HTML> parser itself.\n\
\n\
The functions described in this section allow just this type of low-level\n\
access.  Programming wizards may find these functions useful -- many of\n\
them are here to allow the implementation of core functionality in <Meta-HTML>\n\
in the <Meta-HTML> language itself.\n\
\n\
All of the primitive language operators begin with the two-character sequence\n\
of double percent signs (\"%%\") in order to distinguish them from the\n\
other, higher-level functions in <Meta-HTML>.", "")

DEFUNX (pf_%%eval, &rest body,
"Evaluate the result of evaluating <var body> and return that value.\n\
\n\
It would be a very rare case indeed, where a user-level program would require\n\
the use of this tag.  If you think you want to use this tag, you probably want\n\
to use <tag apply> instead.\n\
\n\
You may use this function to call another function on some arguments,\n\
where the other function is determined dynamically.  For example:\n\
<example>\n\
<if <set-in-session>\n\
    <set-var func=set-session-var>\n\
  <set-var func=set-var>>\n\
.blank\n\
<%%eval <<get-var-once func> <get-var-once name> = <get-var-once value>>>\n\
</example>")
static void
pf_eval (PFunArgs)
{
  char *result = body ? body->buffer : (char *)NULL;

  if (result != (char *)NULL)
    {
      char *expr = mhtml_evaluate_string (result);
      result = mhtml_evaluate_string (expr);

      if (!empty_string_p (result))
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart = start + strlen (result);
	}

      if (expr != (char *)NULL) free (expr);
      if (result != (char *)NULL) free (result);
    }
}

DEFUNX (pf_%%read_sexp, string-var &optional index-var,
"Reads one symbolic expression from <var string-var> and returns it.\n\
Sets <var index-var> to the first unread character in <var string-var>.\n\
If <var index-var> is empty, it defaults to zero.\n\
There isn't the slightest possibility that you need this in your\n\
programs.")
static void
pf_read_sexp (PFunArgs)
{
  char *string_var = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *index_var = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *result = (char *)NULL;

  if (!empty_string_p (string_var))
    {
      char *string = pagefunc_get_variable (string_var);
      int offset = 0;

      if (!empty_string_p (index_var))
	{
	  char *index_ascii = pagefunc_get_variable (index_var);

	  if (!empty_string_p (index_ascii))
	    offset = atoi (index_ascii);
	}

      if (!empty_string_p (string))
	{
	  if (offset < strlen (string))
	    {
	      int first_char;
	      char *sexp;

	      while (whitespace (string[offset])) offset++;
	      first_char = string[offset];

	      sexp = read_sexp_1 (string, &offset, 0, 1);

	      mhtml_set_numeric_variable (index_var, offset);
	      if (sexp)
		{
		  if (first_char == '"')
		    result = strdup (quote_for_setvar (sexp));
		  else
		    result = strdup (sexp);
		}
	    }
	}
    }

  if (!empty_string_p (result))
    {
      int len = strlen (result);
      bprintf_insert (page, start, "%s", result);
      *newstart += len;
    }

  xfree (result);
  xfree (index_var);
  xfree (string_var);
}

DEFUNX (pf_%%write_package_file, filename &key flagged no-code &rest packages,
"Writes the contents of <var packages> to the file specified by <var filename>.\n\
This function is used internally when creating libraries.  There isn't the\n\
slightest possibility that you need this in your programs -- if you think\n\
you do, you probably simply want to use <b>mklib</b>.\n\
\n\
If the keyword argument <var flagged> is set, is means to only write those\n\
symbols whose flags contain a FLAGGED value, i.e., where\n\
<example code><alist-get-var <symbol-info sym> FLAGGED></example> would\n\
return 'true'.\n\
\n\
Keyword argument <var no-code> with a non-null value, means don't bother\n\
saving the textual body of functions with the machine code -- it isn't\n\
generally necessary to do so, but the default is to save the bodies so\n\
that <Meta-HTML> installations which don't have the modmachine.so library\n\
installed can still read and execute the resultant '.lib' files.")
static void
pf_write_package_file (PFunArgs)
{
  char *filename = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *flagged = mhtml_evaluate_string (get_value (vars, "flagged"));
  char *no_source = mhtml_evaluate_string (get_value (vars, "no-source"));
  int packages_written = 0;

  if (!empty_string_p (filename))
    {
      int fd = os_open (filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);

      if (fd > -1)
	{
	  register int i = 1;
	  char *arg;

	  while ((arg = get_positional_arg (vars, i)) != (char *)NULL)
	    {
	      char *packname = mhtml_evaluate_string (arg);

	      if (!empty_string_p (packname))
		{
		  Package *pack = symbol_lookup_package (packname);

		  if (pack != (Package *)NULL)
		    {
		      symbol_dump_package (fd, pack,
					   !empty_string_p (flagged),
					   !empty_string_p (no_source));
		      packages_written++;
		    }
		}

	      if (packname) free (packname);
	      i++;
	    }
	  close (fd);
	}
    }

  xfree (filename);
  xfree (flagged);
  xfree (no_source);

  if (packages_written)
    bprintf_insert (page, start, "%d", packages_written);
}

DEFUNX (pf_%%read_package_file, filename,
"Reads package contents from the file specified by <var filename>, which\n\
had best be created using <funref primitive-operators %%write-package-file>.\n\
This function is used internally when loading libraries.  There isn't the\n\
slightest possibility that you need this in your programs -- if you think\n\
you do, you probably simply want to use <funref file-operators require>.")
static void
pf_read_package_file (PFunArgs)
{
  char *filename = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int packages_read = 0;

  if (!empty_string_p (filename))
    {
      int fd;

      /* If the filename specification doesn't start with a slash, then
	 add the current directory and relative path. */
      if (*filename != '/')
	{
	  BPRINTF_BUFFER *temp = bprintf_create_buffer ();
	  char *pre = pagefunc_get_variable ("mhtml::include-prefix");
	  char *rel = pagefunc_get_variable ("mhtml::relative-prefix");

	  if (pre)
	    {
	      if (!rel) rel = "";
	      bprintf (temp, "%s%s/%s", pre, rel, filename);
	      free (filename);
	      filename = temp->buffer;
	      free (temp);
	    }
	}

      fd = os_open (filename, O_RDONLY, 0666);

      if (fd > -1)
	{
	  Package *pack;

	  while ((pack = symbol_load_package (fd)) != (Package *)NULL)
	    packages_read++;

	  close (fd);
	}
    }

  if (filename) free (filename);

  if (packages_read)
    {
      bprintf_insert (page, start, "%d", packages_read);

      if (mhtml_user_keywords == (Package *)NULL)
	mhtml_user_keywords = 
	  symbol_get_package_hash ("*user-functions*", 577);
    }
}

DEFUNX (pf_%%switch_user, newuser,
"Changes the current user to <var newuser>, and returns \"true\" if the\n\
switch was successful.  This can only take place if <Meta-HTML> is\n\
running under the user ID of the superuser (root).  After calling this\n\
function, it is impossible to switch back to another user, so it really\n\
isn't of much use in Meta-HTML programs.")
static void
pf_switch_user (PFunArgs)
{
  char *username = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

#if defined (HAVE_GETPWNAM)
  if (!empty_string_p (username))
    {
      struct passwd *entry = (struct passwd *)getpwnam (username);

      if (entry != (struct passwd *)NULL)
	{
	  uid_t uid = (uid_t)entry->pw_uid;

	  if (setuid (uid) != -1)
	    result = "true";
	}
    }
#endif /* HAVE_GETPWNAM */

  if (username != (char *)NULL) free (username);

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

DEFUNX (pf_%%the_page,  &optional varname,
"Places the current page into <var varname> as a binary variable, or,\n\
returns the current page as text if <var varname> is not supplied.")
static void
pf_the_page (PFunArgs)
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  PAGE *the_page = parser_top_page ();

  if (empty_string_p (varname))
    {
      PAGE *contents = page_copy_page (the_page);
      bprintf_insert (page, start, "%s", contents->buffer);
      *newstart += contents->bindex;
      page_free_page (contents);
    }
  else
    {
      Symbol *sym = symbol_remove (varname);
      Datablock *block;
      symbol_free (sym);

      sym = symbol_intern (varname);
      block = datablock_create (the_page->buffer, the_page->bindex);
      sym->type = symtype_BINARY;
      sym->values = (char **)block;
    }

  xfree (varname);
}

DEFUNX (pf_%%point, ,
"Returns the current parser marker in the page.\n\
<b>CAVEAT</b>!  Currently, can only be called at top level.")
static void
pf_point (PFunArgs)
{
  bprintf_insert (page, start, "%d", start);
}

DEFUNX (pf_%%function_documentation,  user-function,
"Returns the documentation for <var user-function>.  Only works if the\n\
variable <var mhtml::gather-documentation> was set at the time the\n\
<var user-function> was defined.")
static void
pf_function_documentation (PFunArgs)
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (name))
    {
      UserFunction *uf = mhtml_find_user_function (name);

      if ((uf != (UserFunction *)NULL) && (uf->documentation != (char **)NULL))
	{
	  register int i;

	  for (i = 0; uf->documentation[i] != (char *)NULL; i++)
	    {
	      bprintf_insert (page, start, "%s\n", uf->documentation[i]);
	      start += 1 + strlen (uf->documentation[i]);
	    }

	  *newstart = start;
	}
    }

  xfree (name);
}

DEFUNX (pf_%%function_arguments, user-function,
"Returns an array of the formal parameters for <var user-function>.\n\
Essentially, this returns exactly what was entered at the time the\n\
function definition was defined.")
static void
pf_function_arguments (PFunArgs)
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (name))
    {
      UserFunction *uf = mhtml_find_user_function (name);

      if ((uf != (UserFunction *)NULL) &&
	  (uf->named_parameters != (char **)NULL))
	{
	  register int i;

	  for (i = 0; uf->named_parameters[i] != (char *)NULL; i++)
	    {
	      bprintf_insert (page, start, "%s\n", uf->named_parameters[i]);
	      start += 1 + strlen (uf->named_parameters[i]);
	    }

	  *newstart = start;
	}
    }

  xfree (name);
}

DEFUNX (pf_%%function_alist, user-function,
"Returns all information that we store about the user-defined function\n\
<var user-function>, in the form of an alist.  This function is used\n\
internally by the <Meta-HTML> grinder and beautifier -- it is unlikely\n\
in the extreme that one would use this in application programs -- it is\n\
meant for use in programs that manipulate the <Meta-HTML> language itself.")

static void
pf_function_alist (PFunArgs)
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (!empty_string_p (name))
    {
      UserFunction *uf = mhtml_find_user_function (name);

      if (uf != (UserFunction *)NULL)
	{
	  Package *p = symbol_get_package ((char *)NULL);
	  char *type_string;

	  switch (uf->type)
	    {
	    case user_MACRO:
	      {
		if (uf->flags & user_WEAK_MACRO)
		  type_string = "DEFWEAKMACRO";
		else
		  type_string = "DEFMACRO";
	      }
	      break;

	    case user_SUBST:
	      type_string = "DEFSUBST";
	      break;

	    case user_DEFUN:
	      type_string = "DEFUN";
	      break;

	    default:
	      type_string = "BOGUS TYPE!";
	    }

	  forms_set_tag_value_in_package (p, "type", type_string);
	  forms_set_tag_value_in_package (p, "name", uf->name);
	  forms_set_tag_value_in_package (p, "package",
					  uf->packname ? uf->packname : "");

	  mhtml_set_numeric_variable_in_package
	    (p, "debug-level", uf->debug_level);

	  forms_set_tag_value_in_package (p, "body", uf->body);

	  {
	    char **params = symbol_copy_array (uf->named_parameters);
	    Symbol *sym = symbol_intern_in_package (p, "parameters");
	    if (params != (char **)NULL)
	      {
		register int i;
		for (i = 0; params[i] != (char *)NULL; i++);
		sym->values = params;
		sym->values_index = i;
		sym->values_slots = i;
	      }
	  }

	  {
	    Symbol *sym = symbol_intern_in_package (p, "flags");

	    if (uf->flags & user_WHITESPACE_DELETED)
	      symbol_add_value (sym, "WHITESPACE-DELETED");

	    if (uf->flags & user_WHITESPACE_KEPT)
	      symbol_add_value (sym, "WHITESPACE-KEPT");

	    if (uf->flags & user_WEAK_MACRO)
	      symbol_add_value (sym, "WEAK-MACRO");

	    if (uf->flags & user_ACCEPT_KEYWORDS)
	      symbol_add_value (sym, "ACCEPT-KEYWORDS");
	  }

	  {
	    Symbol *uf_sym = mhtml_find_user_function_symbol (name);
	    if ((uf_sym != (Symbol *)NULL) &&
		(uf_sym->machine != (void *)NULL))
	      {
		Symbol *sym = symbol_intern_in_package (p, "machine?");
		symbol_add_value (sym, "true");
	      }
	  }

	  result = package_to_alist (p, 0);
	  symbol_destroy_package (p);
	}
      else
	{
	  PFunDesc *desc = pagefunc_get_descriptor (name);

	  if (desc != (PFunDesc *)NULL)
	    {
	      Package *p = symbol_get_package ((char *)NULL);
	      Symbol *sym = symbol_intern_in_package (p, "flags");

	      symbol_add_value (sym, "WHITESPACE-DELETED");
	      symbol_add_value (sym, "ACCEPT-KEYWORDS");

	      if (desc->complexp == -1)
		{
		  forms_set_tag_value_in_package (p, "type", "DEFWEAKMACRO");
		  symbol_add_value (sym, "WEAK-MACRO");
		}
	      else if (desc->complexp)
		forms_set_tag_value_in_package (p, "type", "DEFMACRO");
	      else
		forms_set_tag_value_in_package (p, "type", "DEFUN");

	      forms_set_tag_value_in_package (p, "name", desc->tag);

	      mhtml_set_numeric_variable_in_package
		(p, "debug-level", desc->debug_level);

	      forms_set_tag_value_in_package
		(p, "body", ";;; Body unavailable");

	      {
		char hexrep[20];
		sprintf (hexrep, "#prim<0x%010lX>", (unsigned long) desc->fun);
		forms_set_tag_value_in_package (p, "addr", hexrep);
	      }

	      result = package_to_alist (p, 0);
	      symbol_destroy_package (p);
	    }
	}
    }

  xfree (name);

  if (result != (char *)NULL)
    {
      int len = strlen (result);
      bprintf_insert (page, start, "%s", result);
      free (result);
      *newstart += len;
    }
}

DEFUNX (pf_%%function_def, user-function,
"Returns a string, which, if read back into Meta-HTML, would result in the\n\
re-defining of the user function <var user-function>.  This function is used\n\
internally by the <Meta-HTML> compiler -- it is unlikely in the extreme that\n\
one would use this in application programs -- it is meant for use in programs\n\
that manipulate the <Meta-HTML> language itself.")

static void
pf_function_def (PFunArgs)
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (!empty_string_p (name))
    {
      Symbol *sym = symbol_lookup_in_package (mhtml_user_keywords, name);

      if (sym == (Symbol *)NULL)
	sym = symbol_lookup (name);

      if ((sym != (Symbol *)NULL) && (sym->type == symtype_USERFUN))
	result = mhtml_recompose_function (sym);
    }

  xfree (name);

  if (result != (char *)NULL)
    {
      int len = strlen (result);
      bprintf_insert (page, start, "%s", result);
      free (result);
      *newstart += len;
    }
}

DEFUNX (pf_%%quote_for_set_var, &rest body,
"After evaluating <var body>, the results are quoted in such a way that\n\
Meta-HTML will treat it as one argument.  Used internally by the function\n\
invoker.")
static void
pf_quote_for_setvar (PFunArgs)
{
  if (body->buffer)
    {
      register int i;
      char *value;

      for (i = 0; i < body->bindex && whitespace (body->buffer[i]); i++);
      value = mhtml_evaluate_string (body->buffer + i);

      if (value != (char *)NULL)
	{
	  char *x = quote_for_setvar (value);
	  bprintf_insert (page, start, "%s", x);
	  *newstart += strlen (x);
	  free (value);
	}
    }
}

DEFMACROX (pf_%%after_page_return, ,
"Store <var body> for execution at a later time, specifically, after the top\n\
level process is completed.  For the <Meta-HTML> server or engine, this is\n\
after the requested page has been successfully delivered; for <code>mhc</code>,\n\
this is after the main document has finished processing, and the results have\n\
been returned.\n\
\n\
I would be interested if anybody actually needs this function -- if you do,\n\
please drop me a line showing how it made your life easier.")

static PAGE *after_page_return_buffer = (PAGE *)NULL;
static void
pf_after_page_return (PFunArgs)
{
  if ((body != (PAGE *)NULL) && (!empty_string_p (body->buffer)))
    {
      if (after_page_return_buffer == (PAGE *)NULL)
	after_page_return_buffer = page_create_page ();

      bprintf (after_page_return_buffer, "%s", body->buffer);
    }
}

PAGE *
get_after_page_return_buffer (void)
{
  return (after_page_return_buffer);
}
      
DEFUN (pf_apply, func &rest args,
"Apply <var func> to <var args>.\n\
\n\
This <i>weak</i>macro can either be used as a simple tag or as a\n\
complex tag -- its usage is dependent on the function being called.\n\
\n\
Using <tag apply> as a simple tag:\n\
<example>\n\
<apply add 3 4 5>         --> 12\n\
<defun foo &key bar baz>\n\
  <get-var bar>, <get-var baz>\n\
</defun>\n\
<apply foo \"bar=this baz=2\"> --> this,2\n\
</example>\n\
\n\
Using <tag apply> as a complex tag:\n\
<example>\n\
<defmacro upcase-text &key bold? &body body>\n\
   <if <get-var-once bold?> <b>>\n\
   <upcase <get-var-once body>>\n\
   <if <get-var-once bold?> </b>>\n\
</defmacro>\n\
<apply upcase-text> This is a list of things to \"change case of\" </apply>\n\
<apply upcase-text bold?=true> And this is upcased <i>and</i> bold </apply>\n\
</example>")
{
  char *func = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (!empty_string_p (func))
    {
      BPRINTF_BUFFER *funcall = bprintf_create_buffer ();
      int arg_index = 1;
      char *raw_arg;
      UserFunction *uf = mhtml_find_user_function (func);
      PFunDesc *desc = pagefunc_get_descriptor (func);
      int macro_call = 0;

      if (uf != (UserFunction *)NULL)
	{
	  if (uf->type == user_MACRO)
	    macro_call = 1;
	}
      else if (desc != (PFunDesc *)NULL)
	{
	  if (desc->complexp != 0)
	    macro_call = 1;
	}

      bprintf (funcall, "<%s", func);

      while ((raw_arg = get_positional_arg (vars, arg_index)) != (char *)NULL)
	{
	  char *arg = mhtml_evaluate_string (raw_arg);

	  if (empty_string_p (arg))
	    bprintf (funcall, " \"\"");
	  else
	    bprintf (funcall, " %s", arg);
	  xfree (arg);
	  arg_index++;
	}

      /* Now pass in keyword arguments. */
      {
	Symbol **syms = symbols_of_package (vars);

	if (syms != (Symbol **)NULL)
	  {
	    register int i;	    
	    Symbol *sym;

	    for (i = 0; (sym = syms[i]) != (Symbol *)NULL; i++)
	      {
		if ((sym->type == symtype_STRING) &&
		    (strcmp (sym->name, "*PVARS*") != 0) &&
		    (strcmp (sym->name, "*PVALS*") != 0) &&
		    (sym->values_index != 0))
		  {
		    char *val = mhtml_evaluate_string (sym->values[0]);

		    bprintf (funcall, " %s=%s", sym->name,
			     empty_string_p (val) ? "" : 
			     quote_for_setvar (val));
		    xfree (val);
		  }
	      }
	  }
      }

      bprintf (funcall, ">");

      /* If this is a macro call, we must print the body here. */
      if (macro_call)
	{
	  bprintf (funcall, "%s", body->buffer);
	  bprintf (funcall, "</%s>", func);
	}

      if (debug_level > 5)
	page_debug ("Apply: [%s]", funcall->buffer);

      result = funcall->buffer;
      free (funcall);
    }

  if (!empty_string_p (result))
    bprintf_insert (page, start, "%s", result);

  xfree (func);
  xfree (result);
}

DEFUNX (pf_%%bootstrap_metahtml, &optional call-initializer?,
"Bootstrap the user defined functions which are compiled into <Meta-HTML>.\n\
This loads the definitions into the inpterpreter, and then calls\n\
<tag bootstrapper::system-initialize>.\n\
\n\
When <var call-initializer?> is non-empty, this function also calls\n\
<tag bootstrapper::initialize>.\n\
\n\
The one obvious use for this function is in <b>mhc</b> programs which\n\
are run with the `-z' flag (which prevents bootstrapping from taking place).\n\
Such programs may read some files in order to define a set of functions,\n\
remember those functions (by calling\n\
<example code><package-vars *user-functions*></example>), and then\n\
instantiate the standard set of Meta-HTML functions for further processing.")

static void
pf_bootstrap_metahtml (PFunArgs)
{
  static int bootstrapped_already = 0;
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int call_initializer = 0;

  if (!empty_string_p (arg)) call_initializer = 1;

  if (!bootstrapped_already)
    {
      bootstrapped_already = 1;
      bootstrap_metahtml (call_initializer);
    }

  xfree (arg);
}
#if defined (__cplusplus)
}
#endif

