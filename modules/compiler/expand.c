/* expand.c: -*- C -*-  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1996, 2000, E. B. Gamble (ebg@metahtml.com).

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

/* Two passes might be needed.  The first to BLOCK complex tags and to
   expand macros; the second to expand into CORE.  The blocking can't
   be done by the parser because the complex tags can't be matched
   without knowning the tag's denotation (that is, if it denotes a
   complex tag).   */

/* Implementation note.  The order of evaluation in MetaHTML needs to be
   enforced.  Many function calls below perform multiple expansions - one
   for each argument.  In the current implementation, the order of
   evaluation is implicit and inherited from C.  This is not the correct
   convention.  */

#include <setjmp.h>
#include "compiler/compile.h"
#include "compiler/corex.h"
#include "compiler/parse.h"

boolean_t mh_comp_verbose_debugging = true;

/* Never mind! */

/* Used to determine if a STRING parse should be expanded with or
   without quotes.  Generally, when in a simple tag, the quotes are
   removed; whereas, when in a complex tag, the quotes are retained. */
static int tag_count = 0;

#define EXP_ARGS_DEF				\
  mh_parse_t   parse,				\
  mh_env_t     env,				\
  mh_parse_t   operator,			\
  mh_tag_t     tag,				\
  mh_parse_t   operands,			\
  unsigned int count,				\
  mh_parse_t   body

#define EXP_ARGS_APP 					\
  parse, env, operator, tag, operands, count, body 

#define DEFINE_EXPANDER( name, expander, documentation )	\
static mh_core_t						\
expander (EXP_ARGS_DEF)

/* Forward Declaration */
static mh_core_t
mh_exp_as_app (mh_parse_t   parse,
	       mh_env_t     env,
	       mh_parse_t   operator,
	       mh_tag_t     tag,
	       mh_parse_t   operands,
	       unsigned int count,
	       mh_parse_t   body);

static inline mh_core_t
mh_tag_expand (mh_parse_t   parse,
	       mh_env_t     env,
	       mh_parse_t   operator, /* MH_PARSE_TYPE_TOKEN */
	       mh_tag_t     tag,
	       mh_parse_t   operands, /* MH_PARSE_TYPE_LIST */
	       unsigned int operands_count,
	       mh_parse_t   body)
{ return (tag->compile.expander ? *tag->compile.expander : *mh_exp_as_app)
    (parse, env, operator, tag, operands, operands_count, body); }

typedef mh_core_t
(* mh_exp_ander_t) (mh_parse_t parse,
		    mh_env_t   env);

/**************************************************************************
 *
 * mh_exp_warn (), mh_exp_error ()
 *
 *
 */
typedef enum
{
  MH_EXP_ERROR_IGNORE,
  MH_EXP_ERROR_WARN,
  MH_EXP_ERROR_FAIL
} mh_exp_error_type_t;

/* Used non-reentrantly */
static string_t   expand_source;
/* static mh_parse_t expand_parse; */

static jmp_buf  expand_error_jmp_buf;

static void
mh_exp_exception (mh_exp_error_type_t error_type,
		  mh_parse_t error_parse, 
		  string_t   error_explain,
		  ...)
{
  va_list args;

  if (mh_comp_verbose_debugging &&
      (MH_EXP_ERROR_WARN == error_type ||
       MH_EXP_ERROR_FAIL == error_type))
    {
      string_t error_string =
	mh_parse_to_string (error_parse);

      printf ("%s (%d): ", expand_source, 0);
      {
	va_start (args, error_explain);
	vprintf (error_explain, args);
	va_end (args);
      }
      printf ("\n  %-70s\n", error_string);
      xfree (error_string);
    }

  if (MH_EXP_ERROR_FAIL == error_type)
    longjmp (expand_error_jmp_buf, 1);

  return;
}

/* extern */ mh_exp_error_type_t
mh_exp_simple_tag_used_as_complex = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_complex_tag_used_as_simple = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_tag_operator_not_a_name = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_block_is_unknown = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_insufficient_arguments = MH_EXP_ERROR_FAIL;

/* extern */ mh_exp_error_type_t
mh_exp_excessive_arguments = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_incorrect_arguments = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_set_var_requires_keys = MH_EXP_ERROR_FAIL;

/* extern */ mh_exp_error_type_t
mh_exp_odd_number_of_case_clauses = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_function_argument = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_compiled_to_extern = MH_EXP_ERROR_WARN;

/* extern */ mh_exp_error_type_t
mh_exp_strange_var_case = MH_EXP_ERROR_WARN;





/**************************************************************************
 *
 * MHTML_MACHINE_PACKAGE
 *
 */
static void
mh_expand_install (void);

static Package *mhtml_machine_package = (Package *)NULL;

extern Symbol *
mhtml_find_mach_function_symbol (char *name)
{
  if (mhtml_machine_package == (Package *)NULL)
    {
      mhtml_machine_package = symbol_get_package_hash
	("*machine*", symbol_small_prime);
      mh_expand_install ();
    }

  return symbol_intern_in_package (mhtml_machine_package, name);
}


/**************************************************************************
 *
 * MH_ENV_T
 *
 * Static (Comptime) and Dynamic (runtime) Environments 
 *
 * Denotation for a local is always MH_UNKNOWN_TYPE
 */
typedef enum
{
  MH_ENV_WHITE,
  MH_ENV_NAME
} mh_env_type_t;

struct mh_env
{
  mh_env_type_t type;
#define MH_ENV_TYPE( env )    ((env)->type)

  mh_env_t  next;
#define MH_ENV_NEXT( env )      ((env)->next)

  union
  {
    mh_white_type_t white;
#define MH_ENV_WHITE( env )    ((env)->u.white)

    struct 
    {
      string_t  string;
      mh_tag_t  tag;
    } name;
#define MH_ENV_STRING( env )    ((env)->u.name.string)
#define MH_ENV_TAG( env )       ((env)->u.name.tag)
  
  } u;
};

#define MH_ENV_IS_NAME_P( env )    (MH_ENV_NAME  == MH_ENV_TYPE (env))
#define MH_ENV_IS_WHITE_P( env )   (MH_ENV_WHITE == MH_ENV_TYPE (env))

static inline boolean_t
mh_env_is_top_level_p (mh_env_t env)
{ return NULL == env; }


static void
mh_env_free_special (mh_env_t  env,
		     boolean_t recurse)
{
  if (NULL == env) return;

  if (recurse)
    mh_env_free_special (MH_ENV_NEXT (env), recurse);

  xfree (env);
}

static inline void
mh_env_free (mh_env_t env)
{ mh_env_free_special (env, true); }
  
/*
 * MH_ENV_EXTEND () 
 *
 * Extend ENV with a lexical environment containing SYMBOL of TYPE. */
static mh_env_t 
mh_env_extend (mh_env_t env,
	       string_t string,
	       mh_tag_t tag)
{
  mh_env_t new_env = (mh_env_t) xmalloc (sizeof (struct mh_env));

  MH_ENV_TYPE   (new_env) = MH_ENV_NAME;
  MH_ENV_NEXT   (new_env) = env;
  
  MH_ENV_STRING (new_env) = string;
  MH_ENV_TAG    (new_env) = tag;
  return new_env;
}

static mh_env_t
mh_env_extend_white (mh_env_t env,
		     mh_white_type_t white)
{
  mh_env_t new_env = (mh_env_t) xmalloc (sizeof (struct mh_env));

  MH_ENV_TYPE   (new_env) = MH_ENV_WHITE;
  MH_ENV_NEXT   (new_env) = env;
  
  MH_ENV_WHITE (new_env) = white;
  return new_env;
}
  
/*
 * MH_ENV_EXTEND_MANY ()
 *
 * Repeatedly extend ENV COUNT times from the SYMBOLS and TYPES arrays. */
#if defined (UNUSED)
static mh_env_t
mh_env_extend_many (mh_env_t     env,
		    unsigned int count,
		    mh_string_t *strings,
		    mh_tag_t    *tags)
{
  while (count--)
    env = mh_env_extend (env, *strings++, *tags++);
  return (env);
}
#endif

static mh_tag_t
mh_env_lookup_global (mh_env_t env,
		      string_t name)
{
  Symbol *uf, *pf;

  /* Look for a user function */
  uf = mhtml_find_user_function_symbol (name);

  if (uf && uf->machine)
    return (mh_tag_t) (uf->machine);

  pf = mhtml_find_mach_function_symbol (name);

  if (pf && pf->machine)
    return (mh_tag_t) (pf->machine);

  return NULL;
}

/*
 * MH_ENV_LOOKUP () 
 *
 * Return the type/denotation for SYMBOL in ENV
 */
static mh_tag_t
mh_env_lookup (mh_env_t env,
	       string_t string)
{
  return NULL == env
    ? mh_env_lookup_global (env, string)
    : ((MH_ENV_IS_NAME_P (env) && 
	0 == strcasecmp (string, MH_ENV_STRING (env)))
       ? MH_ENV_TAG (env)
       : mh_env_lookup (MH_ENV_NEXT (env), string));
}

static boolean_t
mh_env_lookup_is_lexical_p (mh_env_t env,
			    string_t string)
{
  return NULL == env 
    ? false
    : ((MH_ENV_IS_NAME_P (env) && 
	0 == strcmp (string, MH_ENV_STRING (env)))
       ? true
       : mh_env_lookup_is_lexical_p (MH_ENV_NEXT (env), string));
}

static mh_white_type_t
mh_env_lookup_white (mh_env_t env)
{
  return NULL == env
    ? MH_WHITE_INHERIT
    : (MH_ENV_IS_WHITE_P (env)
       ? MH_ENV_WHITE (env)
       : mh_env_lookup_white (MH_ENV_NEXT (env)));
}

/*****************************************************************************
 *
 * General Expanders
 *
 * (Way) Forward declaration */
static mh_core_t
mh_exp (mh_parse_t parse,
	mh_env_t   env);


#if defined (NEVER_DEFINED)
static mh_core_t *
mh_exp_sequence_1 (mh_parse_t tag_1,
		   mh_env_t   env)
{
  return mh_core_1 (mh_exp (tag_1, env));
}

static mh_core_t *
mh_exp_sequence_2 (mh_parse_t tag_1,
		   mh_parse_t tag_2,
		   mh_env_t   env)
{
  return mh_core_2 (mh_exp (tag_1, env),
		    mh_exp (tag_2, env));
}
#endif

static mh_core_t *
mh_exp_sequence_3 (mh_parse_t tag_1,
		   mh_parse_t tag_2,
		   mh_parse_t tag_3,
		   mh_env_t   env)
{
  return mh_core_3 (mh_exp (tag_1, env),
		    mh_exp (tag_2, env),
		    mh_exp (tag_3, env));
}

/* Expand OPERANDS (via MH_PARSE_LIST_TAIL) into a vector of cores.
   The vector length is mh_parse_count (operands) */
static mh_core_t *
mh_exp_sequence_ander (mh_parse_t operands,
		       mh_env_t   env,
		       mh_exp_ander_t ander)
{
  unsigned int count = mh_parse_count (operands);

  if (0 == count)
    return ((mh_core_t *) NULL); /* Trouble */
  else
    {
      mh_core_t *cores = (mh_core_t *) xmalloc (count * sizeof (mh_core_t));
      mh_core_t *cores_result = cores;

      for (; operands; operands = MH_PARSE_LIST_TAIL (operands))
	*cores++ = (*ander) (MH_PARSE_LIST_HEAD (operands), env);

      return (cores_result);
    }
}

static mh_core_t *
mh_exp_sequence (mh_parse_t operands,
		 mh_env_t   env)
{
  return mh_exp_sequence_ander (operands, env, mh_exp);
}

static mh_core_t
mh_exp_data (mh_parse_t parse,
	     mh_env_t   env)
{
  return mh_core_data_new
    (parse ? mh_parse_to_string (parse) : "");
}

static mh_core_t *
mh_exp_args (mh_tag_t      tag,	/* OPERATOR TAG */
	     mh_parse_t    operands,
	     unsigned int  operands_count,
	     mh_parse_t    parse,
	     mh_parse_t    body,
	     mh_env_t      env,
	     unsigned int *core_count)
{
  mh_argument_t *args       = MH_TAG_ARGS (tag);
  unsigned int   args_count = MH_TAG_ARGS_COUNT (tag);
  unsigned int   args_index = 0;
  
  mh_core_t   *cores       = (mh_core_t *) xcalloc
    (args_count, sizeof (mh_core_t));

  /* OPERANDS must have whitespace removed */
  mh_parse_t parse_keys, parse_non_keys;

  unsigned int   args_non_index = 0;

  /* Split OPERANDS into two disjoint sets: non-keys and keys */
  mh_parse_list_split_keys (operands, &parse_non_keys, &parse_keys);

  for (args_index = 0; args_index < args_count; args_index++)
    {
      mh_argument_t arg       = args [args_index];
      mh_parse_t    arg_parse = MH_PARSE_EMPTY;

      switch (MH_ARGUMENT_TYPE (arg))
	{
	case MH_ARGUMENT_REQUIRED: 
	  arg_parse = mh_parse_list_nth
	    (parse_non_keys, args_non_index++);
	  
	  if (MH_PARSE_EMPTY == arg_parse)
	    mh_exp_exception
	      (mh_exp_insufficient_arguments,
	       parse,
	       "missing required argument");

	  break;

	case MH_ARGUMENT_OPTIONAL:
	  arg_parse = mh_parse_list_nth
	    (parse_non_keys, args_non_index++);
	  break;

	case MH_ARGUMENT_KEY:
	  arg_parse = mh_parse_match_key
	    (parse_keys, MH_ARGUMENT_NAME (arg));

	  /* Expand the KEY VALUE, not the KEY itself */
	  if (MH_PARSE_EMPTY != arg_parse)
	    arg_parse = MH_PARSE_KEY_VALUE (arg_parse);
	  break;

	  /* Always parsed as an array */
	case MH_ARGUMENT_REST:
	  {
	    mh_parse_t parse_rest =
	      mh_parse_list_rest (parse_non_keys, args_non_index);
	    
	    unsigned int parse_rest_count =
	      mh_parse_count (parse_rest);

	    cores [args_index] = mh_core_prim_new
	      (MH_ARRAY_OP,
	       (mh_exp_sequence_ander
		(parse_rest, env,
		 (MH_ARGUMENT_EVAL (arg) == MH_ARGUMENT_UNEVALLED
		  ? mh_exp_data
		  : mh_exp))),
	       parse_rest_count);
			     
	    continue /* FOR */ ;
	  }

	case MH_ARGUMENT_BODY:
	  arg_parse = (false == MH_TAG_COMPLEX_P (tag))
	    ? mh_parse_list_as_body (body)
	    : body;
	  break;

	case MH_ARGUMENT_ATTRIBUTES:
	  arg_parse = (false == MH_TAG_COMPLEX_P (tag))
	    ? body
	    : MH_PARSE_LIST_TAIL (MH_PARSE_BLK_OPEN (parse));
	  break;
	}

      /* Eval or Uneval */
      cores [args_index] =
	(MH_ARGUMENT_EVAL (arg) == MH_ARGUMENT_UNEVALLED
	 ? mh_exp_data
	 : mh_exp)
	(arg_parse, env);
    }

  *core_count = args_index;
  return cores;
}

