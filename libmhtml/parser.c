/* parser.c: -*- C -*-  Parser internals for the page processor. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct 14 14:22:35 1996.  */

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

#define COMPILING_PARSER_C 1
#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

char LEFT_BRACKET = '<';
char RIGHT_BRACKET = '>';

extern void initialize_external_functions (Package *p);

char *mhtml_version_major = MHTML_VERSION_STRING;
char *mhtml_version_minor = MHTML_SUBVER_STRING;

char *metahtml_copyright_string = "\
Copyright (C) 1995, 2001, Brian J. Fox\n";

/* The parser's idea of what the current line number is. */
int parser_current_lineno = 0;

/* Globally known variable holds onto to the reserved words. */
Package *mhtml_function_package = (Package *)NULL;

/* Globally known variable is the package containing user-defined functions. */
Package *mhtml_user_keywords = (Package *)NULL;

/* Variable specifically for the mdb debugger.  This allows the setting
   of a callback function when an mdb breakpoint is hit. */
DEBUGGER_CALLBACK_FUNCTION *mhtml_debug_callback_function = NULL;

/* Variable specifically for interrupting the parser.  When non-zero,
   the parser stops dead in its tracks.  MDB uses this. */
int mhtml_parser_interrupted = 0;
MHTML_PARSER_CALLBACK_FUNCTION *mhtml_parser_callback_function = NULL;

#if defined (METAHTML_COMPILER)
COMPILER_APPLY_FUNCTION *mhtml_machine_apply_function_hook = NULL;
#endif

/* The "DEFAULT" package. */
Package *PageVars = (Package *)NULL;

/* Special case code can throw out from multiple levels deep in order to
   immediately return some HTTP.  You should call the following function
   page_return_this_page (page) in order to make that happen. */
static PAGE *ImmediatePage = (PAGE *)NULL;
jmp_buf page_jmp_buffer;

void
page_return_this_page (PAGE *page)
{
  ImmediatePage = page_copy_page (page);
  longjmp (page_jmp_buffer, 1);
}

/* Sequentially process PAGE. */
static PAGE *ThePage = (PAGE *)NULL;
static int TheOffset = 0;

PAGE *
mhtml_top_level_page (void)
{
  return (ThePage);
}

PageEnv *
pagefunc_save_environment (void)
{
  PageEnv *env = (PageEnv *)xmalloc (sizeof (PageEnv));

  memcpy (&(env->env), &page_jmp_buffer, sizeof (jmp_buf));
  env->page = ThePage;
  env->offset = TheOffset;

  return (env);
}

void
pagefunc_restore_environment (PageEnv *env)
{
  memcpy (&page_jmp_buffer, &(env->env), sizeof (jmp_buf));
  ThePage = env->page;
  TheOffset = env->offset;

  free (env);
}

PAGE *
parser_top_page (void)
{
  return (ThePage);
}

/* Gets 1 when mhtml::inhibit-comment-parsing has a value, 0 otherwise. */
int mhtml_inhibit_comment_parsing = 0;

/* Gets 1 when mhtml::decimal-places has a value, 0 otherwise. */
int mhtml_decimal_notify = 0;
int mhtml_decimal_places = 2;

/* Gets 1 when mhtml::parser-trace-mode has a value, 0 otherwise. */
int mhtml_parser_trace_mode = 0;

/* Gets 1 when mhtml::gather-documentation has a value, 0 otherwise. */
int mhtml_gather_documentation = 0;

#if defined (METAHTML_PROFILER)
/* Gets 1 when mhtml::profile-functions has a value, 0 otherwise. */
int mhtml_profiling_functions = 0;
#endif

/* Gets 1 when mhtml::warn-on-redefine-primitive has a value, 0 otherwise. */
int mhtml_warn_on_redefine_primitive = 0;

/* Gets 1 when mhtml::warn-on-redefine-function has a value, 0 otherwise. */
int mhtml_warn_on_redefine = 0;

/* Gets 1 when mhtml::verbose-error-reporting has a value, 0 otherwise. */
int mhtml_verbose_error_reporting = 0;

/* Gets 1 when mhtml::remember-function-calls, 0 otherwise. */
int mhtml_remember_function_calls = 0;

void
pagefunc_initialize_notifiers (void)
{
  Symbol *sym;

  sym = symbol_intern ("mhtml::inhibit-comment-parsing");
  symbol_notify_value (sym, &mhtml_inhibit_comment_parsing);
  sym = symbol_intern ("mhtml::decimal-places");
  symbol_notify_value (sym, &mhtml_decimal_notify);
  sym = symbol_intern ("mhtml::parser-trace-mode");
  symbol_notify_value (sym, &mhtml_parser_trace_mode);
  sym = symbol_intern ("mhtml::gather-documentation");
  symbol_notify_value (sym, &mhtml_gather_documentation);
  sym = symbol_intern ("mhtml::warn-on-redefine-primitive");
  symbol_notify_value (sym, &mhtml_warn_on_redefine_primitive);
  sym = symbol_intern ("mhtml::warn-on-redefine");
  symbol_notify_value (sym, &mhtml_warn_on_redefine);
  sym = symbol_intern ("mhtml::verbose-error-reporting");
  symbol_notify_value (sym, &mhtml_verbose_error_reporting);
  sym = symbol_intern ("mhtml::remember-function-calls");
  symbol_notify_value (sym, &mhtml_remember_function_calls);
#if defined (METAHTML_PROFILER)
  sym = symbol_intern ("mhtml::profile-functions");
  symbol_notify_value (sym, &mhtml_profiling_functions);
#endif /* METAHTML_PROFILER */

  sym = symbol_intern ("mhtml::flag-newly-interned-symbols");
  symbol_notify_value (sym, &mhtml_flag_newly_interned_symbols);
  pagefunc_set_variable ("mhtml::version-major", mhtml_version_major);
  pagefunc_set_variable ("mhtml::version-minor", mhtml_version_minor);
}

static int syntax_checking = 0;
static int syntax_failure = 0;

int
page_check_syntax (PAGE *page)
{
  int syntax_ok;

  syntax_checking = 1;
  syntax_failure = 0;
  page_process_page_internal (page);
  syntax_ok = !syntax_failure;
  syntax_checking = 0;
  syntax_failure = 0;
  return (syntax_ok);
}

void
page_process_page (PAGE *page)
{
  static int notifiers_initialized = 0;

  if (!notifiers_initialized)
    {
      /* First call to this function, just initialize. */
      pagefunc_initialize_notifiers ();
      notifiers_initialized++;
    }
  else
    {
      /* Subsequent calls, get rid of dangling pointers. */
      forms_gc_pointers ();
    }

  ImmediatePage = (PAGE *)NULL;
  ThePage = (PAGE *)page;
  TheOffset = 0;

  /* The ugliest hack in the world.  Please shoot me. */
  if (setjmp (page_jmp_buffer) != 0)
    {
      page->buffer = ImmediatePage->buffer;
      page->bindex = ImmediatePage->bindex;
      page->bsize  = ImmediatePage->bsize;
    }
  else
    page_process_page_internal ((PAGE *)page);
}

/* For internal use only.  Returns the zeroith value element for
   NAME in PACKAGE. */
char *
get_value (Package *package, char *name)
{
  char *value = (char *)NULL;

  if (package != (Package *)NULL)
    {
      Symbol *sym = symbol_lookup_in_package (package, name);

      if (sym && sym->values_index)
	value = sym->values[0];
    }
  return (value);
}

/* Return the values list of *pvars* in PACKAGE. */
char **
get_vars_names (Package *package)
{
  char **names = (char **)NULL;

  if (package != (Package *)NULL)
    {
      Symbol *sym = symbol_lookup_in_package (package, "*pvars*");

      if (sym != (Symbol *)NULL)
	names = sym->values;
    }

  return (names);
}

/* Return the values list of *pvals* in PACKAGE. */
char **
get_vars_vals (Package *package)
{
  char **vals = (char **)NULL;

  if (package != (Package *)NULL)
    {
      Symbol *sym = symbol_lookup_in_package (package, "*pvals*");

      if (sym != (Symbol *)NULL)
	vals = sym->values;
    }

  return (vals);
}

void
pagefunc_set_variable (char *tag, char *value)
{
  Package *orig_package = CurrentPackage;

  if (PageVars == (Package *)NULL)
    PageVars = symbol_get_package_hash (DEFAULT_PACKAGE_NAME, 577);

  if (CurrentPackage == (Package *)NULL)
    symbol_set_default_package (PageVars);

  forms_set_tag_value (tag, value);
  symbol_set_default_package (orig_package);
}

void
pagefunc_set_variable_readonly (char *tag, char *value)
{
  Package *orig_package = CurrentPackage;
  Symbol *sym;

  if (PageVars == (Package *)NULL)
    PageVars = symbol_get_package_hash (DEFAULT_PACKAGE_NAME, 577);

  if (CurrentPackage == (Package *)NULL)
    symbol_set_default_package (PageVars);

  forms_set_tag_value (tag, value);
  sym = symbol_lookup (tag);
  if (sym != (Symbol *)NULL)
    symbol_set_flag (sym, sym_READONLY);

  symbol_set_default_package (orig_package);
}

char *
pagefunc_get_variable (char *tag)
{
  Package *orig_package = CurrentPackage;
  char *value;

  if (PageVars == (Package *)NULL)
    PageVars = symbol_get_package ("default");

  if (CurrentPackage == (Package *)NULL)
    symbol_set_default_package (PageVars);

  value = forms_get_tag_value (tag);

  symbol_set_default_package (orig_package);
  return (value);
}

char *
get_one_of (Package *package, char *tag, ...)
{
  char *value = (char *)NULL;
  va_list args;

  va_start (args, tag);

  while (tag)
    {
      value = forms_get_tag_value_in_package (package, tag);

      if (value)
	break;

      tag = va_arg (args, char *);
    }

  va_end (args);
  return (value);
}

