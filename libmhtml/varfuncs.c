/* varfuncs.c: -*- C -*-  Functions for the manipulation of variables. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Tue Jul 18 17:50:42 1995.  */

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
/*		 Variable Manipulation Functions	    */
/*							    */
/************************************************************/

static void pf_set_var (PFunArgs);
static void pf_set_var_verbatim (PFunArgs);
static void pf_set_var_readonly (PFunArgs);
static void pf_get_var (PFunArgs);
static void pf_get_var_once (PFunArgs);
static void pf_get_var_eval (PFunArgs);
static void pf_binary_concat (PFunArgs);
static void pf_unset_var (PFunArgs);
static void pf_var_exists (PFunArgs);
static void pf_increment (PFunArgs);
static void pf_decrement (PFunArgs);
static void pf_cgi_encode (PFunArgs);
static void pf_cgi_decode (PFunArgs);
static void pf_symbol_info (PFunArgs);
static void pf_copy_var (PFunArgs);
static void pf_coerce_var (PFunArgs);
static void pf_hex_to_contents (PFunArgs);
static void pf_contents_to_hex (PFunArgs);
static void pf_defvar (PFunArgs);

static PFunDesc func_table[] =
{
  { "SET-VAR-VERBATIM",	0, 0, pf_set_var_verbatim },
  { "SET-VAR",		0, 0, pf_set_var },
  { "SET-VAR-READONLY",	0, 0, pf_set_var_readonly },
  { "GET-VAR",		0, 0, pf_get_var },
  { "GET-VAR-ONCE",	0, 0, pf_get_var_once },
  { "GET-VAR-EVAL",	0, 0, pf_get_var_eval },
  { "BINARY-CONCAT",	0, 0, pf_binary_concat },
  { "UNSET-VAR",	0, 0, pf_unset_var },
  { "VAR-EXISTS",	0, 0, pf_var_exists },
  { "INCREMENT",	0, 0, pf_increment },
  { "DECREMENT",	0, 0, pf_decrement },
  { "CGI-ENCODE",	0, 0, pf_cgi_encode },
  { "CGI-DECODE",	0, 0, pf_cgi_decode },
  { "SYMBOL-INFO",	0, 0, pf_symbol_info },
  { "COPY-VAR",		0, 0, pf_copy_var },
  { "COERCE-VAR",	0, 0, pf_coerce_var },
  { "HEX-TO-CONTENTS",	0, 0, pf_hex_to_contents },
  { "CONTENTS-TO-HEX",	0, 0, pf_contents_to_hex },
  { "DEFVAR",		0, 0, pf_defvar },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_variable_functions)
DEFINE_SECTION (VARIABLES, variables; symbols; lookup,
"<Meta-HTML> provides a simple mechanism for the storage and retrieval\n\
of variables during a single (possibly recursive) processing pass.  In\n\
addition to this, functions are provided which test the value of a\n\
variable, and conditionally execute other code based upon the result.\n\
\n\
There is also a mechanism to group a set of variables using <secref\n\
packages>.",

"<h3>Variable Types and Information</h3>\n\
\n\
There are internal functions in <meta-html> which create <i>binary</i>\n\
variables.  Such variables cannot be used in the ordinary way, since\n\
the data contained within them may not retain integrity once printed\n\
out and read back in.  You can check to see what the type of a\n\
particular variable is with the <funref variables symbol-info>\n\
function.\n\
\n\
Many variables are predefined by <Meta-HTML>, and made available to\n\
the page writer.  The <secref Page-Variables> section of this manual\n\
documents those variables fully, while the <secref server-variables>\n\
section contains more information on variables which control, or were\n\
specifically created by, the web server.")

static void
generic_set_variable (Package *vars, int debug_level, int eval_p, int readonly_p)
{
  char *func = (char *)NULL;

  if (readonly_p)
    func =  "set-var-readonly";
  else if (eval_p)
    func = "set-var";
  else
    func = "set-var-verbatim";

  if (vars)
    {
      char **names = get_vars_names (vars);
      char **vals = get_vars_vals (vars);

      if (names != (char **)NULL)
	{
	  register int i;
	  char *sym_name;

	  for (i = 0; (sym_name = names[i]) != (char *)NULL; i++)
	    {
	      char *name = sym_name;
	      char *value = vals[i];
	      int free_value = 0;

	      if (eval_p)
		name = mhtml_evaluate_string (sym_name);

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
		  if (eval_p)
		    {
		      value = mhtml_evaluate_string (value);
		      if (value) free_value++;
		    }
		}

	      if (debug_level >= 6)
		page_debug ("--> <%s \"%s\"=\"%s\">",
			    func, name ? name : "", value ? value : "");

	      if (name)
		{
		  if (readonly_p)
		    pagefunc_set_variable_readonly (name, value);
		  else
		    pagefunc_set_variable (name, value);
		}

	      if (free_value) free (value);
	      if (name != sym_name) free (name);
	    }
	}
    }
}


