/* machine.h: -*- C -*- The public interface to the Meta-HTML machine. */

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

/* MACHINE.H contains the public interface to the Meta-HTML machine;
   i.e. the Meta-HTML bytecode engine.  The interface includes:

   'Expressed' Objects (strings, numbers, cons, vector)

   'Denoted' Objects (tag) with byte-ops, arguments.

   Scan and Parse

   Machine

*/

#if !defined (_MH_MACHINE_H_)
#define _MH_MACHINE_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>		/* toupper(), tolower() */
#include <string.h>		/* strdup(), strcat() */
#include <assert.h>
#include <unistd.h>
#include <limits.h>		/* CHAR_MAX */

#include <xmalloc/xmalloc.h>	/* xmalloc(), xrealloc(), xcalloc() */

#include "language.h"		/* MetaHTML Proper */

#include "machine/code.h"

#if !defined (macintosh)
extern double strtod (const char *, char **);
#endif

#define MIN(a, b)  (((a) < (b)) ? (a) : (b)) /* From where? */
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

/* STRING and BOOLEAN
 *
 * Get this out-of-the-way early */
typedef char *string_t;		/* Troublesome, likely */

typedef enum {			/* Ditto */
  false = 0,
  true  = 1,
} boolean_t;

extern boolean_t mh_machine_verbose_debugging;

extern boolean_t mh_machine_step_p;
extern boolean_t mh_machine_perfmon_p;
extern boolean_t mh_machine_externmon_p;
extern boolean_t mh_machine_memorymon_p;
extern boolean_t mh_machine_trace_p;
extern unsigned int mh_machine_trace_depth;

extern void
mh_string_to_string_escaped (string_t string);

extern string_t
mh_string_to_string_quoted (string_t string);

extern string_t
mh_string_to_string_quoted_to_depth (string_t string,
				     unsigned int depth);

extern string_t 
mh_string_quotes_at_depth (unsigned int depth);


/*
 *
 *
 *
 */
extern void fail (void);

#define OBJECT_MARKER        0xdeadbeef
#define OBJECT_MARKER_MASK   0xffffffff

typedef struct mh_object *mh_object_t;

/* Number of Bytes in an object */
#define OBJECT_SIZE_IN_BYTES              (sizeof (mh_object_t))

/* Number of objects needed for BYTES */
#define OBJECT_SIZE( bytes )			\
  (((bytes) + (OBJECT_SIZE_IN_BYTES - 1)) / 	\
   OBJECT_SIZE_IN_BYTES)

/* Memory boundary in bytes */
#define OBJECT_ALIGNMENT_IN_BYTES          8

/* Memory boundary in objects */
#define OBJECT_ALIGNMENT_IN_OBJECTS			\
  (OBJECT_ALIGNMENT_IN_BYTES / OBJECT_SIZE_IN_BYTES)

/* Number of objects needed to align OBJECTS */
#define OBJECT_ALIGNMENT_SIZE( objects )     		\
  (((objects) + (OBJECT_ALIGNMENT_IN_OBJECTS - 1)) &	\
   ~(OBJECT_ALIGNMENT_IN_OBJECTS - 1))

#define OBJECT_ALIGNMENT_SIZE_FROM_BYTES( bytes )	\
  OBJECT_ALIGNMENT_SIZE (OBJECT_SIZE (bytes))

extern void
mh_memory_gc (void);

extern void
mh_memory_gc_force (void);

extern void
mh_memory_gc_disable (void);

extern void
mh_memory_gc_enable (void);

extern mh_object_t
mh_memory_alloc (unsigned int objects);

extern void
mh_memory_summarize (void);

extern void
mh_memory_add_root (mh_object_t *location);

extern void
mh_memory_rem_root (mh_object_t *location);



/*
 * MH_TYPE_T
 *
 * These are the runtime types for 'records' in MetaHTML.  For now
 * they are enumerated as part of the mh_type_t declaration.  In the
 * future, there might be mh_class_t instances which would serve to
 * differentiate types and which would maintain generic operations
 * like print(), equal(), allocate(), etc. */