static mh_core_t *
mh_exp_prim_args (mh_tag_t   tag,	/* OPERATOR TAG */
		  mh_parse_t operands,
		  mh_env_t   env)
{
  return mh_exp_sequence_ander (operands, env, mh_exp);
}

/*****************************************************************************
 * 
 * MH_EXP_EXTERN ()
 *
 */
static mh_core_t
mh_exp_as_extern (mh_parse_t  parse,
		  mh_env_t    env)
{
  mh_core_t core;
  int ignore = 0;

  /* No questions asked - the PARSE goes directly to STRING. */
  string_t string = mh_parse_to_string (parse);

  if (tag_count > 0)
    {
      string_t read_string = read_sexp (string, &ignore, 0);

      xfree (string);
      string = read_string;
    }

  core = mh_core_extern_new (string);
  xfree (string);

  mh_exp_exception
    (mh_exp_compiled_to_extern,
     parse,
     "Compiled to extern");

  return core;
}

static mh_core_t
mh_exp_as_get (mh_parse_t parse,
	       mh_env_t   env)
{
  return mh_core_get_new (mh_exp (parse, env));
}

static mh_core_t
mh_exp_as_get_raw (mh_parse_t parse,
		   mh_env_t   env)
{
  return mh_core_get_new
    (mh_core_array_new (mh_exp (parse, env), NULL));
}

/* If PARSE is a TOKEN that won't be a NUMBER then expand as a
   variable reference. */
static mh_core_t
mh_exp_as_number (mh_parse_t parse,
		  mh_env_t   env)
{
  if (NULL == parse) return MH_CORE_NULL;

  if (mh_parse_type_p (parse, MH_PARSE_TYPE_TOKEN))
    {
      mh_token_t token  = MH_PARSE_TOKEN (parse);
      string_t   string = mh_token_string (token);
      string_t   end;
      
      /* If readable as a number then generate core data  */
      strtod (string, &end);	/* Wrong.... but sadly not by much... */

      if (end != string)
	return mh_core_data_new (mh_token_string (token));
    }

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
    case MH_PARSE_TYPE_ARRAY:
      return mh_exp_as_get (parse, env);

    default:
      return mh_exp (parse, env);
    }
}

static mh_core_t
mh_exp_as_app (mh_parse_t   parse,
	       mh_env_t     env,
	       mh_parse_t   operator,
	       mh_tag_t     tag,
	       mh_parse_t   operands,
	       unsigned int count,
	       mh_parse_t   body)
{
  /* Check CALL count; expand OPERANDS based on ARGUMENTS */
  unsigned int core_count;
  mh_core_t   *core =
    mh_exp_args (tag, operands, count, parse, body, env, &core_count);

  return mh_core_app_new (mh_exp (operator, env), core, core_count);
}

static mh_core_t
mh_exp_as_prim (mh_parse_t   parse,
		mh_env_t     env,
		mh_parse_t   operator,
		mh_tag_t     tag,
		mh_parse_t   operands,
		unsigned int count,
		mh_parse_t   body)
{
  if (tag->compile.primitive_p)
    return mh_core_prim_new
      (tag->compile.primitive_opcode,
       mh_exp_prim_args (tag, operands, env),
       count);
  else
    return mh_exp_as_extern (parse, env);
}

static mh_core_t
mh_exp_as_math (mh_parse_t   parse,
		mh_env_t     env,
		mh_parse_t   operator,
		mh_tag_t     tag,
		mh_parse_t   operands,
		unsigned int count,
		mh_parse_t   body)
{
  if (tag->compile.primitive_p)
    return mh_core_prim_new
      (tag->compile.primitive_opcode,
       mh_exp_sequence_ander (operands, env, mh_exp_as_number),
       count);
  else
    return mh_exp_as_extern (parse, env);
}

/*****************************************************************************
 *
 * Top-Level PARSE-OP Expanders
 *
 */
static mh_core_t
mh_exp_list (mh_parse_t parse, 
	     mh_env_t   env);

static mh_core_t
mh_exp_token (mh_parse_t parse,
	      mh_env_t   env)
{
  mh_token_t token;
  assert (mh_parse_type_p (parse, MH_PARSE_TYPE_TOKEN));

  token = MH_PARSE_TOKEN (parse);
  return mh_core_data_new (mh_token_string (token));
}

static mh_core_t
mh_exp_string (mh_parse_t parse,
	       mh_env_t   env)
{
  assert (mh_parse_type_p (parse, MH_PARSE_TYPE_STRING));

  return MH_PARSE_EMPTY == MH_PARSE_STRING (parse)
    ? mh_core_data_new ("")
    : (tag_count > 0
       ? mh_exp (MH_PARSE_STRING (parse), env)
       : mh_core_fmt_3_new (mh_core_data_new ("\""),
			    mh_exp (MH_PARSE_STRING (parse), env),
			    mh_core_data_new ("\"")));
}

static mh_parse_t
mh_parse_clean_and_normalize (mh_parse_t parse)
{
  parse = mh_parse_delete_space (parse);

  return (mh_parse_type_p (parse, MH_PARSE_TYPE_LIST) &&
	  1 == mh_parse_list_count (parse))
    ? MH_PARSE_LIST_HEAD (parse)
    : parse;
}

/* Biases towards SET-VAR - which might not be appropriate */
static mh_core_t
mh_exp_key (mh_parse_t parse,
	    mh_env_t   env)
{
  /* KEY_NAME might have surrounding whitespace and should be a single
     expression. */ 
  mh_parse_t key_name  =  
    mh_parse_clean_and_normalize (MH_PARSE_KEY_NAME (parse));

  /* Ditto KEY_VALUE */
  mh_parse_t key_value =  
    mh_parse_clean_and_normalize (MH_PARSE_KEY_VALUE (parse));

  return mh_core_key_new (mh_exp (key_name,  env),
			  mh_exp (key_value, env));
}

static mh_core_t
mh_exp_array (mh_parse_t parse,
	      mh_env_t   env)
{
  /* NAME must be a TOKEN or an expression evaluating to a NAME */
  /*  OK:   foo[], foo[2], <get-var bar>[],
   *  NOK:  "abc\ndef"[0], 
   */

  mh_parse_t name = 
    mh_parse_clean_and_normalize (MH_PARSE_ARRAY_NAME  (parse));

  mh_parse_t indx = MH_PARSE_ARRAY_INDEX (parse)
    ? mh_parse_clean_and_normalize (MH_PARSE_ARRAY_INDEX (parse))
    : MH_PARSE_EMPTY;

  return mh_core_array_new (mh_exp (name, env),
			    mh_exp_as_number (indx, env));
}

static boolean_t
mh_exp_tag_whitespace_delete_p (mh_white_type_t tag_white,
				mh_env_t        env)
{
  return MH_WHITE_KEEP == tag_white
    ? false
    : (MH_WHITE_DELETE == tag_white
       ? true
       : (MH_WHITE_INHERIT == tag_white &&
	  MH_WHITE_DELETE  == mh_env_lookup_white (env)));
}

static mh_core_t
mh_exp_tag (mh_parse_t parse,
	    mh_env_t   env)
{
  mh_parse_t body     = MH_PARSE_TAG_BODY (parse);
  mh_parse_t operator = mh_parse_tag_operator (parse);
  mh_token_t operator_token;
  string_t   operator_name;
  mh_tag_t   operator_tag;

  switch (operator ? MH_PARSE_TYPE (operator) : -1)
    {
    case MH_PARSE_TYPE_TOKEN:
      /* <foo ...> */
      operator_token = MH_PARSE_TOKEN (operator);
      operator_name  = mh_token_string (operator_token);

      /* Redundant with above */
#if defined (SKIP_OVER_WHITESPACE_IN_TAG_OPERATOR)
      assert (mh_token_match_type_p (operator_token, MH_TOKEN_WORD));
#endif

      switch (mh_token_type (operator_token))
	{
	case MH_TOKEN_WORD:
	  /* Find the binding for TOKEN */
	  operator_tag = mh_env_lookup (env, operator_name);

	  if (NULL == operator_tag)
	    {
	      /* Warn */
	      return mh_exp_as_extern (parse, env);
	    }
	  else
	    {
	      mh_parse_t body_clean = body;

	      /* Check if complex */
	      if (true  == MH_TAG_COMPLEX_P (operator_tag) &&
		  false == MH_TAG_WEAK_P    (operator_tag))
		{
		  mh_exp_exception
		    (mh_exp_complex_tag_used_as_simple,
		     parse,
		     "Complex tag used as Simple: %s",
		     operator_name);
		  return mh_exp_as_extern (parse, env);
		}

	      /* Guaranteed that BODY's HEAD is not whitespace; thus, 
		 MH_PARSE_TAG_BODY(parse) is not involved. */
	      if (mh_exp_tag_whitespace_delete_p
		  (MH_TAG_WHITESPACE (operator_tag), env))

		if (body && mh_parse_type_p (body, MH_PARSE_TYPE_LIST))
		  body_clean = mh_parse_list_delete_space
		    (mh_parse_list_copy (body));

	      /* Invoke EXPANDER */
	      {
		mh_core_t result;

		tag_count++;
		result = mh_tag_expand
		  (parse, env, operator, operator_tag, 
		   MH_PARSE_LIST_TAIL (body_clean),
		   mh_parse_count (MH_PARSE_LIST_TAIL (body_clean)),
		   MH_PARSE_LIST_TAIL (body));
		tag_count--;

		if (body != body_clean)
		  mh_parse_list_free (body_clean);

	      return result;
	      }
	    }

	default:
	  mh_exp_exception
	    (mh_exp_tag_operator_not_a_name,
	     parse,
	     "TAG operator must be a name");
	  
	  return mh_exp_as_extern (parse, env);
	}
     
    default:
      /* <"foo" ...>
         <<foo> ...>
      *
      *  '<' <strange> '>'
      *
      * where 'strange' is everything but a MH_PARSE_TOKEN of
      * MH_TOKEN_WORD.
      *
      * Expand as MH_EXP_LIST and add delimiters or FAIL */
      
      /* Warn */
      mh_exp_exception
	(mh_exp_tag_operator_not_a_name,
	 parse,
	 "TAG operator must be a name");

      return mh_exp_as_extern (parse, env);
    }
}

static mh_core_t
mh_exp_blk (mh_parse_t blk,
	    mh_env_t   env)
{
  mh_parse_t blk_open  = MH_PARSE_BLK_OPEN  (blk);
  mh_parse_t blk_body  = MH_PARSE_BLK_BODY  (blk);
  mh_parse_t blk_close = MH_PARSE_BLK_CLOSE (blk);

  mh_parse_t operator = mh_parse_tag_operator (blk_open);

  mh_token_t operator_token;
  string_t   operator_name;
  mh_tag_t   operator_tag;

  switch (operator ? MH_PARSE_TYPE (operator) : -1)
    {
    case MH_PARSE_TYPE_TOKEN:
      /* Find the 'TAG' */
      operator_token = MH_PARSE_TOKEN (operator);
      operator_name  = mh_token_string (operator_token);

      switch (mh_token_type (operator_token))
	{
	case MH_TOKEN_WORD:
	  /* Find the binding for TOKEN */
	  operator_tag = mh_env_lookup (env, operator_name);

	  if (NULL == operator_tag)
	    {
	      mh_exp_exception
		(mh_exp_block_is_unknown,
		 blk,
		 "Block is unknown: %s",
		 operator_name);

	      return mh_exp_as_extern (blk, env);
	    }
	  else
	    {
	      mh_parse_t blk_open_body =  
		MH_PARSE_TAG_BODY (blk_open);

	      if (false == MH_TAG_COMPLEX_P (operator_tag))
		{
		  mh_exp_exception
		    (mh_exp_simple_tag_used_as_complex,
		     blk,
		     "Simple tag used as complex");

		  return mh_core_fmt_new
		    (mh_exp_sequence_3 (blk_open, blk_body, blk_close, env),
		     3);
		}

	      /* Unconditionally */
	      if (mh_parse_type_p (blk_open_body, MH_PARSE_TYPE_LIST))
		blk_open_body = mh_parse_list_delete_space
		  (mh_parse_list_copy (blk_open_body));
	      
	      if (blk_body && 
		  mh_parse_type_p (blk_body, MH_PARSE_TYPE_LIST))

		MH_PARSE_BLK_BODY (blk) = blk_body = 
		  (0 != strcmp (operator_name, "defun")    &&
		   0 != strcmp (operator_name, "defsubst") &&
		   0 != strcmp (operator_name, "defmacro") &&
		   mh_exp_tag_whitespace_delete_p
		   (MH_TAG_WHITESPACE (operator_tag), env))

		  /* Delete ALL whitespace */
		  ? mh_parse_list_delete_interline_space (blk_body)

		  /* Delete only bounding whitespace */
		  : mh_parse_list_delete_bounding_space  (blk_body);

	      /* Check if complex */

	      /* Invoke EXPANDER */
	      {
		mh_core_t result;

		result = mh_tag_expand
		  (blk, env, operator, operator_tag, 
		   MH_PARSE_LIST_TAIL (blk_open_body),
		   mh_parse_count (MH_PARSE_LIST_TAIL (blk_open_body)),
		   blk_body);

#if 0
		/* Produces core dump */
		if (blk_open_body != MH_PARSE_TAG_BODY (blk_open))
		  mh_parse_list_free (blk_open_body);
#endif

		return result;
	      }
	    }

	default:
	  mh_exp_exception
	    (mh_exp_tag_operator_not_a_name,
	     blk,
	     "TAG operator must be a name");

	  return mh_exp_as_extern (blk, env);
	}
      
    default:
      /* Impossible (?!) Cause a SEGMENTATION fault
         No way to form a block without an operator of MH_PARSE_TYPE_TOKEN. */
      mh_exp_exception
	(mh_exp_tag_operator_not_a_name,
	 blk,
	 "TAG operator must be a name");

      return mh_exp_as_extern (blk, env);
    }
}

static mh_core_t
mh_exp_list (mh_parse_t parse, 
	     mh_env_t   env)
{
  unsigned int count = mh_parse_count (parse);

  switch (count)
    {
    case 0:  return mh_core_empty ();
    case 1:  return mh_exp (MH_PARSE_LIST_HEAD (parse), env);
    default: 
      {
	mh_core_t *cores = (mh_core_t *) 
	  xmalloc (count * sizeof (mh_core_t));
	unsigned int cores_index;

	for (cores_index = 0; cores_index < count; cores_index++)
	  {
	    cores [cores_index] = mh_exp (MH_PARSE_LIST_HEAD (parse), env);

	    if (mh_core_is_type (cores [cores_index], MH_CORE_FUNC))
	      {
		mh_tag_t tag = MH_CORE_FUNC_TAG (cores [cores_index]);
		env = mh_env_extend
		  (env, MH_TAG_NAME (tag), tag);
	      }
	    
	    parse = MH_PARSE_LIST_TAIL (parse);
	  }
	/* Free ENV */
	
	return mh_core_fmt_new (cores, count);
      }
    }
}


/***************************************************************************
 *
 * MH_EXP - Top-Level, Recursive Expansion
 *
 *
 */