char *
get_positional_arg (Package *package, int position)
{
  char *result = (char *)NULL;
  int pos = 0;

  if (package != (Package *)NULL)
    {
      Symbol *pvars = symbol_lookup_in_package (package, "*pvars*");

      if (pvars != (Symbol *)NULL)
	{
	  register int i;
	  Symbol *sym;

	  for (i = 0; i < pvars->values_index; i++)
	    {
	      sym = symbol_lookup_in_package (package, pvars->values[i]);

	      if ((sym != (Symbol *)NULL) && (sym->values_index == 0))
		{
		  if (position == pos)
		    {
		      result = pvars->values[i];
		      break;
		    }
		  else
		    pos++;
		}
	    }
	}
    }
  return (result);
}

char *
read_sexp_1 (char *string, int *start, int stop_at_equals_p, int one_list)
{
  static char *workspace = (char *)NULL;
  static int wsize = 0;
  int expr_present = 0;
  char *result = (char *)NULL;

  if (string != (char *)NULL)
    {
      register int i = *start;
      register int string_len = strlen (string);
      int windex, gobbled, quoted, depth;

      windex = gobbled = quoted = depth = 0;

      if (string_len >= wsize)
	workspace = (char *)xrealloc (workspace, (wsize = 10 + string_len));

      workspace[0] = '\0';

      /* Skip leading whitespace. */
      while (whitespace (string[i])) i++;

      gobbled = 0;
      while (!gobbled)
	{
	  register int orig_c = string[i++];
	  register int c;

	  c = orig_c;
	  if (orig_c == LEFT_BRACKET) c = '<';
	  if (orig_c == RIGHT_BRACKET) c = '>';

	  switch (c)
	    {
	    case '\\':
	      c = string[i++];

	      if (depth == 0)
		{
		  switch (c)
		    {
		    case 'n':
		      workspace[windex++] = '\n';
		      break;

		    case 't':
		      workspace[windex++] = '\t';
		      break;

		    case 'r':
		      workspace[windex++] = '\r';
		      break;

		    case 'f':
		      workspace[windex++] = '\f';
		      break;

		    case '\0':
		      workspace[windex] = '\\';
		      gobbled++;
		      break;

		    default:
		      workspace[windex++] = c;
		      break;
		    }
		}
	      else
		{
		  /* Skip the backslash, and the character which follows it.
		     We have to do this for the case of bizarre constructs,
		     such as <get-var <get-var \>>>. */
		  if (c != '\0')
		    {
		      workspace[windex++] = '\\';
		      workspace[windex++] = c;
		    }
		  else
		    {
		      workspace[windex] = '\\';
		      gobbled++;
		    }
		}
	      break;

	    case '<':
	      workspace[windex++] = orig_c;
	      if (!quoted)
		depth++;
	      break;

	    case '>':
	      workspace[windex++] = orig_c;
	      if (!quoted)
		{
		  depth--;
		  if (one_list && (depth == 0))
		    {
		      workspace[windex] = '\0';
		      gobbled++;
		    }
		}
	      break;

	    case '"':
	      quoted = !quoted;
	      if (depth)
		workspace[windex++] = '"';
	      else
		expr_present++;
	      break;

	    case '\r':
	      if (string[i] == '\n')
		{
		  if (!quoted && depth <= 0)
		    {
		      i++;
		      workspace[windex] = '\0';
		      gobbled++;
		    }
		  else
		    workspace[windex] = c;
		}
	      break;

	    case ';':
	      if ((string[i] == ';') && (string[i + 1] == ';') && !quoted)
		{
		  i += 2;
		  while ((string[i] != '\0') && (string[i] != '\n')) i++;
		}
	      else
		workspace[windex++] = c;
	      break;

	    case ' ':
	    case '\t':
	    case '\n':
	      if (!quoted && depth <= 0)
		{
		  workspace[windex] = '\0';
		  gobbled++;
		}
	      else
		workspace[windex++] = c;
	      break;

	    case '=':
	      if (stop_at_equals_p && !quoted && depth <= 0)
		{
		  workspace[windex] = '\0';
		  gobbled++;
		  i--;
		}
	      else
		workspace[windex++] = c;
	      break;

	    case '\0':
	      workspace[windex] = '\0';
	      gobbled++;
	      i--;
	      break;

	    default:
	      workspace[windex++] = c;
	      break;
	    }
	}

      if (windex || expr_present)
	result = strdup (workspace);

      *start = i;
    }

  return (result);
}

char *
read_sexp (char *string, int *start, int stop_at_equals_p)
{
  return (read_sexp_1 (string, start, stop_at_equals_p, 0));
}

/* If you want to delete a package, you should probably call this function
   rather than calling symbol_destroy_package () from symbols.c.  This 
   allows the engine to reset a bunch of internal variables if necessary. */
void
pagefunc_destroy_package (char *package_name)
{
  Package *package = symbol_lookup_package (package_name);

  if (package != (Package *)NULL)
    {
      if (package == PageVars)
	PageVars = (Package *)NULL;
      else if (package == mhtml_function_package)
	mhtml_function_package = (Package *)NULL;
      else if (package == mhtml_user_keywords)
	mhtml_user_keywords = (Package *)NULL;

      symbol_destroy_package (package);
    }
}

/* Gather arguments from STRING and return a newly consed anonymous package
   containing those arguments.  If second arg ALLOW_ASSIGNMENTS_P is non-zero,
   allow equals signs to indicate keyword values. */
Package *
pagefunc_snarf_vars (char *string, int allow_assignments_p)
{
  Package *package = (Package *)NULL;
  int offset = 0;
  int string_len;

  if (string == (char *)NULL)
    return (package);
  else
    string_len = strlen (string);

  /* Gobble name and value pairs. */
  while (offset < string_len)
    {
      char *name = (char *)NULL;
      char *value = (char *)NULL;

      name = read_sexp (string, &offset, allow_assignments_p);

      /* Skip any whitespace between the name and the '='
	 starting the value. */
      while (whitespace (string[offset])) offset++;

      /* If there is an equals sign here, get the value string. */
      if (string[offset] == '=')
	{
	  offset++;
	  if (name)
	    value = read_sexp (string, &offset, 0);
	}

      if (!name)
	continue;

      /* Add this pair to our list. */
      if (package == (Package *)NULL)
	package = symbol_get_package ((char *)NULL);

      if (value == (char *)NULL)
	symbol_intern_in_package (package, name);
      else
	forms_set_tag_value_in_package (package, name, value);

      /* Add the name and value to the list of ordered variables. */
      {
	Symbol *symbol = symbol_intern_in_package (package, "*pvars*");
	symbol_add_value (symbol, name);
	symbol = symbol_intern_in_package (package, "*pvals*");
	symbol_add_value (symbol, value ? value : "");
      }

      free (name);
      xfree (value);
    }

  return (package);
}

/* Return the primitive descriptor for TAG, or NULL if there is none. */
PFunDesc *
pagefunc_get_descriptor (char *tag)
{
  PFunDesc *desc = (PFunDesc *)NULL;
  Symbol *sym;

  if (mhtml_function_package == (Package *)NULL)
    {
      mhtml_function_package = symbol_get_package_hash ("*meta-html*", 577);
      initialize_external_functions (mhtml_function_package);
    }

  sym = symbol_lookup_in_package (mhtml_function_package, tag);

  if ((sym != (Symbol *)NULL) && (sym->type == symtype_FUNCTION))
    desc = (PFunDesc *)(sym->values);

  return (desc);
}

/* Return non-zero if STRING is non-zero or all whitespace. */
int
empty_string_p (char *string)
{
  int result = 1;

  if (string != (char *)NULL)
    {
      while (whitespace (*string)) string++;

      if (*string != '\0')
	result = 0;
    }

  return (result);
}

void
mhtml_set_numeric_variable (char *name, int value)
{
  static char rep[128];
  sprintf (rep, "%d", value);
  pagefunc_set_variable (name, rep);
}

void
mhtml_set_numeric_variable_in_package (Package *p, char *name, int value)
{
  static char rep[128];
  sprintf (rep, "%d", value);
  forms_set_tag_value_in_package (p, name, rep);
}

/* Read STRING, and convert the contents to a list of variables in PACKAGE. */
Package *
alist_to_package (char *string)
{
  WispObject *list = wisp_from_string (string);
  Package *package = (Package *)NULL;

  if (!CONS_P (list))
    return (package);

  while (list != NIL)
    {
      WispObject *pair;

      pair = CAR (list);
      list = CDR (list);

      if (CONS_P (pair) & STRING_P (CAR (pair)))
	{
	  char *tag;
	  Symbol *sym;

	  tag = STRING_VALUE (CAR (pair));

	  if (package == (Package *)NULL)
	    {
	      int old_prime = symbol_small_prime;
	      symbol_small_prime = 23;
	      package = symbol_get_package ((char *)NULL);
	      symbol_small_prime = old_prime;
	    }

	  if (STRING_P (CDR (pair)))
	    {
	      sym = symbol_intern_in_package (package, tag);
	      symbol_add_value (sym, STRING_VALUE (CDR (pair)));
	    }
	  else
	    {
	      WispObject *values = CDR (pair);

	      sym = symbol_intern_in_package (package, tag);

	      while (CONS_P (values) && STRING_P (CAR (values)))
		{
		  symbol_add_value (sym, STRING_VALUE (CAR (values)));
		  values = CDR (values);
		}
	    }
	}
    }
  gc_wisp_objects ();
  return (package);
}

/* Convert PACKAGE to an ASCII readable string -- an alist representing
   the contents of PACKAGE.  If STRIP is non-zero, the package name prefix
   is not prepended to each variable name in the alist, otherwise, the
   package name appears before each variable.  If PACKAGE is anonymous,
   no package name is associated with the variables. */
char *
package_to_alist (Package *package, int strip)
{
  char *result = (char *)NULL;
  Symbol **symbols = symbols_of_package (package);

  if (symbols != (Symbol **)NULL)
    {
      register int i;
      BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
      char *packname = package->name;
      Symbol *sym;

      bprintf (buffer, "(");

      for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	{
	  static char *fullname = (char *)NULL;
	  static int fn_size = 0;
	  int name_len = package->name_len + sym->name_len + 3;
	  char *item_name;

	  if (name_len >= fn_size)
	    fullname = (char *)xrealloc (fullname, (fn_size = name_len + 20));

	  if (package->name_len && !strip)
	    sprintf (fullname, "%s::%s", packname, sym->name);
	  else
	    strcpy (fullname, sym->name);

	  item_name = strdup (wisp_readable (fullname));

	  switch (sym->values_index)
	    {
	    case 0:
	      bprintf (buffer, "(%s)", item_name);
	      break;

	    case 1:
	      bprintf (buffer, "(%s . %s)",
		       item_name, wisp_readable (sym->values[0]));
	      break;

	    default:
	      {
		register int j;

		bprintf (buffer, "(%s", item_name);
		for (j = 0; j < sym->values_index; j++)
		  bprintf (buffer, " %s", wisp_readable (sym->values[j]));
		bprintf (buffer, ")");
	      }
	    }
	  free (item_name);
	}

      free (symbols);
      bprintf (buffer, ")");
      result = buffer->buffer;
      free (buffer);
    }
  return (result);
}