typedef enum
{
  MH_STRING_TAG,		/* + BUFFER */
  MH_NUMBER_TAG,
  MH_VECTOR_TAG,
  MH_NIL_TAG,
  MH_ALIST_TAG,
  MH_FUNCTION_TAG,		/* Functions, user defined */
  MH_UNBOUND_TAG		/* Symbol without an object */
} mh_type_t;

#define MH_NUMBER_OF_TYPES        (1 + MH_UNBOUND_TYPE)

/*
 * MH_OBJECT_T
 *
 */
#define MH_OBJECT_SLOTS				\
  unsigned int marker;				\
  mh_type_t    tag

struct mh_object
{
  MH_OBJECT_SLOTS;
};

#define MH_OBJECT_TAG( obj )          ((obj)->tag)
#define MH_OBJECT_NULL                ((mh_object_t) NULL)

#define MH_OBJECT_ALLOC_SIZE	      (sizeof (struct mh_object))

#define MH_AS_OBJECT( obj )           ((mh_object_t) (obj))

#define MH_OBJECT_HAS_TAG_P( obj, tag )		\
  ((tag) == MH_OBJECT_TAG (MH_AS_OBJECT (obj)))

#define MH_OBJECT_CHUNK( size )              (size)

extern void
mh_object_init (void);

extern mh_object_t
mh_object_read (string_t string);

extern boolean_t
mh_object_equal (mh_object_t object1,
		 mh_object_t object2);

extern boolean_t
mh_object_equal_with_case (mh_object_t object1,
			   mh_object_t object2);

extern void
mh_object_to_buffer (mh_object_t object,
		     boolean_t   quoted,
		     BPRINTF_BUFFER *b);

extern string_t
mh_object_to_string (mh_object_t object,
		     boolean_t   quoted);

extern void
mh_object_to_file (mh_object_t object,
		   boolean_t   quoted,
		   FILE       *out);

extern mh_object_t
mh_object_alloc (mh_type_t    type,
		 unsigned int size_in_bytes);

/* Size in OBJECTS */
extern unsigned int
mh_object_size (mh_object_t object);

extern mh_object_t
mh_buffer_serialize_to_object (BPRINTF_BUFFER *b);

extern BPRINTF_BUFFER *
mh_object_serialize_to_buffer (mh_object_t object);

extern void
mh_object_serialize_to_file (mh_object_t object,
			     FILE *file);


/***************************************************************************
 *
 * MH_STRING_T
 *
 */ 
typedef struct mh_string
{
  MH_OBJECT_SLOTS;		/* Standard  */

  size_t   length;		/* Length of this string. */
  char     chars[1];
} *mh_string_t;

#define MH_STRING_LENGTH( str )    ((str)->length)
#define MH_STRING_SLOTS( str )     ((str)->slots)
#define MH_STRING_CHARS( str )     ((str)->chars)
#define MH_STRING_NULL             ((mh_string_t) NULL)

#define MH_STRING_ALLOC_SIZE( len )					\
  MH_OBJECT_CHUNK (sizeof (struct mh_string) + ((len) * sizeof (char)))

#define MH_STRING_P( obj )			\
  MH_OBJECT_HAS_TAG_P (obj, MH_STRING_TAG)

#define MH_AS_STRING( obj )        ((mh_string_t) (obj))

typedef mh_string_t mh_bool_t;

extern mh_string_t MH_EMPTY;
#define MH_OBJECT_EMPTY       (MH_AS_OBJECT (MH_EMPTY))
#define MH_EMPTY_P( obj )     (MH_AS_OBJECT (obj) == MH_AS_OBJECT (MH_EMPTY))
#define MH_NOT_EMPTY_P( obj ) (MH_AS_OBJECT (obj) != MH_AS_OBJECT (MH_EMPTY))

extern mh_string_t MH_TRUE;
#define MH_OBJECT_TRUE       (MH_AS_OBJECT (MH_TRUE))
#define MH_TRUE_P( obj )     (MH_AS_OBJECT (obj) == MH_AS_OBJECT (MH_TRUE))
#define MH_NOT_TRUE_P( obj ) (MH_AS_OBJECT (obj) != MH_AS_OBJECT (MH_TRUE))


