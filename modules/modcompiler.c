/* modcompiler.c: -*- C -*-  */

/*  Copyright (c) 2000, SupplySolution, Inc.
    Author: E. B. Gamble Jr. (ed.gamble@alum.mit.edu) */

#include <sys/stat.h>		/* stat() */
#include <unistd.h>

#define MODULE_INITIALIZER_EXTRA_CODE mh_module_initialize ();

#include "modules.h"
#include "compiler/compile.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static int compiler_version_major = 0;
static int compiler_version_minor = 90;

static void pf_comp_run (PFunArgs);
static void pf_comp_inc (PFunArgs);
static void pf_comp_fun (PFunArgs);
static void pf_comp_dis (PFunArgs);
static void pf_comp_dmp (PFunArgs);
static void pf_comp_ver (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag	                    complex? debug_level    code    */
  { "COMP-RUN",				0,	 0,	pf_comp_run },
  { "COMP-INC",				0,	 0,	pf_comp_inc },
  { "COMP-FUN",				0,	 0,	pf_comp_fun },
  { "COMP-DIS",				0,	 0,	pf_comp_dis },
  { "COMP-DMP",				0,	 0,	pf_comp_dmp },
  { "COMPILER::COMPILER-VERSION",	0,	 0,	pf_comp_ver },
  { "COMPILER::COMPILE-FUNCTION",	0,	 0,	pf_comp_fun },
  { "COMPILER::COMPILE-SYMBOL",		0,	 0,	pf_comp_fun },
  { "COMPILER::COMPILE-AND-RUN",	0,	 0,	pf_comp_run },
  { "COMPILER::COMPILE-FILE",		0,	 0,	pf_comp_inc },
  { "COMPILER::DISASSEMBLE",		0,	 0,	pf_comp_dis },

  { (char *)NULL,			0,	 0,	(PFunHandler *)NULL }
};

static void mh_module_initialize (void);

MODULE_INITIALIZE ("modcompiler", ftab)

static void
mh_module_initialize (void)
{
  Symbol *sym;

  mh_object_init ();

  sym = symbol_intern ("COMPILER::COMPILE-VERBOSE-DEBUGGING");
  symbol_notify_value (sym, (void*) &mh_comp_verbose_debugging);
  /* symbol_add_value (sym, "any value"); */

}

/* This function should not directly talk to stdout, but should fill a buffer
   instead.  The buffer can then be system-error-output, or printed, or simply
   returned. */
static mh_object_t
compiler_evaluate (string_t    input_source,
		   mh_string_t input, 
		   int  debug_level)
{
  mh_parse_t    parse    = MH_PARSE_EMPTY;
  mh_core_t     core_exp = MH_CORE_NULL;
  mh_core_t     core_opt = MH_CORE_NULL;
  mh_object_t   object   = MH_AS_OBJECT (MH_EMPTY);
  mh_tag_t func;

  func = mh_compile_with_intermediates
    (input_source, input, &parse, &core_exp, &core_opt);

  if (debug_level > 5)
    {
      if (parse)
	{
	  printf ("\n\nParse:");
	  mh_parse_show (parse);
	  /* A bug prevents this freeing */
	  /* mh_parse_free (parse); */
	}

      if (core_exp)
	{
	  printf ("\n\nCore:");
	  mh_core_show (core_exp);
	  mh_core_free (core_exp);
	}

      if (core_opt)
	{
	  printf ("\n\nCore (Optimized):");
	  mh_core_show (core_opt);
	  mh_core_free (core_opt);
	  printf ("\n");
	}

      if (func)
	mh_tag_disassemble (func, stdout);
    }

  if (func)
    {
      object = mh_welcome_to_the_machine (func, (mh_object_t *) NULL, 0);

      /* Free FUNC */
    }

  if (debug_level > 5)
    {
      string_t result = 
	mh_object_to_string (object, true);

      page_debug ("\n\nResult: %s\n", result);

      xfree (result);
    }

  return object;
}


DEFINE_SECTION (COMPILER-MODULE, lisp;scheme;lists,
"The functions in this module treat their arguments ...", "")