typedef mh_core_t (*mh_exp_func_t) (mh_parse_t parse,
				    mh_env_t   env);

static mh_core_t
mh_exp (mh_parse_t parse,
	mh_env_t   env)
{
  /* Ordering must match mh_parse_op_t */
  static mh_exp_func_t mh_expanders [MH_NUMBER_OF_PARSE_TYPES] =
  {
    mh_exp_token,
    mh_exp_string,
    mh_exp_key,
    mh_exp_array,
    mh_exp_tag,
    mh_exp_blk,
    mh_exp_list
  };

  return MH_PARSE_EMPTY == parse
    ? mh_core_data_new ("")
    : (* mh_expanders [MH_PARSE_TYPE (parse)]) (parse, env);
}

/*
 * MH_EXPAND
 *
 * TOP-LEVEL, visible EXPAND function.  Always returns a THUNK
 * (function of no arguments) because METAHTML has a recursive
 * top-level and thus there is nothing to 'close over'.  We might
 * normally close over the values of the free variables but METAHTML
 * free variables are all global symbols - so we get a kind of
 * closure for free.  
 *
 * Convert PACKAGE into a GLOBAL-ENV and expand PARSE.  This is one of two
 * non-static functions in this file. */ 
extern mh_core_t 
mh_expand (string_t   parse_source,
	   mh_parse_t parse)
{
  mh_core_t core;

  tag_count     = 0;
  expand_source = parse_source;
  core          = (0 != setjmp (expand_error_jmp_buf)
		   ? mh_core_empty ()
		   : mh_exp (parse, NULL));
  expand_source = NULL;

  return mh_core_func_new
    (mh_tag_new ("TOP-LEVEL-FUNCTION", TAG_TYPE_DEFUN,
		 false,
		 false,
		 true,
		 NULL,
		 strdup (DEFAULT_PACKAGE_NAME),
		 NULL,
		 NULL,
		 0),
     core);
}


/*****************************************************************************
 *
 * Special Form - IF
 *
 * <if predicate consequent alternate> */
DEFINE_EXPANDER ("if", mh_exp_if,
"This is <IF ....>")
{
  switch (count)
    {
    case 0: /* <if> */
      return mh_core_empty ();

    case 1: /* <if pred> */
      {
	mh_parse_t pred = 
	  MH_PARSE_LIST_HEAD (operands);

	return mh_exp (pred, env);
      }

    case 2: /* <if pred cons> */
    case 3: /* <if pred cons alt> */
      {
	mh_parse_t pred = 
	  MH_PARSE_LIST_HEAD (operands);

	mh_parse_t cons = 
	  MH_PARSE_LIST_HEAD (MH_PARSE_LIST_TAIL (operands));

	mh_parse_t alt  = (count == 2)
	  ? MH_PARSE_EMPTY
	  : MH_PARSE_LIST_HEAD (MH_PARSE_LIST_TAIL
				(MH_PARSE_LIST_TAIL (operands)));

	/* If PRED is MH_CORE_SET, could warn */

	return mh_core_if_new (mh_exp (pred, env),
			       mh_exp (cons, env),
			       (MH_PARSE_EMPTY == alt
				? mh_core_empty ()
				: mh_exp (alt,  env)));
      }

    default:
      /* <if pred cons alt junk ...> */
      return mh_core_empty ();
    }
}

/*****************************************************************************
 *
 * Special Form - OR
 *
 * <or exp ...> */
DEFINE_EXPANDER ("or", mh_exp_or,
"This is <or ....>")
{
  switch (count)
    {
    case 0:
      /* <or> => "" */
      return mh_core_data_new ("");

    case 1:
      /* <or exp> => exp */
      return mh_exp (operands, env);

    default:
      /* <or exp ...> => CORE */
      return mh_core_or_new (mh_exp_sequence (operands, env), count);
    }
}

/*****************************************************************************
 *
 * Special Form - AND
 *
 * <and exp ...> */
static mh_core_t
mh_exp_and_recurse (mh_parse_t   exp,
		    unsigned int exp_count,
		    mh_env_t     env)
{
  /* EXP has whitespace removed */
  switch (exp_count)
    {
    case 0:
      /* <and> => "true" */
      return mh_core_data_new ("true");
    case 1:
      /* <and exp> => exp */
      return mh_exp (MH_PARSE_LIST_HEAD (exp), env);
    default:
      /* <and exp1 exp2 ...> => <if exp1 <and exp2 ...> "false"> */
      return mh_core_if_new
	(mh_exp (MH_PARSE_LIST_HEAD (exp), env),
	 mh_exp_and_recurse (MH_PARSE_LIST_TAIL (exp), 
			     exp_count - 1,
			     env),
	 mh_core_data_new (""));
    }
}
  
DEFINE_EXPANDER ("and", mh_exp_and,
"This is <AND ....>")
{
  assert (mh_parse_type_p (operands, MH_PARSE_TYPE_LIST));
  return mh_exp_and_recurse (operands, count, env);
}


/*****************************************************************************
 *
 * Special Form - GET-VAR
 *
 * <get-var name ...> */
static mh_core_t 
mh_exp_get_var_one (mh_parse_t form,
		    mh_env_t   env)
{
  switch (MH_PARSE_TYPE (form))
    {
    case MH_PARSE_TYPE_BLK:
    case MH_PARSE_TYPE_LIST:
    case MH_PARSE_TYPE_KEY:
      mh_exp_exception
	(mh_exp_incorrect_arguments,
	 form,
	 "GET-VAR with incorrect arguments");
      return mh_core_empty ();

    default:
      return mh_exp_as_get (form, env);
    }
}

DEFINE_EXPANDER ("get-var", mh_exp_get_var,
"This is <GET-VAR ....>")
{
  switch (count)
    {
    case 0:
      return mh_core_empty ();

    case 1:
      return mh_exp_get_var_one (MH_PARSE_LIST_HEAD (operands), env);

    default:
      /* Expand as OR */
      return mh_core_fmt_new
	(mh_exp_sequence_ander (operands, env, mh_exp_get_var_one),
	 count);
    }
}

DEFINE_EXPANDER ("get-var-eval", mh_exp_get_var_eval,
"This is <GET-VAR-EVAL ....>")
{
  return mh_core_prim_1_new
    (MH_EVAL_OP,
     mh_exp_get_var (EXP_ARGS_APP));
}


/*****************************************************************************
 *
 * Special Form - SET-VAR
 *
 * <set-var name ...> */
static mh_core_t
mh_exp_set_var_once (mh_parse_t parse,
		     mh_env_t   env)
{
  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_KEY:
      return mh_exp (parse, env);

    default:
      return mh_core_key_new (mh_exp (parse, env),
			      mh_core_data_new (""));
    }
}

DEFINE_EXPANDER ("set-var", mh_exp_set_var,
"This is <SET-VAR ....>")
{

  /* Note that <set-var foo bar[1]> is identical to <set-var foo=""
     bar[1]=""> */
  switch (count)
    {
    case 0:
      return mh_core_empty ();

    default:
      return mh_core_set_new
	(mh_exp_sequence_ander (operands, env, mh_exp_set_var_once),
	 count);
    }
}

/*****************************************************************************
 *
 * Special Form - PROG // CONCAT
 *
 */
static mh_core_t 
mh_exp_prog_and_concat (mh_parse_t   operands,
			unsigned int exp_tag_count,
			mh_env_t     env)
{
  mh_core_t result;
  int old_tag_count = tag_count;

  tag_count = exp_tag_count;
  /* The following mh_exp() produces a PROG core for the common case where
     OPERANDS is a LIST parse type. */
  result    = mh_exp (operands, env);
  tag_count = old_tag_count;
  return result;
}

DEFINE_EXPANDER ("prog", mh_exp_prog,
"This is <prog ...>")
{
  return count == 0
    ? mh_core_data_new ("")
    : mh_exp_prog_and_concat (mh_parse_list_as_body (operands), 0, env);
}

DEFINE_EXPANDER ("concat", mh_exp_concat,
"This is <concat ...>")
{
  mh_parse_t concat;
  mh_core_t  core;

  if (count == 0) return mh_core_data_new ("");

  /* Destructure OPERANDS by: flattening parse BLOCKS, deleting all
     whitespace, re-parsing the result and praying */
  concat = mh_parse_list_as_concat (operands);
  core   = mh_exp_prog_and_concat (concat, 1, env);
  
  /* What if CONCAT is not a LIST? */
  if (mh_parse_type_p (concat, MH_PARSE_TYPE_LIST))
    mh_parse_list_free (concat);

  return core;
}

/*****************************************************************************
 *
 * Special Form - BREAK
 *
 * <break> */
DEFINE_EXPANDER ("break", mh_exp_break,
"This is <break>")
{
  if (count != 0)
    mh_exp_exception
      (mh_exp_excessive_arguments,
       parse,
       "RETURN with excessive predicates.");

  return mh_core_break_new (NULL);
}


/*****************************************************************************
 *
 * Special Form - RETURN
 *
 * <return> */
DEFINE_EXPANDER ("return", mh_exp_return,
"This is <return>")
{
  switch (count)
    {
    case 0: return mh_core_break_new (mh_core_data_new (""));
    default:
      {
	mh_parse_t value = mh_parse_list_nth (operands, 0);
	
	if (count > 1)
	  mh_exp_exception
	    (mh_exp_excessive_arguments,
	     parse,
	     "RETURN with excessive predicates.");

	return  mh_core_break_new (mh_exp (value, env));
      }
    }
}


/*****************************************************************************
 *
 * Special Form - INCLUDE
 *
 * <include file> */
DEFINE_EXPANDER ("include", mh_exp_include,
"This is <include ...>")
{
  switch (count)
    {
    case 1:
      {
#if 0
	mh_parse_t file_parse = MH_PARSE_LIST_HEAD (operands);
	string_t   filename   = mh_parse_to_string (file_parse);

	/* Read entire file in, parse as text.... */

	/* Expand inline */
#endif	
	return mh_exp_as_extern (parse, env);
      }

    default:
      return mh_exp_as_extern (parse, env);
    }
}

/*****************************************************************************
 *
 * Special Form - VAR_CASE
 *
 * <var_case exp ...> */
static mh_core_t
mh_exp_var_case_iterator (mh_parse_t   parse,
			  mh_parse_t   operands,
			  unsigned int count,
			  mh_env_t     env)
{
  switch (count)
    {
    case 0:
      return mh_core_empty ();

    case 1:
      mh_exp_exception
	(mh_exp_odd_number_of_case_clauses,
	 operands,
	 "VAR-CASE");
      return mh_core_empty ();
      
    default:
      {
	mh_parse_t key  = 
	  MH_PARSE_LIST_HEAD (operands);

	mh_parse_t cons = 
	  MH_PARSE_LIST_HEAD (MH_PARSE_LIST_TAIL (operands));

	mh_core_t test; 

	/* Ought to check if KEY is a KEY! */
	if (mh_parse_type_p  (key, MH_PARSE_TYPE_KEY))
	  test  = mh_core_prim_2_new
	    (MH_ID_OP,
	     mh_exp_as_get (MH_PARSE_KEY_NAME  (key), env),
	     mh_exp (MH_PARSE_KEY_VALUE (key), env));
	else if (mh_parse_token_match_p (key, MH_TOKEN_WORD, "default"))
	  test = mh_core_data_new ("true");
	else
	  {
	    mh_exp_exception
	      (mh_exp_strange_var_case,
	       parse,
	       "VAR-CASE with a strange clause.");
	    test = mh_core_data_new ("");
	  }

	operands = MH_PARSE_LIST_TAIL (MH_PARSE_LIST_TAIL (operands));
	count   -= 2;

	return mh_core_if_new (test,
			       mh_exp (cons, env),
			       mh_exp_var_case_iterator (parse,
							 operands,
							 count, 
							 env));
      }
    }
}

DEFINE_EXPANDER ("var-case", mh_exp_var_case,
"This is <var-case ...>")
{
  return mh_exp_var_case_iterator (parse, operands, count, env);
}

/*****************************************************************************
 *
 * Special Form - WHEN
 *
 * <when test> body </when> */
DEFINE_EXPANDER ("when", mh_exp_when,
"This is <when pred><body>*</when>")
{
  switch (count)
    {
    case 0:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "WHEN without a predicate.");
      return mh_core_empty ();
      
    case 1:
      /* An 'if' expression with a 'prog' consequent
	 and an "" (empty) alternate */
      {
	mh_parse_t test = MH_PARSE_LIST_HEAD (operands);

	return mh_core_if_new (mh_exp (test, env),
			       mh_exp (body, env),/* ?? PROG ?? */
			       mh_core_empty());
      }
    default:
      /* <when pred1 pred2 ...><body></when> */
      mh_exp_exception
	(mh_exp_excessive_arguments,
	 parse,
	 "WHEN with excessive predicates.");
      return mh_core_empty ();
    }
}

/*****************************************************************************
 *
 * Special Form - WHILE
 *
 */
DEFINE_EXPANDER ("while", mh_exp_while,
"This is <WHILE pred><body>*</while>")
{
  switch (count)
    {
    case 0:
      /* <while><body></while> => <body> */
#if defined (INCORRECT_BEHAVIOUR)
      return mh_exp (body, env);
#else
      return mh_core_empty ();
#endif /* !INCORRECT_BEHAVIOUR */

    case 1:
      return mh_core_fmt_1_new 
	(mh_core_while_new (mh_exp (MH_PARSE_LIST_HEAD (operands), env),
			    mh_exp (body, env)));

    default:
      /* Warn ... Excessive elements in <WHILE pred> */
      return mh_exp_as_extern (parse, env);
    }
}

/*****************************************************************************
 *
 * Special Form - WITH
 *
 */
DEFINE_EXPANDER ("with", mh_exp_with,
"This is <WITH key ...><body>*</with>")
{
  switch (count)
    {
    case 0:
      /* <with><body></with> => <body> */
      return mh_exp (body, env);

    default:
      /* All operands must be keys */

      /* No extending the environment here!!!!  Because body can't
	 possibly care.  But don't get any idea that that is good! */  
      return mh_core_with_new
	(mh_core_set_new
	 (mh_exp_sequence_ander (operands, env, mh_exp_set_var_once),
	  count),
	 mh_exp (body, env));
    }
}

/*****************************************************************************
 *
 * Special Form - FOREACH
 *
 */
static boolean_t
mh_exp_foreach_copy_or_not (mh_parse_t ncpy_key, 
			    mh_parse_t ycpy_key,
			    boolean_t  default_value)
{
  mh_parse_t ncpy_val = ncpy_key == MH_PARSE_EMPTY
    ? MH_PARSE_EMPTY
    : MH_PARSE_KEY_VALUE (ncpy_key);

  mh_parse_t ycpy_val = ycpy_key == MH_PARSE_EMPTY
    ? MH_PARSE_EMPTY
    : MH_PARSE_KEY_VALUE (ycpy_key);

  return ycpy_key
    ? mh_parse_token_match_p (ycpy_val, MH_TOKEN_WORD, "true")
    : (ncpy_key
       ? ! mh_parse_token_match_p (ncpy_val, MH_TOKEN_WORD, "true")
       : default_value);
}


