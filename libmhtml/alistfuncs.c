/* alistfuncs.c: -*- C -*-  Functions for the manipulation of alists. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Tue Jul 18 17:50:42 1995.  */

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
/*		    Alist Manipulation Functions	    */
/*							    */
/************************************************************/

static void pf_alist_p (PFunArgs);
static void pf_alist_merge (PFunArgs);
static void pf_make_alist (PFunArgs);

static void pf_package_to_alist (PFunArgs);
static void pf_alist_to_package (PFunArgs);
static void pf_alist_package_names (PFunArgs);
static void pf_alist_package_vars (PFunArgs);
static void pf_alist_package_delete (PFunArgs);

/* The "convenience" functions follow. */
static void pf_alist_get_var (PFunArgs);
static void pf_alist_set_var (PFunArgs);
static void pf_alist_unset_var (PFunArgs);
static void pf_alist_var_exists (PFunArgs);
static void pf_alist_defvar (PFunArgs);

static PFunDesc func_table[] =
{
  { "ALIST?",		0, 0, pf_alist_p },
  { "ALIST-MERGE",	0, 0, pf_alist_merge },
  { "MAKE-ALIST",	0, 0, pf_make_alist },

  { "PACKAGE-TO-ALIST",		0, 0, pf_package_to_alist },
  { "ALIST-TO-PACKAGE",		0, 0, pf_alist_to_package },

  { "ALIST-PACKAGE-NAMES",	0, 0, pf_alist_package_names },
  { "ALIST-PACKAGE-VARS",	0, 0, pf_alist_package_vars },
  { "ALIST-PACKAGE-DELETE",	0, 0, pf_alist_package_delete },
  { "ALIST-GET-VAR",		0, 0, pf_alist_get_var },
  { "ALIST-SET-VAR",		0, 0, pf_alist_set_var },
  { "ALIST-DEFVAR",		0, 0, pf_alist_defvar },
  { "ALIST-UNSET-VAR",		0, 0, pf_alist_unset_var },
  { "ALIST-VAR-EXISTS",		0, 0, pf_alist_var_exists },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_alist_functions)
