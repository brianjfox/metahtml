/* mhc.c: -*- C -*-  Run Meta-HTML on a single file. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Thu Nov 30 13:31:36 1995.  */

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

#include "language.h"
#if !defined (MHTML_SYSTEM_TYPE)
#  define MHTML_SYSTEM_TYPE "Incorrectly Compiled"
#endif

#if !defined (MHTML_VERSION_STRING)
static char *mhtml_version_string = "";
#else
static char *mhtml_version_string = MHTML_VERSION_STRING;
#endif

static int call_bootstrap = 1;

#if !defined (macintosh)
extern char **environ;
#endif /* !macintosh */

static void parse_program_args (int argc, char *argv[]);
static void prep_for_page_process (PAGE *page, int argc, char *argv[]);
static void usage (void);
static void non_local_exit (int exit_code);

static char *progname = (char *)NULL;
static PAGE *input_contents = (PAGE *)NULL;
static PAGE *config_page = (PAGE *)NULL;

extern char *metahtml_copyright_string;

/* This program takes a single filename as an argument, and runs the
   page processor on it.  You can set variables on the command line
   with the `--set' flag.  The output is written to standard out.
   If no filename is passed, the file used is argv[0]. */
int
main (int argc, char *argv[])
{
  int exit_code = 0;

#if defined (NOTDEF)
  register int i;

  for (i = 0; i < argc; i++)
    fprintf (stdout, "argv[%d] = `%s'<br>\n", i, argv[i]);
#endif

  parse_program_args (argc, argv);

  if (!input_contents)
    usage ();

  prep_for_page_process (input_contents, argc, argv);

  if (config_page != (PAGE *)NULL)
    {
      page_process_page (config_page);
      if (config_page) page_free_page (config_page);
    }

  page_process_page (input_contents);
  if (input_contents && input_contents->buffer)
    {
      if (pagefunc_get_variable ("mhc::explicit-output-only") == (char *)NULL)
	fprintf (stdout, "%s", input_contents->buffer);
    }

  /* Perhaps return a different exit code. */
  {
    char *exit_code_text = pagefunc_get_variable ("mhc::exit-code");
    if (!empty_string_p (exit_code_text))
      exit_code = atoi (exit_code_text);
  }

  /* This doesn't actually return. */
  non_local_exit (exit_code);
  return (0);
}

static void
non_local_exit (int exit_code)
{
  page_process_page_internal (get_after_page_return_buffer ());
  exit (exit_code);
}


static void
usage (void)
{
  if (isatty (fileno (stdin)))
    {
      fprintf (stderr, "mhc: %s%s\n\n", MHTML_SYSTEM_TYPE,
	       metahtml_copyright_string);
      fprintf (stderr, "Usage: %s [--set NAME VALUE]... FILENAME\n", progname);
    }
  exit (1);
}

static void
pf_mhc_exit (PFunArgs)
{
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int exit_code = 0;

  if (!empty_string_p (arg))
    exit_code = atoi (arg);

  non_local_exit (exit_code);
}


static void
pf_mhc_print (PFunArgs)
{
  char *item = mhtml_evaluate_string (get_positional_arg (vars, 0));
  if ((item != (char *)NULL) && (item[0] != '\0'))
    fprintf (stdout, "%s", item);
  fflush (stdout);

  xfree (item);
}