/* Constructors */
extern mh_string_t
mh_string_new (string_t string);

extern mh_string_t
mh_string_buffer (string_t string,
		  size_t   length);

extern mh_string_t
mh_string_2_new (string_t str1,
		 string_t str2);

extern mh_string_t
mh_string_3_new (string_t str1,
		 string_t str2,
		 string_t str3);

extern mh_string_t
mh_string_dup (mh_string_t string);

extern void
mh_string_copy (mh_string_t target,
		mh_string_t source);

/* Accessors */
extern size_t
mh_string_length (mh_string_t string);

extern string_t
mh_string_chars (mh_string_t string);

extern char
mh_string_char_at (mh_string_t  string, 
		   unsigned int offset);

extern mh_string_t
mh_string_compare (mh_string_t str1,
		   mh_string_t str2);

/* Equality */
#define DEC_MH_STRING_EQUALITY( name )		\
extern mh_bool_t				\
name (mh_string_t str1,				\
      mh_string_t str2,				\
      boolean_t   caseless_p)

DEC_MH_STRING_EQUALITY (mh_string_equal);
DEC_MH_STRING_EQUALITY (mh_string_not_equal);


/* Case Conversions */
#define DEC_MH_STRING_CASE( name )		\
extern void					\
name (mh_string_t target,			\
      mh_string_t source)

DEC_MH_STRING_CASE (mh_string_downcase);
DEC_MH_STRING_CASE (mh_string_upcase);
DEC_MH_STRING_CASE (mh_string_capitalize);

extern void
mh_string_extend (mh_string_t buffer,
		  mh_string_t extra);

extern mh_object_t
mh_string_concat (mh_object_t buffer,
		  mh_object_t extra);

extern mh_string_t
mh_object_to_string_object (mh_object_t object,
			    boolean_t   quoted);

/***************************************************************************
 *
 * MH_NUMBER_T
 *
 */ 
typedef struct mh_number
{
  MH_OBJECT_SLOTS;
  long double value;
} *mh_number_t;

#define MH_NUMBER_VALUE( num )    ((num)->value)
#define MH_NUMBER_NULL            ((mh_number_t) NULL)

#define MH_NUMBER_ALLOC_SIZE			\
  MH_OBJECT_CHUNK (sizeof (struct mh_number))

#define MH_NUMBER_P( obj )			\
  MH_OBJECT_HAS_TAG_P (obj, MH_NUMBER_TAG)

#define MH_AS_NUMBER( obj )        ((mh_number_t) (obj))

extern mh_number_t MH_NUMBER_ZERO;
extern mh_number_t MH_NUMBER_ONE;

/* Constructors */

extern mh_number_t
mh_number_new (long double value);

/* Accessors */
extern long double
mh_number_value (mh_number_t number);

extern mh_number_t
mh_object_to_number (mh_object_t object);

/* Equality */
#define DEC_MH_NUMBER_REL_OP( name )		\
extern mh_bool_t				\
name (mh_number_t num1,				\
      mh_number_t num2)

DEC_MH_NUMBER_REL_OP (mh_number_eq);
DEC_MH_NUMBER_REL_OP (mh_number_ne);
DEC_MH_NUMBER_REL_OP (mh_number_lt);
DEC_MH_NUMBER_REL_OP (mh_number_le);
DEC_MH_NUMBER_REL_OP (mh_number_gt);
DEC_MH_NUMBER_REL_OP (mh_number_ge);

/* Unary Ops */
#define DEC_MH_NUMBER_UN_OP( name )		\
extern mh_number_t				\
name (mh_number_t number)

DEC_MH_NUMBER_UN_OP (mh_number_sqrt);
DEC_MH_NUMBER_UN_OP (mh_number_inc);
DEC_MH_NUMBER_UN_OP (mh_number_dec);
DEC_MH_NUMBER_UN_OP (mh_number_integer);
DEC_MH_NUMBER_UN_OP (mh_number_random);
DEC_MH_NUMBER_UN_OP (mh_number_randomize);

extern mh_bool_t
mh_number_eqz (mh_number_t number);

extern mh_bool_t
mh_number_nez (mh_number_t number);