static mh_core_t
mh_exp_foreach_full (EXP_ARGS_DEF)
{
  mh_parse_t start_key = mh_parse_match_key (operands, "start");
  mh_parse_t end_key   = mh_parse_match_key (operands, "end");
  mh_parse_t step_key  = mh_parse_match_key (operands, "step");
  mh_parse_t iter_key  = mh_parse_match_key (operands, "iter");
  mh_parse_t ncpy_key  = mh_parse_match_key (operands, "no-copy");
  mh_parse_t ycpy_key  = mh_parse_match_key (operands, "copy");

  boolean_t copy_p = 
    mh_exp_foreach_copy_or_not (ncpy_key, ycpy_key, true);

  /* Two names */
  mh_parse_t element = mh_parse_list_nth (operands, 0);
  mh_parse_t array   = mh_parse_list_nth (operands, 1);

  /* Need to properly implement WITH or to gensym START, etc. */

  /* We do these inside out.  The goal is:

     <defmacro foreach element array &key start end step iter body>
     <with arrayx = array
     startx = <or <get-var start> 0>
     endx   = <or <get-var end>   <array-size arrayx>>
     stepx  = <or <get-var step>  1>
     iterx  = iter ;;; no
     >
     <if <eq stepx 0> <set-var stepx = 1>>
     <set-var iterx = <if <lt stepx 0> <sub endx 1> startx>>
     <while <and <lt iterx endx> <ge iterx startx>>>
     <set-var element = arrayx[iterx]>
     body
     <set-var iterx = <add iterx stepx>>
     </while>
     </with>
     </defmacro>

  */
  mh_core_t dat_iter  = mh_core_data_new
    (MH_PARSE_EMPTY != iter_key
     ? mh_token_string (MH_PARSE_TOKEN (MH_PARSE_KEY_VALUE (iter_key)))
     : mhtml_make_identifier (NULL, 16));
  mh_core_t dat_start = 
    mh_core_data_new (mhtml_make_identifier (NULL, 16));
  mh_core_t dat_step  = 
    mh_core_data_new (mhtml_make_identifier (NULL, 16));
  mh_core_t dat_end   = 
    mh_core_data_new (mhtml_make_identifier (NULL, 16));
  mh_core_t dat_arr   = mh_core_data_new
    (mh_parse_type_p (array, MH_PARSE_TYPE_TOKEN) && !copy_p
     ? mh_token_string (MH_PARSE_TOKEN (array))
     : mhtml_make_identifier (NULL, 16));

  mh_core_t dat_zero  = mh_core_data_new ("0");
  mh_core_t dat_one   = mh_core_data_new ("1");
  mh_core_t dat_empty = mh_core_data_new ("");

  mh_core_t dat_ele   = 
    mh_core_data_new (mh_token_string (MH_PARSE_TOKEN (element)));

#define mcd(x)            mh_core_dup (x)
#define get(x)            mh_core_get_new (mcd (x))

  /* <set-var iterx = <add iterx stepx>> */
  mh_core_t inc_iter_core = mh_core_set_1_new
    (mcd (dat_iter),
     mh_core_prim_2_new (MH_ADD_OP,
			 get (dat_iter),
			 get (dat_step)));

  /* Possible because BODY doesn't care a bit about ENV!!!! */
  mh_core_t body_core     = mh_exp (body, env);

  /* <set element = array[iterx]> */
  mh_core_t ele_core = mh_core_set_1_new
    (mcd (dat_ele),
     mh_core_get_new (mh_core_array_new (mcd (dat_arr),
					 get (dat_iter))));

  mh_core_t while_body = mh_core_fmt_new
    (mh_core_3 (ele_core, body_core, inc_iter_core), 3);

  /* <and <lt iterx endx> <ge iterx startx>> */
  mh_core_t while_pred = mh_core_if_new
    (mh_core_prim_2_new (MH_LT_OP,
			 get (dat_iter),
			 get (dat_end)),
     mh_core_prim_2_new (MH_GE_OP,
			 get (dat_iter),
			 get (dat_start)),
     mcd (dat_empty));

  mh_core_t while_core =
    mh_core_while_new (while_pred, while_body);

  /* <set-var iterx = <if <lt stepx 0> <sub endx 1> startx>> */
  mh_core_t set_iter_core = mh_core_set_1_new
    (mcd (dat_iter),
     mh_core_if_new (mh_core_prim_2_new (MH_LT_OP,
					 get (dat_step),
					 mcd (dat_zero)),

		     mh_core_prim_2_new (MH_SUB_OP,
					 get (dat_end),
					 mcd (dat_one)),
		     get (dat_start)));

  /* <if <eq stepx 0> <set-var stepx = 1>> */
  mh_core_t set_step_core = mh_core_if_new
    (mh_core_prim_2_new (MH_EQ_OP, 
			 get (dat_step),
			 mcd (dat_zero)),
     mh_core_set_1_new (mcd (dat_step),
			mcd (dat_one)),
     mcd (dat_empty));	   

  /* Only used when COPY_P is true */
  mh_core_t with_set_array = mh_core_key_new
    (mh_core_array_new (mcd (dat_arr), NULL),
     mh_core_prim_1_new (MH_ARRAY_COPY_OP,
			 mh_exp_as_get_raw (array, env)));

  /* startx = <or <get-var start> 0> */
  mh_core_t with_set_start = mh_core_key_new
    (mcd (dat_start),
     (MH_PARSE_EMPTY != start_key
      ? mh_exp (MH_PARSE_KEY_VALUE (start_key), env)
      : mcd (dat_zero)));

  /* endx   = <or <get-var end>   <array-size array>> */
  mh_core_t with_set_end = mh_core_key_new
    (mcd (dat_end),
     (MH_PARSE_EMPTY != end_key
      ? mh_exp (MH_PARSE_KEY_VALUE (end_key), env)
      : mh_core_prim_1_new (MH_ARRAY_LEN_OP,
			    mh_core_get_new
			    (mh_core_array_new
			     (dat_arr, NULL)))));

  /* stepx  = <or <get-var step>  1> */
  mh_core_t with_set_step = mh_core_key_new
    (mcd (dat_step),
     (MH_PARSE_EMPTY != step_key
      ? mh_exp (MH_PARSE_KEY_VALUE (step_key), env)
      : mcd (dat_one)));

  mh_core_t with_set_core = 
    (mh_parse_type_p (array, MH_PARSE_TYPE_TOKEN) && !copy_p
     ? mh_core_set_new (mh_core_3 (with_set_start,
				   with_set_end,
				   with_set_step),
			3)
     : mh_core_set_new (mh_core_4 (with_set_array,
				   with_set_start,
				   with_set_end,
				   with_set_step),
			4));

  mh_core_t foreach_core = mh_core_with_new
    (with_set_core,
     mh_core_fmt_3_new (set_step_core, set_iter_core, while_core));

  /* Clean up */
  mh_core_free (dat_iter);
  mh_core_free (dat_start);
  mh_core_free (dat_step);
  mh_core_free (dat_end);
  mh_core_free (dat_zero);
  mh_core_free (dat_one);
  mh_core_free (dat_empty);
	
  return foreach_core;
	
}

static mh_core_t
mh_exp_foreach_small (EXP_ARGS_DEF)
{
  mh_parse_t iter_key  = mh_parse_match_key (operands, "iter");
  mh_parse_t ncpy_key  = mh_parse_match_key (operands, "no-copy");
  mh_parse_t ycpy_key  = mh_parse_match_key (operands, "copy");
  
  boolean_t copy_p = 
    mh_exp_foreach_copy_or_not (ncpy_key, ycpy_key, true);

  /* Two names */
  mh_parse_t element = mh_parse_list_nth (operands, 0);
  mh_parse_t array   = mh_parse_list_nth (operands, 1);

  /* Need to properly implement WITH or to gensym START, etc. */

  mh_core_t dat_iter  = mh_core_data_new
    (MH_PARSE_EMPTY != iter_key
     ? mh_token_string (MH_PARSE_TOKEN (MH_PARSE_KEY_VALUE (iter_key)))
     : mhtml_make_identifier (NULL, 16));
  mh_core_t dat_end   = 
    mh_core_data_new (mhtml_make_identifier (NULL, 16));
  mh_core_t dat_arr   = mh_core_data_new
    (mh_parse_type_p (array, MH_PARSE_TYPE_TOKEN) && !copy_p
     ? mh_token_string (MH_PARSE_TOKEN (array))
     : mhtml_make_identifier (NULL, 16));

  mh_core_t dat_zero  = mh_core_data_new ("0");
  mh_core_t dat_one   = mh_core_data_new ("1");
  mh_core_t dat_empty = mh_core_data_new ("");

  mh_core_t dat_ele   = 
    mh_core_data_new (mh_token_string (MH_PARSE_TOKEN (element)));

#define mcd(x)            mh_core_dup (x)
#define get(x)            mh_core_get_new (mcd (x))

  mh_core_t while_body = mh_core_fmt_new
    (mh_core_3
     (/* <set element = array[iterx]> */
      mh_core_set_1_new (mcd (dat_ele),
			 mh_core_get_new (mh_core_array_new (mcd (dat_arr),
							     get (dat_iter)))),
      /* Possible because BODY doesn't care a bit about ENV!!!! */
      mh_exp (body, env),
	
      /* <set-var iterx = <inc iterx>> */
      mh_core_set_1_new (mcd (dat_iter),
			 mh_core_prim_1_new (MH_INC_OP, get (dat_iter)))),
     3);

  /* <while <lt iterx endx> body >*/
  mh_core_t while_core =
    mh_core_while_new (mh_core_prim_2_new (MH_LT_OP,
					   get (dat_iter),
					   get (dat_end)), 
		       while_body);

  /* <set-var iterx = 0 */
  mh_core_t set_iter_core = 
    mh_core_key_new (mcd (dat_iter),
		     mcd (dat_zero));

  /* <set-var endx = <array-size array>> */
  mh_core_t set_endx_core =
    mh_core_key_new (mcd (dat_end),
		     mh_core_prim_1_new (MH_ARRAY_LEN_OP,
					 mh_core_get_new
					 (mh_core_array_new
					  (dat_arr, NULL))));

  mh_core_t with_set_core = 
    (mh_parse_type_p (array, MH_PARSE_TYPE_TOKEN) && !copy_p

     ? (mh_core_set_2_key_new
	(set_iter_core,
	 set_endx_core))

     : (mh_core_set_3_key_new
	(mh_core_key_new (mh_core_array_new (mcd (dat_arr), NULL),
			  (!copy_p
			   ? mh_exp_as_get_raw (array, env)
			   : mh_core_prim_1_new (MH_ARRAY_COPY_OP,
						 (mh_exp_as_get_raw
						  (array, env))))),
	 set_iter_core,
	 set_endx_core)));

  mh_core_t foreach_core = mh_core_with_new
    (with_set_core, while_core);

  /* Clean up */
  mh_core_free (dat_iter);
  mh_core_free (dat_end);
  mh_core_free (dat_zero);
  mh_core_free (dat_one);
  mh_core_free (dat_empty);
	
  return foreach_core;
}

DEFINE_EXPANDER ("foreach", mh_exp_foreach,
"This is <FOREACH element array &key start end step iter><body>*</foreach>")
{
  switch (count)
    {
    case 0:
    case 1:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "FOREACH missing element and/or array.");
      /* <foreach><body></foreach> => <body> */
      return mh_exp (body, env);

    default:
      {
	mh_parse_t start_key = mh_parse_match_key (operands, "start");
	mh_parse_t end_key   = mh_parse_match_key (operands, "end");
	mh_parse_t step_key  = mh_parse_match_key (operands, "step");

	return (!start_key && !end_key && !step_key
		? mh_exp_foreach_small
		: mh_exp_foreach_full)
	  (EXP_ARGS_APP);
      }
    }
}

/*****************************************************************************
 *
 * Special Form - VERBATIM
 *
 */
DEFINE_EXPANDER ("verbatim", mh_exp_verbatim,
"This is <verbatim><body>*</verbatim>")
{
  return mh_core_data_new (mh_parse_to_string (body));
}

/*****************************************************************************
 *
 * Special Form - COMMENT
 *
 */
DEFINE_EXPANDER ("comment", mh_exp_comment,
"This is <comment><body>*</comment>")
{
  return mh_core_empty ();
}

/*****************************************************************************
 *
 * Special Form - DEFUN
 *
 */

/* If the KEY value is "keep" return true otherwise if the KEY value
   is "delete" return false otherwise return default. */
static mh_white_type_t
mh_exp_defun_get_whitespace (mh_parse_t      key,
			     mh_white_type_t default_value)
{
  mh_parse_t value;

  if (MH_PARSE_EMPTY == key || ! mh_parse_type_p (key, MH_PARSE_TYPE_KEY))
    return default_value;

  value = MH_PARSE_KEY_VALUE (key);

  return mh_parse_token_match_p (value, MH_TOKEN_WORD, "keep")
    ? MH_WHITE_KEEP
    : (mh_parse_token_match_p (value, MH_TOKEN_WORD, "delete")
       ? MH_WHITE_DELETE
       : default_value);
}

static string_t
mh_exp_defun_get_packname (mh_parse_t key,
			   string_t   default_value)
{
  mh_parse_t value;

  if (MH_PARSE_EMPTY == key || ! mh_parse_type_p (key, MH_PARSE_TYPE_KEY))
    return default_value;

  value = MH_PARSE_KEY_VALUE (key);

  return mh_parse_token_match_type_p (value, MH_TOKEN_WORD)
    ? strdup (mh_token_string (MH_PARSE_TOKEN (value)))
    : (default_value ? strdup (default_value) : default_value);
}
  
static mh_token_t
mh_exp_defun_get_arg (mh_parse_t arg,
		      mh_argument_array_t *type)
{
  switch (MH_PARSE_TYPE (arg))
    {
    case MH_PARSE_TYPE_KEY:
      mh_exp_exception
	(mh_exp_function_argument,
	 arg,
	 "Key directive found but ignored");
      return NULL;

    case MH_PARSE_TYPE_TOKEN:
      *type = MH_ARGUMENT_VALUE;
      return MH_PARSE_TOKEN (arg);

    case MH_PARSE_TYPE_ARRAY:
      {
	mh_parse_t name = MH_PARSE_ARRAY_NAME  (arg);
	mh_parse_t indx = MH_PARSE_ARRAY_INDEX (arg);

	/* NAME  must be a TOKEN */
	/* INDEX must be NULL */
	if (MH_PARSE_EMPTY == indx &&
	    mh_parse_type_p (name, MH_PARSE_TYPE_TOKEN))
	  {
	    *type = MH_ARGUMENT_ARRAY;
	    /* Actually return name[] */
	    return MH_PARSE_TOKEN (name);
	  }

	/* Fall through .... */
      }

    default:
      mh_exp_exception
	(mh_exp_function_argument,
	 arg,
	 "Strange argument");
      return NULL;
    }
}

static boolean_t
mh_exp_defun_arg_percolate_p (mh_argument_t arg)
{
  return
    MH_ARGUMENT_BODY       == MH_ARGUMENT_TYPE (arg) ||
    MH_ARGUMENT_ATTRIBUTES == MH_ARGUMENT_TYPE (arg);
}

/* Produce a canonical ordering of ARGS.  BODY and ATTRIBUTES args
 * percolate to the top and become required.  At call sites mh_exp_as_app
 * will do the right thing by producing values for BODY and ATTRIBUTES.
 * This modifies ARGS */