DEFUN (pf_set_var, &optional name=value...,
"Gives the variable <var name> the value of <var value> for the\n\
current processing run.  Any number of name/value pairs may be given,\n\
and whitespace is not significant.  Where <var =value> is omitted, the\n\
value is the empty string.\n\
\n\
<example>\n\
<set-var foo=bar bar=baz>\n\
<get-var foo>              --> bar\n\
<get-var bar>              --> baz\n\
<get-var <get-var foo>>    --> baz\n\
</example>")
{
  generic_set_variable (vars, debug_level, 1, 0);
}

DEFUN (pf_set_var_verbatim, &optional name=value...,
"Gives the variable <var name> the value of <var value> for the\n\
current processing run.  The difference between\n\
<code>set-var-verbatim</code> and <funref variables set-var> is that\n\
in <code>set-var-verbatim</code> the right-hand side of assignments\n\
are not evaluated.\n\
\n\
Example:\n\
<complete-example>\n\
<set-var-verbatim foo=<get-var bar>>\n\
<get-var-once foo>\n\
</complete-example>")
{
  generic_set_variable (vars, debug_level, 0, 0);
}

DEFUN (pf_set_var_readonly, &optional name=value...,
"For each <var name> specified, if that name is not already assigned a\n\
value with <code>set-var-readonly</code>, assigns the associated <var\n\
value> to it, exactly in the way that <funref variables set-var> would.\n\
\n\
Once <var name> has had a value assigned to it with\n\
<code>set-var-readonly</code>, then that variable is <i>immutable</i>,\n\
i.e., its value cannot be changed using any Meta-HTML commands.\n\
\n\
A useful construct for setting various site specific variables in your\n\
<code>engine.conf</code> or <code>mhttpd.conf</code> file, this allows\n\
one to create globally defined variables which cannot be changed by\n\
the execution of Meta-HTML statements in a page.\n\
\n\
Variables which can usefully benefit from this type of setting\n\
include <varref mhtml::include-prefix> and <varref\n\
mhtml::relative-prefix> among others.")
{
  generic_set_variable (vars, debug_level, 1, 1);
}

static void
get_var_internal (PFunArgs, int once)
{
  register int i;
  char *name;

  for (i = 0; (name = get_positional_arg (vars, i)) != (char *)NULL; i++)
    {
      char *insertion = mhtml_evaluate_string (name);
      char *value = pagefunc_get_variable (insertion);

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

      if (once)
	*newstart = start;
    }
}

DEFUN (pf_get_var, &optional name ..., "Synonym for <tag get-var-eval>".)
{
  pf_get_var_eval (PassPFunArgs);
}

DEFUN (pf_get_var_eval, &optional name...,
"Return the value of the <var name>s given.  Each <var name> is a\n\
variable name which has had a value assigned to it with <funref\n\
variables set-var>, <funref variables set-var-readonly>, or was\n\
created implicity via <funref alists alist-to-package> or <funref\n\
variables coerce-var>.\n\
\n\
The values are returned in the order in which the <var name>s appear.\n\
\n\
Examples:\n\
<complete-example>\n\
<set-var foo=Var-1 bar=Var-2>\n\
<get-var-eval foo>, <get-var bar>\n\
</complete-example>\n\
\n\
When multiple <var name>s are given:\n\
<complete-example>\n\
<set-var foo=Var-1 bar=Var-2>\n\
<get-var foo bar foo>\n\
</complete-example>")
{
  get_var_internal (PassPFunArgs, 0);
}

DEFUN (pf_get_var_once, &optional name...,
"Returns the current value of the variables named by the <var name>s\n\
given.  The interpreter pointer is then moved to after the returned\n\
data, thus preventing further interpretation of the data.\n\
\n\
Example:\n\
<example>\n\
<set-var bar=HELLO>\n\
<set-var-verbatim foo=<get-var bar>>\n\
<get-var-once foo>   --> &lt;get-var bar&gt;\n\
</example>\n\
but...\n\
<example>\n\
<get-var foo>        --> HELLO\n\
</example>")
{
  get_var_internal (PassPFunArgs, 1);
}