char *
mhtml_make_identifier (char *alphabet, int limit)
{
  register int i;
  int alphabet_length;
  static char *default_alphabet = "ABCDEFGHJK23456789MNPQRSTYVWXYZ98765432";
  static int randomize_called = 0;
  char *identifier = (char *)NULL;

  if (empty_string_p (alphabet))
    {
      alphabet = pagefunc_get_variable ("MI::ALPHABET");

      if (empty_string_p (alphabet))
	alphabet = default_alphabet;
    }

  alphabet_length = strlen (alphabet);

  if (!limit) limit = 16;

  identifier = (char *)xmalloc (1 + limit);

  if (!randomize_called)
    {
      srandom ((unsigned int)getpid ());
      randomize_called++;
    }

  for (i = 0; i < limit; i++)
    {
      int offset = (random () % alphabet_length);
      identifier[i] = alphabet[offset];
    }

  identifier[i] = '\0';
  return (identifier);
}

static int parser_trace_indentation_counter = 0;

static char *
parser_trace_indentation (void)
{
  static char *blanks = (char *)NULL;
  static int blanks_size = 0;
  int indent_amount = 2 * parser_trace_indentation_counter;

  if (!indent_amount)
    return ("");

  if ((indent_amount + 2) > blanks_size)
    {
      register int i;

      blanks = (char *)xrealloc
	(blanks, (blanks_size += (indent_amount + 50)));
      for (i = 0; i < blanks_size - 1; i++)
	blanks[i] = ' ';
      blanks[i] = '\0';
    }

  /* Return the end of the string. */
  return (blanks + (blanks_size - indent_amount));
}

/* #define METAHTML_READER_MACROS 1 */
/* This can't work unless we rewrite read_sexp_1 from scratch.  In
   addition, it isn't the right thing for a passive-parsing language.
   I've left the code here to discourage other people from asking for
   it as a `feature'. */
#undef METAHTML_READER_MACROS
#if defined (METAHTML_READER_MACROS)
static int
reader_macro (PAGE *page, int point)
{
  int result = 0;

  if (page->buffer[point] == '$')
    {
      /* Surround the following symbol with <get-var ...>. */
      register int i;
      register int found = 0;

      for (i = point + 1; ((i < page->bindex) && ((point - i) < 50)); i++)
	{
	  register unsigned char c = (unsigned char)page->buffer[i];

	  if ((c > 'z') || (c < 'A') || ((c > 'Z') && (c < 'a')))
	    {
	      found++;
	      break;
	    }
	}

      if (found && ((i - point) != 1))
	{
	  bprintf_delete_range (page, point, point + 1);
	  i--;
	  bprintf_insert (page, i, ">");
	  bprintf_insert (page, point, "<get-var ");
	  result = 1;
	}
    }

  return (result);
}
#endif

/* A little help for the debugger. */
int debugging_with_mdb = 0;

char **pushed_function_names = (char **)NULL;
int pushed_function_names_index = 0;
int pushed_function_names_slots = 0;
static void
push_function_name (char *name)
{
  if (pushed_function_names_index + 2 > pushed_function_names_slots)
    pushed_function_names = (char **)xrealloc
      (pushed_function_names, (pushed_function_names_slots += 10)
       * sizeof (char *));

  pushed_function_names[pushed_function_names_index++] = name;
  pushed_function_names[pushed_function_names_index] = (char *)NULL;
}

static void
pop_function_name (void)
{
  /* Why the test?  Because a user can turn on pushing and popping in the
     middle of a function.  We don't want Meta-HTML to crash in that way. */
  if (pushed_function_names_index)
    {
      pushed_function_names_index--;
      pushed_function_names[pushed_function_names_index] = (char *)NULL;
    }
}

char **
mdb_pushed_function_names (void)
{
  return (pushed_function_names);
}

void
mdb_reset_pushed_functions (void)
{
  if (pushed_function_names)
    {
      pushed_function_names_index = 0;
      pushed_function_names[0] = (char *)NULL;
    }
}

static int total_advanced = 0;
static int total_not_advanced = 0;

/* Actually process PAGE in place.  The result of processing PAGE is placed
   within PAGE.  This is likely to change shortly, when we pre-parse the
   PAGE and write sequential output to a different destination. */
