/* modfuncs.c: -*- C -*-  Dynamically loaded Meta-HTML modules. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Tue Dec 24 10:06:54 1996.  */

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

/* This version of Meta-HTML can handle dynamically loaded modules. */
#define PACKAGE_INITIALIZER_EXTRA_CODE \
  pagefunc_set_variable ("mhtml::module-capable", "true");

#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

#if defined (USE_SHL_LOAD)
#  include <dl.h>
#else
#  include <dlfcn.h>
#endif

#if !defined (RTLD_LAZY)
#  define RTLD_LAZY 1
#endif

#if !defined (RTLD_NOW)
#  define RTLD_NOW 2
#endif

static void pf_load_module (PFunArgs);
static void pf_unload_module (PFunArgs);
static void pf_module_function (PFunArgs);
static void pf_module_call_function (PFunArgs);

static PFunDesc func_table[] =
{
  { "LOAD-MODULE",		0, 0, pf_load_module },
  { "UNLOAD-MODULE",		0, 0, pf_unload_module },
  { "MODULE-FUNCTION",		0, 0, pf_module_function },
  { "MODULE-CALL-FUNCTION",	0, 0, pf_module_call_function },
  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_module_functions)
DEFINE_SECTION (DYNAMIC-MODULES, dynamic; load-module, 
"You can write C, C++, or other language code to implement your own built-in
<Meta-HTML> functions.  Such functions are first-class objects, and
can manipulate the <Meta-HTML> environment directly, reset the parser pointer,
manage hardware devices or talk on proprietery networks, or anything else that
you would like to do. 

It works by supplying you with the necessary <Meta-HTML> header files
and a Makefile for building modules on your system.  Using the supplied
Makefile provides you with a resultant <i>dynamically loadable</i> module,
which can be loaded into an invocation of the <Meta-HTML> server, engine,
debugger, or standalone processor with the <funref dynamic-modules
load-module> function.

Combined with the <funref language-operators autoload> function, one
can extend the <Meta-HTML> language without limits, and without
forcing all of the functionality to be present all the time.

See the <code>modules</code> directory of the distribution for more
details.", "")

DEFVAR (mhtml::module-directories,
"An array of pathnames used to locate dynamic modules.

When <funref dynamic-modules load-module> is called with a non-fully qualified
pathname as an argument, the location of the file containing the module code
is searched for throughout the list of directories in
<var mhtml::module-directories>.  The default list of directories includes at
least the following, and may have more dependent on your system type:
<example>
/www/lib, /www/bin, /www/metahtml/lib, /www/metahtml/bin,
/opt/lib, /opt/bin, /opt/metahtml/lib, /opt/metahtml/bin,
/usr/lib/metahtml, /usr/lib/mhtml, /home/mhttpd/lib, /usr/local/mhtml/lib,
and /www/src/metahtml/modules
</example>")

#define MODULE_SYM_NAME "mhtml::loaded-modules"

DEFVAR (mhtml::loaded-modules,
"An array of fully qualified pathnames to the modules which have been loaded
via the function <funref dynamic-modules load-module>.  Used internally
during the module search process.")

DEFVAR (mhtml::module-capable,
"This value of this symbol is \"true\" when the system on which <Meta-HTML>
is running is capable of loading dynamic modules.  The value is determined
at the time <Meta-HTML> is compiled.")

static char *static_dirs[] =
 {
   ".", "./modules", 
#if defined (__cplusplus)
   "/www/lib++", "/www/bin++", "/www/metahtml++/lib", "/www/metahtml++/bin",
#endif
#if defined (COMPILE_TIME_MODULE_DIRECTORY)
   COMPILE_TIME_MODULE_DIRECTORY,
#endif

   "/usr/lib/metahtml", "/usr/lib/mhtml", "/home/mhttpd/lib",
   "/usr/local/mhtml/lib",

   "/www/lib", "/www/bin", "/www/metahtml/lib", "/www/metahtml/bin",
   "/opt/lib", "/opt/bin", "/opt/metahtml/lib", "/opt/metahtml/bin",
   "/www/src/metahtml/modules",
   (char *)NULL
 };

static char *
fully_qualified_module_name (char *name)
{
  int namelen = strlen (name);

  if ((*name != '/') && (namelen < 1024))
    {
      register int i;
      static char buffer[2048];
      char **dirs = symbol_get_values ("mhtml::module-directories");
      struct stat finfo;
      int add_extension = 0;

      {
	char *temp = strrchr (name, '.');

	if (!temp)
	  add_extension++;
      }

      if (dirs == (char **)NULL)
	{
	  Symbol *dirsym = symbol_intern ("mhtml::module-directories");

	  for (i = 0; static_dirs[i] != (char *)NULL; i++)
	    symbol_add_value (dirsym, static_dirs[i]);

	  dirs = &static_dirs[0];
	}

      for (i = 0; dirs[i] != (char *)NULL; i++)
	{
	  sprintf (buffer, "%s/%s", dirs[i], name);
	  if (add_extension)
	    {
#if defined (__CYGWIN32__)
	      strcat (buffer, ".dll");
#else /* !__CYGWIN32__ */
#  if defined (USE_SHL_LOAD)
	      strcat (buffer, ".O");
#  else
	      strcat (buffer, ".so");
#  endif
#endif /* ! __CYGWIN32__ */
	    }

	  if (stat (buffer, &finfo) == 0)
	    return (strdup (buffer));
	}
    }

  return (strdup (name));
}

static char *
find_loaded_module (char *name)
{
  register int i;
  char **modules = symbol_get_values (MODULE_SYM_NAME);
  char *fqn = fully_qualified_module_name (name);
  char *result = (char *)NULL;

  if (modules != (char **)NULL)
    {
      for (i = 0; modules[i] != (char *)NULL; i++)
	if (strcmp (fqn, modules[i]) == 0)
	  {
	    result = strdup (fqn);
	    break;
	  }
    }
  free (fqn);
  return (result);
}

static void
remove_module_references (char *name)
{
  register int i;
  Symbol *sym = symbol_lookup (MODULE_SYM_NAME);
  char **modules = symbol_get_values (MODULE_SYM_NAME);
  char *fqn = fully_qualified_module_name (name);

  if (modules != (char **)NULL)
    {
      for (i = 0; modules[i] != (char *)NULL; i++)
	if (strcmp (fqn, modules[i]) == 0)
	  {
	    register int j;

	    for (j = i + 1; modules[j] != (char *)NULL; j++)
	      modules[i++] = modules[j];

	    modules[i] = (char *)NULL;
	    sym->values_index--;
	    sym = symbol_remove_in_package ((Package *)sym->package, fqn);
	    symbol_free (sym);
	    break;
	  }

      /* Now remove functions which reference this module.
	 The function names are stored in mhtml::syms-of-<module-name>. */
      {
	char *short_name = strrchr (fqn, '/');

	if (short_name != (char *)NULL)
	  {
	    char *modsym = (char *)xmalloc (40 + strlen (short_name));
	    char *end = strrchr (fqn, '.');
	    char *symname;

	    short_name++;
	    if (end != (char *)NULL) *end = '\0';

	    sprintf (modsym, "modules::syms-of-%s", short_name);
	    xfree (modules);
	    modules = symbol_get_values (modsym);

	    if (modules != (char **)NULL)
	      for (i = 0; (symname = modules[i]) != (char *)NULL; i++)
		{
		  char *fun = (char *)xmalloc (16 + strlen (symname));
		  sprintf (fun, "*meta-html*::%s", symname);
		  symbol_free (symbol_remove (fun));
		  free (fun);
		}

	    symbol_free (symbol_remove (modsym));
	    free (modsym);
	  }
      }
    }

  xfree (modules);
  xfree (fqn);
}

static void *
find_module_handle (char *name)
{
  register int i;
  char **modules = symbol_get_values (MODULE_SYM_NAME);
  char *fqn = fully_qualified_module_name (name);
  void *result = (void *)NULL;
  int found = 0;

  if (modules != (char **)NULL)
    {
      for (i = 0; modules[i] != (char *)NULL; i++)
	if (strcmp (fqn, modules[i]) == 0)
	  {
	    found = 1;
	    break;
	  }
    }

  if (found != 0)
    {
      Symbol *sym = symbol_lookup (MODULE_SYM_NAME);
      char *parsed = (char *)NULL;
      unsigned long addr = 0;

      sym = symbol_lookup_in_package ((Package *)sym->package, fqn);
      if ((sym != (Symbol *)NULL) && (sym->values_index != 0))
	addr = strtoul (sym->values[0], &parsed, 16);

      if ((parsed != (char *)NULL) &&
	  (*parsed == '\0') &&
	  (sym->values[0] != '\0'))
	result = (void *)addr;
    }

  xfree (fqn);

  return (result);
}

#if defined (USE_SHL_LOAD)
extern int errno;
#  define module_recent_error(file) \
	page_syserr ("LOAD-MODULE: (%s) %s", file, strerror (errno))
#else
#  define module_recent_error(file) \
	page_syserr ("LOAD-MODULE: (%s) %s", file, dlerror ())
#endif

typedef void VFUN (void);
typedef void HVFUN (char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);

char *
mhtml_load_module (char *module_name, char *mode_arg,
		   int no_init_p, char *initfunc_name,
		   int allow_output_p)
{
  char *result = (char *)NULL;
#if defined (USE_SHL_LOAD)
  int mode_flag = BIND_IMMEDIATE;
#else
  int mode_flag = RTLD_NOW;
#endif

  if (!empty_string_p (mode_arg))
    {
      if (strcasecmp (mode_arg, "lazy") == 0)
	{
#if defined (USE_SHL_LOAD)
	  mode_flag = BIND_DEFERRED;
#else
	  mode_flag = RTLD_LAZY;
#endif
	}
    }

  if (module_name != (char *)NULL)
    {
      char *fqn = fully_qualified_module_name (module_name);

      if (fqn)
	result = find_loaded_module (module_name);

      if (result == (char *)NULL)
	{
#if defined (USE_SHL_LOAD)
	  void *handle = (void *)shl_load (fqn, mode_flag, 0L);
#else
	  void *handle = (void *)dlopen (fqn, mode_flag);
#endif

	  if (handle != (void *)NULL)
	    {

	      /* Some operating systems lie.  For example, FreeBSD
		 says that dlsym works.  But it apparently doesn't.
		 Don't know why, can't say how.  It ALWAYS calls
		 _init() though, so I guess we can just change all
		 of our loadable libraries to have that function.
		 Can you say "UGH?"  (Ditto for linux) */
#if defined (__FreeBSD__) || defined (__OpenBSD__) || defined (linux)
	      no_init_p = 1;
#endif

#if defined (__cplusplus) || defined (Solaris)
	      if (!initfunc_name)
		initfunc_name = "module_initialize";
#endif

	      if (initfunc_name)
		{
		  no_init_p = 0;
		}

	      if (empty_string_p (initfunc_name))
		{
		  initfunc_name = "module_initialize";
		}

	      if (!no_init_p)
		{
		  VFUN *initfunc = (VFUN *)NULL;

#if defined (USE_SHL_LOAD)
		  shl_findsym (&handle, initfunc_name, TYPE_PROCEDURE,
			       &initfunc);
#else
		  initfunc = (VFUN *) dlsym (handle, initfunc_name);
#endif

		  if (initfunc != (VFUN *)NULL)
		    {
		      (*initfunc) ();
		      result = strdup (fqn);
		    }
		  else
		    {
		      module_recent_error (fqn);
#if defined (USE_SHL_LOAD)
		      shl_unload (handle);
#else
		      dlclose (handle);
#endif
		    }
		}
	      else
		result = strdup (fqn);

	      if (result != (char *)NULL)
		{
		  Symbol *sym = symbol_intern (MODULE_SYM_NAME);
		  char hexrep[40];

		  sprintf (hexrep, "0x%010lx", (unsigned long)handle);
		  symbol_add_value (sym, fqn);
		  sym = symbol_intern_in_package((Package *)sym->package, fqn);
		  symbol_add_value (sym, hexrep);
		}
	    }
	  else
	    {
	      /* Error opening library. */
	      if (allow_output_p)
		module_recent_error (module_name);
	    }
	}
    }

  return (result);
}

DEFUN (pf_load_module, module-name &key noinitialize initfunc,
"Loads a module dynamically at runtime, adding the function
definitions found therein to the global package of <Meta-HTML>
functions.

<code>load-module</code> returns a non-empty string if the module is
loaded successfully, or places an error in <funref language-operators
system-error-output> if not.  The string returned is the fully
qualified pathname of the module just loaded.

<var module-name> is searched for by looking in each directory
specified in the array <varref mhtml::module-directories>, or by
loading the module as if the name specified is the full pathname to
that file.

Once a module is loaded, the functions within it can be invoked just
as with any standard <Meta-HTML> command.

See the <code>examples.c</code> file in the <code>modules</code>
directory of your distribution for more information on writing
<Meta-HTML> modules.

The keyword argument <var noinitialize> when set to \"true\" says
not to call the default initialization function of the module
(<i>module_initialize</i>).  This function is typically used to install
the function names of the Meta-HTML callable functions present within the
module.  When such initialization is not required (perhaps you are planning
to do this from within a different module), the use of this argument
prevents the attempted call.

The keyword argument <var initfunc> may be set to the name of the
initialization function to call immediately after the module is
loaded.  If it is not set, or it is empty, the function
<b>module_initialize</b> is called.

Example:

<example>
<set-var loaded? = <load-module /www/lib/example.so>>
</example>")
{
  char *module_name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;
  char *mode_arg = mhtml_evaluate_string (get_value (vars, "mode"));
  char *no_init = mhtml_evaluate_string (get_value (vars, "noinitialize"));
  char *initfunc_name = mhtml_evaluate_string (get_value (vars, "initfunc"));

  result = mhtml_load_module (module_name, mode_arg, !empty_string_p (no_init),
			      initfunc_name, 1);

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      xfree (result);
    }

  xfree (module_name);
  xfree (mode_arg);
  xfree (no_init);
  xfree (initfunc_name);
}

DEFUN (pf_unload_module, module-name,
"Unloads a previously loaded dynamic module.  You might want this if you
are running Meta-HTML as a FastCGI, and the module was temporarily needed
for some service, and was inordinately large.

Returns \"true\" if the module was successfully unloaded.")
{
  char *module_name = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (module_name))
    {
      void *handle = find_module_handle (module_name);

      if (handle != (void *)NULL)
	{
#if defined (USE_SHL_LOAD)
	  shl_unload (handle);
#else
	  dlclose (handle);
#endif
	  remove_module_references (module_name);
	  bprintf_insert (page, start, "true");
	}
    }

  xfree (module_name);
}

DEFUN (pf_module_function, module-name function-name,
"Returns the address of <var function-name> in <var module-name> if
<var module-name> is an already loaded dynamic module, and <var function-name>
is a function defined within that module.

Otherwise, it returns the empty string.
<example>
<load-module example>                 --> /www/lib/example.so
<module-function example pf_apropos>  --> 0X0EF8C437C
</example>")
{
  char *module_name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *function_name = mhtml_evaluate_string (get_positional_arg (vars, 1));
  static char digits[40];
  char *result = (char *)NULL;

  if ((!empty_string_p (module_name)) && (!empty_string_p (function_name)))
    {
      void *handle = find_module_handle (module_name);

      if (handle != (void *)NULL)
	{
	  VFUN *func = (VFUN *)NULL;
#if defined (USE_SHL_LOAD)
	  shl_findsym (&handle, function_name, TYPE_PROCEDURE, &func);
#else
	  func = (VFUN *) dlsym (handle, function_name);
#endif

	  if (func != (VFUN *)NULL)
	    {
	      sprintf (digits, "0x%010lx", (unsigned long)func);
	      result = digits;
	    }
	}
    }

  xfree (module_name);
  xfree (function_name);

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

DEFUN (pf_module_call_function, module-name function-name &rest args,
"Call the function <var function-name> in the dynamically loaded module
<var module_name> with character arguments of <var args>.
It returns \"true\" if the function invocation took place.  More likely,
you will be crashing Meta-HTML without a way to debug it.

Up to ten arguments can be passed to the function.

It is so unlikely that you need this, that you will have to write me E-mail
or read the source to see exactly how this works.  If you are calling this
function, the called function should not produce any output at all, and,
in fact, there isn't a way for you to get output back from the function,
since you can't pass the address of a variable, etc.  This might be used to
re-initialize some internals of your module, but you should really just
provide an interface for that instead.")
{
  char *module_name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *function_name = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if ((!empty_string_p (module_name)) && (!empty_string_p (function_name)))
    {
      void *handle = find_module_handle (module_name);

      if (handle != (void *)NULL)
	{
	  HVFUN *func = (HVFUN *)NULL;
#if defined (USE_SHL_LOAD)
	  shl_findsym (&handle, function_name, TYPE_PROCEDURE, &func);
#else
	  func = (HVFUN *) dlsym (handle, function_name);
#endif

	  if (func != (HVFUN *)NULL)
	    {
	      int which = 2;
	      char *arg[11];

	      while (which < 12)
		{
		  arg[which - 2] = 
		    mhtml_evaluate_string (get_positional_arg (vars, which));
		  which++;
		}

	      (*func) (arg[0], arg[1], arg[2], arg[3], arg[4],
		       arg[5], arg[6], arg[7], arg[8], arg[9]);

	      which = 0;
	      while (which < 10)
		{
		  xfree (arg[which]);
		  which++;
		}

	      bprintf_insert (page, start, "true");
	    }
	}
    }

  xfree (module_name);
  xfree (function_name);
}

#if defined (__cplusplus)
}
#endif