static void
mh_exp_defun_sort_args (mh_argument_t *args,
			unsigned int   args_count)
{
  unsigned int   args_index;
  mh_argument_t *new_args;
  unsigned int   new_args_index = 0;

  if (args_count < 2) return;

  new_args = (mh_argument_t *) 
    xmalloc (args_count * sizeof (mh_argument_t));

  /* Percolate BODY and ATTRIBUTES */
  for (args_index = 0; args_index < args_count; args_index++)
    if (mh_exp_defun_arg_percolate_p (args [args_index]))
      new_args [new_args_index++] = args [args_index];

  /* Get other args into NEW_ARGS */
  for (args_index = 0; args_index < args_count; args_index++)
    if (! mh_exp_defun_arg_percolate_p (args [args_index]))
      new_args [new_args_index++] = args [args_index];

  /* Copy NEW_ARGS into the to-be-modified ARGS */
  for (args_index = 0; args_index < args_count; args_index++)
    args [args_index] = new_args [args_index];

  free (new_args);
}

/* Ignore the keywords that are allowed to occur in DEFUN:
 * whitespace, ... */
static mh_argument_t *
mh_exp_defun_munge_args (mh_parse_t    args,
			 unsigned int *args_vector_count)
{
  /* boolean_t in_req = true; */
  boolean_t in_opt = false;
  boolean_t in_key = false;
  boolean_t in_rst = false;
  boolean_t in_bdy = false;
  boolean_t in_att = false;

  boolean_t in_uneval = false;

  mh_argument_t *args_vector;

  if (! args)
    {
      *args_vector_count = 0;
      return (mh_argument_t *) NULL;
    }

  args_vector = (mh_argument_t *) xcalloc
    (mh_parse_list_count (MH_PARSE_LIST_TAIL (args)),
     sizeof (mh_argument_t));

  args = mh_parse_list_delete_key (args, "whitespace");
  args = mh_parse_list_delete_key (args, "package");

  *args_vector_count = 0;

  for (; args != MH_PARSE_EMPTY; args = MH_PARSE_LIST_TAIL (args))
    {
      mh_parse_t arg   = MH_PARSE_LIST_HEAD (args);
      
      mh_argument_array_t arg_array_type;
      mh_token_t          arg_token = 
	mh_exp_defun_get_arg (arg, & arg_array_type);

      if (NULL != arg_token)
	{
	  /* ARG_ARRAY_TYPE is now valid */
#define mh_arg_is_string_p( string )			\
  mh_token_match_p (arg_token, MH_TOKEN_WORD, string)

	  /* Ought to negate others.  Ought to check duplicates */

	  if (mh_arg_is_string_p ("&unevalled"))
	    in_uneval = true;
	  else if (mh_arg_is_string_p ("&optional"))
	    in_opt = true;
	  else if (mh_arg_is_string_p ("&key"))
	    in_key = true;
	  else if (mh_arg_is_string_p ("&rest"))
	    in_rst = true;
	  else if (mh_arg_is_string_p ("&body"))
	    in_bdy = true;
	  else if (mh_arg_is_string_p ("&attributes"))
	    in_att = true;
	  else
	    {
	      string_t      name = mh_token_string (arg_token);

	      mh_argument_t argument =
		mh_argument_new (name,
				 (in_att == true
				  ? MH_ARGUMENT_ATTRIBUTES
				  : (in_bdy == true
				     ? MH_ARGUMENT_BODY
				     : (in_rst == true
					? MH_ARGUMENT_REST
					: (in_key == true
					   ? MH_ARGUMENT_KEY
					   : (in_opt == true
					      ? MH_ARGUMENT_OPTIONAL
					      : MH_ARGUMENT_REQUIRED))))),
				 ((in_uneval == true || in_att == true)
				  ? MH_ARGUMENT_UNEVALLED
				  : MH_ARGUMENT_EVALLED),
				 arg_array_type);

	      args_vector [(*args_vector_count)++] = argument;

	      in_uneval = false;
	    }
#undef mh_arg_is_string_p
	}
      else
	{
	  mh_exp_exception
	    (mh_exp_complex_tag_used_as_simple,
	     arg,
	     "Argument not a name");
	  /* error */
	}
    }

  mh_exp_defun_sort_args (args_vector, *args_vector_count);

  return args_vector;
}

#if defined (NOT_USED)
static boolean_t
mh_exp_defun_args_has_array_p (mh_argument_t *args,
			       unsigned int   args_count)
{
  unsigned int indx = 0;

  for (; indx < args_count; indx++)
    if (MH_ARGUMENT_ARRAY == MH_ARGUMENT_ARRAY (args[indx]))
      return true;
  return false;
}

static mh_core_t
mh_exp_defun_args_array_init (mh_argument_t *args,
			      unsigned int   args_count)
{
  unsigned int indx = 0;
  unsigned int cores_count = 0;
  mh_core_t   *cores;

  for (; indx < args_count; indx++)
    if (MH_ARGUMENT_ARRAY == MH_ARGUMENT_ARRAY (args[indx]))
      cores_count++;

  cores = (mh_core_t *) xmalloc (cores_count * sizeof (mh_core_t));

  cores_count = 0;
  for (indx = 0; indx < args_count; indx++)
    {
      mh_argument_t arg = args[indx];

      if (MH_ARGUMENT_ARRAY == MH_ARGUMENT_ARRAY (arg))
	{
	  mh_core_t dat = mh_core_data_new (MH_ARGUMENT_NAME (arg));
	  cores[cores_count++] = mh_core_set_1_new
	    (mh_core_array_new (dat, NULL),
	     mh_core_get_new (mh_core_dup (dat)));
	}
    }

  return mh_core_fmt_new (cores, cores_count);
}
#endif /* defined (NOT_USED) */

static mh_core_t
mh_exp_def_internal (EXP_ARGS_DEF)
{
  mh_parse_t name_parse;

  /* What about non-top-level function definitions */

  switch (count)
    {
    case 0:
      /* <defun> ... </defun> */
      mh_exp_exception
	(mh_exp_function_argument,
	 MH_PARSE_BLK_OPEN (parse),
	 "DEFUN without a tag name");

      return mh_core_fmt_new
	(mh_exp_sequence_3 (MH_PARSE_BLK_OPEN  (parse),
			    MH_PARSE_BLK_BODY  (parse),
			    MH_PARSE_BLK_CLOSE (parse),
			    env),
	 3);

    default:
      name_parse = MH_PARSE_LIST_HEAD (operands);

      if (! mh_parse_type_p (name_parse, MH_PARSE_TYPE_TOKEN))
	{
	  mh_exp_exception
	    (mh_exp_tag_operator_not_a_name,
	     name_parse,
	     "TAG operator must be a name");
	}

      {
	string_t  name          = 
	  mh_token_to_string (MH_PARSE_TOKEN (name_parse));
	boolean_t complex_p     = false;
	boolean_t weak_p        = false;

	mh_parse_t package_key    =
	  mh_parse_match_key (operands, "package");

	/* "delete" or "keep" only */
	mh_parse_t whitespace_key = 
	  mh_parse_match_key (operands, "whitespace");

	mh_white_type_t whitespace  = 
	  mh_exp_defun_get_whitespace (whitespace_key, 
				       MH_TAG_WHITESPACE (tag));

	/* Strip whitespace, optionally in BODY as well */
	string_t  fbody         = mh_parse_to_string (body);
	string_t  packname      = 
	  mh_exp_defun_get_packname (package_key, NULL);
	string_t  documentation = NULL;

	unsigned int   args_count;

	mh_argument_t *args =
	  mh_exp_defun_munge_args (MH_PARSE_LIST_TAIL (operands),
				   & args_count);
	
	/* Note in the following that we are creating a SIMPLE TAG and thus
	   the whitespace parameter is MH_WHITE_DELETE.  This is the
	   default for all simple tags.  */
	mh_tag_t func_tag = mh_tag_new
	  (name, TAG_TYPE_DEFUN,
	   complex_p,
	   weak_p,
	   MH_WHITE_DELETE,
	   fbody,
	   packname,
	   documentation,
	   args,
	   args_count);

	/* Add two environments; one for WHITESPACE; one for NAME */
	mh_env_t body_env = mh_env_extend
	  (mh_env_extend_white (env, whitespace),
	   name,
	   func_tag);

	mh_core_t  body_core;
	mh_parse_t body_copy = body;

	if (body && mh_parse_type_p (body, MH_PARSE_TYPE_LIST))
	  {
	    body_copy = mh_parse_list_copy
	      (mh_parse_list_trim_whitespace (body_copy));

	    if (MH_WHITE_DELETE == whitespace)
	      body_copy = mh_parse_list_delete_interline_space (body_copy);
	  }

	body_core = (body_copy != NULL
		     ? mh_exp (body_copy, body_env)
		     : mh_core_empty ());

#if defined (NEVER_DEFINED) /* Happens in the caller */
	if (mh_exp_defun_args_has_array_p (args, args_count))
	  body_core = mh_core_fmt_2_new
	    (mh_exp_defun_args_array_init (args, args_count),
	     body_core);
#endif

#if 0
	if (body_copy != body)
	  mh_parse_list_free (body_copy);
#endif

	mh_env_free_special (MH_ENV_NEXT (body_env), false);
	mh_env_free_special (body_env, false);

	/* Free ARGS */

	return mh_core_func_new (func_tag, body_core);
      }
    }
}

DEFINE_EXPANDER ("defun", mh_exp_defun,
"This is <DEFUN name arg ...><body>*</defun>")
{
  mh_core_t core;
 
  /* By default DEFUN uses MH_WHITE_DELETE */
  env  = mh_env_extend_white (env, MH_WHITE_DELETE);

  core = mh_exp_def_internal (EXP_ARGS_APP);
 
  /* Recover a tiny bit of memory */
  mh_env_free_special (env, false);

  return core;
}

DEFINE_EXPANDER ("defsubst", mh_exp_defsubst,
"This is <DEFSUBST name arg ...><body>*</defsubst>")
{
  mh_core_t core;
  mh_tag_t  func;

  /* Any DEFSUBST that contains $n parameters anywhere won't be compiled */
  if (mh_parse_has_positional_reference_p (body))
    return mh_exp_as_extern (parse, env);

  /* By default DEFSUBST uses MH_WHITE_KEEP */
  env  = mh_env_extend_white (env, MH_WHITE_KEEP);

  core = mh_exp_def_internal (EXP_ARGS_APP);
  func = MH_CORE_FUNC_TAG (core);

  /* Recover a tiny bit of memory */
  mh_env_free_special (env, false);

  /* Critical difference between DEFSUBST and DEFUN */
  MH_TAG_CURRENT_PACKAGE_P (func) = true;

  return core;
}

DEFINE_EXPANDER ("defmacro", mh_exp_defmacro, "")
{
  return mh_exp_as_extern (parse, env);
}

/*****************************************************************************
 *
 * Special Form - BLOCK
 *
 */
DEFINE_EXPANDER ("block", mh_exp_block,
"This is <block><body>*</block>")
{
  mh_parse_t p_open = MH_PARSE_BLK_OPEN  (parse);
  mh_parse_t p_body = MH_PARSE_BLK_BODY  (parse);
  mh_parse_t p_clos = MH_PARSE_BLK_CLOSE (parse);

  /* Transform BODY into an anonymous function in the default package
     and invoke that function through an external call for the complex
     tag block  */

  string_t func_name = mhtml_make_identifier (NULL, 16);

  /* 1: Create an anonymous TAG */
  mh_tag_t func_tag = mh_tag_new
    (func_name, TAG_TYPE_DEFUN,
     false,			/* complex_p */
     false,			/* weak_p */
     false,			/* whitespace_p */
     "",			/* fbody */
     NULL,			/* packname */
     "",			/* documentation */
     NULL,
     0);

  /* 2: Compile BODY */
  mh_core_t body_core = mh_exp (p_body, env);

  /* 3: Produce a core FUNC */
  mh_core_t func_core = mh_core_func_new (func_tag, body_core);

  mh_core_t blk_core;

  /* 4: Produce an EXTERN for BLOCK */
  {
    string_t str_open = 
      mh_parse_to_string_with_space (MH_PARSE_TAG_BODY (p_open));
    string_t str_clos = mh_parse_to_string (p_clos);

    string_t str_extern = (string_t) xmalloc
      (1 + 1 + strlen (str_open) + 1 + 
       1 + strlen (func_name) + 
       1 + strlen (str_clos));

    sprintf (str_extern, "<%s><%s>%s", str_open, func_name, str_clos);

    blk_core = mh_core_extern_new (str_extern);

    xfree (str_open);
    xfree (str_clos);
  }

  /* Don't forget this! */
  MH_TAG_CURRENT_PACKAGE_P (func_tag) = true;

  /* 5: Return a FMT with FUNC and EXTERN */
  return mh_core_fmt_2_new (func_core, blk_core);
}

/*****************************************************************************
 *
 * Special Form - DB_QUERY
 *
 */
DEFINE_EXPANDER ("sql::database-query", mh_exp_db_query,
"This is <sql::database-query db bool format=exp query>")
{
  switch (count)
    {
    case 4:
      {
	/* mh_parse_t p_db    = mh_parse_list_nth_nonkey (operands, 0); */
	/* mh_parse_t p_flag  = mh_parse_list_nth_nonkey (operands, 1); */
	/* mh_parse_t p_query = mh_parse_list_nth_nonkey (operands, 2); */
	mh_parse_t p_fmt   = mh_parse_match_key (operands, "format");

	if (MH_PARSE_EMPTY == p_fmt)
	  return mh_exp_as_extern (parse, env);

	/* Transform P_FMT into an anonymous function call */
	{
	  /*  mh_parse_t p_fmt_name  = MH_PARSE_KEY_NAME  (p_fmt); */
	  mh_parse_t p_fmt_value = MH_PARSE_KEY_VALUE (p_fmt);

	  string_t func_name = mhtml_make_identifier (NULL, 16);

	  /* 1: Create an anonymous TAG */
	  mh_tag_t func_tag = mh_tag_new
	    (func_name, TAG_TYPE_DEFUN,
	     false,		/* complex_p */
	     false,		/* weak_p */
	     false,		/* whitespace_p */
	     "",		/* fbody */
	     NULL,		/* packname */
	     "",		/* documentation */
	     NULL,
	     0);

	  /* 2: Compile FMT_VALUE */
	  mh_core_t fmt_value_core = mh_exp (p_fmt_value, env);

	  /* 3: Produce a core FUNC */
	  mh_core_t func_core = mh_core_func_new (func_tag, fmt_value_core);

	  mh_core_t tag_core;

	  /* 4: Produce an EXTERN for TAG */
	  {
	    string_t str_tag;

	    string_t str_fmt = (string_t) xmalloc
	      (1 + 2 + strlen (func_name));

	    sprintf (str_fmt, "<%s>", func_name);

	    MH_PARSE_KEY_VALUE (p_fmt) =
	      mh_parse ("SQL::DATABASE-QUERY", str_fmt);

	    str_tag = mh_parse_to_string_with_space (parse);
	    
	    tag_core = mh_core_extern_new (str_tag);

	    xfree (str_tag);
	    xfree (str_fmt);
	  }

	  /* Don't forget this! */
	  MH_TAG_CURRENT_PACKAGE_P (func_tag) = true;

	  /* 5: Return a FMT with FUNC and EXTERN */
	  return mh_core_fmt_2_new (func_core, tag_core);
	}
      }
      break;

    default:
      return mh_exp_as_extern (parse, env);
    }
}