void
page_process_page_internal (PAGE *page)
{
  register int i, c;
  int search_start = 0, search_start_modified = -1;
  int done = 0;
  int semicolon_comments;
  static char *fname = (char *)NULL;
  static int fname_size = 0;

  if (page == (PAGE *)NULL)
    return;

  while (!done)
    {
      PFunDesc *desc = (PFunDesc *)NULL;
      static PFunDesc uf_desc;
      UserFunction *uf = (UserFunction *)NULL;

      semicolon_comments = !mhtml_inhibit_comment_parsing;

      if (search_start_modified != -1)
	{
	  search_start = search_start_modified;
	  search_start_modified = -1;
	}

      if (mhtml_parser_interrupted)
	{
	  if (mhtml_parser_callback_function)
	    {
	      mhtml_parser_interrupted = 0;
	      (*mhtml_parser_callback_function) ();
	    }
	  else
	    return;
	}

      for (i = search_start; i < page->bindex; i++)
	{
#undef METAHTML_READER_MACROS /* See comment at reader_macro () */
#if defined METAHTML_READER_MACROS
	  if (reader_macro (page, i))
	    {
	      /* reader_macro () has already expanded the read macro.
		 Just read again. */
	      i--;
	      continue;
	    }
#endif
	  if ((page->buffer[i] == LEFT_BRACKET) || (page->buffer[i] == '<'))
	    break;

	  if (page->buffer[i] == '\n')
	    parser_current_lineno++;

	  /* If there is a semicolon comment here, ignore it now. */
	  if (semicolon_comments && page->buffer[i] == ';')
	    {
	      if (((i + 2) < page->bindex) &&
		  (page->buffer[i + 1] == ';') &&
		  (page->buffer[i + 2] == ';'))
		{
		  int marker = i;

		get_another_comment:
		  i += 3;
		  while (i < page->bindex &&
			 (!return_sequence (page->buffer[i],
					    page->buffer[i + 1])))
		    i++;

		  /* Handle CR/LF. */
		  if (page->buffer[i] == '\r') i++;

		  parser_current_lineno++;

		  /* If the very next item is another semi-colon comment,
		     delete it in one chunk with the current one. */
		  if (((i + 3) < page->bindex) &&
		      (page->buffer[i + 1] == ';') &&
		      (page->buffer[i + 2] == ';') &&
		      (page->buffer[i + 3] == ';'))
		    {
		      i++;
		      goto get_another_comment;
		    }

		  bprintf_delete_range (page, marker, i + 1);
		  i = marker - 1;
		}
	    }
	}

      if (i >= page->bindex)
	{
	  done = 1;
	  continue;
	}
      else
	{
	  Symbol *sym;
	  int fname_beg;
	  int fname_end;
	  int fname_len;
	  char openers_opener = page->buffer[i];

	  search_start = i;
	  fname_beg = ++i;

	  for (; (c = page->buffer[i]) != '\0'; i++)
	    if ((c == ' ') || (c == '>') || (c == RIGHT_BRACKET) ||
		(c == '\t') || (c == '\r') || (c == '\n'))
	      break;

	  if (!c)
	    {
	      search_start++;
	      continue;
	    }

	  fname_end = i;
	  fname_len = fname_end - fname_beg;

	  if (fname_len + 4 > fname_size)
	    fname = (char *)xrealloc (fname, fname_size += (20 + fname_len));

	  strncpy (fname, page->buffer + fname_beg, fname_len);
	  fname[fname_len] = '\0';

	  /* Look for a user-defined command before a static one. */
	  sym = mhtml_find_user_function_symbol (fname);
	  if (sym && (uf = (UserFunction *) sym->values))
	    {
	      desc = &uf_desc;
	      desc->tag = uf->name;
	      if (uf->type == user_MACRO)
		{
		  if (uf->flags & user_WEAK_MACRO)
		    desc->complexp = -1;
		  else
		    desc->complexp = 1;
		}
	      else
		desc->complexp = 0;
	      desc->debug_level = uf->debug_level;
	      desc->fun = (PFunHandler *)NULL;
	    }

	  /* Find the description of this function, so we know how to find
	     it in the page. */
	  if (!desc)
	    desc = pagefunc_get_descriptor (fname);

	  if (!desc)
	    {
	      search_start++;
	      continue;
	    }
	  else
	    {
	      int start, end;
	      int found, complexp = desc->complexp;

	      start = search_start;

	      if (desc->complexp)
		{
		  int saved = start;
		  found = page_complex_tag_bounds (page, fname, &start, &end);

		  /* If a closer wasn't found, and this is a weak macro,
		     then treat this occurence as if the tag didn't
		     require a closer. */
		  if (!found && (desc->complexp == -1))
		    {
		      start = saved;
		      end = page_find_tag_end (page, start);
		      found = end != -1;
		      complexp = 0;
		    }
		}
	      else
		{
		  end = page_find_tag_end (page, start);
		  found = end != -1;
		}

	      if (!found)
		{
		  /* The MTHML programmer didn't close the opener correctly.
		     Tell them, and then ignore the text. */
		  char *thisfile = pagefunc_get_variable
		    ("*parser*::filename-pdl[0]");
		  page_debug ("%s: Closing tag missing for %c%s ...%c",
			      thisfile ? thisfile : "???",
			      LEFT_BRACKET, desc->tag, RIGHT_BRACKET);

		  if (mhtml_verbose_error_reporting)
		    {
		      int pindex;
		      char partial[100];

		      for (pindex = 0;
			   (pindex < 99) && (start + pindex < page->bindex);
			   pindex++)
			partial[pindex] = page->buffer[start + pindex];

		      partial[pindex] = '\0';

		      page_debug
			("[Text Following Missing Tag:\n%s\nEnd of Text]",
			 partial);
		    }

		  if (syntax_checking)
		    {
		      syntax_failure = 1;
		      done = 1;
		    }
		  search_start += fname_len;
		  continue;
		}
	      else
		{
		  char *open_body = (char *)NULL;
		  char *strbody = (char *)NULL;
		  Package *vars = (Package *)NULL;
		  int open_start, open_end, open_body_len;

		  /* For simple and complex tags alike, we want to eat the
		     variables which appear in the opener. */
		  open_start = start;
		  if (complexp)
		    open_end = page_find_tag_end (page, start);
		  else
		    open_end = end;

		  open_body_len = open_end - open_start;
		  open_body = (char *)xmalloc (1 + open_body_len);
		  strncpy (open_body, page->buffer + start, open_body_len);
		  open_body[open_body_len] = '\0';

		  /* Kill the closing '>'. */
		  open_body[open_body_len - 1] = '\0';
		  memmove (open_body, open_body + 1 + fname_len,
			   (open_body_len - (1 + fname_len)));

		  {
		    int allow_keywords = 1;

		    if (uf)
		      allow_keywords = (uf->flags & user_ACCEPT_KEYWORDS);
#if 0
		    else
		      allow_keywords = (desc->flags & user_ACCEPT_KEYWORDS);
#endif
		    vars = pagefunc_snarf_vars (open_body, allow_keywords);
		  }

		  if (!complexp)
		    {
		      if (!desc->complexp)
			strbody = open_body;
		      else
			strbody = strdup ("");
		    }
		  else
		    {
		      int open_len = open_end - open_start;
		      int body_len, body_end;
		      char *closer = (char *)xmalloc (3 + fname_len);

		      closer[0] = openers_opener;

		      closer[1] = '/';
		      /* We'd like to copy the desc->tag, but if this is
			 a user-function that has been copy-var'd, they
			 may not be the same.  So use fname instead. */
#if 0
		      strcpy (closer + 2, desc->tag);
#else
		      strcpy (closer + 2, fname);
#endif

		      strbody = page_complex_tag_extract
			(page, fname, &open_start, &end);

		      /* Get rid of the opening tag. */
		      {
			int extra = 0;
			if ((strbody[open_len] == '\r') &&
			    (strbody[1 + open_len] == '\n'))
			  extra += 2;
			else if (strbody[open_len] == '\n')
			  extra++;

			memmove (strbody, strbody + open_len + extra,
				 (1 + strlen (strbody) - (open_len + extra)));
		      }

		      /* Get rid of the closing tag. */
		      body_len = strlen (strbody);
		      body_end = body_len - (1 + fname_len);

		      while (strncasecmp
			     (strbody + body_end, closer, 1 + fname_len) != 0)
			body_end--;

		      if (body_end > 0 && strbody[body_end - 1] == '\n')
			body_end--;

		      strbody[body_end] = '\0';
		      free (closer);
		    }

		  /* Call the handler function. */
		  if (syntax_checking)
		    {
		      search_start = end;
		    }
		  else
		    {
		      char *display_body = (char *)NULL;
		      PAGE *body = page_create_page ();

		      page_set_contents (body, strbody);

		      /* This text is no longer in the page. */
#if defined (BREAK_SEMANTICS)
		      if (page->buffer[end + 1] == '\n')
			bprintf_delete_range (page, start, end + 1);
		      else
#endif
			bprintf_delete_range (page, start, end);

		      if (mhtml_parser_trace_mode || desc->debug_level > 5)
			{
			  display_body = strdup (open_body ? open_body : "");
			  if ((!mhtml_verbose_error_reporting) &&
			      (strlen (display_body) > 79))
			    strcpy (display_body + 76, "...");

			  if (mhtml_parser_trace_mode)
			    page_debug ("%sEntering %s: %c%s %s%c",
					parser_trace_indentation (),
					desc->tag,
					LEFT_BRACKET, desc->tag,
					mhtml_funargs (vars),
					RIGHT_BRACKET);
			  else
			    page_debug ("Entering %c%s %s%c",
					LEFT_BRACKET, desc->tag,
					display_body, RIGHT_BRACKET);
			}

		      parser_trace_indentation_counter++;
		      if (debugging_with_mdb || mhtml_remember_function_calls)
			push_function_name (desc->tag);
		      {
			int debug_continue = 1;

#if defined (METAHTML_PROFILER)
			struct timeval start_time = { 0, 0 };
			struct timeval end_time = { 0, 0 };

			if (mhtml_profiling_functions)
			  gettimeofday (&start_time, (struct timezone *)NULL);
#endif
			page_push_page (page, start, &search_start_modified);

			if ((desc->debug_level < 0) &&
			    (mhtml_debug_callback_function !=
			     (DEBUGGER_CALLBACK_FUNCTION *)NULL))
			  {
			    debug_continue =
			      (*mhtml_debug_callback_function)
			      (page, body, vars, start, end,
			       &search_start, 0, FNAME_COMMA
			       desc, uf, open_body);
			  }
			if (debug_continue)
			  {
			    if (uf)
			      mhtml_execute_function
				(sym, uf, page, body, vars, start, end,
				 &search_start, desc->debug_level,
				 FNAME_COMMA open_body);
			    else
			      (*desc->fun)
				(page, body, vars, start, end, 
				 &search_start, desc->debug_level
				 COMMA_FNAME);
			  }
			page_pop_page ();
#if defined (METAHTML_PROFILER)
			if (mhtml_profiling_functions &&
			    ((start_time.tv_usec != 0) &&
			     (start_time.tv_sec != 0)))
			  {
			    double usecs_start, usecs_end, elapsed;
			    gettimeofday (&end_time, (struct timezone *)NULL);
			    usecs_start = (double)((start_time.tv_sec * 1.0e6)
						   + start_time.tv_usec);
			    usecs_end = (double)((end_time.tv_sec * 1.0e6)
						 + end_time.tv_usec);
			    elapsed = usecs_end - usecs_start;

			    if (uf)
			      {
				if (!uf->profile_info)
				  {
				    uf->profile_info = (PROFILE_INFO *)
				      xmalloc (sizeof (PROFILE_INFO));
				    uf->profile_info->usecs_spent = 0.0;
				    uf->profile_info->times_called = 0;
				  }
				uf->profile_info->times_called++;
				uf->profile_info->usecs_spent += elapsed;
			      }
			    else
			      {
				if (!desc->profile_info)
				  {
				    desc->profile_info = (PROFILE_INFO *)
				      xmalloc (sizeof (PROFILE_INFO));
				    desc->profile_info->usecs_spent = 0.0;
				    desc->profile_info->times_called = 0;
				  }
				desc->profile_info->times_called++;
				desc->profile_info->usecs_spent += elapsed;
			      }
			  }
#endif /* METAHTML_PROFILER */
		      }

		      if (search_start == start)
			total_not_advanced++;
		      else
			total_advanced++;

		      if (search_start < 0)
			{
			  page_debug ("PPI: `%s' bashed SEARCH_START!",
				      desc->tag);
			  search_start = page->bindex;
			}

		      parser_trace_indentation_counter--;

		      if (mhtml_parser_trace_mode || desc->debug_level > 5)
			{
			  if (mhtml_parser_trace_mode)
			    page_debug ("%sLeaving %s: %c%s %s%c",
					parser_trace_indentation (),
					desc->tag,
					LEFT_BRACKET, desc->tag,
					mhtml_funargs (vars),
					RIGHT_BRACKET);
			  else
			    page_debug ("Leaving %c%s %s%c",
					LEFT_BRACKET,
					desc->tag, display_body,
					RIGHT_BRACKET);
			  free (display_body);
			}
		      if (debugging_with_mdb || mhtml_remember_function_calls)
			pop_function_name ();

		      page_free_page (body);
		    }

		  /* Free up the variables and the body. */
		  if (strbody != open_body) free (open_body);
		  symbol_destroy_package (vars);
		  free (strbody);
		}
	    }
	}
    }
}

/* Evaluate the string BODY in the current environment, returning the results
   as a newly consed string, or NULL if BODY was NULL. */

static char *
mhtml_evaluate_string_1 (char *body, int top_level_p)
{
  PAGE *evaluated;
  char *result = (char *)NULL;
  int clear_whitespace_p = 0;

  if (!body)
    return ((char *)NULL);

  evaluated = page_create_page ();
  page_set_contents (evaluated, body);

  {
    int lineno = parser_current_lineno;

    if (top_level_p)
      page_process_page (evaluated);
    else
      page_process_page_internal (evaluated);

    parser_current_lineno = lineno;
  }

  result = evaluated->buffer;
  free (evaluated);

  /* Strip leading and trailing whitespace from the string.  Yes? */
  if (clear_whitespace_p && (result != (char *)NULL))
    {
      register int i;
      char *temp = result;

      /* Strip leading. */
      while (whitespace (*temp)) temp++;

      if (temp != result)
	memmove (result, temp, 1 + strlen (temp));

      /* Strip trailing. */
      for (i = strlen (result) - 1; i > -1 && whitespace (result[i]); i--);
      if (i > -1)
	{
	  i++;
	  result[i] = '\0';
	}

      /* If there was nothing but whitespace, return the NULL string. */
      if (*result == '\0')
	{
	  free (result);
	  result = (char *)NULL;
	}
    }

  return (result);
}

char *
mhtml_top_level_eval (char *body)
{
  return (mhtml_evaluate_string_1 (body, 1));
}

char *
mhtml_evaluate_string (char *body)
{
  return (mhtml_evaluate_string_1 (body, 0));
}

extern Symbol *
mhtml_find_user_function_symbol (char *name)
{
  Symbol *sym = symbol_lookup_in_package (mhtml_user_keywords, name);

  return (sym && sym->type == symtype_USERFUN)
    ? sym
    : (Symbol *) NULL;
}