DEFUN (pf_unset_var, &optional name...,
"Make <var name>s be non-existent in the page environment.\n\
\n\
This is different than <example code><set-var foo=\"\"></example>\n\
because the variable actually ceases to exist, rather than is given no\n\
value.\n\
\n\
Example:\n\
<example>\n\
<set-var foo=\"\">\n\
<var-exists foo>      --> true\n\
<get-var foo>         -->\n\
<unset-var foo>\n\
<var-exists foo>      -->\n\
</example>")
{
  register int i;
  char *name;

  for (i = 0; (name = get_positional_arg (vars, i)) != (char *)NULL; i++)
    {
      char *varname = mhtml_evaluate_string (name);
      Symbol *sym = varname ? symbol_lookup (varname) : (Symbol *)NULL;

      if (sym)
	{
	  /* Don't really remove this symbol if it has a notifier
	     attached to it, simply zap the contents. */
	  if (sym->notifier)
	    {
	      register int j;

	      *(sym->notifier) = 0;

	      for (j = 0; j < sym->values_index; j++)
		free (sym->values[j]);

	      if (sym->values_index)
		sym->values[0] = (char *)NULL;

	      sym->values_index = 0;
	    }
	  else if (!symbol_get_flag (sym, sym_READONLY))
	    {
	      sym = symbol_remove (varname);
	      if (sym) symbol_free (sym);
	    }
	}

      xfree (varname);
    }
}

DEFUN (pf_var_exists, name,
"<code>var-exists</code> checks for the <i>existence</i> of\n\
the variable named by <var varname>, and returns <code>true</code> if that\n\
variable exists.\n\
\n\
The existence of a variable has nothing to do with its value -- a variable\n\
exists if it has been created with <funref variables set-var>, and doesn't\n\
exist after it has been unset with <funref variables unset-var>.\n\
\n\
<example>\n\
  <set-var foo=1 bar>\n\
  <var-exists foo>       --> true\n\
  <var-exists bar>       --> true\n\
  <get-var bar>          -->\n\
  <unset-var foo>\n\
  <var-exists foo>       -->\n\
</example>")
{
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int set_p = 0;

  if (!empty_string_p (arg) && (symbol_lookup (arg) != (Symbol *)NULL))
    set_p++;

  xfree (arg);

  if (set_p)
    {
      bprintf_insert (page, start, "true");
      *newstart += 4;
    }
}

DEFUN (pf_binary_concat, collector &rest binary-vars,
"Concatenate the contents of <var collector> and the supplied <var binary-var>s\n\
into a single binary variable, and store the results back in <var collector>.")
{
  register int i = 0;
  char *collector_name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (collector_name))
    {
      BPRINTF_BUFFER *buf = bprintf_create_buffer ();
      char *varname;

      while ((varname = mhtml_evaluate_string (get_positional_arg (vars, i))))
	{
	  i++;

	  if (!empty_string_p (varname))
	    {
	      Symbol *sym = symbol_lookup (varname);

	      if ((sym != (Symbol *)NULL) && sym->type == symtype_BINARY)
		{
		  Datablock *b = (Datablock *)sym->values;

		  bprintf_insert_binary (buf, buf->bindex, b->data, b->length);
		}
	    }
	  free (varname);
	}

      {
	Symbol *collector = (Symbol *)NULL;
	Symbol *remsym = symbol_remove (collector_name);
	Datablock *b = datablock_create (buf->buffer, buf->bindex);

	symbol_free (remsym);
	collector = symbol_intern (collector_name);
	collector->type = symtype_BINARY;
	collector->values = (char **)b;
      }

      bprintf_free_buffer (buf);
    }

  xfree (collector_name);
}