/*****************************************************************************
 *
 *
 *
 */
static mh_core_t
mh_exp_crement (mh_parse_t   parse,
		mh_parse_t   operands,
		unsigned int count,
		mh_env_t     env,
		mh_byte_op_t primop)
{
  switch (count)
    {
    case 0:
      return mh_core_data_new ("");

    case 1:
    case 2:
      {
	mh_parse_t var   = mh_parse_list_nth  (operands, 0);
	mh_parse_t by = mh_parse_match_key (operands, "by");

	return mh_core_set_1_new
	  (mh_exp (var, env),
	   (MH_PARSE_EMPTY == by
	    ? mh_core_prim_1_new (primop, mh_exp_as_get (var, env))
	    : mh_core_prim_2_new (primop == MH_DEC_OP ? MH_SUB_OP : MH_ADD_OP,
				  mh_exp_as_get (var, env),
				  mh_exp (MH_PARSE_KEY_VALUE (by),
					  env))));
      }
    default:
      return mh_exp_as_extern (parse, env);
    }
}

DEFINE_EXPANDER ("decrement", mh_exp_decrement,
"This is <decrement x> ...")
{
  return mh_exp_crement (parse, operands, count, env, MH_DEC_OP);
}

DEFINE_EXPANDER ("increment", mh_exp_increment,
"This is <increment x> ...")
{
  return mh_exp_crement (parse, operands, count, env, MH_INC_OP);
}

static boolean_t
mh_exp_get_caseless_p (mh_parse_t operands,
		       boolean_t  default_value)
{
  mh_parse_t value,
    key = mh_parse_match_key (operands, "caseless");

  if (MH_PARSE_EMPTY == key || ! mh_parse_type_p (key, MH_PARSE_TYPE_KEY))
    return default_value;

  value = MH_PARSE_KEY_VALUE (key);

  return mh_parse_token_match_p (value, MH_TOKEN_WORD, "true")
    ? true
    : default_value;
}

static mh_core_t
mh_exp_if_cmp (mh_parse_t   parse,
	       mh_parse_t   operands,
	       unsigned int count,
	       mh_env_t     env,
	       boolean_t    is_eq_p)

{
  switch (count)
    {
    case 0:
    case 1:
    case 2:
      return mh_core_data_new ("");

    case 3:
    case 4:
    default:			/* Ignore everything beyond four */
      {
	mh_parse_t pred1 = mh_parse_list_nth (operands, 0);
	mh_parse_t pred2 = mh_parse_list_nth (operands, 1);
	mh_parse_t cons  = mh_parse_list_nth (operands, 2);
	mh_parse_t alt   = count == 4
	  ? mh_parse_list_nth (operands, 3)
	  : MH_PARSE_EMPTY;

	return mh_core_if_new
	  (mh_core_prim_3_new
	   (is_eq_p ? MH_STR_EQ_OP : MH_STR_NEQ_OP,
	    mh_exp (pred1, env),
	    mh_exp (pred2, env),
	    mh_core_data_new (mh_exp_get_caseless_p (operands, false)
			      ? "true"
			      : "")),
	   mh_exp (cons, env),
	   mh_exp (alt,  env));
      }
    }
}

DEFINE_EXPANDER ("ifeq", mh_exp_ifeq,
"This is <ifeq a b c d> ...")
{
  return mh_exp_if_cmp (parse, operands, count, env, true);
}

DEFINE_EXPANDER ("ifneq", mh_exp_ifneq,
"This is <ifneq a b c d> ...")
{
  return mh_exp_if_cmp (parse, operands, count, env, false);
}

static mh_core_t 
mh_exp_var_unset_one (mh_parse_t form,
		      mh_env_t   env)
{
  return mh_core_prim_1_new
    (MH_VAR_UNSET_OP,
     mh_exp (form, env));
}

DEFINE_EXPANDER ("unset-var", mh_exp_var_unset,
"This is <unset-var &optional name ...>")
{
  switch (count)
    {
    case 0:
      return mh_core_empty ();

    case 1:
      return mh_exp_var_unset_one (MH_PARSE_LIST_HEAD (operands), env);

    default:
      /* Expand as OR */
      return mh_core_fmt_new
	(mh_exp_sequence_ander (operands, env, mh_exp_var_unset_one),
	 count);
    }
}

DEFINE_EXPANDER ("var-exists", mh_exp_var_exists,
"This is <var-exists name>")
{
  switch (count)
    {
    case 0:
      return mh_core_empty ();

    case 1:
      return mh_core_prim_1_new
	(MH_VAR_EXISTS_OP,
	 mh_exp (MH_PARSE_LIST_HEAD (operands), env));

    default:
      return mh_exp_as_extern (parse, env);
    }
}

DEFINE_EXPANDER ("defvar", mh_exp_def_var,
"This is <defvar name value>")
{
  switch (count)
    {
    case 0:
    case 1: 
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in DEFVAR");
      return mh_core_empty ();

    case 2:
      {
	mh_parse_t name  = mh_parse_list_nth (operands, 0);
	mh_parse_t value = mh_parse_list_nth (operands, 1);

	/*
	 * <if <not <get-var-once name>>
	 *     <set-var name = value>>
	 */
	return mh_core_if_new
	  (mh_exp_as_get (name, env),
	   mh_core_data_new (""),
	   mh_core_set_1_new (mh_exp (name,  env),
			      mh_exp (value, env)));
      }

    default:
      return mh_exp_as_extern (parse, env);
    }
}

/**************************************************************************
 *
 *
 *
 */
static mh_core_t
mh_exp_math_binop_iterator (mh_byte_op_t prim_op,
			    mh_core_t    core,
			    mh_parse_t   operands,
			    unsigned int count,
			    mh_env_t     env)
{
  if (0 == count)
    return core;
  else
    {
      mh_core_t arg1 = core;
      mh_core_t arg2 = 
	mh_exp_as_number (MH_PARSE_LIST_HEAD (operands), env);

      mh_core_t core_extended =
	mh_core_prim_2_new (prim_op, arg1, arg2);

      return mh_exp_math_binop_iterator
	(prim_op,
	 core_extended, 
	 MH_PARSE_LIST_TAIL (operands),
	 count - 1,
	 env);
    }
}

static mh_core_t
mh_exp_math_binop (mh_byte_op_t prim_op,
		   string_t     identity,
		   mh_parse_t   operands,
		   unsigned int count,
		   mh_env_t     env)
{
  /* <tag>               ==> identity
     <tag arg>           ==> <prim identity arg>
     <tag arg1 arg2>     ==> <prim arg1 arg2)
     <tag arg1 arg2 ...> ==> <tag <prim arg1 arg2> ...> */
  switch (count)
    {
    case 0: return mh_core_data_new (identity);
    case 1: return mh_exp_math_binop_iterator
	      (prim_op, 
	       mh_core_data_new (identity),
	       operands,
	       count,
	       env);
    default: return mh_exp_math_binop_iterator
	       (prim_op, 
		mh_exp_as_number (MH_PARSE_LIST_HEAD (operands), env),
		MH_PARSE_LIST_TAIL (operands),
		count - 1,
		env);
    }
}

DEFINE_EXPANDER ("add", mh_exp_math_add,
"This is <add ...>")
{
  return mh_exp_math_binop (MH_ADD_OP, "0", operands, count, env);
}

DEFINE_EXPANDER ("sub", mh_exp_math_sub,
"This is <sub ...>")
{
  return mh_exp_math_binop (MH_SUB_OP, "0", operands, count, env);
}

DEFINE_EXPANDER ("mul", mh_exp_math_mul,
"This is <mul ...>")
{
  return mh_exp_math_binop (MH_MUL_OP, "1", operands, count, env);
}

DEFINE_EXPANDER ("div", mh_exp_math_div,
"This is <div ...>")
{
  if (0 == count)
    return mh_core_data_new ("<DIV>");
  else if (mh_parse_list_any (operands, 
			      (mh_parse_tester_t) mh_parse_type_p, 
			      (mh_parse_maparg_t) MH_PARSE_TYPE_KEY))
    return mh_exp_as_extern (parse, env);
  else
    return mh_exp_math_binop (MH_DIV_OP, "1", operands, count, env);
}

static mh_core_t
mh_exp_math_relop (mh_byte_op_t prim_op,
		   mh_parse_t   operands,
		   unsigned int count,
		   mh_env_t     env)
{
  /* <lt>               ==> true
     <lt arg>           ==> true
     <lt arg1 arg2>     ==> <prim arg1 arg2>
     <lt arg1 arg2 ...> ==> <if <prim arg1 arg2> <lt arg2 ...>> */
  switch (count)
    {
    case 0:
    case 1:
      return mh_core_data_new ("true");

    case 2:
      return mh_core_prim_new
	(prim_op,
	 mh_exp_sequence_ander (operands, env, mh_exp_as_number),
	 count);

    default:
      {
	mh_parse_t parse1 = mh_parse_list_nth (operands, 0);
	mh_parse_t parse2 = mh_parse_list_nth (operands, 1);

	mh_core_t pred = mh_core_prim_2_new
	  (prim_op,
	   mh_exp_as_number (parse1, env),
	   mh_exp_as_number (parse2, env));

	return mh_core_if_new
	  (pred,
	   mh_exp_math_relop (prim_op,
			      MH_PARSE_LIST_TAIL (operands),
			      count - 1,
			      env),
	   mh_core_data_new (""));
      }
    }
}

DEFINE_EXPANDER ("eq", mh_exp_math_eq,
"This is <eq ...>")
{
  return mh_exp_math_relop (MH_EQ_OP, operands, count, env);
}

DEFINE_EXPANDER ("neq", mh_exp_math_neq,
"This is <neq ...>")
{
  return mh_exp_math_relop (MH_NE_OP, operands, count, env);
}

DEFINE_EXPANDER ("lt", mh_exp_math_lt,
"This is <lt ...>")
{
  return mh_exp_math_relop (MH_LT_OP, operands, count, env);
}

DEFINE_EXPANDER ("le", mh_exp_math_le,
"This is <le ...>")
{
  return mh_exp_math_relop (MH_LE_OP, operands, count, env);
}

DEFINE_EXPANDER ("gt", mh_exp_math_gt,
"This is <gt ...>")
{
  return mh_exp_math_relop (MH_GT_OP, operands, count, env);
}

DEFINE_EXPANDER ("ge", mh_exp_math_ge,
"This is <ge ...>")
{
  return mh_exp_math_relop (MH_GE_OP, operands, count, env);
}

DEFINE_EXPANDER ("random", mh_exp_math_random,
"This is <random ...>")
{
  mh_parse_t limit = (count >= 1
		      ? mh_parse_list_nth (operands, 0)
		      : MH_PARSE_EMPTY);

  return mh_core_prim_1_new
    (MH_RANDOM_OP,
     (limit != MH_PARSE_EMPTY
      ? mh_exp_as_number (limit, env)
      : mh_core_data_new ("0")));
}

DEFINE_EXPANDER ("integer?", mh_exp_math_integer_p,
"This is <integer ...>")
{
  switch (count)
    {
    case 0:  return mh_core_data_new ("");
    case 1:  return mh_exp_as_math (EXP_ARGS_APP);
    default: return mh_exp_as_extern (parse, env) ;     
    }
}


DEFINE_EXPANDER ("real?", mh_exp_math_real_p,
"This is <real ...>")
{
  switch (count)
    {
    case 0:  return mh_core_data_new ("");
    case 1:  return mh_exp_as_math (EXP_ARGS_APP);
    default: return mh_exp_as_extern (parse, env) ;     
    }
}

static mh_core_t
mh_exp_alist_make_key (mh_parse_t key,
		       mh_env_t   env)
{
  if (! mh_parse_type_p (key,  MH_PARSE_TYPE_KEY))
    {
      mh_exp_exception
	(mh_exp_set_var_requires_keys,
	 key,
	 "ALIST-SET-VAR requires all keys");
      return mh_core_key_new
	(mh_core_data_new (""),
	 mh_core_data_new (""));
    }
  else
    {
      mh_parse_t key_name, key_value;

      key_name = mh_parse_clean_and_normalize
	(MH_PARSE_KEY_NAME (key));

      key_value = mh_parse_clean_and_normalize
	(MH_PARSE_KEY_VALUE (key));

      /* Check for a KEY that use ARRAY syntax as:
       *   <alist-set-var alist key_name[]=key_value> */
      if (mh_parse_type_p (key_name, MH_PARSE_TYPE_ARRAY))
	{
	  mh_parse_t key_name_array = mh_parse_clean_and_normalize
	    (MH_PARSE_ARRAY_NAME (key_name));

	  mh_parse_t key_name_index = MH_PARSE_ARRAY_INDEX (key_name)
	    ? mh_parse_clean_and_normalize (MH_PARSE_ARRAY_INDEX (key_name))
	    : MH_PARSE_EMPTY;

	  return mh_core_key_new
	    (mh_exp (key_name_array, env),
	     (MH_PARSE_EMPTY == key_name_index
	      ? mh_core_prim_1_new (MH_ARRAY_FORCE_OP,
				    mh_exp (key_value, env))
	      : mh_core_prim_2_new (MH_ARRAY_NEW_AT_OP,
				    mh_exp (key_name_index, env),
				    mh_exp (key_value, env))));
	}
      else
	return mh_exp (key, env);
    }
}

DEFINE_EXPANDER ("make-alist", mh_exp_alist_make,
"This is <make-alist ...> ...")
{
  return mh_core_prim_new
    (MH_ALIST_OP,
     mh_exp_sequence_ander (operands, env, mh_exp_alist_make_key),
     count);
}

DEFINE_EXPANDER ("alist-get-var", mh_exp_alist_get_var,
"This is <alist-get-var alist name> ...")
{
  switch (count)
    {
    case 0:
    case 1:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in ALIST-GET-VAR");
      return mh_core_empty ();
      
    case 2:
      {
	mh_parse_t alist = mh_parse_list_nth (operands, 0);
	mh_parse_t label = mh_parse_list_nth (operands, 1);

	/* If NAME has ARRAY syntax then do a little dance */
	if (mh_parse_type_p (label, MH_PARSE_TYPE_ARRAY))
	  {
	    mh_parse_t name = mh_parse_clean_and_normalize
	      (MH_PARSE_ARRAY_NAME (label));

	    mh_parse_t indx = MH_PARSE_ARRAY_INDEX (label)
	      ? mh_parse_clean_and_normalize (MH_PARSE_ARRAY_INDEX (label))
	      : MH_PARSE_EMPTY;

	    return indx == MH_PARSE_EMPTY
	      ? mh_core_prim_2_new (MH_ALIST_GET_RAW_OP,
				   mh_exp_as_get (alist, env),
				   mh_exp (name, env))
	      : (mh_core_prim_2_new
		 (MH_ARRAY_REF_OP,
		  mh_core_prim_2_new (MH_ALIST_GET_RAW_OP,
				      mh_exp_as_get (alist, env),
				      mh_exp (name, env)),
		  mh_exp_as_number (indx, env)));
	  }
	else
	  return mh_core_prim_2_new
	    (MH_ALIST_GET_OP,
	     mh_exp_as_get (alist, env),
	     mh_exp (label, env));
      }
    default:
      return mh_exp_as_extern (parse, env);
    }
}