DEFINE_SECTION (ALISTS, alists; association lists; symbols; lisp; lookup,
"<Meta-HTML> provides a textual way to manipulate complex data structures
which would (in normal use) be stored in <i>packages</i>.

The textual representation of a package is called an <i>alist</i>, which
is short for <i>association list</i>, and which is a construct well known
to Lisp and Scheme programmers.

An <b>alist</b> consists of name and value pairs, surrounded by parentheses,
with the entire set of name and value pairs also parenthesized.

Functions exist to create alists from scratch, to modify the variable settings
within an alist, to create an alist from the contents of a package, and to
populate a package with the contents of an alist.",

"Association lists are an efficient and extensible way to hold on to complex
data structure across invocations of <Meta-HTML>, akin to printing out
and reading back in complex C structure.  Fortunately, you don't have to
do the parsing of the association list yourself, since <Meta-HTML> provides
that functionality for you.")

DEFUNX (pf_alist?, string,
"Return \"t\" if <var string> is a representation of an association list.")

static void
pf_alist_p (PFunArgs)
{
  char *string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  WispObject *list = string ? wisp_from_string (string) : NIL;
  int is_alist = 0;

  if ((CONS_P (list)) &&
      (list != NIL) &&
      (CONS_P (CAR (list))) &&
      (STRING_P (CAR (CAR (list)))))
    is_alist++;

  gc_wisp_objects ();
  xfree (string);

  if (is_alist)
    {
      bprintf_insert (page, start, "t");
      *newstart = *newstart + 1;
    }
}

DEFUN (pf_alist_merge, &rest alist-vars &key strip append,
"Merge the alists stored in the passed variable names into a single alist,
and return that new alist.

By default, each <var alist-var> encountered overrides values set in
previous alists that were encountered -- the values do not \"pile up\" --
they instead replace each other.

Passing the keyword argument <var append=true> changes this behavior; in
this case values seen in subsequent alists are appended to the values in
previous alists, creating array variables in the output.
<complete-example>
<set-var a1=<make-alist foo=foo1 bar=bar1>
         a2=<make-alist foo=foo2 newvar=newval>>
<alist-merge a1 a2>
<alist-merge a1 a2 append=true>
</complete-example>")
{
  register int i;
  Package *p = symbol_get_package ((char *)NULL);
  char *varname = (char *)NULL;
  char *result = (char *)NULL;
  char *strip_arg = mhtml_evaluate_string (get_value (vars, "strip"));
  char *append_arg = mhtml_evaluate_string (get_value (vars, "append"));
  int stripping = (!empty_string_p (strip_arg));
  int appending = (!empty_string_p (append_arg));

  xfree (strip_arg);
  xfree (append_arg);

  for (i = 0;
       (varname = mhtml_evaluate_string (get_positional_arg (vars, i))) != 
	 (char *)NULL;
       i++)
    {
      if (!empty_string_p (varname))
	{
	  char *val = pagefunc_get_variable (varname);

	  if (!empty_string_p (val))
	    {
	      Package *this_alist = alist_to_package (val);

	      if (appending)
		{
		  Symbol **newsyms = symbols_of_package (this_alist);

		  if (newsyms != (Symbol **)NULL)
		    {
		      register int j;
		      Symbol *sym;

		      for (j = 0; (sym = newsyms[j]) != (Symbol *)NULL; j++)
			{
			  register int symi;
			  Symbol *appendsym;

			  appendsym = symbol_intern_in_package (p, sym->name);

			  for (symi = 0; symi < sym->values_index; symi++)
			    symbol_add_value (appendsym, sym->values[symi]);
			}
		    }
		}
	      else
		{
		  symbol_copy_package (this_alist, p);
		  symbol_destroy_package (this_alist);
		}
	    }
	}

      xfree (varname);
      varname = (char *)NULL;
    }

  result = package_to_alist (p, stripping);
  symbol_destroy_package (p);

  if (!empty_string_p (result))
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }

  xfree (result);
}

DEFUN (pf_make_alist, &rest name-value-pairs,
"Return an alist from the <var name-value> pairs passed.
<complete-example>
 <make-alist foo=bar baz=this>
</complete-example>")
{
  BPRINTF_BUFFER *expr = bprintf_create_buffer ();
  char *result = (char *)NULL;

  bprintf (expr, "<with __alist__=\"\">");
  bprintf (expr, "<alist-set-var __alist__ %s>", body->buffer);
  bprintf (expr, "<get-var-once __alist__></with>");
  result = mhtml_evaluate_string (expr->buffer);
  bprintf_free_buffer (expr);

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }
}

DEFUN (pf_package_to_alist, &optional package &key strip,
"Returns a Lisp readable string containing the names and values of the
 variables in <var package>.  If <var strip=true> is supplied, the
 package name is removed from the variables before placing them in the
 list.  See the following code sequence:

<complete-example>
<set-var
  foo::bar=baz
  foo::array[0]=Elt-0
  foo::array[1]=Elt-1>

  The contents of Foo: <package-to-alist foo>
The stripped contents: <package-to-alist foo strip=true>
</complete-example>")
{
  char *packname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *strip = mhtml_evaluate_string (get_value (vars, "STRIP"));
  char *result = (char *)NULL;
  Package *package = (Package *)NULL;

  if (!empty_string_p (packname))
    package = symbol_lookup_package (packname);
  else
    package = CurrentPackage;

  if (package != (Package *)NULL)
    result = package_to_alist (package, !empty_string_p (strip));

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }

  xfree (packname);
  xfree (strip);
}

DEFUN (pf_alist_to_package, alist &optional package,
"Takes the textual list representation of a package, and creates (or
modifies) the package named by <var package-name>.

<code>alist-to-package</code> is the inverse of the <funref packages
package-to-alist> function -- given an \"alist\" (short for
`association list') you can create a package, and vice-versa.  The
following expression is one way to copy all of the variables from the
package <code>FOO</code> into the package <code>BAR</code>:

<example>
<alist-to-package <package-to-alist foo> bar>
</example>

But don't use that, of course.  Use <funref packages copy-package> instead.")
{
  char *alist = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *packname = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if (!empty_string_p (alist))
    {
      Package *from = (Package *)NULL;
      Package *to = (Package *)NULL;

      from = alist_to_package (alist);

      if (!empty_string_p (packname))
	to = symbol_get_package (packname);
      else
	to = CurrentPackage;

      if (from && to)
	{
	  Symbol **symbols = symbols_of_package (from);

	  if (symbols != (Symbol **)NULL)
	    {
	      register int i;
	      Symbol *sym, *copy;

	      for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
		{
		  char *sym_name = sym->name;
		  char *temp = strstr (sym_name, "::");

		  if (temp)
		    sym_name = temp + 2;

		  copy = symbol_copy (sym, to);
		  if (temp)
		    symbol_rename (copy, sym_name);
		}
	      free (symbols);
	    }
	}

      symbol_destroy_package (from);
    }

  xfree (alist);
  xfree (packname);
}

static Package *
get_alist_var (Package *vars)
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  Package *resultpack = (Package *)NULL;

  if (!empty_string_p (varname))
    {
      register int i;
      char *alist_text;

      for (i = 0; whitespace (varname[i]); i++);

      if (varname[i] == '(')
	alist_text = varname + i;
      else
	alist_text = pagefunc_get_variable (varname);

      if (empty_string_p (alist_text))
	resultpack = symbol_get_package ((char *)NULL);
      else
	resultpack = alist_to_package (alist_text);
    }

  xfree (varname);
  return (resultpack);
}
    
static void
set_alist_var (Package *vars, Package *value)
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (varname))
    {
      char *alist_text = package_to_alist (value, 0);
      pagefunc_set_variable (varname, alist_text);
      xfree (alist_text);
    }

  xfree (varname);

  if (value != (Package *)NULL)
    symbol_destroy_package (value);
}

DEFUN (pf_alist_defvar, alistvar name value,
"<b>DEF</b>ault the value of the <b>VAR</b>iable named by <var name>
to <var value>, in the association list referenced by <var alistvar>.

<code>alist-defvar</code> assigns <var value> to <var name> if, and only if,
<var name> has a non-empty value.")
{
  Package *alist = get_alist_var (vars);
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((alist != (Package *)NULL) && (!empty_string_p (name)))
    {
      char *current_value = forms_get_tag_value_in_package (alist, name);

      if (empty_string_p (current_value))
	{
	  char *new_value = mhtml_evaluate_string (get_positional_arg
						   (vars, 2));
	  if (!empty_string_p (new_value))
	    forms_set_tag_value_in_package (alist, name, new_value);
	  xfree (new_value);
	}

      set_alist_var (vars, alist);
    }
  else if (alist != (Package *)NULL)
    symbol_destroy_package (alist);

  xfree (name);
}

DEFUN (pf_alist_unset_var, alistvar &rest names...,
"Make <var name>s be non-existent in the association list specified
by <var alistvar>.")
{
  Package *alist = get_alist_var (vars);
  register int i;
  char *name;

  if (alist != (Package *)NULL)
    {
      for (i = 1; (name = get_positional_arg (vars, i)) != (char *)NULL; i++)
	{
	  char *varname = mhtml_evaluate_string (name);

	  if (!empty_string_p (varname))
	    {
	      Symbol *sym = symbol_remove_in_package (alist, varname);
	      if (sym) symbol_free (sym);
	    }

	  xfree (varname);
	}

      set_alist_var (vars, alist);
    }
}

DEFUN (pf_alist_var_exists, alistvar name,
"<code>var-exists</code> checks for the <i>existence</i> of
the variable named by <var varname>, in the association list specified by
<var alistvar>, and returns <code>true</code> if that variable does in
fact exist.

The existence of a variable has nothing to do with its value -- a
variable exists if it is present within the list, whether or not it
has a value.")
{
  Package *alist = get_alist_var (vars);
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 1));
  int set_p = 0;

  if ((!empty_string_p (arg)) &&
      (alist != ((Package *)NULL)) &&
      (symbol_lookup_in_package (alist, arg) != (Symbol *)NULL))
    set_p++;

  xfree (arg);

  if (set_p)
    {
      bprintf_insert (page, start, "true");
      *newstart += 4;
    }

  symbol_destroy_package (alist);
}

DEFUN (pf_alist_set_var, alistvar &optional name=value...,
"Gives the variable <var name> the value of <var value> in the association
list specified by <var alistvar>. Any number of name/value pairs may be given,
and whitespace is not significant.  Where <var =value> is omitted, the
value is the empty string.

<example>
<alist-set-var myalist foo=bar bar=baz>
<alist-get-var myalist foo>    --> bar
<alist-get-var myalist bar>    --> baz
<alist-get-var myalist <alist-get-var myalist foo>>    --> baz
</example>")
{
  Package *alist = get_alist_var (vars);
  char *func = "alist-set-var";

  if ((alist != (Package *)NULL))
    {
      char **names = get_vars_names (vars);
      char **vals = get_vars_vals (vars);

      if (names != (char **)NULL)
	{
	  register int i;
	  char *sym_name;

	  for (i = 1; (sym_name = names[i]) != (char *)NULL; i++)
	    {
	      char *name = mhtml_evaluate_string (sym_name);
	      char *value = vals[i];
	      int free_value = 0;

	      if (debug_level >= 5)
		{
		  if (value)
		    page_debug ("<%s \"%s\"=\"%s\">", func, sym_name, value);
		  else
		    page_debug ("<%s \"%s\">", func, sym_name);
		}

	      if (value == (char *)NULL)
		{
		  if (debug_level)
		    page_debug ("<%s %s ...> missing `='", func, sym_name);
		}
	      else
		{
		  value = mhtml_evaluate_string (value);
		  if (value) free_value++;
		}

	      if (debug_level >= 6)
		page_debug ("--> <%s \"%s\"=\"%s\">",
			    func, name ? name : "", value ? value : "");

	      if (name)
		forms_set_tag_value_in_package (alist, name, value);

	      if (free_value) free (value);
	      if (name != sym_name) free (name);
	    }
	}

      set_alist_var (vars, alist);
    }
}