static void
change_increment (PFunArgs, int default_amount)
{
  char *var_name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (var_name))
    {
      char *var_value = pagefunc_get_variable (var_name);
      char *incr = get_one_of (vars, "BY", "AMOUNT", (char *)NULL);
      double value = 0.0;
      double amount = (double)default_amount;
      static char number[40];
      int dot_present = 0;

      if (var_value != (char *)NULL)
	{
	  dot_present = (strchr (var_value, '.') != (char *)NULL);
	  value = strtod (var_value, (char **)NULL);
	}

      if (!empty_string_p (incr))
	{
	  incr = mhtml_evaluate_string (incr);
	  if (incr)
	    {
	      if (!dot_present)
		dot_present = (strchr (incr, '.') != (char *)NULL);
	      amount = default_amount * strtod (incr, (char **)NULL);
	      free (incr);
	    }
	}

      value += amount;
      if (dot_present)
	sprintf (number, "%.2f", value);
      else
	sprintf (number, "%ld", (long)value);

      pagefunc_set_variable (var_name, number);
    }

  xfree (var_name);
}

DEFUN (pf_increment, name &key by=amount,
"Add <var amount> (default 1) to the contents of the variable named by\n\
<var varname>.\n\
\n\
<example>\n\
<set-var foo=1>\n\
<get-var foo> --> 1\n\
<increment foo>\n\
<get-var foo> --> 2\n\
</example>\n\
\n\
Also see <funref variables decrement>.")
{
  change_increment (PassPFunArgs, 1);
}

DEFUN (pf_decrement, name &key by=amount,
"Subtract <var amount> (default 1) from the contents of the variable named\n\
by <var varname>.\n\
\n\
<example>\n\
   <set-var foo=1>\n\
   <get-var foo> --> 1\n\
   <decrement foo>\n\
   <get-var foo> --> 0\n\
</example>\n\
\n\
Also see <funref variables increment>.")
{
  change_increment (PassPFunArgs, -1);
}

DEFUN (pf_cgi_encode, &rest vars &key preserve-case=true strip=true,
"A CGI readable string is created from the names of the <var var>s given,\n\
and the associated values of those variables.  For example, if the\n\
variable <code>FOO-VAR</code> has the value <code>\"Foo &\n\
Value\"</code>, then the invocation\n\
<set-var foo-var=\"Foo & Value\">\n\
<complete-example-global>\n\
<cgi-encode FOO-VAR>\n\
</complete-example-global>\n\
\n\
Given the optional keyword argument of <var preserve-case=true>,\n\
<code>cgi-encode</code> encodes the variables preserving the case of\n\
them as they were input.\n\
\n\
<complete-example-global>\n\
<cgi-encode Foo-Var preserve-case=true>\n\
</complete-example-global>\n\
\n\
Finally, the keyword flag <var strip=true>, when present, says to\n\
strip off the package name of each variable before placing it in an\n\
assignment statement (also see <funref packages package-vars>).\n\
\n\
<complete-example>\n\
<set-var FOO::BAR=value>\n\
<cgi-encode Foo::Bar preserve-case=true strip=true>\n\
</complete-example>")
{
  char **names = get_vars_names (vars);

  if (names != (char **)NULL)
    {
      register int i;
      char *name;
      char *result = (char *)NULL;
      Package *cgivars = symbol_get_package ((char *)NULL);
      Symbol **symbols = (Symbol **)NULL;
      int save_case_p = 0;
      int strip_package = 0;

      {
	char *temp = mhtml_evaluate_string (get_value (vars, "preserve-case"));
	if (!empty_string_p (temp)) save_case_p++;
	temp = mhtml_evaluate_string (get_value (vars, "strip"));
	if (!empty_string_p (temp)) strip_package++;
      }

      for (i = 0; (name = names[i]) != (char *)NULL; i++)
	{
	  name = mhtml_evaluate_string (name);

	  if (!empty_string_p (name))
	    {
	      Symbol *sym = symbol_lookup (name);
	      if ((sym != (Symbol *)NULL) && (sym->type == symtype_STRING))
		{
		  register int j;
		  Symbol *newsym;

		  if (strip_package)
		    {
		      char *tempname = strstr (name, "::");
		      if (tempname)
			{
			  tempname = strdup (tempname + 2);
			  free (name);
			  name = tempname;
			}
		    }

		  newsym = symbol_intern_in_package (cgivars, name);

		  if (save_case_p)
		    newsym->preserved_name = strdup (name);

		  for (j = 0; j < sym->values_index; j++)
		    symbol_add_value (newsym, sym->values[j]);
		}
	    }

	  xfree (name);
	}

      symbols = symbols_of_package (cgivars);
      result = forms_unparse_items (symbols);

      if (!empty_string_p (result))
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart = start + strlen (result);
	}

      if (result) free (result);
      if (symbols) free (symbols);
      symbol_destroy_package (cgivars);
    }
}