static mh_core_t
mh_exp_alist_set_var_recurse (mh_parse_t parse,
			      mh_parse_t alist,
			      mh_parse_t keys,
			      mh_env_t   env)
{
  mh_parse_t key, rest;
  mh_parse_t key_name, key_value;
  mh_core_t  set_core, key_core;

  /* Assumes count is correct and even */
  if (MH_PARSE_EMPTY == keys) return mh_core_empty ();

  key  = MH_PARSE_LIST_HEAD (keys);
  rest = MH_PARSE_LIST_TAIL (keys);

  if (! mh_parse_type_p (key,  MH_PARSE_TYPE_KEY))
    {
      mh_exp_exception
	(mh_exp_set_var_requires_keys,
	 parse,
	 "ALIST-SET-VAR requires all keys");
      set_core = mh_core_empty ();
    }
  else
    {
      key_name = mh_parse_clean_and_normalize
	(MH_PARSE_KEY_NAME (key));

      key_value = mh_parse_clean_and_normalize
	(MH_PARSE_KEY_VALUE (key));

      /* Check for a KEY that use ARRAY syntax as:
       *   <alist-set-var alist key_name[]=key_value> */
      if (mh_parse_type_p (key_name, MH_PARSE_TYPE_ARRAY))
	{
	  mh_parse_t key_name_array = mh_parse_clean_and_normalize
	    (MH_PARSE_ARRAY_NAME (key_name));

	  mh_parse_t key_name_index = MH_PARSE_ARRAY_INDEX (key_name)
	    ? mh_parse_clean_and_normalize (MH_PARSE_ARRAY_INDEX (key_name))
	    : MH_PARSE_EMPTY;

	  /* Expand <ALIST-SET-VAR ALIST REF[N]=VAL> into core of
	   *   {SET alist =
	   *     {PRIM mh_alist_set_op alist REF = 
	   *       {WITH {SET AR = {PRIM mh_alist_get_var_raw alist ref}}
	   *         {FMT {SET AR[N] = VAL}
	   *              {GET AR[]}}}}}
	   */

	  mh_core_t key_value_core = mh_core_prim_4_new
	    (MH_ALIST_MOD_OP,
	     mh_exp_as_get (alist, env),
	     mh_exp (key_name_array, env),
	     mh_exp (key_name_index, env),
	     mh_exp (key_value, env));
		
	  key_core = mh_core_key_new
	    (mh_exp (key_name_array, env),
	     key_value_core);
	}
      else
	key_core = mh_exp (key, env);

      set_core = mh_core_set_1_new
	(mh_exp (alist, env), 
	 mh_core_prim_2_new (MH_ALIST_SET_OP,
			     mh_exp_as_get (alist, env),
			     key_core));
     
    }    
  
  return MH_PARSE_EMPTY == rest
    ? set_core
    : mh_core_fmt_2_new (set_core,
			 (mh_exp_alist_set_var_recurse
			  (parse, alist, rest, env)));
}
  
DEFINE_EXPANDER ("alist-set-var", mh_exp_alist_set_var,
"This is <alist-set-var ...> ...")
{
  switch (count)
    {
    case 0:
    case 1:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in ALIST-SET-VAR");
      return mh_core_empty ();

    default:
      return mh_exp_alist_set_var_recurse
	(parse,
	 mh_parse_list_nth  (operands, 0),
	 mh_parse_list_rest (operands, 1),
	 env);
    }
}

DEFINE_EXPANDER ("alist-defvar", mh_exp_alist_def_var,
"This is <alist-defvar alist key value>")
{
  switch (count)
    {
    case 0:
    case 1:
    case 2:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in ALIST-DEFVAR");
      return mh_core_empty ();

    case 3:
      {
	mh_parse_t alist = mh_parse_list_nth (operands, 0);
	mh_parse_t name  = mh_parse_list_nth (operands, 1);
	mh_parse_t value = mh_parse_list_nth (operands, 2);

	/*
	 * <if <not <alist-has-value alist name>>
	 *     <alist-set-var alist name = value>>
	 */
	mh_core_t pred = mh_core_prim_2_new
	  (MH_ALIST_GET_OP,
	   mh_exp_as_get (alist, env),
	   mh_exp (name, env));
	
	mh_core_t aset = mh_core_prim_2_new
	  (MH_ALIST_SET_OP,
	   mh_exp_as_get (alist, env),
	   mh_core_key_new (mh_exp (name,  env),
			    mh_exp (value, env)));

	mh_core_t asgn = mh_core_set_1_new
	  (mh_exp (alist, env),
	   aset);

	return mh_core_if_new
	  (pred,
	   mh_core_data_new (""),
	   asgn);
      }

    default:
      return mh_exp_as_extern (parse, env);
    }
}

DEFINE_EXPANDER ("alist-var-exists", mh_exp_alist_has_var,
"This is <alist-var-exists ...> ...")
{
  switch (count)
    {
    case 0:
    case 1:
      return mh_core_empty ();

    case 2:
      {
	mh_parse_t alist = mh_parse_list_nth (operands, 0);
	mh_parse_t name  = mh_parse_list_nth (operands, 1);

	/* Work hard to evaluate ALIST; less so for NAME */
	return mh_core_prim_2_new
	  (MH_ALIST_HAS_OP,
	   mh_exp_as_get (alist, env),
	   mh_exp (name, env));
      }
    default:
      return mh_exp_as_extern (parse, env);
    }
}

DEFINE_EXPANDER ("alist-unset-var", mh_exp_alist_unset_var,
"This is <alist-unset-var ...> ...")
{
  switch (count)
    {
    case 0:
    case 1:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in ALIST-UNSET-VAR");
      return mh_core_empty ();

    default:
      {
	mh_parse_t alist = mh_parse_list_nth  (operands, 0);
	mh_parse_t keys  = mh_parse_list_rest (operands, 1);

	unsigned int cores_count = mh_parse_count (keys);

	mh_core_t *cores = 
	  (mh_core_t *) xmalloc (cores_count * sizeof (mh_core_t));
	mh_core_t *cores_result = cores;

	for (; keys; keys = MH_PARSE_LIST_TAIL (keys))
	  {
	    mh_parse_t key = MH_PARSE_LIST_HEAD (keys);
	    /* <set-var alist=<%%alist-rem-var alist key>> */
	    *cores++ = mh_core_set_1_new
	      (mh_exp (alist, env),
	       mh_core_prim_2_new (MH_ALIST_REM_OP,
				   mh_exp_as_get (alist, env),
				   mh_exp (key, env)));
	  }

	return mh_core_fmt_new (cores_result, cores_count);
      }
    }
}


#if defined (NOT_YET)
DEFINE_EXPANDER ("alist-merge", mh_exp_alist_merge,
"This is <alist-merge ...> ...")
{
  switch (count)
    {
    case 0:
    case 1: 
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in ALIST-MERGE");
      return mh_core_empty ();

    case 2:
      {
	mh_parse_t alist1 = mh_parse_list_nth (operands, 0);
	mh_parse_t alist2 = mh_parse_list_nth (operands, 1);
	
	/* <set-var alist=<%%alist-rem-var alist name>> */
	return mh_core_set_1_new
	  (mh_exp (alist1, env),
	   mh_core_prim_2_new (MH_ALIST_MERGE_OP,
			       mh_exp_as_get (alist1, env),
			       mh_exp_as_get (alist2, env)));
      }

    default:
      mh_exp_exception
	(mh_exp_excessive_arguments,
	 parse,
	 "Excessive arguments in ALIST-MERGE");
      return mh_exp_as_extern (parse, env);
    }
}
#endif

DEFINE_EXPANDER ("package-to-alist", mh_exp_package_to_alist,
"This is <package-to-alist packname strip> ...")
{

  switch (count)
    {
    case 0:
    case 1:
    case 2:
      {
	mh_parse_t pack = count > 0
	  ? mh_parse_list_nth_nonkey (operands, 0)
	  : MH_PARSE_EMPTY;

	mh_parse_t strip     = mh_parse_match_key (operands, "strip");
	mh_core_t  core_pack = mh_exp (pack, env);

	if (MH_PARSE_EMPTY != strip)
	  {
	    mh_parse_t strip_val = MH_PARSE_KEY_VALUE (strip);

	    return mh_core_prim_2_new (MH_PACKAGE_TO_ALIST_OP,
				       core_pack,
				       mh_exp (strip_val, env));
	  }
	else
	  return mh_core_prim_2_new (MH_PACKAGE_TO_ALIST_OP,
				     core_pack, mh_core_data_new (""));
      }
    default:
      return mh_exp_as_extern (parse, env);
    }
}


DEFINE_EXPANDER ("alist-to-package", mh_exp_alist_to_package,
"This is <alist-to-package alist packname> ...")
{
  return mh_exp_as_extern (parse, env);
}

static mh_core_t
mh_exp_str_comp_internal (mh_parse_t   parse,
			  mh_env_t     env,
			  mh_parse_t   operands,
			  unsigned int count,
			  boolean_t    is_eq_p)
{
  switch (count)
    {
    case 0:
    case 1:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in STRING-{EQ,NEQ}");
      return mh_core_empty ();
      
    case 2:
    case 3:
      {
	mh_parse_t str1 = mh_parse_list_nth (operands, 0);
	mh_parse_t str2 = mh_parse_list_nth (operands, 1);

	return mh_core_prim_3_new
	  (is_eq_p ? MH_STR_EQ_OP : MH_STR_NEQ_OP,
	   mh_exp (str1, env),
	   mh_exp (str2, env),
	   mh_core_data_new (mh_exp_get_caseless_p (operands, false)
			     ? "true"
			     : ""));
      }

    default:
      return mh_exp_as_extern (parse, env);
    }
}


DEFINE_EXPANDER ("string-eq", mh_exp_str_eq,
"This is <string-eq str1 str2> ...")
{
  return mh_exp_str_comp_internal
    (parse, env, operands, count, true);
}

DEFINE_EXPANDER ("string-neq", mh_exp_str_ne,
"This is <string-neq str1 str2> ...")
{
  return mh_exp_str_comp_internal
    (parse, env, operands, count, false);
}

static mh_core_t
mh_exp_str_1_internal (mh_parse_t    parse,
		       mh_env_t      env,
		       mh_parse_t    operands,
		       unsigned int  count,
		       mh_byte_op_t  primop,
		       string_t      defstr)
{
  return 0 == count
    ? mh_core_data_new (defstr)
    /* Simply ignore anything more than one operator - outright */
    : mh_core_prim_1_new (primop,
			  mh_exp (mh_parse_list_nth (operands, 0),
				  env));
}

DEFINE_EXPANDER ("string-length", mh_exp_str_len,
"This is <string-length str> ...")
{
  return mh_exp_str_1_internal
    (parse, env, operands, count, MH_STR_LEN_OP, "0");
}


DEFINE_EXPANDER ("upcase", mh_exp_str_up,
"This is <upcase str> ...")
{
  return mh_exp_str_1_internal
    (parse, env, operands, count, MH_STR_UP_OP, "");
}

DEFINE_EXPANDER ("downcase", mh_exp_str_down,
"This is <downcase str> ...")
{
  return mh_exp_str_1_internal
    (parse, env, operands, count, MH_STR_DOWN_OP, "");
}

DEFINE_EXPANDER ("capitalize", mh_exp_str_cap,
"This is <downcase str> ...")
{
  return mh_exp_str_1_internal
    (parse, env, operands, count, MH_STR_CAP_OP, "");
}


DEFINE_EXPANDER ("make-array", mh_exp_array_make,
"This is <make-array ...> ...")
{
  return mh_core_prim_new
    (MH_ARRAY_OP,
     mh_exp_sequence (operands, env),
     count);
}

DEFINE_EXPANDER ("array-size", mh_exp_array_size,
"This is <array-size ...> ...")
{
  switch (count)
    {
    case 0:
      return mh_core_data_new ("0");

    case 1:
      {
	mh_parse_t vector = mh_parse_list_nth (operands, 0);
	
	return mh_core_prim_1_new
	  (MH_ARRAY_LEN_OP,	/* mh_exp_as_get (vector, env) */
	   mh_exp_as_get_raw (vector, env));
      }

    default:
      /* Error */
      return mh_exp_as_extern (parse, env);
    }
}

DEFINE_EXPANDER ("array-reverse", mh_exp_array_reverse,
"This is <array-reverse ...> ...")
{
  /* Should be like */
  switch (count)
    {
    case 0:
      return mh_core_empty ();

    case 1:
      {
	mh_parse_t vector = mh_parse_list_nth (operands, 0);
	
	/* <set-var vector[]=<%%array-reverse <get-var vector[]>>> */
	return mh_core_set_1_new
	  (mh_exp (vector, env),
	   mh_core_prim_1_new (MH_ARRAY_REV_OP,
			       mh_exp_as_get (vector, env)));
      }

    default:
      /* Error */
      return mh_exp_as_extern (parse, env);
    }
}


DEFINE_EXPANDER ("array-member", mh_exp_array_member,
"This is <array-member ...> ...")
{
  switch (count)
    {
    case 0:
    case 1:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in ARRAY-MEMBER");
      return mh_core_empty ();
      
    case 2:
    case 3:
      {
	mh_parse_t item = mh_parse_list_nth (operands, 0);
	mh_parse_t var  = mh_parse_list_nth (operands, 1);

	return mh_core_prim_3_new
	  (MH_ARRAY_MEM_OP,
	   mh_exp (item, env),
	   mh_exp_as_get (var, env),
	   mh_core_data_new (mh_exp_get_caseless_p (operands, false)
			     ? "true"
			     : ""));
      }

    default:
      return mh_exp_as_extern (parse, env);
    }
}


DEFINE_EXPANDER ("array-append", mh_exp_array_append,
"This is <array-append ...> ...")
{
  switch (count)
    {
    case 0:
    case 1:
      mh_exp_exception
	(mh_exp_insufficient_arguments,
	 parse,
	 "Insufficient arguments in ARRAY-APPEND");
      return mh_core_empty ();
      
    case 2:
      {
	mh_parse_t element   = mh_parse_list_nth (operands, 0);
	mh_parse_t arrayvar  = mh_parse_list_nth (operands, 1);

	/* <set-var x[]="a\nb\nc">
	   <array-append d x>

	   leaves X with the values "a\nb\nc\nd". */

	/* Implies that mh_exp() should not evaluate MH_PARSE_ARRAY,
	   only <get-var-once exp> should. */
	return mh_core_set_1_new
	  (mh_core_array_new (mh_exp (arrayvar, env), NULL),
	   mh_core_prim_2_new (MH_ARRAY_APP_OP,
			       mh_exp (element, env),
			       mh_exp_as_get_raw (arrayvar, env)));
      }

    default:
      return mh_exp_as_extern (parse, env);
    }
}

/*
 *
 *
 *
 *
 */