#define COMPILE_AND_RUN_DOC_STRING \
"Compile the code that is passed in <var code-string>.  Note that\n\
<var code-string> is not evaluated by the interpreter -- it is completely\n\
up to the compiler to do that.\n\
\n\
After the code is compiled, run it right away."
DEFUNX (compiler::compiler-and-run, code-string, COMPILE_AND_RUN_DOC_STRING)
DEFUN (pf_comp_run, code-string, COMPILE_AND_RUN_DOC_STRING)
{
  char       *comp_arg    = get_positional_arg (vars, 0);
  mh_string_t comp_string = comp_arg != NULL
    ? mh_string_new (comp_arg)
    : MH_EMPTY;
  mh_object_t object = compiler_evaluate ("stdin", comp_string, debug_level);
  char *result = mh_object_to_string (object, false);

  bprintf_insert (page, start, "%s", result);
  *newstart += strlen (result);
  xfree (result);

  /* Free OBJECT */
}

#define COMPILE_FUNCTION_DOC_STRING \
"Compile the function named by <var function-name>, and install the resultant\n\
compiled code in the function cell of <var function-name>.  This assumes that\n\
<var function-name> has already been defined by the interpreter, either through\n\
an explicit <tag defun>, or by having been loaded via <tag require> or\n\
<tag %%load-package-file>"

DEFUNX (compiler::compile-function, function-name, COMPILE_FUNCTION_DOC_STRING)

extern void mh_object_restore_symbol (Symbol *sym);

static int
mh_compilable_function_p (char *definition)
{
  int result = 1;

  if (definition != (char *)NULL)
    {
      if ((strstr (definition, "%body")) ||
	  (strstr (definition, "%xbody")) ||
	  (strstr (definition, "%qbody")) ||
	  (strstr (definition, "nocompile=true")) ||
	  (strstr (definition, "%attributes")))
	result = 0;

      if (result)
	{
	  register int i, c;
	  for (i = 0; (c = definition[i]) != '\0'; i++)
	    {
	      if ((c == '%') && isdigit (definition[i + 1]))
		{
		  result = 0;
		  break;
		}
	    }
	}

#if defined (MODCOMPILER_EXCLUDES_BODY_AND_ATTRIBUTES)
      if (result)
	{
	  register int i, end, c;

	  for (end = 0; (c = definition[end]) != '\0'; end++)
	    if (c == '>') break;

	  for (i = 0; i < end; i++)
	    {
	      if (definition[i] == '&')
		{
		  if ((strncasecmp (definition + i, "&body", 5) == 0) ||
		      (strncasecmp (definition + i, "&attributes", 11) == 0))
		    {
		      result = 0;
		      break;
		    }
		}
	    }
	}
#endif /* defined (MODCOMPILER_EXCLUDES_BODY_AND_ATTRIBUTES) */
    }
  else
    result = 0;

  return (result);
}

DEFUN (pf_comp_fun, function-name, COMPILE_FUNCTION_DOC_STRING)
{
  /* The NAME of the function to compile. */
  char *symname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (!empty_string_p (symname))
    {
      Symbol *sym = symbol_lookup_in_package (mhtml_user_keywords, symname);

      if (sym == (Symbol *)NULL)
	sym = symbol_lookup (symname);

      if (sym != (Symbol *)NULL)
	{
	  if (sym->type == symtype_USERFUN)
	    {
	      char *def_text = mhtml_recompose_function (sym);

	      if (mh_compilable_function_p (def_text))
		{
		  /* Make a machine string with the definition. */
		  mh_string_t comp_string = mh_string_new (def_text);
		  mh_object_t object = 
		    compiler_evaluate (def_text, comp_string, debug_level);

		  result = mh_object_to_string (object, false);
		}

	      xfree (def_text);
	    }
	  else
	    {
	      mh_object_restore_symbol (sym);
	    }
	}
    }

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }

  xfree (symname);
}

#define COMPILE_DISASSEMBLE_DOC_STRING \
"Compile the function named by <var function-name>, and install the\n\
resultant compiled code in the function cell of <var function-name>.\n\
This assumes that <var function-name> has already been defined by the\n\
interpreter, either through an explicit <tag defun>, or by having been\n\
loaded via <tag require> or <tag %%load-package-file>"