DEFUN (pf_cgi_decode, string &optional package,
"Decode <var string> into <var package>.\n\
\n\
If <var package> is not specified the current package is used.\n\
\n\
<var string> is a string that might have appeared in\n\
<code>QUERY_STRING</code> or the contents of the data that was posted\n\
to a document, such that it consists of name value pairs:\n\
\n\
<example>\n\
FOO=bar&STRING=this+is+a+string%2C+other+chars\n\
</example>\n\
\n\
<var package> is the name of a package to bind the variables in.  So,\n\
given the above example as the text in a variable called <var string>,\n\
here is what you get:\n\
\n\
<complete-example>\n\
<set-var string=\"FOO=bar&STRING=this+is+a+string%2C+other+chars\">\n\
<cgi-decode <get-var string> mypack>\n\
<get-var mypack::string>\n\
</complete-example>\n\
\n\
Also see <funref variables cgi-encode>.")
{
  char *string, *packname = (char *)NULL;
  char *temp;
  Package *package = CurrentPackage;
  int offset = 0;

  string = read_sexp (body->buffer, &offset, 0);
  packname = read_sexp (body->buffer, &offset, 0);

  if (string != (char *)NULL)
    {
      temp = mhtml_evaluate_string (string);
      free (string);
      string = temp;
    }

  if (!empty_string_p (string))
    {
      if (packname != (char *)NULL)
	{
	  temp = mhtml_evaluate_string (packname);
	  free (packname);
	  packname = temp;

	  if (!empty_string_p (packname))
	    package = symbol_get_package (packname);

	  if (packname != (char *)NULL)
	    free (packname);
	}

      forms_parse_data_string (string, package);
    }

  if (string != (char *)NULL)
    free (string);
}

DEFUN (pf_symbol_info, symbol,
"Returns information about the symbol <var symbol>.\n\
\n\
The information is returned in an alist containing:\n\
<ul>\n\
<li> The type of the symbol (TYPE), which is one of:\n\
<ol>\n\
<li><code>STRING</code>,\n\
<li><code>BINARY</code>,\n\
<li><code>USER DEFUN</code>,\n\
<li><code>USER SUBST</code>,\n\
<li><code>USER MACRO</code>,\n\
<li><code>PRIM DEFUN</code>, or\n\
<li><code>PRIM MACRO</code>.\n\
</ol>\n\
<li> The \"size\" of the symbol.\n\
<li> The symbol's flags, including:\n\
<ol>\n\
<li><code>READONLY</code>\n\
<li><code>INVISIBLE</code>\n\
<li><code>NOEXPAND</code>\n\
<li><code>MODIFIED</code>\n\
<li><code>MACH_RES</code>\n\
<li><code>FLAGGED</code>\n\
</ol>\n\
</ul>\n\
\n\
For <code>STRING</code>. variables, the size value is the number of\n\
elements in the array.<br>\n\
For BINARY variables, the size value is the number of bytes of binary\n\
data stored within.\n\
\n\
The size value is zero for all other variable types.")
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (name)
    {
      Symbol *sym = symbol_lookup (name);

      if (sym != (Symbol *)NULL)
	{
	  Package *p = symbol_get_package ((char *)NULL);
	  char num[20];

	  switch (sym->type)
	    {
	    case symtype_STRING:
	      forms_set_tag_value_in_package (p, "TYPE", "STRING");
	      sprintf (num, "%d", sym->values_index);
	      forms_set_tag_value_in_package (p, "SIZE", num);
	      break;

	    case symtype_FUNCTION:
	      {
		PFunDesc *desc = (PFunDesc *)sym->values;
		if (desc->complexp)
		  forms_set_tag_value_in_package (p, "TYPE", "PRIM MACRO");
		else
		  forms_set_tag_value_in_package (p, "TYPE", "PRIM DEFUN");
		forms_set_tag_value_in_package (p, "SIZE", "0");
	      }
	      break;

	    case symtype_BINARY:
	      forms_set_tag_value_in_package (p, "TYPE", "BINARY");
	      sprintf (num, "%ld", (long)((Datablock *)sym->values)->length);
	      forms_set_tag_value_in_package (p, "SIZE", num);
	      break;

	    case symtype_USERFUN:
	      {
		switch (((UserFunction *)sym->values)->type)
		  {
		  case user_MACRO:
		    forms_set_tag_value_in_package (p, "TYPE", "USER MACRO");
		    break;
		  case user_SUBST:
		    forms_set_tag_value_in_package (p, "TYPE", "USER SUBST");
		    break;
		  case user_DEFUN:
		    forms_set_tag_value_in_package (p, "TYPE", "USER DEFUN");
		    break;
		  }
		forms_set_tag_value_in_package (p, "SIZE", "0");
	      }
	    break;
	    }

	  if (sym->flags & sym_READONLY)
	    forms_set_tag_value_in_package (p, "READONLY", "true");

	  if (sym->flags & sym_INVISIBLE)
	    forms_set_tag_value_in_package (p, "INVISIBLE", "true");

	  if (sym->flags & sym_NOEXPAND)
	    forms_set_tag_value_in_package (p, "NOEXPAND", "true");

	  if (sym->flags & sym_MODIFIED)
	    forms_set_tag_value_in_package (p, "MODIFIED", "true");

	  if (sym->flags & sym_MACH_RES)
	    forms_set_tag_value_in_package (p, "MACH_RES", "true");

	  if (sym->flags & sym_FLAGGED)
	    forms_set_tag_value_in_package (p, "FLAGGED", "true");

	  if (sym->machine)
	    {
	      sprintf (num, "%0xld", (unsigned int)sym->machine);
	      forms_set_tag_value_in_package (p, "MACHINE", num);
	    }

	  result = package_to_alist (p, 0);
	  symbol_destroy_package (p);
	}
      free (name);
    }

  if (!empty_string_p (result))
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }

  xfree (result);
}