extern mh_bool_t
mh_number_integer_p (mh_number_t number);

extern mh_bool_t
mh_number_real_p (mh_number_t number);

/* Binary Ops */
#define DEC_MH_NUMBER_BIN_OP( name )		\
extern mh_number_t				\
name (mh_number_t num1,				\
      mh_number_t num2)

DEC_MH_NUMBER_BIN_OP (mh_number_add);
DEC_MH_NUMBER_BIN_OP (mh_number_sub);
DEC_MH_NUMBER_BIN_OP (mh_number_mul);
DEC_MH_NUMBER_BIN_OP (mh_number_div);

extern mh_string_t
mh_string_sub (mh_string_t string,
	       mh_number_t start,
	       mh_number_t end);


/***************************************************************************
 *
 * MH_VECTOR_T
 *
 */ 
typedef struct mh_vector
{
  MH_OBJECT_SLOTS;
  size_t      length;
  mh_object_t values[1];
} *mh_vector_t;

#define MH_VECTOR_LENGTH( vect )    ((vect)->length)
#define MH_VECTOR_VALUES( vect )    ((vect)->values)
#define MH_VECTOR_NULL              ((mh_vector_t) NULL)

#define MH_VECTOR_REF( vect, index ) ((vect)->values[(index)])

#define MH_VECTOR_ALLOC_SIZE( len )			\
  MH_OBJECT_CHUNK (sizeof (struct mh_vector) + 		\
		   ((len) - 1) * sizeof (mh_object_t))

#define MH_VECTOR_P( obj )			\
  MH_OBJECT_HAS_TAG_P (obj, MH_VECTOR_TAG)

#define MH_AS_VECTOR( obj )        ((mh_vector_t) (obj))

/* Constructors */

extern mh_vector_t
mh_vector_new (size_t length);

extern mh_vector_t
mh_vector_fill (mh_object_t *objects,
		size_t       length);

static inline mh_vector_t
mh_vector_copy (mh_vector_t vector)
{
  return mh_vector_fill (MH_VECTOR_VALUES (vector),
			 MH_VECTOR_LENGTH (vector));
}

extern size_t
mh_vector_length (mh_vector_t vector);

extern mh_object_t
mh_vector_ref (mh_vector_t vector,
	       mh_object_t offset);

extern void
mh_vector_set (mh_vector_t vector,
	       mh_object_t offset,
	       mh_object_t value);

extern mh_vector_t
mh_vector_extend  (mh_vector_t vector,
		   mh_object_t offset,
		   mh_object_t value);

extern mh_object_t
mh_vector_member (mh_vector_t vector,
		  mh_object_t item,
		  boolean_t   caseless_p);

extern mh_object_t
mh_vector_append (mh_vector_t vector,
		  mh_object_t item);

extern void
mh_vector_reverse_inplace (mh_vector_t vector);

extern mh_vector_t
mh_vector_reverse (mh_vector_t vector);

extern mh_object_t
mh_object_to_vector (mh_object_t object);

extern mh_object_t
mh_string_to_vector (mh_string_t string, char *delimiters);

typedef mh_object_t (*mh_vector_unop_fun_t) (mh_vector_t vector);

extern mh_object_t
mh_array_length (mh_vector_t vector);

extern mh_object_t
mh_vector_unop (mh_object_t object,
		mh_vector_unop_fun_t fun);

/***************************************************************************
 *
 * MH_ALIST_T
 *
 */ 
extern mh_object_t mh_nil;
#define MH_NIL                mh_nil
#define MH_NIL_P( obj )       (MH_AS_OBJECT (obj) == MH_NIL)
#define MH_NOT_NIL_P( obj )   (MH_AS_OBJECT (obj) != MH_NIL)

typedef struct mh_alist *mh_alist_t;

struct mh_alist
{
  MH_OBJECT_SLOTS;
  mh_object_t name;
  mh_object_t value;
  mh_alist_t  tail;
};

#define MH_ALIST_NAME( alist )     ((alist)->name)
#define MH_ALIST_VALUE( alist )    ((alist)->value)
#define MH_ALIST_TAIL( alist )     ((alist)->tail)
#define MH_ALIST_NULL              ((mh_alist_t) NULL)

