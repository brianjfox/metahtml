/* packfuncs.c: -*- C -*-  Functions which manipulate packages. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Tue Jul 22 22:01:08 1997.  */

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

static void pf_package_names (PFunArgs);
static void pf_package_vars (PFunArgs);
static void pf_package_delete (PFunArgs);
static void pf_in_package (PFunArgs);
static void pf_with_local_package (PFunArgs);

/************************************************************/
/*							    */
/*		  Package Manipulation Functions	    */
/*							    */
/************************************************************/

static PFunDesc func_table[] =
{
  { "PACKAGE-NAMES",		0, 0, pf_package_names },
  { "PACKAGE-VARS",		0, 0, pf_package_vars },
  { "PACKAGE-DELETE",		0, 0, pf_package_delete },
  { "IN-PACKAGE",		1, 0, pf_in_package },
  { "WITH-LOCAL-PACKAGE",	1, 0, pf_with_local_package },

  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_package_functions)
DEFINE_SECTION (PACKAGES, variables; module; package; group, 
"<i>Packages</i> are repositories which contain symbols and their values.

Each time a symbol is referenced, a package must first be found, and
then the symbol may be found within that package. You indicate which
package to look the symbol up in by supplying a package <i>prefix</i>.
When the prefix is not supplied, the symbol is looked up in the
current package.",
"For example, a full reference to the symbol <var bar> stored in the
<var foo> package looks like: 

<example>
   FOO::BAR
</example>

There are very few commands specifically for dealing with packages,
because most of the operations are performed implicitly, rather than
explicitly.  To create a package, simply give the package name as part
of the symbol, in the place where the symbol is normally used.

<example>
   <set-var foo::bar = \"Hello\">
</example>

This has the effect of creating the package <var foo> if it didn't
already exist.

The majority of the functions documented here perform package maintenance, as
opposed to variable manipulation.  There are functions for querying a package
about its contens, for deleting an entire package, for exporting and importing
packages to and from sessions, for copying the contents of packages, and for
converting packages from an internal representation to a printable
representation, called an <i>alist</i>.")

DEFUN (pf_package_names, ,
"Return a newline separated list of all of the named packages
which are currently defined.  Because the list is newline separated,
the result can easily be assigned to an array variable:

<example>
<set-var all-packages[]=<package-names>>
</example>")
{
  if (AllPackages)
    {
      register int i;
      Package *pack;

      for (i = 0; (pack = AllPackages[i]) != (Package *)NULL; i++)
	if (pack->name != (char *)NULL)
	  {
	    bprintf_insert (page, start, "%s\n", pack->name);
	    start += strlen (pack->name) + 1;
	  }

      *newstart = start;
    }
}

DEFUN (pf_package_vars, &optional package-name &key strip=true,
"Returns a newline separated list of the fully qualified variable
names found in the package named by <var package-name>, or in the
current package if <var package-name> is not given.  When <var
strip=true> is supplied, the returned variable names have the package
prefix stripped off, making them <i>not</i> fully qualified.  The
names are not returned in any significant order.  Because the list is
newline separated, the results can easily be assigned to an array
variable:

<complete-example>
<set-var foo::bar=baz>
<set-var foo::baz=bar>
<set-var names[]=<package-vars foo>>
<get-var names[1]>
</complete-example>")
{
  register int pos = 0;
  char *strip = get_value (vars, "STRIP");
  char *name;

  if ((CurrentPackage != (Package *)NULL) &&
      (get_positional_arg (vars, 0) == (char *)NULL))
    {
      Symbol **symbols = symbols_of_package (CurrentPackage);

      if (symbols != (Symbol **)NULL)
	{
	  register int i;

	  for (i = 0; symbols[i] != (Symbol *)NULL; i++)
	    {
	      bprintf_insert (page, start, "%s\n", symbols[i]->name);
	      start += symbols[i]->name_len + 1;
	    }

	  free (symbols);
	}
    }

  while ((name = get_positional_arg (vars, pos)) != (char *)NULL)
    {
      Package *pack = (Package *)NULL;

      pos++;

      name = mhtml_evaluate_string (name);

      if (!empty_string_p (name))
	pack = symbol_lookup_package (name);

      if (pack)
	{
	  Symbol **symbols = symbols_of_package (pack);

	  if (symbols != (Symbol **)NULL)
	    {
	      register int i;

	      for (i = 0; symbols[i] != (Symbol *)NULL; i++)
		{
		  if (((pack->name != (char *)NULL) && (pack->name[0] != '\0'))
		      && (strip == (char *)NULL))
		    {
		      bprintf_insert (page, start, "%s::%s\n",
				      pack->name,
				      symbols[i]->preserved_name ?
				      symbols[i]->preserved_name :
				      symbols[i]->name);
		      start += pack->name_len + 3 + symbols[i]->name_len;
		    }
		  else
		    {
		      bprintf_insert (page, start, "%s\n",
				      symbols[i]->preserved_name ?
				      symbols[i]->preserved_name :
				      symbols[i]->name);
		      start += symbols[i]->name_len + 1;
		    }
		}

	      free (symbols);
	    }
	}

      if (name) free (name);
    }

  *newstart = start;
}

DEFUN (pf_package_delete, &rest package-names...,
"Remove the definition of the packages named by <var package-names>,
and all of the variables defined within them.")
{
  char **names = get_vars_names (vars);

  if (names != (char **)NULL)
    {
      register int i;

      for (i = 0; names[i] != (char *)NULL; i++)
	{
	  char *name = names[i];

	  name = mhtml_evaluate_string (name);

	  if (name)
	    {
	      pagefunc_destroy_package (name);
	      free (name);
	    }
	}
    }
}

DEFMACRO (pf_in_package, package-name,
"Evaluate <var body> in an environment where variables which are not
specifically prefixed with a package name are looked up and stored
within <var package-name>.

The special package name <code>\"local\"</code> creates an anonymous
package within which to work.  The contents of local packages are only
accessible within the expressions surrounded by the
<code>in-package</code> operator.")
{
  char *packname;
  char *result = (char *)NULL;
  int jump_again = 0;

  packname = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (empty_string_p (packname))
    {
      if (packname) free (packname);
      packname = strdup (DEFAULT_PACKAGE_NAME);
    }

  if (strcasecmp (packname, "local") == 0)
    {
      free ((char *)packname);
      packname = (char *)NULL;
    }

  {
    PageEnv *page_environ = pagefunc_save_environment ();

    symbol_push_package (symbol_get_package (packname));

    if ((jump_again = setjmp (page_jmp_buffer)) == 0)
      result = mhtml_evaluate_string (body->buffer);

    symbol_pop_package ();

    pagefunc_restore_environment (page_environ);
  }

  if (result != (char *)NULL)
    {
      if (jump_again == 0)
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart += strlen (result);
	}
      free (result);
    }

  if (packname) free (packname);
  if (jump_again) longjmp (page_jmp_buffer, 1);
}

DEFMACRO (pf_with_local_package, ,
"Shorthand for <example code><in-package local> <i>body</i>
</in-package></example>")
{
  int jump_again = 0;
  char *result = (char *)NULL;

  {
    PageEnv *page_environ = pagefunc_save_environment ();

    symbol_push_package (symbol_get_package ((char *)NULL));

    if ((jump_again = setjmp (page_jmp_buffer)) == 0)
      result = mhtml_evaluate_string (body->buffer);

    symbol_pop_package ();
    pagefunc_restore_environment (page_environ);
  }

  if (result != (char *)NULL)
    {
      if (jump_again == 0)
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart += strlen (result);
	}
      free (result);
    }
  if (jump_again) longjmp (page_jmp_buffer, 1);
}

#if defined (__cplusplus)
}
#endif