DEFUN (pf_copy_var, from-var &optional to-var...,
"Copies the variable <var from-var> to each of the named <var to-var>s.\n\
\n\
Each <var to-var> becomes the repository of a copy of the information\n\
already stored under <var from-var>.  This is a <i>true</i> copy; not\n\
an alias to the original variable.\n\
\n\
<example>\n\
<set-var foo=bar>\n\
<get-var foo>      --> bar\n\
<get-var new>      --> \n\
<copy-var foo new> -->\n\
<get-var new>      --> bar\n\
\n\
<copy-var *meta-html*::get-var *meta-html*::foo>\n\
<foo new>          --> bar\n\
</example>")
{
  char *source_name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (source_name))
    {
      Symbol *source = symbol_lookup (source_name);
      register int i = 1;
      char *dest_name = (char *)NULL;
      int done = 0;

      while (!done)
	{
	  dest_name = mhtml_evaluate_string (get_positional_arg (vars, i));
	  i++;

	  if (dest_name == (char *)NULL)
	    {
	      done = 1;
	      continue;
	    }

	  if (debug_level > 5)
	    page_debug ("--><copy-var %s %s>", source_name, dest_name);

	  if (!empty_string_p (dest_name))
	    {
	      Symbol *dest = symbol_intern (dest_name);

	      if (source != (Symbol *)NULL)
		{
		  if (dest != source)
		    {
		      Package *temp = symbol_get_package ((char *)NULL);
		      Symbol *copy = symbol_copy (source, temp);

		      copy = symbol_rename (copy, dest->name);
		      symbol_move (copy, (Package *)dest->package);
		      symbol_destroy_package (temp);
		    }
		}
	      else
		{
		  /* Source has nothing in it.  Delete the destination var. */
		  Symbol *t = symbol_remove (dest_name);
		  symbol_free (t);
		}
	    }
	  free (dest_name);
	}
    }

  xfree (source_name);
}