#define MH_ALIST_ALLOC_SIZE        (sizeof (struct mh_alist))

#define MH_ALIST_P( obj )			\
  MH_OBJECT_HAS_TAG_P (obj, MH_ALIST_TAG)

#define MH_AS_ALIST( obj )        ((mh_alist_t) (obj))

extern boolean_t
mh_alist_p (mh_object_t object);

extern mh_alist_t
mh_alist_new (mh_alist_t  alist,
	      mh_object_t name,
	      mh_object_t value);

extern mh_alist_t
mh_alist_reverse (mh_alist_t alist);

extern unsigned int
mh_alist_length (mh_alist_t alist);

extern boolean_t
mh_alist_has (mh_alist_t  alist,
	      mh_object_t name);

extern mh_alist_t
mh_alist_rem (mh_alist_t  alist,
	      mh_object_t name);

extern mh_object_t
mh_alist_get (mh_alist_t alist,
	      mh_object_t name);  /* CAR */

extern mh_alist_t
mh_alist_set (mh_alist_t alist,
	      mh_object_t name,   /* CAR */
	      mh_object_t value); /* CDR */

extern mh_alist_t
mh_alist_merge (mh_alist_t alist1,
		mh_alist_t alist2);

extern mh_vector_t
mh_alist_names (mh_alist_t alist);

extern mh_vector_t
mh_alist_values (mh_alist_t alist);

extern mh_alist_t
mh_alist_fill (mh_object_t *names,
	       mh_object_t *values,
	       unsigned int count);

extern mh_alist_t
mh_alist_fill_pairs (mh_object_t *names_and_values,
		     unsigned int count);

extern mh_object_t 
mh_object_to_alist (mh_object_t object);


/***************************************************************************
 *
 * MH_BYTE_CODE_T
 *
 * Bytecode consists of byte operators and byte operands.  The
 * mh_byte_op_t type must be coercible to mh_byte_code_t.  A byte code
 * vector is a pointer to mh_byte_code_t although we don't use an
 * expicit typedef for that.
 *
 * This declaration is here as opposed to in code.h for the benefit of
 * the upcoming mh_function_t declaration.  This is not an object in
 * MetaHTML.
 */ 
typedef unsigned char mh_byte_code_t;

extern void 
mh_byte_code_instr_disassemble (mh_byte_code_t *opcodes, 
				FILE *file);

/***************************************************************************
 *
 * MH_ARGUMENT_T
 *
 */
typedef enum
{
  MH_ARGUMENT_REQUIRED,
  MH_ARGUMENT_OPTIONAL,
  MH_ARGUMENT_KEY,
  MH_ARGUMENT_REST,
  MH_ARGUMENT_BODY,
  MH_ARGUMENT_ATTRIBUTES
} mh_argument_type_t;

typedef enum
{
  MH_ARGUMENT_EVALLED,
  MH_ARGUMENT_UNEVALLED
} mh_argument_eval_t;

typedef enum
{
  MH_ARGUMENT_VALUE,
  MH_ARGUMENT_ARRAY
} mh_argument_array_t;

typedef struct mh_argument
{
  string_t            name;	/* mh_string_t or string_t ... or index */
  mh_argument_type_t  type;
  mh_argument_eval_t  eval;
  mh_argument_array_t array;
} *mh_argument_t;

#define MH_ARGUMENT_NAME( arg )         ((arg)->name)
#define MH_ARGUMENT_TYPE( arg )         ((arg)->type)
#define MH_ARGUMENT_EVAL( arg )         ((arg)->eval)
#define MH_ARGUMENT_ARRAY( arg )        ((arg)->array)

extern mh_argument_t
mh_argument_new (string_t            name,
		 mh_argument_type_t  type,
		 mh_argument_eval_t  eval,
		 mh_argument_array_t array);

extern void
mh_argument_free (mh_argument_t arg);

/***************************************************************************
 *
 * MH_TAG/FUNCTION_T
 *
 * forward declarations */
typedef struct mh_env   *mh_env_t;
typedef struct mh_tag   *mh_tag_t;
typedef struct mh_core  *mh_core_t;
typedef struct mh_parse *mh_parse_t;