static mh_core_t
mh_exp_tag_as_html (mh_parse_t parse,
		    mh_env_t   env)
{
  mh_parse_t body = MH_PARSE_TAG_BODY (parse);

  /* Start with the TAG-BODY but reparse it. */
  mh_parse_t html = mh_parse_type_p (body, MH_PARSE_TYPE_LIST)
    ? mh_parse_list_as_body (body)
    : mh_parse_list_new (body, MH_PARSE_EMPTY);

  /* Add enclosing angle brackets */
  mh_parse_t parse_open =
    mh_parse_token_new (MH_PARSE_TAG_OPEN (parse), true);
    
  mh_parse_t parse_close =
    mh_parse_token_new (MH_PARSE_TAG_CLOSE (parse), true);
  
  mh_core_t result;

  html = mh_parse_list_new
    (parse_open,
     mh_parse_list_append (html,
			   mh_parse_list_new (parse_close,
					      MH_PARSE_EMPTY)));
  
  result = mh_exp_prog_and_concat (html, 0, env);

  mh_parse_list_free (html);

  return result;
}

static mh_core_t
mh_exp_html_blk (EXP_ARGS_DEF)
{
  mh_parse_t p_open = MH_PARSE_BLK_OPEN  (parse);
  mh_parse_t p_body = MH_PARSE_BLK_BODY  (parse);
  mh_parse_t p_clos = MH_PARSE_BLK_CLOSE (parse);

  return mh_core_fmt_3_new
    (mh_exp_tag_as_html (p_open, env),
     mh_exp (p_body, env),
     mh_exp_tag_as_html (p_clos, env));
}

static mh_core_t
mh_exp_html_tag (EXP_ARGS_DEF)
{
  return 0 == count
    ? mh_core_data_new (mh_parse_to_string (parse))
    : mh_exp_tag_as_html (parse, env);
}

DEFINE_EXPANDER ("html", mh_exp_html, "")
{
  /* Argument PARSE could be either a BLK for a TAG */
  return
    (mh_parse_type_p (parse, MH_PARSE_TYPE_BLK)
     ? mh_exp_html_blk
     : mh_exp_html_tag)
    (EXP_ARGS_APP);
}

/*
 *
 *
 *
 *
 */
static mh_tag_t
mh_expand_install_tag (string_t        name,
		       boolean_t       complex_p,
		       mh_white_type_t whitespace,
		       mh_expander_t   expander)
{
  Symbol*   symbol = 
    mhtml_find_mach_function_symbol (name);

  mh_tag_t  tag =
    mh_tag_new (name, TAG_TYPE_DEFUN,
		complex_p,
		false,		/* weak_p */
		whitespace,
		NULL,
		strdup (DEFAULT_PACKAGE_NAME), /* libmhtml/symbols.h */
		NULL,		/* documentation */
		NULL,		/* args */
		0);		/* args_count */

  tag->compile.expander = expander;

  symbol->machine = tag;
  mh_memory_add_root ((mh_object_t *) & symbol->machine);

  return (tag);
}
  
#define DEFINE_SPECIAL( name, expander )		\
  mh_expand_install_tag (name, false, MH_WHITE_DELETE, expander)

#define DEFINE_COMPLEX_SPECIAL( name, expander )	\
  mh_expand_install_tag (name, true, MH_WHITE_INHERIT, expander)

#define DEFINE_WEAK_SPECIAL()

#define DEFINE_PRIMITIVE( name, byte_op, expander )	\
do							\
{							\
  mh_tag_t tag = mh_expand_install_tag			\
    (name, false, false, expander);			\
							\
  tag->compile.primitive_p = true;			\
  tag->compile.primitive_opcode = byte_op;		\
} while (0)

#define DEFINE_HTML( name, complex_p, weak_p, expander )	\
do								\
{								\
  mh_tag_t tag = mh_expand_install_tag				\
    (name, complex_p, MH_WHITE_DELETE, expander);		\
  MH_TAG_WEAK_P (tag) = weak_p;					\
} while (0)

static void
mh_expand_install (void)
{
  DEFINE_SPECIAL ("if",           mh_exp_if);
  DEFINE_SPECIAL ("or",           mh_exp_or);
  DEFINE_SPECIAL ("and",          mh_exp_and);
  DEFINE_SPECIAL ("get-var",      mh_exp_get_var); /* extern? */
  DEFINE_SPECIAL ("get-var-once", mh_exp_get_var); /* ;-> */
  DEFINE_SPECIAL ("get-var-eval", mh_exp_get_var_eval); /* ;-> */
  DEFINE_SPECIAL ("set-var",      mh_exp_set_var);
  DEFINE_SPECIAL ("include",      mh_exp_include);
  DEFINE_SPECIAL ("var-case",     mh_exp_var_case);
  DEFINE_SPECIAL ("concat",       mh_exp_concat);
  DEFINE_SPECIAL ("%%concat",     mh_exp_concat);
  mh_expand_install_tag ("prog", false, MH_WHITE_KEEP, mh_exp_prog);

  DEFINE_COMPLEX_SPECIAL ("when",        mh_exp_when);
  DEFINE_COMPLEX_SPECIAL ("while",       mh_exp_while);
  DEFINE_COMPLEX_SPECIAL ("with",        mh_exp_with);
  DEFINE_COMPLEX_SPECIAL ("foreach",     mh_exp_foreach);
  DEFINE_COMPLEX_SPECIAL ("verbatim",    mh_exp_verbatim);
  DEFINE_COMPLEX_SPECIAL ("comment",     mh_exp_comment);

  /* These do their own body whitespace processing */
  mh_expand_install_tag  ("defsubst", true, MH_WHITE_KEEP,   mh_exp_defsubst);
  mh_expand_install_tag  ("defun",    true, MH_WHITE_DELETE, mh_exp_defun);
  mh_expand_install_tag  ("defmacro", true, MH_WHITE_KEEP,   mh_exp_defmacro);

  DEFINE_SPECIAL ("return",       mh_exp_return);
  DEFINE_SPECIAL ("break",        mh_exp_break);

  DEFINE_COMPLEX_SPECIAL ("with-open-stream",        mh_exp_block);
  DEFINE_COMPLEX_SPECIAL ("with-open-database",      mh_exp_block);
  DEFINE_COMPLEX_SPECIAL ("sql::with-open-database", mh_exp_block);
  DEFINE_SPECIAL         ("sql::database-query",     mh_exp_db_query);

  /* IFEQ, IFNEQ */
  DEFINE_SPECIAL ("ifeq",           mh_exp_ifeq);
  DEFINE_SPECIAL ("ifneq",          mh_exp_ifneq);


  DEFINE_SPECIAL   ("unset-var",     mh_exp_var_unset);
  DEFINE_SPECIAL   ("var-exists",    mh_exp_var_exists);
  DEFINE_SPECIAL   ("defvar",        mh_exp_def_var);

  DEFINE_PRIMITIVE ("not",      MH_NOT_OP, mh_exp_as_prim);

  DEFINE_PRIMITIVE ("add",      MH_ADD_OP, mh_exp_math_add);
  DEFINE_PRIMITIVE ("sub",      MH_SUB_OP, mh_exp_math_sub);
  DEFINE_PRIMITIVE ("mul",      MH_MUL_OP, mh_exp_math_mul);
  DEFINE_PRIMITIVE ("div",      MH_DIV_OP, mh_exp_math_div);

  DEFINE_PRIMITIVE ("eq",      MH_EQ_OP, mh_exp_math_eq);
  DEFINE_PRIMITIVE ("neq",     MH_NE_OP, mh_exp_math_neq);
  DEFINE_PRIMITIVE ("lt",      MH_LT_OP, mh_exp_math_lt);
  DEFINE_PRIMITIVE ("le",      MH_LE_OP, mh_exp_math_le);
  DEFINE_PRIMITIVE ("gt",      MH_GT_OP, mh_exp_math_gt);
  DEFINE_PRIMITIVE ("ge",      MH_GE_OP, mh_exp_math_ge);
  DEFINE_PRIMITIVE ("random",  MH_RANDOM_OP, mh_exp_math_random);
  DEFINE_PRIMITIVE ("sqrt",    MH_SQRT_OP,   mh_exp_as_math);
  DEFINE_PRIMITIVE ("integer", MH_INTEGER_OP,   mh_exp_as_math);
  DEFINE_PRIMITIVE ("integer?",MH_INTEGER_P_OP, mh_exp_math_integer_p);
  DEFINE_PRIMITIVE ("real?",   MH_REAL_P_OP,    mh_exp_math_real_p);
  DEFINE_SPECIAL ("decrement",     mh_exp_decrement);
  DEFINE_SPECIAL ("increment",     mh_exp_increment);

  /* ALIST */
  DEFINE_PRIMITIVE ("alist?",           MH_ALIST_P_OP,   mh_exp_as_prim);
  DEFINE_SPECIAL   ("make-alist",       mh_exp_alist_make);
  DEFINE_SPECIAL   ("alist-get-var",    mh_exp_alist_get_var);
  DEFINE_SPECIAL   ("alist-set-var",    mh_exp_alist_set_var);
  DEFINE_SPECIAL   ("alist-var-exists", mh_exp_alist_has_var);
  DEFINE_SPECIAL   ("alist-unset-var",  mh_exp_alist_unset_var);
  /*  DEFINE_SPECIAL   ("alist-merge",      mh_exp_alist_merge);*/
  DEFINE_SPECIAL   ("alist-defvar",     mh_exp_alist_def_var);

  DEFINE_SPECIAL   ("package-to-alist",  mh_exp_package_to_alist);
  DEFINE_SPECIAL   ("alist-to-package",  mh_exp_alist_to_package);

  /* STRING */
  DEFINE_PRIMITIVE ("string-compare",      MH_STR_COMP_OP, mh_exp_as_prim);
  DEFINE_PRIMITIVE ("string-eq",           MH_STR_EQ_OP,   mh_exp_str_eq);
  DEFINE_PRIMITIVE ("string-neq",          MH_STR_NEQ_OP,  mh_exp_str_ne);
  DEFINE_PRIMITIVE ("string-length",       MH_STR_LEN_OP,  mh_exp_str_len);
  DEFINE_PRIMITIVE ("downcase",            MH_STR_DOWN_OP, mh_exp_str_down);
  DEFINE_PRIMITIVE ("upcase",              MH_STR_UP_OP,   mh_exp_str_up);
  DEFINE_PRIMITIVE ("capitalize",          MH_STR_CAP_OP,  mh_exp_str_cap);

  /* ARRAY */
  DEFINE_SPECIAL   ("make-array",      mh_exp_array_make);
  DEFINE_SPECIAL   ("array-size",      mh_exp_array_size);
  DEFINE_SPECIAL   ("array-reverse",   mh_exp_array_reverse);
  DEFINE_SPECIAL   ("array-member",    mh_exp_array_member);
#if 0
  DEFINE_SPECIAL   ("array-append",    mh_exp_array_append);
#endif

  /* HTML */
  DEFINE_HTML ("html",    true, true, mh_exp_html);
  DEFINE_HTML ("body",    true, true, mh_exp_html);
  DEFINE_HTML ("br",      true, true, mh_exp_html);
  DEFINE_HTML ("p",       true, true, mh_exp_html);
  DEFINE_HTML ("select",  true, true, mh_exp_html);
  DEFINE_HTML ("a",       true, true, mh_exp_html);
  DEFINE_HTML ("table",   true, true, mh_exp_html);
  DEFINE_HTML ("td",      true, true, mh_exp_html);
  DEFINE_HTML ("tr",      true, true, mh_exp_html);
  DEFINE_HTML ("center",  true, true, mh_exp_html);
  DEFINE_HTML ("title",   true, true, mh_exp_html);
  DEFINE_HTML ("script",  true, true, mh_exp_html);
  DEFINE_HTML ("font",    true, true, mh_exp_html);
  DEFINE_HTML ("a",       true, true, mh_exp_html);
  DEFINE_HTML ("b",       true, true, mh_exp_html);
  DEFINE_HTML ("i",       true, true, mh_exp_html);
  DEFINE_HTML ("img",     true, true, mh_exp_html);
  DEFINE_HTML ("ul",      true, true, mh_exp_html);
  DEFINE_HTML ("li",      true, true, mh_exp_html);
  DEFINE_HTML ("form",    true, true, mh_exp_html);
  DEFINE_HTML ("input",   true, true, mh_exp_html);
  DEFINE_HTML ("pre",     true, true, mh_exp_html);
}

/* Wed Nov  6 16:27:45 1996.  */


#if defined (EXPAND_TEST)
extern int
main (int   argc,
      char *argv[])
{
#if 0
  string_t source =
    "<add 10 20><if <not <get-var foo>> bar baz><if 1 2>foo[]=<get-var bar>
<if 1 2><get-var zzz>=1000</if>
<foo>bar</foo>
<and 1 2 3 4><add 10 20>
<while <get-var x>><set-var x=\"\"></while>
<<foo> bar>
<defun foo &rest x>< a >ghi< /a ></defun><foo a b c>
<jkl < mno > pqr> stu<>><\"<get-var foo>\"";

  string_t source = "<set-var foo=10 <get-var-once foo>=30 foo[1]=20 <get-var-once foo>[2]=40 <get-var-once foo[1]>[2]=50><get-var-once foo[1]><alist-get-var foo key><alist-set-var foo key=value><add foo 1><foo 1>";

string_t source =
  "<defsubst parser::canonicalize-var :pcv-var :pcv-pack whitespace=delete>
  <defvar :pcv-pack \"^\">
  <if <not <match <get-var-once <get-var-once :pcv-var>> \"::\">>
      <set-var <get-var-once :pcv-var> =
	<get-var-once :pcv-pack>::<get-var-once <get-var-once :pcv-var>>>>
</defsubst>";

string_t source = 
  "<defun foo bar &rest bars[]><set-var xx[]=<alist-get-var bar xx[]>></defun>";

string_t source =
  "<defun foo &rest bars[]><foreach bar bars><get-var-once bar></foreach></defun>";

string_t source =
  "<defun foo a b whitespace=keep>
<get-var-once a>,
<get-var-once b>
</defun>";
 
string_t source =
  "<concat
 <when <eq 0 0>>
\"This is a test.\"
</when>>";

string_t source =
   "<a href=\"<concat abc ?key=def>\"></a>";
#endif

string_t source =
  "<defun foo a &optional b &key c d &rest e></defun>
   <foo 1 2 c=3 4 5 6>
   <foo 1>

   <defun bar a &unevalled &optional b &key c d &rest e></defun>
   <bar 1 <get-var x> c=3 4 5 6>

   <defun bar a &unevalled &optional b &key &unevalled c d &rest e></defun>
   <bar 1 <get-var x> c=<get-var z> d=<get-var y> 4 5 6>";


  mh_parse_t parse = mh_parse ("expand", source);

  mh_core_t core;

  printf ("STRING: %s\nPARSE :\n", source);
  mh_parse_show (parse);
  printf ("\n");

  /* expand */
  mh_object_init ();
  mh_expand_install ();
  core = mh_expand ("main", parse);
  /*  mh_parse_free (parse); */
  mh_core_show (core);
  printf ("\n");

  return (0);
}
#endif /* defined (PARSE_TEST) */