#if !defined (symtype_ARRAY)
#define symtype_ARRAY 78
#endif
DEFUN (pf_coerce_var, varname &key type=(STRING|BINARY|ARRAY),
"Coerces <var varname>'s data to have the type specified by the\n\
argument to <var type>.  You can convert a binary object to a string\n\
object, and vice-versa.\n\
\n\
A binary variable might be created as the result of a call to <funref\n\
stream-commands stream-get-contents>, for example.  Once the data is\n\
read, you might wish to perform some substitutions on it, or otherwise\n\
get its value.  To do so, you call <code>coerce-var</code> on the\n\
value:\n\
\n\
<example>\n\
<with-open-stream s /tmp/file mode=read>\n\
  <stream-get-contents s foo>\n\
</with-open-stream>\n\
\n\
<coerce-var foo type=string>\n\
<subst-in-string <get-var-once foo> \"Hello\" \"Goodbye\">\n\
</example>")
{
  char *source_name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (source_name))
    {
      Symbol *source;

      source = symbol_lookup (source_name);

      if (source != (Symbol *)NULL)
	{
	  int dest_type = -1;

	  {
	    char *type_name = mhtml_evaluate_string (get_value (vars, "type"));

	    if (!empty_string_p (type_name))
	      {
		if (strcasecmp (type_name, "binary") == 0)
		  dest_type = symtype_BINARY;
		else if (strcasecmp (type_name, "string") == 0)
		  dest_type = symtype_STRING;
		else if (strcasecmp (type_name, "array") == 0)
		  dest_type = symtype_ARRAY;
	      }

	    if (type_name != (char *)NULL) free (type_name);
	  }

	  if ((source->type != dest_type) && (dest_type != -1))
	    {
	      /* Do the coercion. */
	      switch (dest_type)
		{
		case symtype_ARRAY:
		  {
		    if (source->type == symtype_BINARY)
		      {
			Datablock *block = (Datablock *)source->values;
			char *data = (char *)xmalloc (2 + block->length);

			memcpy (data, block->data, block->length);
			data[block->length] = '\0';
			free (block->data);
			free (block);
			source->values_index = 1;
			source->values_slots = 2;
			source->values =(char **)xmalloc (2 * sizeof (char *));
			source->values[0] = data;
			source->values[1] = (char *)NULL;
			source->type = symtype_STRING;
			symbol_set_modified (source);
		      }

		    if ((source->type == symtype_STRING) &&
			(source->values_index == 1))
		      {
			/* Make each line of the source (including blanks)
			   be a single element in the destination array. */
			register int i, beg;
			int dst_index  = 0, dst_slots = 0;
			char **array = (char **)NULL;
			char *string = source->values[0];

			beg = 0; i = 0;
			while (string[beg] != '\0')
			  {
			    /* Find end of line, or end of data. */
			    for (i = beg;
				 ((string[i] != '\0') && (string[i] != '\n'));
				 i++);

			    if ((beg == i) && (string[i] == '\0'))
			      break;
			    else
			      {
				int size = i - beg;
				char *line = (char *)xmalloc (1 + size);
				strncpy (line, string + beg, size);
				line[size] = '\0';

				if (dst_index + 2 > dst_slots)
				  array = (char **)xrealloc
				    (array, (dst_slots += 10)
				     * sizeof (char *));

				array[dst_index++] = line;
				array[dst_index] = (char *)NULL;
				if (string[i]) i++;
				beg = i;
			      }
			  }

			symbol_store_array (source_name, array);
		      }
		  }
		break;

		case symtype_STRING:
		  switch (source->type)
		    {
		    case symtype_STRING:
		    case symtype_FUNCTION:
		      break;

		    case symtype_BINARY:
		      {
			Datablock *block = (Datablock *)source->values;
			char *data = (char *)xmalloc (2 + block->length);

			memcpy (data, block->data, block->length);
			data[block->length] = '\0';
			free (block->data);
			free (block);
			source->values_index = 1;
			source->values_slots = 2;
			source->values =
			  (char **)xmalloc (2 * sizeof (char *));
			source->values[0] = data;
			source->values[1] = (char *)NULL;
			source->type = symtype_STRING;
			symbol_set_modified (source);
		      }
		      break;
		    }
		  break;

		case symtype_BINARY:
		  switch (source->type)
		    {
		    case symtype_BINARY:
		    case symtype_FUNCTION:
		      break;

		    case symtype_STRING:
		      {
			register int i;
			Datablock *block;
			BPRINTF_BUFFER *buffer;
			  
			block = (Datablock *)xmalloc (sizeof (Datablock));
			buffer = bprintf_create_buffer ();
			
			for (i = 0; i < source->values_index; i++)
			  {
			    bprintf (buffer, "%s%s",
				     i != 0 ? "\n" : "", source->values[i]);
			    free (source->values[i]);
			  }
			  
			block->data = buffer->buffer;
			block->length = 1 + buffer->bindex;
			free (buffer);
			free (source->values);
			source->values_index = 0;
			source->values = (char **)block;
			source->type = symtype_BINARY;
			symbol_set_modified (source);
		      }
		      break;
		    }
		  break;
		}
	    }
	}
    }

  if (source_name != (char *)NULL) free (source_name);
}