static PFunDesc mhc_func_table[] =
{
  { "MHC::PRINT",	0, 0, pf_mhc_print },
  { "MHC::EXIT",	0, 0, pf_mhc_exit },
  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

static void
define_mhc_primitives (Package *package)
{
  register int i;
  register Symbol *sym;

  for (i = 0; mhc_func_table[i].tag != (char *)NULL; i++)
    {
      sym = symbol_intern_in_package (package, mhc_func_table[i].tag);
      sym->type = symtype_FUNCTION;
      sym->values = (char **) (&mhc_func_table[i]);
    }
}

static void
prep_for_page_process (PAGE *page, int argc, char *argv[])
{
  register int i;

  mhtml_system_preload (1);
  define_mhc_primitives (mhtml_function_package);

  if (call_bootstrap)
    {
      char *ignore = mhtml_evaluate_string ("<%%bootstrap-metahtml>");
      xfree (ignore);
    }

  if (page && page->buffer)
    {
      if (page->buffer[0] == '#' && page->buffer[1] == '!')
	{
	  for (i = 2; i < page->bindex; i++)
	    {
	      if (page->buffer[i] == '\n')
		{
		  i++;
		  break;
		}
	    }

	  bprintf_delete_range (page, 0, i);
	}

      pagefunc_set_variable ("mhtml::version", mhtml_version_string);
      pagefunc_set_variable ("mhtml::system-type", MHTML_SYSTEM_TYPE);
      pagefunc_set_variable ("mhtml::exec-path", (char *)getenv ("PATH"));

      /* Now set Meta-HTML variables, and variables from the environment. */
      forms_input_data (argc, argv);

      /* The call to forms_input_data () bashed default::program-arguments.
	 Replace that with the arguments to our script. */
      {
	  char *ignore = mhtml_evaluate_string
	    ("<copy-var mhtml::program-arguments default::program-arguments>");
	  xfree (ignore);
	}
    }
}

static void
parse_program_args (int argc, char *argv[])
{
  int arg_index = 1;
  char *filename = (char *)NULL;
  Symbol *program_args = symbol_intern ("mhtml::program-arguments");

  progname = argv[0];
  pagefunc_set_variable ("mhc::mhc-executatble", progname);

  /* Remember the argv array in a symbol reserved for that purpose. */
  {
    register int i;
    Symbol *s = symbol_intern ("mhc::argv");

    for (i = 0; i < argc; i++)
      symbol_add_value (s, argv[i]);
  }

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if (strcasecmp (arg, "--version") == 0)
	{
	  fprintf (stdout, "%s\n", mhtml_version_string);
	  if (arg_index + 1 >= argc)
	    exit (0);
	}
      else if ((strcasecmp (arg, "--set") == 0) && (arg_index + 1 < argc))
	{
	  char *name = argv[arg_index++];
	  char *value = argv[arg_index++];

	  pagefunc_set_variable (name, value);
	}
      else if (!filename && (strcmp (arg, "-z") == 0))
	call_bootstrap = 0;
      else if (!filename && (strcmp (arg, "--config") == 0))
	{
	  char *config_file = argv[arg_index];

	  if (config_file == (char *)NULL)
	    usage ();
	  else
	    {
	      config_page = page_read_template (config_file);
	      arg_index++;
	    }
	}
      else if (!filename && (*arg != '-'))
	{
	  filename = arg;
	  symbol_add_value (program_args, filename);
	  pagefunc_set_variable ("mhc::script-name", filename);
	}
      else if (filename)
	symbol_add_value (program_args, arg);
      else
	usage ();
    }

  if (!filename)
    {
      char thisdir[1120];

      if (getcwd (thisdir, 1023) < 0)
	sprintf (thisdir, ".");

      strcat (thisdir, "/*standard-input*");

      filename = strdup (thisdir);

      if (isatty (fileno (stdin)))
	usage ();
      else
	{
	  PAGE *page = page_create_page ();
	  char buffer[1024];
	  int done = 0;
	  register int i;
	  
	  while (!done)
	    {
	      for (i = 0; i < 1024; i++) buffer[i] = '\0';
	      fread (buffer, 1023, 1, stdin);
	      bprintf (page, "%s", buffer);

	      if (feof (stdin))
		done = 1;
	    }

	  input_contents = page;
	}
    }
  else
    input_contents = page_read_template (filename);

  {
    char *temp = pagefunc_get_variable ("mhtml::include-prefix");

    if (empty_string_p (temp))
      {
	temp = strrchr (filename, '/');

	if (temp != (char *)NULL)
	  {
	    *temp = '\0';
	    pagefunc_set_variable ("mhtml::include-prefix", filename);
	  }
	else
	  {
	    char dir[1024];
	    temp = getcwd (dir, sizeof (dir));
	    if (temp != (char *)NULL)
	      pagefunc_set_variable ("mhtml::include-prefix", dir);
	    else
	      pagefunc_set_variable ("mhtml::include-prefix", "/");
	  }
      }
  }
}