typedef mh_core_t
(*mh_expander_t) (mh_parse_t   parse,
		  mh_env_t     env,
		  mh_parse_t   operator, /* MH_PARSE_TYPE_TOKEN */
		  mh_tag_t     tag,      /* TAG for OPERATOR */
		  mh_parse_t   operands, /* MH_PARSE_TYPE_LIST */
		  unsigned int operands_count,
		  mh_parse_t   body);    /* NULL if not complex */

typedef void			/* Like PFunHandler */
(*mh_interpreter_t) (PFunArgs);

typedef enum
{
  TAG_TYPE_MACRO,
  TAG_TYPE_SUBST,
  TAG_TYPE_DEFUN,
  TAG_TYPE_PRIM
} mh_tag_type_t;

typedef enum
{
  MH_WHITE_DELETE,
  MH_WHITE_KEEP,
  MH_WHITE_INHERIT
} mh_white_type_t;

struct mh_tag
{
  MH_OBJECT_SLOTS;

  UserFunction *uf;

  mh_tag_type_t Xtype;

  /* Interesting Bits */
  boolean_t       Xcomplex_p;	/* TRUE to parse a complex tag */
  boolean_t       Xweak_p;	/* TRUE to parse ... */
  mh_white_type_t Xwhitespace;	/* TRUE to retain whitespace */
  boolean_t       Xcurrent_package_p;

  /* Debugging */
  unsigned int  Xdebug_level;

  /*
   *
   *
   */
  string_t Xname;
  string_t Xbody;
  string_t Xpackname;
  string_t Xdocumentation;

  /* 
   * Arguments
   *
   */
  mh_argument_t *Xargs;
  unsigned int   Xargs_count;

  /* Any ARGUMENTS with type of REST and REST[]  */
  boolean_t      Xargs_has_rest_p;
  boolean_t      Xargs_has_rest_array_p;

  /* Any ARGUMENTS with eval of UNEVALLED */
  boolean_t      Xargs_has_unevalled_p;

  /* 
   * Parse (TAG or BLK) [for non-TAG_TYPE_PRIM]
   *
   * Eventually, an interpreter will use PARSE */ 

  /*
   * Interpreter
   *
   */
  struct
  {
    mh_parse_t parse;
    mh_interpreter_t interpreter;
  } interpret;

  /*
   * Compiler 
   *
   */
  struct
  {
    boolean_t     primitive_p;
    mh_byte_op_t  primitive_opcode;
    mh_expander_t expander;
  } compile;

  /* 
   * Machine (Compiled Function)
   *
   */
  struct
  {
    /* Code, if function */
    /* Code Vector */
    mh_byte_code_t *Xcode;
    unsigned int    Xcode_count;
    
    /* Constants Vector */
    mh_object_t      Xconstants_vector;

    /* Object Stack Size */
    unsigned int     Xstack_size;
  } machine;

#if defined (METAHTML_PROFILER)
  PROFILE_INFO *profile_info;
#endif

};

#define MH_TAG_TYPE( tag )              ((tag)->Xtype)
#define MH_TAG_NAME( tag )              ((tag)->Xname)
#define MH_TAG_COMPLEX_P( tag )         ((tag)->Xcomplex_p)
#define MH_TAG_WEAK_P( tag )            ((tag)->Xweak_p)
#define MH_TAG_WHITESPACE( tag )        ((tag)->Xwhitespace)
#define MH_TAG_CURRENT_PACKAGE_P( tag ) ((tag)->Xcurrent_package_p)
#define MH_TAG_BODY( tag )              ((tag)->Xbody)
#define MH_TAG_PACKNAME( tag )          ((tag)->Xpackname)
#define MH_TAG_DOCUMENTATION( tag )     ((tag)->Xdocumentation)
#define MH_TAG_DEBUG_LEVEL( tag )       ((tag)->Xdebug_level)