DEFUNX (compiler::disassemble, function-name, COMPILE_DISASSEMBLE_DOC_STRING)

DEFUN (pf_comp_dis, function-name, COMPILE_DISASSEMBLE_DOC_STRING)
{
  /* The NAME of the function to compile */
  char *fun_arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *fun_dis = NULL;

  /* Lookup the mh_tag_t for FUN_ARG */
  Symbol *fun_sym =
    mhtml_find_user_function_symbol (fun_arg);

  /* Disassemble it */
  if (NULL != fun_sym && NULL != fun_sym->machine) 
    {
      mh_tag_t tag = (mh_tag_t) fun_sym->machine;

      mh_tag_disassemble (tag, stdout);
    }

  bprintf_insert (page, start, "%s", fun_dis);
  xfree (fun_arg);
}

DEFUN (pf_comp_dmp, function-name,  "")
{
  /* The NAME of the function to compile */
  char *fun_arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  Symbol *fun_sym = mhtml_find_user_function_symbol (fun_arg);

  /* Disassemble it */
  if (NULL != fun_sym && NULL != fun_sym->machine) 
    {
#if 0
      extern Datablock *
	mh_object_to_datablock (mh_object_t object);

      mh_tag_t   tag   = (mh_tag_t) fun_sym->machine;
      Datablock *block = mh_object_to_datablock (MH_AS_OBJECT (tag));

      bprintf_insert_binary (page, start,
			     block->data,
			     block->length);
#endif
    }

  xfree (fun_arg);
}
#define COMPILER_VERSION_DOC_STRING "Return the version of the compiler"

DEFUNX (compiler::compiler-version, , COMPILER_VERSION_DOC_STRING)
DEFUN (pf_comp_ver, , COMPILER_VERSION_DOC_STRING)
{
  bprintf_insert (page, start, "%d.%d",
		  compiler_version_major, compiler_version_minor);
}

#define COMPILER_INCLUDE_DOC_STRING \
"Read the contents of the file named by <var filename>, and compile it,\n\
and run the resultant compilation."
DEFUN (pf_comp_inc, filename, COMPILER_INCLUDE_DOC_STRING)
{
  char *filename = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (filename))
    {
      PAGE *file = page_read_template (filename);

      if (file != (PAGE *)NULL)
	{
	  mh_string_t comp_string =
	    mh_string_buffer (file->buffer, file->bindex);

	  mh_object_t object =
	    compiler_evaluate (filename, comp_string, debug_level);

	  char *result = mh_object_to_string (object, false);

	  bprintf_insert (page, start, "%s", result);
	  *newstart += strlen (result);
	  xfree (result);

	  /* Free OBJECT */
	  page_free_page (file);

	}
    }

  xfree (filename);
}

#if defined (__cplusplus)
}
#endif


#if defined (THIS_IS_A_COMMENT)
/*
(mdb) <comp-run "<defun sb x y><sub x y></defun>">
;; Name     : sb
;; Address  : 0x81a42f8
;; StackSize: 2
;; Constants:
;; Code     :
;;       0-1:  sget   0
;;       2-3:  sget   1
;;         4:  sub  
;;         5:  ret  

(mdb) <sb 10 2>
Machine Apply: <sb ...>
Machine Args[0]: 10, 10, 10
Machine Args[1]: 2, 2, 2
Machine Result: 8
8

(mdb) <al-foo 1 2 3>                  ;;; Previously compiled
Machine Apply: <al-foo ...>
Machine Args[0]: 1, 1, 1
Machine Args[1]: 2, 2, 2
Machine Args[2]: 3, 3, 3
Machine Result: 1
1

(mdb) <make-alist xyz=<al-foo 1 2 3> abc=10>
Machine Apply: <al-foo ...>
Machine Args[0]: 1, 1, 1
Machine Args[1]: 2, 2, 2
Machine Args[2]: 3, 3, 3
Machine Result: 1
(("XYZ" . "1")("ABC" . "10"))

*/
#endif /* THIS IS A COMMENT */