/* Return a pointer to the UserFunction structure describing the user level
   function named by NAME, or NULL if no such function exists. */
UserFunction *
mhtml_find_user_function (char *name)
{
  Symbol *sym = mhtml_find_user_function_symbol (name);

  return (UserFunction *) (sym ? sym->values : NULL);
}


#if defined (METAHTML_COMPILER)
extern Symbol *
mhtml_find_prim_function_symbol (char *name)
{
  return pagefunc_get_descriptor (name)
    ? symbol_lookup_in_package (mhtml_function_package, name)
    : (Symbol *) NULL;
}
#endif

#define REDEFINED_PRIMITIVE 0x01
#define REDEFINED_UFUNCTION 0x02

/* Given that we are about to define a new function, check to see if
   the definition would overwrite an existing definition.  We only
   perform this check if the user has set one of the variables
   mhtml::warn-on-redefine-primitive or mhtml::warn-on-redefine. */
void
mhtml_maybe_warn_redefine (char *name, int type)
{
  if (mhtml_warn_on_redefine_primitive || mhtml_warn_on_redefine)
    {
      int redefined = 0;
      PFunDesc *prim = pagefunc_get_descriptor (name);

      if (prim != (PFunDesc *)NULL)
	redefined = REDEFINED_PRIMITIVE;

      if (mhtml_warn_on_redefine)
	{
	  UserFunction *ufun = mhtml_find_user_function (name);

	  if (ufun != (UserFunction *)NULL)
	    redefined |= REDEFINED_UFUNCTION;
	}

      if (redefined)
	{
	  char *definer = "primitive?";

	  switch (type)
	    {
	    case user_DEFUN: definer = "defun"; break;
	    case user_SUBST: definer = "defsubst"; break;
	    case user_MACRO: definer = "defmacro"; break;
	    }

	  if (redefined & REDEFINED_PRIMITIVE)
	    page_syserr ("WARNING: <%s %s...> redefines a primitive function",
			 definer, name);

	  if (redefined & REDEFINED_UFUNCTION)
	    page_syserr ("WARNING: <%s %s...> redefines a user function",
			 definer, name);
	}
    }
}

/* Add or replace a function of TYPE with NAME, BODY in the
   *user-functions* package. The definition is modified by variable
   names and values specified in the package passed in VARS. */
void
mhtml_add_user_function (int type, char *name, char *body, Package *vars)
{
  UserFunction *uf = mhtml_find_user_function (name);
  char *body_whitespace = get_value (vars, "whitespace");
  char *dont_compile = get_value (vars, "nocompile");
  char *debug_level = get_value (vars, "debug");
  char *wrapper_packname = mhtml_evaluate_string (get_value (vars, "package"));
  char **named_parameters = (char **)NULL;
  int np_size = 0;
  int np_index = 0;

  /* If this is a redefinition of a primitive, and we have those warnings
     turned on, then produce an error message in system-error-output. */
  if (mhtml_warn_on_redefine_primitive || mhtml_warn_on_redefine)
    mhtml_maybe_warn_redefine (name, type);

  if (type == user_DEFUN)
    {
      if ((!empty_string_p (body_whitespace)) &&
	  (strcasecmp (body_whitespace, "keep") == 0))
	body_whitespace = (char *)NULL;
      else
	body_whitespace = "delete";

      if (!wrapper_packname)
	wrapper_packname = strdup ("local");
    }

  /* Gather named parameters if present. */
  {
    register int i;
    char *param;

    for (i = 1; (param = get_positional_arg (vars, i)) != (char *)NULL; i++)
      {
	if (np_index + 2 > np_size)
	  named_parameters = (char **) xrealloc
	  (named_parameters, (np_size += 10) * sizeof (char *));

	named_parameters[np_index++] = strdup (param);
	named_parameters[np_index] = (char *)NULL;
      }
  }

  if (empty_string_p (wrapper_packname))
    {
      if (wrapper_packname) free (wrapper_packname);
      wrapper_packname = (char *)NULL;
    }

  if (uf == (UserFunction *)NULL)
    {
      Symbol *sym;

      uf = (UserFunction *)xmalloc (sizeof (UserFunction));
      memset (uf, 0, sizeof (UserFunction));
      uf->type = type;
      uf->debug_level = debug_level ? atoi (debug_level) : 0;
      uf->name = strdup (name);
      uf->packname = wrapper_packname;
      uf->named_parameters = named_parameters;
      uf->body = strdup (body ? body : "");

      if (mhtml_user_keywords == (Package *)NULL)
	mhtml_user_keywords =
	  symbol_get_package_hash ("*user-functions*", 577);
      sym = symbol_intern_in_package (mhtml_user_keywords, name);
      sym->values = (char **)uf;
      sym->type = symtype_USERFUN;
    }
  else
    {
      Symbol *sym;

      sym = mhtml_find_user_function_symbol (name);

      if (sym != (Symbol *)NULL)
	{
	  symbol_set_modified (sym);

	  /* If this definition is being redefined, it is enough for us to
	     know that -- for purposes of reasonablity, we mark it as flagged
	     if we are already marking new symbols.  See write_symbol_package
	     and friends. */
	  if (mhtml_flag_newly_interned_symbols != 0)
	    sym->flags |= sym_FLAGGED;
	}

      uf->type = type;
      xfree (uf->packname);

      if (mhtml_gather_documentation && uf->documentation)
	{
	  register int i;

	  for (i = 0; uf->documentation[i] != (char *)NULL; i++)
	    free (uf->documentation[i]);

	  free (uf->documentation);
	  uf->documentation = (char **)NULL;
	}

      uf->packname = wrapper_packname;
      if (uf->named_parameters)
	{
	  register int i;

	  for (i = 0; uf->named_parameters[i] != (char *)NULL; i++)
	    free (uf->named_parameters[i]);
	  free (uf->named_parameters);
	}

      uf->named_parameters = named_parameters;

      free (uf->body);
      uf->body = strdup (body ? body : "");
    }

  uf->flags = 0;

  /* Can the caller supply keyword parameters for this function? */
  if (var_present_p (vars, "&key"))
    uf->flags |= user_ACCEPT_KEYWORDS;

  if (!empty_string_p (dont_compile))
    uf->flags |= user_DONT_COMPILE;

  /* Strip comments now, rather than later. */
  {
    register int i = 0;
    char *b = uf->body;
    int passed_documentation = 1;
    char **docstrings = (char **)NULL;
    int docstrings_index = 0;
    int docstrings_size = 0;

    /* Perhaps remember leading comments as documentation. */
    if (mhtml_gather_documentation)
      passed_documentation = 0;

    while (b[i] != '\0')
      {
	if ((b[i] == ';') && (b[i + 1] == ';') && (b[i + 2] == ';'))
	  {
	    register int j;
	    
	    for (j = i; (b[j] != '\0') && (b[j] != '\n'); j++);

	    if (!passed_documentation)
	      {
		int start = i + 3, len = j - start;

		if (docstrings_index + 2 > docstrings_size)
		  docstrings = (char **)xrealloc
		    (docstrings, (docstrings_size += 8) * sizeof (char *));

		docstrings[docstrings_index] = (char *)xmalloc (1 + len);
		strncpy (docstrings[docstrings_index], b + start, len);
		docstrings[docstrings_index][len] = '\0';
		docstrings_index++;
		docstrings[docstrings_index] = (char *)NULL;
	      }

	    memmove (b + i, b + j, strlen (b + j) + 1);
	  }
	else
	  {
	    if (!passed_documentation && !whitespace (b[i]))
	      passed_documentation++;

	    i++;
	  }
      }

    uf->documentation = docstrings;
  }
     
  /* If the user wants special behaviour for the whitespace present in
     the macro body, then handle it now. */
  if (body_whitespace != (char *)NULL)
    {
      char *b = uf->body;

      if (strcasecmp (body_whitespace, "delete") == 0)
	{
	  register int i, c, l, start;
	  int brace_level = 0;
	  int quote_level = 0;

	  uf->flags |= user_WHITESPACE_DELETED;

	  l = strlen (b);

	  /* Delete all occurences of whitespace outside of 
	     `< ...>' and `" ... "'. */
	  i = 0;
	  while ((c = b[i]) != '\0')
	    {
	      if (c == LEFT_BRACKET) c = '<';
	      if (c == RIGHT_BRACKET) c = '>';

	      switch (c)
		{
		case '"':
		  quote_level = !quote_level;
		  break;

		case '<':
		  if (!quote_level)
		    brace_level++;
		  break;

		case '>':
		  if (!quote_level)
		    brace_level--;
		  break;

		case '\\':
		  if (b[i + 1])
		    i++;
		  break;

		  /* Handle comments. */
		case ';':
		  if (!quote_level && !mhtml_inhibit_comment_parsing &&
		      ((i + 2) < l) && (b[i + 1] == ';' && b[i + 2] == ';'))
		    {
		      start = i;
		      while (b[i] && b[i] != '\n') i++;
		      memmove (b + start, b + i, 1 + strlen (b + i));
		      i = start - 1;
		    }
		  break;

		case '\r':
		  if ((b[i + 1] == '\n') || (b[i + 1] == '\0'))
		    {
		      if (!quote_level && !brace_level)
			{
			  start = i;
			  while (whitespace (b[i])) i++;
			  memmove (b + start, b + i, 1 + strlen (b + i));
			  i = start - 1;
			}
		    }
		  break;

		case ' ':
		case '\t':
		case '\n':
		  {
		    int at_eol_p = 0;

#if 0
		    if ((!brace_level && !quote_level) && (c != '\n'))
		      {
			register int j = i;
			while (whitespace (b[j]) &&
			       ((b[j] != '\n') && (b[j] != '\0'))) j++;
			if ((b[j] == '\n') || (b[j] == '\0'))
			  at_eol_p = 1;
		      }
#endif

		    if (((c == ' ' || c == '\t') && ((i == 0) || at_eol_p)) ||
			(c == '\n' && !quote_level && !brace_level))
		      {
			start = i;
			while (whitespace (b[i])) i++;

#if 0
			/* Maybe move the start backwards. */
			if ((i != 0) && (c == '\n'))
			  {
			    while (whitespace (b[start])) start--;
			    start++;
			  }
#endif
			memmove (b + start, b + i, 1 + strlen (b + i));
			i = start - 1;
		      }
		  }
		  break;
		}
	      i++;
	    }
	}
    }
}