#define MH_TAG_ARGS( tag )            ((tag)->Xargs)
#define MH_TAG_ARGS_COUNT( tag )      ((tag)->Xargs_count)
#define MH_TAG_ARGS_HAS_REST_P( tag ) ((tag)->Xargs_has_rest_p)
#define MH_TAG_ARGS_HAS_REST_ARRAY_P( tag ) ((tag)->Xargs_has_rest_array_p)
#define MH_TAG_ARGS_HAS_UNEVALLED_P( tag )  ((tag)->Xargs_has_unevalled_p)

#define MH_TAG_CODE( tag )            ((tag)->machine.Xcode)
#define MH_TAG_CODE_COUNT( tag )      ((tag)->machine.Xcode_count)
#define MH_TAG_CONSTANTS_VECTOR( tag )((tag)->machine.Xconstants_vector)
#define MH_TAG_STACK_SIZE( tag )      ((tag)->machine.Xstack_size)


#define MH_TAG_CONSTANTS( tag )						\
  MH_VECTOR_VALUES (MH_AS_VECTOR (MH_TAG_CONSTANTS_VECTOR (tag)))
#define MH_TAG_CONSTANTS_COUNT( tag )					\
  MH_VECTOR_LENGTH (MH_AS_VECTOR (MH_TAG_CONSTANTS_VECTOR (tag)))

#define MH_TAG_NULL                        ((mh_tag_t) NULL)

#define MH_TAG_ALLOC_SIZE()			\
  MH_OBJECT_CHUNK (sizeof (struct mh_tag))

#define MH_TAG_P( obj )			\
  MH_OBJECT_HAS_TAG_P (obj, MH_FUNCTION_TAG)

#define MH_AS_TAG( obj )        ((mh_tag_t) (obj))

extern mh_tag_t
mh_tag_new (string_t        name,
	    mh_tag_type_t   type,
	    boolean_t       complex_p,
	    boolean_t       weak_p,
	    mh_white_type_t whitespace,
	    string_t        body,
	    string_t        packname,
	    string_t        documentation,
	    mh_argument_t  *args,
	    unsigned int    args_count);

extern void
mh_tag_install_machine (mh_tag_t tag,
			mh_byte_code_t *code,
			unsigned int    code_count,
			mh_object_t    *constants,
			unsigned int    constants_count,
			unsigned int    stack_size);

extern boolean_t
mh_tag_arguments_have_type_p (mh_tag_t      tag,
			      mh_argument_type_t type);

extern unsigned int
mh_tag_arguments_type_count  (mh_tag_t      tag,
			      mh_argument_type_t type);
  
extern mh_argument_t
mh_tag_keyword_argument (mh_tag_t tag,
			 mh_object_t   name);

#if 0
extern boolean_t
mh_tag_keyword_p (mh_tag_t tag,
		  mh_symbol_t   name);
#endif

extern void 
mh_tag_disassemble (mh_tag_t tag, 
		    FILE *file);



extern mh_object_t
mh_welcome_to_the_machine (mh_tag_t tag,
			   mh_object_t  *args,
			   size_t        args_count);

extern mh_object_t
mh_welcome_to_the_machine_with_time (mh_tag_t   tag,
				     mh_object_t  *args,
				     size_t        args_count,
				     struct timeval *tm);

extern void
mh_object_fill_symbol_if_appropriate (Symbol *name);

extern void
mh_object_intern_symbol (Symbol *name);

extern void
mh_object_free_symbol (Symbol *name);

#if defined (NEVER_DEFINED)
extern void
mh_object_free (mh_object_t object,
		boolean_t   recurse);

extern void
mh_cons_free (mh_object_t cons,
	      boolean_t   recurse);
#endif


/* 
 * MH_BROKEN_HEART_T
 *
 */
typedef struct mh_broken_heart
{
  MH_OBJECT_SLOTS;
  mh_object_t object;
} *mh_broken_heart_t;

#define MH_BROKEN_HEART_TAG  MH_UNBOUND_TAG
#define MH_BROKEN_HEART_OBJECT( heart )      ((heart)->object)

#define MH_BROKEN_HEART_P( obj )			\
  MH_OBJECT_HAS_TAG_P (obj, MH_BROKEN_HEART_TAG)

#define MH_AS_BROKEN_HEART( obj )        ((mh_broken_heart_t) (obj))

#endif /* ! _MH_MACHINE_H_ */