static char
tohex (int nibble)
{
  switch (nibble)
    {
    case 0: return ('0');
    case 1: return ('1');
    case 2: return ('2');
    case 3: return ('3');
    case 4: return ('4');
    case 5: return ('5');
    case 6: return ('6');
    case 7: return ('7');
    case 8: return ('8');
    case 9: return ('9');
    case 10: return ('A');
    case 11: return ('B');
    case 12: return ('C');
    case 13: return ('D');
    case 14: return ('E');
    case 15: return ('F');

    default: abort ();
    }

  return ('0');
}
      
static int
hexval (char c)
{
  if (c == '0') return (0);
  if (c == '1') return (1);
  if (c == '2') return (2);
  if (c == '3') return (3);
  if (c == '4') return (4);
  if (c == '5') return (5);
  if (c == '6') return (6);
  if (c == '7') return (7);
  if (c == '8') return (8);
  if (c == '9') return (9);
  if ((c == 'A') || (c == 'a')) return (10);
  if ((c == 'B') || (c == 'b')) return (11);
  if ((c == 'C') || (c == 'c')) return (12);
  if ((c == 'D') || (c == 'd')) return (13);
  if ((c == 'E') || (c == 'e')) return (14);
  if ((c == 'F') || (c == 'f')) return (15);
  return (0);
}

DEFUN (pf_hex_to_contents, string varname,
"Convert the string of hexadecimal digits passed in <var string> to their\n\
binary contents, placing the result in <var varname>.")
{
  register int i;
  char *hex_string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((!empty_string_p (hex_string)) && !(empty_string_p (varname)))
    {
      Datablock *block = (Datablock *)xmalloc (sizeof (Datablock));
      int length = strlen (hex_string);
      unsigned char *bdata;

      block->length = length / 2;
      bdata = (unsigned char *)xmalloc (1 + block->length);

      for (i = 0; i < length - 1; i += 2)
	{
	  bdata[i / 2] = (hexval (hex_string[i]) * 16) + hexval (hex_string[i + 1]);
	}

      block->data = (char *)bdata;

      {
	Symbol *sym;
	symbol_free (symbol_remove (varname));
	sym = symbol_intern (varname);
	sym->type = symtype_BINARY;
	sym->values = (char **)block;
	symbol_set_modified (sym);
      }
    }
}

DEFUN (pf_contents_to_hex, varname,
"Convert the contents of the binary variable named by <var varname> to\n\
be a sequence of hex characters.")
{
  register int i, j;
  char *result = (char *)NULL;
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (varname))
    {
      Symbol *sym = symbol_lookup (varname);

      if (sym != (Symbol *)NULL)
	{
	  if (sym->type == symtype_BINARY)
	    {
	      Datablock *block = (Datablock *)sym->values;
	      result = (char *)xmalloc ((2 * block->length) + 1);

	      for (i = 0, j = 0; i < block->length; i++)
		{
		  int hi_nibble = ((unsigned char)block->data[i]) / 16;
		  int lo_nibble =
		    ((unsigned char)block->data[i]) - (hi_nibble * 16);

		  result[j++] = tohex (hi_nibble);
		  result[j++] = tohex (lo_nibble);
		}
	      result[j] = '\0';
	    }
	}
    }

  if (result != (char *)NULL)
    {
      int len = strlen (result);
      bprintf_insert (page, start, "%s", result);
      *newstart += len;
    }
  xfree (varname);
}

DEFUN (pf_defvar, name value,
"<b>DEF</b>ault the value of the <b>VAR</b>iable named by <var name>\n\
to <var value>.\n\
\n\
<code>defvar</code> assigns <var value> to <var name> if, and only if,\n\
<var name> has a empty value.\n\
\n\
<code>defvar</code> could have been defined in <Meta-HTML> using\n\
<funref macro-commands define-tag>:\n\
\n\
<example>\n\
<define-tag defvar var &optional val>\n\
  <if <not <get-var <get-var-once var>>>\n\
      <set-var <get-var-once var>=<get-var-once val>>>\n\
</define-tag>\n\
</example>")
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (name))
    {
      char *current_value = pagefunc_get_variable (name);

      if (empty_string_p (current_value))
	{
	  char *new_value;

	  new_value = mhtml_evaluate_string (get_positional_arg (vars, 1));
	  if (!empty_string_p (new_value))
	    pagefunc_set_variable (name, new_value);

	  if (new_value) free (new_value);
	}
    }

  xfree (name);
}

#if defined (__cplusplus)
}
#endif