char *
mhtml_recompose_function (Symbol *sym)
{
  char *result = (char *)NULL;
  UserFunction *uf = (UserFunction *)NULL;

  if ((sym != (Symbol *)NULL) && (sym->type == symtype_USERFUN))
    uf = (UserFunction *)sym->values;

  if (uf != (UserFunction *)NULL)
    {
      char *type_string = "???";
      BPRINTF_BUFFER *def = bprintf_create_buffer ();

      switch (uf->type)
	{
	case user_MACRO:
	  {
	    if (uf->flags & user_WEAK_MACRO)
	      type_string = "defweakmacro";
	    else
	      type_string = "defmacro";
	  }
	  break;

	case user_SUBST:
	  type_string = "defsubst";
	  break;

	case user_DEFUN:
	  type_string = "defun";
	  break;

	default:
	  type_string = "defprim";
	}

      bprintf (def, "<%s %s", type_string, sym->name);

      if (uf->named_parameters)
	{
	  register int i;

	  for (i = 0; uf->named_parameters[i] != (char *)NULL; i++)
	    bprintf (def, " %s", uf->named_parameters[i]);
	}

      if (uf->packname)
	{
	  if ((strcasecmp (uf->packname, "local") != 0) ||
	      (uf->type != user_DEFUN))
	    bprintf (def, " package=%s", uf->packname);
	}

      if (uf->debug_level != 0)
	bprintf (def, " debug=%d", uf->debug_level);

      if (uf->flags & user_WHITESPACE_DELETED)
	bprintf (def, " whitespace=delete");

      if (uf->flags & user_WHITESPACE_KEPT)
	bprintf (def, " whitespace=keep");

      if (uf->flags & user_DONT_COMPILE)
	bprintf (def, " nocompile=true");

      bprintf (def, ">\n");

      bprintf (def, "%s", uf->body ? uf->body : "");
      if (!empty_string_p (uf->body))
	bprintf (def, "\n");
      bprintf (def, "</%s>", type_string);
      result = def->buffer;
      free (def);
    }
  return (result);
}

char *
quote_for_setvar (char *value)
{
  register int j = 0, k = 1;
  static char *setval = (char *)NULL;
  static int setval_size = 0;
  int max_space_needed = (2 * strlen (value)) + 4;

  if (max_space_needed > setval_size)
    setval = (char *)xrealloc (setval, (setval_size = max_space_needed));

  setval[0] = '"';

  if (value != (char *)NULL)
    for (j = 0, k = 1; value[j] != '\0'; j++)
      {
	if ((value[j] == '"') || (value[j] == '\\'))
	  setval[k++] = '\\';
	
	setval[k++] = value[j];
      }

  setval[k++] = '"';
  setval[k] = '\0';

  return (setval);
}

void
mhtml_let (PFunArgs)
{
  register int i = 0;
  Package *saved_vars = symbol_get_package ((char *)NULL);
  Package *pack = CurrentPackage;
  char *packname;
  int jump_again = 0;
  Symbol **symbols = symbols_of_package (vars);

  packname = pack && pack->name ? strdup (pack->name) : (char *)NULL;

  /* Save the current variables in our local package, and bind those
     variables to their new values. */
  for (i = 0; ((symbols != (Symbol **)NULL) &&
	       (symbols[i] != (Symbol *)NULL)); i++)
    {
      Symbol *from = symbols[i];
      Symbol *sym = symbol_intern_in_package (pack, from->name);

      symbol_copy (sym, saved_vars);
      if (from != sym)
	symbol_copy (from, pack);
    }

  if (debug_level)
    mhtml_set_numeric_variable_in_package (pack, ":debug-level", debug_level);

  /* Execute the body. */
  {
    PageEnv *page_environ;
    PAGE *body_code = page_copy_page (body);

    page_environ = pagefunc_save_environment ();

    if ((jump_again = setjmp (page_jmp_buffer)) == 0)
      page_process_page_internal (body_code);
    pagefunc_restore_environment (page_environ);

    if (body_code != (PAGE *)NULL)
      {
	if (!jump_again && (body_code->buffer != (char *)NULL))
	  {
	    PagePDL *tpage = page_pdl_tos ();
	    if ((tpage != (PagePDL *)NULL) && (tpage->page == page) &&
		(*tpage->search_start_modified != -1))
	      {
		start = *tpage->search_start_modified;
		*tpage->search_start_modified = -1;
	      }

	    bprintf_insert (page, start, "%s", body_code->buffer);
	    *newstart = start + (body_code->bindex);
	  }

	page_free_page (body_code);
      }
  }

  /* If the package had a name before, get the named package again, because
     the body code might have deleted it!  If it had no name, there isn't
     any way that the body code could delete it, so trust that it exists. */
  if (packname)
    {
      pack = symbol_get_package (packname);
      free (packname);
    }

  symbol_copy_package (saved_vars, pack);
  symbol_destroy_package (saved_vars);

  if (jump_again) longjmp (page_jmp_buffer, 1);
}