DEFUN (pf_alist_get_var, alistvar &optional name...,
"Return the value of the <var name>s given from the association list
specified by <var alistvar>.  Each <var name> is a
variable name which has had a value assigned to it with <funref
variables alist-set-var>, or was created implicity via
<funref packages alist-to-package>.

The values are returned in the order in which the <var name>s appear.")
{
  Package *alist = get_alist_var (vars);
  register int i;
  char *name;

  for (i = 1; (name = get_positional_arg (vars, i)) != (char *)NULL; i++)
    {
      char *insertion;
      char *value;

      insertion = mhtml_evaluate_string (name);
      value = forms_get_tag_value_in_package (alist, insertion);

      if (debug_level > 5)
	page_debug ("<get-var \"%s\">", insertion ? insertion : "");

      if (value)
	{
	  int len = strlen (value);
	  bprintf_insert_binary (page, start, value, len);
	  start += len;
	}
      else
	{
	  if (debug_level > 9)
	    page_debug ("<get-var \"%s\">: Unbound Variable \"%s\"!",
			insertion, insertion);
	}

      if (debug_level > 5)
	page_debug ("--> `%s'", value ? value : "");

      xfree (insertion);

      *newstart = start;
    }

  symbol_destroy_package (alist);
}

DEFUN (pf_alist_package_names, alistvar,
"Return a newline separated list of all of the packages which are
defined within the association list stored within <var alistvar>.
Because the list is newline separated, the result can easily be
assigned to an array variable:")
{
  Package *alist = get_alist_var (vars);

  if (alist != (Package *)NULL)
    {
      register int i;
      Symbol **syms = symbols_of_package (alist);
      char **values = (char **)NULL;
      int values_slots = 0;
      int values_index = 0;

      for (i = 0; syms[i] != (Symbol *)NULL; i++)
	{
	  char *name = syms[i]->name;
	  char *dots = name ? strstr (name, "::") : (char *)NULL;

	  if (dots != (char *)NULL)
	    {
	      register int j, found = 0;
	      int len = (int) (dots - name);

	      for (j = 0; j < values_index; j++)
		if (strncmp (values[j], name, len) == 0)
		  {
		    found = 1;
		    break;
		  }

	      if (!found)
		{
		  char *pack = (char *)xmalloc (1 + len);

		  strncpy (pack, name, len);
		  pack[len] = '\0';

		  if (values_index + 2 > values_slots)
		    values = (char **)xrealloc
		      (values, (values_slots += 10) * sizeof (char *));

		  values[values_index++] = pack;
		  values[values_index] = (char *)NULL;
		}
	    }
	}

      if (values != (char **)NULL)
	{
	  for (i = 0; i < values_index; i++)
	    {
	      bprintf_insert (page, start, "%s\n", values[i]);
	      start += 1 + strlen (values[i]);
	      free (values[i]);
	    }

	  free (values);
	  *newstart = start;
	}

      symbol_destroy_package (alist);
    }
}

DEFUN (pf_alist_package_vars, alistvar &key strip=true,
"Returns a newline separated list of the fully qualified variable
names found in the alist named by <var alistvar>

When <var strip=true> is supplied, the returned variable names have
the package prefix stripped off, making them <i>not</i> fully qualified.
The names are not returned in any significant order.  Because the list is
newline separated, the results can easily be assigned to an array
variable:

<complete-example>
<set-var alist=<make-alist this::foo=bar this::bar=baz>>
<set-var names[]=<alist-package-vars alist>>
<get-var-once names[1]> is <alist-get-var alist <get-var-once names[1]>>
</complete-example>")
{
  Package *alist = get_alist_var (vars);

  if (alist != (Package *)NULL)
    {
      register int i;
      Symbol **syms = symbols_of_package (alist);
      int strip = !(empty_string_p (get_value (vars, "strip")));

      if (syms != (Symbol **)NULL)
	{
	  for (i = 0; syms[i] != (Symbol *)NULL; i++)
	    {
	      char *name = syms[i]->name;
	      char *dots = name ? strstr (name, "::") : (char *)NULL;

	      if ((strip != 0) && (dots != (char *)NULL))
		name = dots + 2;

	      bprintf_insert (page, start, "%s\n", name);
	      start += 1 + strlen (name);
	    }
	  *newstart = start;
	}
    }
}

DEFUN (pf_alist_package_delete, alistvar &rest packages[],
"For each variable in the association list within <var alistvar>, remove
the variable from the association list if it is prefixed with one of the
specifed package names.  For example, given that the variable <var alist>
contained the alist:

<example>
((\"FOO::BAR\" . \"bar\") (\"FOO::BAZ\" . \"baz\") (\"BAR::X\" . \"val\"))
</example>

then calling:

<example>
<alist-package-delete alist foo>
<get-var-once alist>
</example>

produces:

<example>
((\"BAR::X\" . \"val\"))
</example>")
{
  Package *alist = get_alist_var (vars);

  if (alist != (Package *)NULL)
    {
      Symbol **syms = symbols_of_package (alist);

      if (syms != (Symbol **)NULL)
	{
	  register int i, j;
	  char **packnames = (char **)NULL;
	  int packnames_index = 0;
	  int packnames_slots = 0;
	  char *arg;

	  /* Collect the names of the packages passed as parameters. */
	  for (i = 1;
	       (arg = (mhtml_evaluate_string (get_positional_arg (vars, i))))
		 != (char *)NULL;
	       i++)
	    {
	      /* Force to all upper case. */
	      for (j = 0; arg[j] != '\0'; j++)
		if (islower (arg[j]))
		  arg[j] = toupper (arg[j]);

	      /* Append it to our list of package names to check. */
	      if (packnames_index + 2 > packnames_slots)
		packnames = (char **)xrealloc
		  (packnames, (packnames_slots += 10) * sizeof (char *));

	      packnames[packnames_index++] = arg;
	      packnames[packnames_index] = (char *)NULL;
	    }

	  /* Okay, have all of the arguments.  Now figure out which variables
	     to remove. */
	  for (i = 0; syms[i] != (Symbol *)NULL; i++)
	    {
	      char *name = syms[i]->name;
	      char *dots = name ? strstr (name, "::") : (char *)NULL;

	      if (dots != (char *)NULL)
		{
		  int len = (int) (dots - name);

		  for (j = 0; j < packnames_index; j++)
		    if (strncmp (packnames[j], name, len) == 0)
		      {
			Symbol *sym = symbol_remove_in_package (alist, name);
			symbol_free (sym);
		      }
		}
	    }

	  /* All variables removed.  Replace the package. */
	  set_alist_var (vars, alist);

	  /* Free the memory used to store the package names. */
	  for (i = 0; i < packnames_index; i++)
	    free (packnames[i]);
	  xfree (packnames);
	}
      else
	symbol_destroy_package (alist);
    }
}

#if defined (__cplusplus)
}
#endif