/* Execute the subst, function or macro described by UF. */
void
mhtml_execute_function (Symbol *us, UserFunction *uf, PFunArgs, char *attr)
{
  PAGE *subber = (PAGE *)NULL;

  if (uf->debug_level == 21)
    {
      /* Magic debugging location.  Set a GDB breakpoint here and use
	 <debugging-on function=21> */
      uf->debug_level = 21;
    }

#if defined (METAHTML_COMPILER)
  if ((us->machine != (void *)NULL) &&
      (!symbol_get_modified (us)) &&
      (mhtml_machine_apply_function_hook != (COMPILER_APPLY_FUNCTION *)NULL))
    {
      /* A Compiled Function Call */
      if (uf->debug_level > 5)
	page_debug ("Machine Function: %s", fname);
      ((*mhtml_machine_apply_function_hook) (us->machine, PassPFunArgs, attr));
      return;
    }
#endif

  if (!empty_string_p (uf->body))
    {
      register int i = 0, j;

      subber = page_create_page ();
      page_set_contents (subber, uf->body);

      /* Process the body. */
      while (i < subber->bindex)
	{
	  for (; (i < subber->bindex) && (subber->buffer[i] != '%'); i++);

	  i++;
	  if (i < subber->bindex)
	    {
	      if (isdigit (subber->buffer[i]))
		{
		  int which = subber->buffer[i] - '0';
		  char *arg = get_positional_arg (vars, which);

		  i--;
		  bprintf_delete_range (subber, i, i + 2);

		  if (!empty_string_p (arg))
		    {
		      bprintf_insert (subber, i, "%s", arg);
		      i += strlen (arg);
		    }
		}
	      else if (subber->buffer[i] == '\\')
		{
		  bprintf_delete_range (subber, i, i + 1);
		  continue;
		}
	      else if (((subber->bindex - i) > 3) &&
		       (strncasecmp (subber->buffer + i, "BODY", 4) == 0))
		{
		  i--;
		  bprintf_delete_range (subber, i, i + 5);

		  if (!empty_string_p (body->buffer))
		    {
		      j = 0;
		      if (uf->type != user_MACRO)
			while (whitespace (body->buffer[j])) j++;

		      bprintf_insert (subber, i, "%s", body->buffer + j);
		      i += strlen (body->buffer + j);
		    }
		}
	      else if (((subber->bindex - i) > 4) &&
		       (strncasecmp (subber->buffer + i, "QBODY", 5) == 0))
		{
		  i--;
		  bprintf_delete_range (subber, i, i + 6);

		  if (!empty_string_p (body->buffer))
		    {
		      char *setval;

		      j = 0;
		      if (uf->type != user_MACRO)
			while (whitespace (body->buffer[j])) j++;

		      setval = quote_for_setvar (body->buffer + j);
		      bprintf_insert (subber, i, "%s", setval);
		      i += strlen (setval);
		    }
		}
	      else if (((subber->bindex - i) > 4) &&
		       (strncasecmp (subber->buffer + i, "XBODY", 5) == 0))
		{
		  i--;
		  bprintf_delete_range (subber, i, i + 6);
		  if (body && body->buffer)
		    {
		      char *evalled = (char *)NULL;

		      j = 0;
		      if (uf->type != user_MACRO)
			while (whitespace (body->buffer[j])) j++;

		      evalled = mhtml_evaluate_string (body->buffer + j);

		      if (evalled != (char *)NULL)
			{
			  bprintf_insert (subber, i, "%s", evalled);
			  i += strlen (evalled);
			  free (evalled);
			}
		    }
		}
	      else if (((subber->bindex - i) > 9) &&
		       (strncasecmp (subber->buffer + i, "ATTRIBUTES", 10)
			== 0))
		{
		  i--;
		  bprintf_delete_range (subber, i, i + 11);

		  if (!empty_string_p (attr))
		    {
		      for (j = 0; whitespace (attr[j]); j++);

		      bprintf_insert (subber, i, "%s", attr + j);
		      i += strlen (attr + j);
		    }
		}
	      else
		i++;
	    }
	}

      /* Done doing textual replacement in BODY.  If there is anything left,
	 then handle named parameters, etc. */
      if (!empty_string_p (subber->buffer))
	{
	  char *packname = uf->packname;
	  Package *param_package = (Package *)NULL;

	  if (uf->named_parameters != (char **)NULL)
	    {
	      int gathering_optional = 0;
	      int gathering_keys = 0;
	      int gathering_body = 0;
	      int unevalled = 0;
	      int arg_offset = 0;

	      if ((packname != (char *)NULL) &&
		  (strcmp (packname, "local") == 0))
		param_package = symbol_get_package ((char *)NULL);
	      else
		param_package = symbol_get_package (packname);

	      for (i = 0; uf->named_parameters[i] != (char *)NULL; i++)
		{
		  char *param = uf->named_parameters[i];
		  char *value = (char *)NULL;

		  /* Handle &optional argument. */
		  if (strcasecmp (param, "&optional") == 0)
		    {
		      gathering_optional++;
		      continue;
		    }

		  /* Handle &key argument. */
		  if (strcasecmp (param, "&key") == 0)
		    {
		      gathering_keys++;
		      continue;
		    }

		  /* Handle &unevalled argument. */
		  if (strcasecmp (param, "&unevalled") == 0)
		    {
		      unevalled++;
		      continue;
		    }

		  /* Handle &body argument. */
		  if (strcasecmp (param, "&body") == 0)
		    {
		      gathering_body++;
		      continue;
		    }

		  /* Handle &attributes argument. */
		  if (strcasecmp (param, "&attributes") == 0)
		    {
		      param = uf->named_parameters[i + 1];

		      if ((param != (char *)NULL) &&
			  (strcasecmp (param, "&unevalled") == 0))
			{
			  /* Arguments should be unevalled? Already are so. */
			  i++;
			  param = uf->named_parameters[i + 1];
			}

		      if (param != (char *)NULL)
			{
			  /* Is this an array symbol? */
			  Symbol *sym;
			  char *tem = param;

			  i++;
			  param = symbol_canonical_name (param);
			  sym = symbol_intern_in_package
			    (param_package, param);

			  if (strcmp (param, tem) != 0)
			    /* The var was specified as an array.
			       Set each parameter as a value in the array. */
			    {
			      int pos = 0;
			      Package *attrvars=pagefunc_snarf_vars (attr, 0);

			      while ((value = get_positional_arg
				      (attrvars, pos++))
				     != (char *)NULL)
				{
				  symbol_add_value (sym, value);
				}
			    }
			  else
			    /* The var is a "normal" variable.  Return
			       all parameters as a string. */
			    {
			      symbol_add_value (sym, attr);
			    }
			  
			  free (param);
			  continue;
			}
		      else
			param = uf->named_parameters[i];
		    }

		  /* Handle &rest argument. */
		  if (strcasecmp (param, "&rest") == 0)
		    {
		      param = uf->named_parameters[i + 1];

		      if ((param != (char *)NULL) &&
			  (strcasecmp (param, "&unevalled") == 0))
			{
			  /* Arguments should be unevalled! */
			  i++;
			  unevalled = 1;
			  param = uf->named_parameters[i + 1];
			}

		      if (param != (char *)NULL)
			{
			  Symbol *sym;
			  char *tem = param;

			  i++;
			  param = symbol_canonical_name (param);
			  sym = symbol_intern_in_package
			    (param_package, param);

			  if (strcmp (param, tem) != 0)
			    {
			      int array_offset = 0;

			      while ((value = get_positional_arg
				      (vars, arg_offset++))
				     != (char *)NULL)
				{
				  char *setval = value;

				  if (!unevalled)
				    {
				      setval = mhtml_evaluate_string (setval);
				      symbol_add_value (sym, setval);
				      free (setval);
				    }
				  else
				    {
				      symbol_add_value (sym, setval);
				    }
				  array_offset++;
				}
			    }
			  else
			    {
			      BPRINTF_BUFFER *gather = bprintf_create_buffer();

			      while ((value = get_positional_arg
				      (vars, arg_offset++))
				     != (char *)NULL)
				bprintf (gather, "%s ", value);

			      if (gather->bindex)
				{
				  char *setval = gather->buffer;

				  /* Get rid of extra space at buffer end. */
				  gather->bindex--;
				  gather->buffer[gather->bindex] = '\0';

				  if (!unevalled)
				    {
				      setval = mhtml_evaluate_string (setval);
				      symbol_add_value (sym, setval);
				      free (setval);
				    }
				  else
				    symbol_add_value (sym, setval);
				}

			      bprintf_free_buffer (gather);
			    }

			  free (param);
			  continue;
			}
		      else
			param = uf->named_parameters[i];
		    }

		  /* Get argument value of the right type. */
		  if (gathering_body)
		    {
		      gathering_body = 0;

		      if (!empty_string_p (body->buffer))
			{
			  j = 0;
			  if (uf->type != user_MACRO)
			    while (whitespace (body->buffer[j])) j++;

			  value = body->buffer + j;
			}
		      else
			value = (char *)NULL;
		    }
		  else if (gathering_keys)
		    {
		      if (vars != (Package *)NULL)
			value = forms_get_tag_value_in_package (vars, param);
		      else
			value = (char *)NULL;
		    }
		  else
		    value = get_positional_arg (vars, arg_offset++);

		  if (value != (char *)NULL)
		    {
		      if (!unevalled)
			{
			  value = mhtml_evaluate_string (value);
			  forms_set_tag_value_in_package
			    (param_package, param, value);
			  free (value);
			}
		      else forms_set_tag_value_in_package
			     (param_package, param, value);
		    }
		  /* Reset unevalled -- it must be specified for each
		     variable which is to be unevalled. */
		  unevalled = 0;
		}
	    }

	  if (packname)
	    {
	      /* Handle "local" package inline. */
	      if (strcmp (packname, "local") == 0)
		{
		  Package *current_package = CurrentPackage;
		  char *savepack = current_package->name ?
		    strdup (current_package->name) : (char *)NULL;
		  Package *local = param_package;
		  char *x;

		  if (param_package == (Package *)NULL)
		    local = symbol_get_package ((char *)NULL);
		  else
		    local = param_package;

		  symbol_push_package (local);

		  /* Allow user level debugging to take place. */
		  if (debug_level)
		    mhtml_set_numeric_variable (":debug-level", debug_level);

		  x = mhtml_evaluate_string (subber->buffer);

		  if ((x != (char *)NULL) && (x[0] != '\0'))
		    {
		      bprintf_insert (page, start, "%s", x);
		      *newstart += strlen (x);
		    }

		  xfree (x);
		  symbol_pop_package ();

		  if (param_package == (Package *)NULL)
		    symbol_destroy_package (local);

		  if (savepack)
		    {
		      current_package = symbol_get_package (savepack);
		      free (savepack);
		    }

		  CurrentPackage = current_package;
		}
	      else
		{
		  Package *letpack = symbol_get_package (packname);
		  Package *savepack = CurrentPackage;
		  char *savename = (char *)NULL;

		  if (savepack->name)
		    savename = strdup (savepack->name);

		  CurrentPackage = letpack;
		  mhtml_let (page, subber, param_package, start, end,
			     newstart, debug_level COMMA_FNAME);

		  if (savename)
		    {
		      CurrentPackage = symbol_get_package (savename);
		      free (savename);
		    }
		  else
		    CurrentPackage = savepack;
		}
	    }
	  else
	    {
	      mhtml_let (page, subber, param_package, start, end,
			 newstart, debug_level COMMA_FNAME);
	    }

	  if (param_package && !param_package->name)
	    symbol_destroy_package (param_package);
	}

      page_free_page (subber);
    }
}

/* Canonicalize the filename INPUT such that it is a complete and
   valid path to a file. */
char *
mhtml_canonicalize_file_name (char *input, char *docroot, char *relpref,
			      char **web_relative_name)
{
  register int i;
  char *result = (char *)NULL;
  static char *workbuff = (char *)NULL;
  static int workbuff_len = 0;
  int docroot_len = docroot ? strlen (docroot) : 0;
  int relpref_len = relpref ? strlen (relpref) : 0;
  int input_len = input ? strlen (input) : 0;
  int maxlen = 10 + input_len + docroot_len + relpref_len;
  static char *saved_webname = (char *)NULL;
  static int saved_webname_len = 0;

  if (input == (char *)NULL)
    return (input);

  if (!docroot) docroot = "";
  if (!relpref) relpref = "";

  if (maxlen >= workbuff_len)
    workbuff = (char *)xrealloc (workbuff, workbuff_len = maxlen + 10);

  /* Ignore leading and trailing whitespace. */
  input = strdup (input);
  for (i = 0; input[i] && whitespace (input[i]); i++);

  if (i != 0)
    memmove (input, input + i, strlen (input + i) + 1);

  for (i = strlen (input) - 1; i > 0 && whitespace (input[i]); i--);
  if (input[i])
    input[i + 1] = '\0';

  /* If not absolute, root this document at RELPREF. */
  if (input[0] != '/')
    sprintf (workbuff, "%s/%s", relpref, input);
  else
    strcpy (workbuff, input);
    
  /* Clean up the work buffer so that "." and ".." disappear. */
  {
    register int last_slash = 0;

    for (i = 0; workbuff[i] != '\0'; i++)
      {
	/* If in eligible spot for "./" or "../" removal, do it now. */
	if ((i == 0) || (workbuff[i] == '/'))
	  {
	    if ((workbuff[i + 1] == '.') && (workbuff[i + 2] == '/'))
	      {
		/* Remove "./". */
		memmove (workbuff + i, workbuff + i + 2,
			 1 + strlen (workbuff + i + 2));
		i--;
	      }
	    else if ((workbuff[i + 1] == '.') &&
		     (workbuff[i + 2] == '.') &&
		     (workbuff[i + 3] == '/'))
	      {
		/* Remove "../" back to previous slash location. */
		memmove (workbuff + last_slash, workbuff + i + 3,
			 1 + strlen (workbuff + i + 3));
		i = last_slash - 1;

		/* Move the last slash back. */
		for (last_slash = i; last_slash > 0; last_slash--)
		  if (workbuff[last_slash] == '/')
		    break;

		if (last_slash < 0) last_slash = 0;
	      }
	    else
	      last_slash = i;
	  }
      }
  }

  /* Save the web relative pathname here.  The reason it is 20 bytes
     longer is that require_search() likes to tack on an extension to
     the web-relative name.  Always make sure there is enough room. */
  if ((strlen (workbuff) + 20) > saved_webname_len)
    saved_webname = (char *)xrealloc
      (saved_webname, (saved_webname_len = 20 + strlen (workbuff)));

  strcpy (saved_webname, workbuff);
  *web_relative_name = saved_webname;

#if defined (HAVE_GETPWNAM)
  /* If username expansion is being allowed, we allow it to work here
     as well. */
  if (workbuff[0] == '/' && workbuff[1] == '~')
    {
      char *homedir = pagefunc_get_variable ("mhtml::~directory");

      if (homedir != (char *)NULL)
	{
	  char *username;
	  struct passwd *entry;

	  for (i = 2; (workbuff[i] != '\0') && (workbuff[i] != '/'); i++);

	  username = (char *)xmalloc (i);
	  strncpy (username, workbuff + 2, i - 2);
	  username[i - 2] = '\0';
	  entry = (struct passwd *)getpwnam (username);
	  free (username);

	  if ((entry != (struct passwd *)NULL) &&
	      (entry->pw_dir != (char *)NULL))
	    {
	      char *temp = strdup (workbuff + i);

	      if ((3 +
		   strlen (entry->pw_dir) +
		   strlen (homedir) +
		   strlen (temp)) >= workbuff_len)
		workbuff = (char *)xrealloc
		  (workbuff, workbuff_len = (3 +
					     strlen (entry->pw_dir) +
					     strlen (homedir) +
					     strlen (temp)));

	      sprintf (workbuff, "%s/%s%s", entry->pw_dir, homedir, temp);
	      free (temp);
	      docroot_len = 0;
	    }
	}
    }
#endif /* HAVE_GETPWNAM */

  /* The semantics of INCLUDE are similar to the semantics of web-space.
     This means that "<include /header.mhtml>" gets `header.mhtml' from
     the root directory, and not from the local directory. */
  if (docroot_len != 0)
    {
      memmove (workbuff + docroot_len, workbuff, 1 + strlen (workbuff));
      memmove (workbuff, docroot, docroot_len);
    }

  result = strdup (workbuff);

#if defined (macintosh) || defined (__CYGWIN32__)
  /* Fix pathname separators. */
  if (result)
    {
      register int i;

#if defined (__CYGWIN32__)
      if ((result[0] != '\0') && (result[1] == ':'))
	memmove (result, result + 2, strlen (result) - 2);
#endif /* __CYGWIN32__ */

      for (i = 0; result[i] != '\0'; i++)
	{
#if defined (macintosh)
	  if (result[i] == '/')
	    result[i] = ':';
#endif /* macintosh */
#if defined (__CYGWIN32__)
	  if (result[i] == '\\')
	    result[i] = '/';
#endif /* __CYGWIN32__ */
	}
    }
#endif /* mac || CYGWIN32 */

  return (result);
}

char *
mhtml_canonicalize_file_name_argument (char *argument)
{
  char *temp = mhtml_evaluate_string (argument);
  char *result = (char *)NULL;
  char *ign;

  if (!empty_string_p (temp))
    {
      if (pagefunc_get_variable ("isp::web-relative-pathnames"))
	{
	  char *incpref = pagefunc_get_variable ("%%::incpref");
	  char *relpref = pagefunc_get_variable ("%%::relpref");

	  if (!incpref)
	    incpref = pagefunc_get_variable ("mhtml::include-prefix");
	  if (!relpref)
	    relpref = pagefunc_get_variable ("mhtml::relative-prefix");

	  result = mhtml_canonicalize_file_name (temp, incpref, relpref, &ign);
	}
      else
	result = temp;
    }

  if (result != temp) xfree (temp);
  return (result);
}

/* Set the debugging level for the function named in SYM to
   be the value of SYM. */
void
mhtml_set_debugging_on (Symbol *sym)
{
  UserFunction *uf = mhtml_find_user_function (sym->name);
  PFunDesc *desc = pagefunc_get_descriptor (sym->name);

  if ((uf != (UserFunction *)NULL)  || (desc != (PFunDesc *)NULL))
    {
      int new_debug_level = 1;

      if (sym->values && sym->values[0])
	new_debug_level = atoi (sym->values[0]);

      if (uf)
	uf->debug_level = new_debug_level;
      else
	desc->debug_level = new_debug_level;
    }
}

/* Deliver a string which looks like the string that might have been
   passed to a function.  PACKAGE is the package returned from
   PAGEFUNC_SNARF_VARS. */
char *
mhtml_funargs (Package *pack)
{
  char **names = get_vars_names (pack);
  char *result = (char *)NULL;

  if (names != (char **)NULL)
    {
      register int i;
      char **values = get_vars_vals (pack);
      BPRINTF_BUFFER *string = bprintf_create_buffer ();

      for (i = 0; names[i] != (char *)NULL; i++)
	{
	  if (i > 0)
	    bprintf (string, " ");

	  if ((values[i] != (char *)NULL) && (values[i][0] != '\0'))
	    {
	      /* Place quotes around this string on the way out. */
	      bprintf (string, "%s=%s", names[i], quote_for_setvar(values[i]));
	    }
	  else
	    {
	      /* Just print the attribute name. */
	      bprintf (string, "%s", names[i]);
	    }
	}

      result = string->buffer;
      free (string);
    }

  return (result);
}

/* Returns non-zero if STRING consists exclusively of all digits.
   A decimal point is NOT a digit. */
int
mhtml_all_digits (char *string)
{
  register int i;
  int result = 0;

  /* Skip leading whitespace. */
  for (i = 0; whitespace (string[i]); i++);

  if (string[i])
    {
      result = 1;

      for (; string[i]; i++)
	if (!isdigit (string[i]))
	  {
	    result = 0;
	    break;
	  }
    }
  return (result);
}

#define DECODE(c) transtab[(int)c]
char *
mhtml_base64decode (char *encoded, int *len)
{
  register int i, count, c;
  int newlines = 0;
  char *decoded;
  int decoded_size = 0;
  static int called_once = 0;
  static unsigned char transtab[256];

  /* If not already called, initialize the translation map. */
  if (!called_once)
    {
      static char lut[64] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9','+','/'
      };
      called_once++;

      for (i = 0; i < 256; i++)
	transtab[i] = 64;

      for (i = 0; i < 64; i++)
	transtab[(int) lut[i]] = (unsigned char) i;
   }

  for (i = 0; (c = transtab[(int)encoded[i]]) != -1; i++)
    if ((encoded[i] != '\n') && (c >= 64))
      break;
    else if (encoded[i] == '\n')
      newlines++;

  count = (i - newlines) - 1;
  decoded_size = ((count + 3) / 4) * 3;

  decoded = (char *)xmalloc (1 + decoded_size);
  if (len != (int *)NULL) *len = decoded_size;

  i = 0;
  while (count > 0)
    {
      decoded[i] = (DECODE (encoded[0]) << 2 | DECODE (encoded[1]) >> 4); i++;
      decoded[i] = (DECODE (encoded[1]) << 4 | DECODE (encoded[2]) >> 2); i++;
      decoded[i] = (DECODE (encoded[2]) << 6 | DECODE (encoded[3])); i++;
      encoded += 4;
      count -= 4;
      while (*encoded == '\n') encoded++;
    }

  i += ++count;
  decoded[i] = '\0';

  return (decoded);
}

static void
output64chunk (int c1, int c2, int c3, int pads, BPRINTF_BUFFER *buffer)
{
  static char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  bprintf (buffer, "%c", basis_64[(c1 >> 2)]);
  bprintf (buffer, "%c", basis_64[((c1 & 0x3) << 4) |
				 ((c2 & 0xF0) >> 4)]);
  if (pads == 2)
    bprintf (buffer, "==");
  else if (pads)
    {
      bprintf (buffer, "%c=", basis_64[((c2 & 0xF) << 2) |
				      ((c3 & 0xC0) >> 6)]);
    }
  else
    {
      bprintf (buffer, "%c", basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >> 6)]);
      bprintf (buffer, "%c", basis_64[c3 & 0x3F]);
    }
}

char *
mhtml_base64encode (char *data, int length, int shortlines)
{
  register int i;
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *result = (char *)NULL;
  int c1, c2, c3, ct=0;

  i = 0;
  while (i < length)
    {
      c1 = data[i++];

      if (i >= length)
	{
	  output64chunk (c1, 0, 0, 2, buffer);
	}
      else
	{
	  c2 = data[i++];

	  if (i >= length)
	    {
	      output64chunk (c1, c2, 0, 1, buffer);
	    }
	  else
	    {
	      c3 = data[i++];
	      output64chunk (c1, c2, c3, 0, buffer);
	    }
	}

      ct += 4;
      
      if ((shortlines != 0) && ((ct % shortlines) == 0))
	bprintf (buffer, "\n");
    }

  result = buffer->buffer;
  free (buffer);
  return (result);
}

int
float_p (char *string)
{
  int result = 0;

  if (!empty_string_p (string))
    {
      char *endptr;
      double value = strtod (string, &endptr);

      /* God, how gauche.  Required for RedHat 6.0, since it happily assumes
	 that "0x20" == "1".  Don't ask me why... */
      if (strncasecmp (string, "0x", 2) == 0)
	result = 0;
      else if (empty_string_p (endptr))
	result = 1;
      else
	/* Don't ask me, ask GCC.   This means that GCC "optimized"
	   away the reference to value (and thus the right hand side
	   of it) when this assignment wasn't here.  Naturally, that
	   meant that strtod () was never called, and endptr was thus
	   never changed.  It doesn't really matter whether or not gcc
	   fixes this soon -- I still have to support the earlier
	   versions of it.  Ugh. */
	value = 0;
    }

  return (result);
}

int
integer_p (char *string, int base)
{
  int result = 0;

  if (!empty_string_p (string))
    {
      char *endptr;
      long value = strtol (string, &endptr, base);

      if ((endptr[0] == '\0') || ((endptr[0] == '.') && (endptr[1] == '\0')))
	result = 1;
      else
	/* Make sure that GCC doesn't optimize away the strtod call. */
	value += 1;
    }
  return (result);
}

int
number_p (char *string)
{
  return ((integer_p (string, 0)) || (float_p (string)));
}

int
url_has_protocol_p (char *url)
{
  int result = 0;

  if (url != (char *)NULL)
    result = ((strncasecmp (url, "http:", 5) == 0) ||
	      (strncasecmp (url, "https:", 6) == 0) ||
	      (strncasecmp (url, "shttp:", 5) == 0) ||
	      (strncasecmp (url, "ftp:", 4) == 0) ||
	      (strncasecmp (url, "mailto:", 7) == 0));
  return (result);
}

char *
url_protocol (char *url)
{
  static char buffer[20];
  char *result = (char *)NULL;

  if (url != (char*)NULL)
    {
      register int i;

      for (i = 0; i < 19; i++)
	{
	  buffer[i] = url[i];
	  if (buffer[i] == ':')
	    break;
	}

      buffer[i] = '\0';

      if (i < 19)
	result = buffer;
    }

  return (buffer);
}

#if defined (__cplusplus)
}
#endif
