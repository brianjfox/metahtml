/* engine.c: -*- C -*-  */

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

#include <sys/time.h>		/* gettimeofday() */
#include <setjmp.h>
#include "machine/machine.h"

#define MH_MACH_DEBUG

boolean_t mh_machine_verbose_debugging = true;

static jmp_buf  engine_err_jmp_buf;
static string_t engine_err_message = (string_t) NULL;

typedef enum
{
  ENGINE_ERR_IGNORE,		/* Avoid 0 in longjmp() */
  ENGINE_ERR_NOT_A_FUNCTION,
  ENGINE_ERR_NOT_A_BYTEOP,
  ENGINE_ERR_UNIMPLEMENTED_BYTEOP,
  ENGINE_ERR_GENERAL
} engine_err_type_t;

static string_t engine_err_type_name [] =
{
  "Trouble in jmp_buf city!",
  "Not A Function",
  "Not A ByteOp",
  "Unimplemented ByteOp",
  "General"
};

static void
mh_machine_fail (string_t   message,
		 engine_err_type_t err_type)
{
  engine_err_message = message;
  longjmp (engine_err_jmp_buf, err_type);
}

/*
 * A Utility
 *
 */
#if defined (MH_MACH_DEBUG)
static double
timeval_to_seconds (struct timeval *tm)
{
  return (tm->tv_sec + ((double) tm->tv_usec) / 1e6);
}
#endif

/*
 * mh_object_intern_symbol ()
 *
 * An unused hook function for MetaHTML's symbol_intern() function. */
extern void
mh_object_intern_symbol (Symbol *name)
{ return; }

/*
 * mh_object_free_symbol ()
 *
 * The function mh_object_free_symbol() is installed as a hook function on
 * MetaHTML's symbol_free() function.  The MACHINE cleans up by removing a
 * memory root for name->machine.  That root will only exist if
 * name->machine's binding is non-NULL; although there is no real harm in
 * removing a root for a location that does not exist. */
extern void
mh_object_free_symbol (Symbol *name)
{
  if (name->machine)
    mh_memory_rem_root ((mh_object_t *) & name->machine);
}


/*
 * mh_symbol_intern_safely () // mh_symbol_intern_in_package_safely()
 *
 * These functions are called when the machine needs to lookup a symbol
 * from NAME.  They exclude the possibility of mh_object_fill_symbol(),
 * being called, through MetaHTML's symbol_retrieve_hook, and thus changing
 * SYMBOL->MACHINE inappropriately. */ 
static Symbol *
mh_symbol_intern_safely (string_t name)
{
  Symbol *symbol;
  SYMBOL_FUNCTION *hook = symbol_retrieve_hook;

  symbol_retrieve_hook = (SYMBOL_FUNCTION *) NULL;
  symbol = symbol_intern (name);
  symbol_retrieve_hook = hook;

  return symbol;
}

static Symbol *
mh_symbol_intern_in_package_safely (string_t name,
				    Package *package)
{
  Symbol *symbol;
  SYMBOL_FUNCTION *hook = symbol_retrieve_hook;

  symbol_retrieve_hook = (SYMBOL_FUNCTION *) NULL;
  symbol = symbol_intern_in_package (package, name);
  symbol_retrieve_hook = hook;

  return symbol;
}

/*
 * mh_object_fill_symbol ()
 *
 * Using the value of NAME->MACHINE; fill in the interpreter's
 * NAME->VALUES and related slots.  This is done unconditionally;
 * NAME->MACHINE must never be NULL */
static void
mh_object_fill_symbol (Symbol *symbol)
{
  mh_object_t  *values;
  mh_object_t   object = symbol->machine;

  unsigned int count, idx = 0;

  /* There is no way the MACH_RES can be set but SYMBOL->MACHINE can be
     NULL.  Absolutely no way */
  assert (object);

  /* Specify the SYMBOL is not longer modified; only slightly
     premature */
  symbol_clear_mach_res (symbol);

  count  = MH_VECTOR_P (object)
    ? MH_VECTOR_LENGTH (MH_AS_VECTOR (object))
    : 1;
      
  values = MH_VECTOR_P (object)
    ? MH_VECTOR_VALUES (MH_AS_VECTOR (object))
    : & object;

  /* Clear SYMBOL then symbol_add_value() to SYMBOL one-by-one */
  symbol_reset (symbol);
  while (idx++ < count)
    {
      string_t str = mh_object_to_string (*values++, false);
      symbol_add_value (symbol, str);
      xfree (str);
    }

  /* The above symbol_add_value() will cause SYMBOL to appear to be
     modified.  But, we know that to be true and inconsequential; so, we
     clear the modified bit unconditionally.  The following and
     symbol_clear_mach_res() above imply that MACHINE and INTERPRETER are
     consistent. */
  symbol_clear_modified (symbol);
}

/*
 * mh_object_fill_symbol_if_appropriate ()
 *
 * The following function is installed as symbol_retrieve_hook and is
 * called whenever symbol_intern() or symbol_lookup() are called in the
 * MetaHTML interpreter.  If the MACHINE has been mucking with
 * SYMBOL->MACHINE and has thus set the MACH_RES flag, then the
 * symbol->values are inconsistent with symbol->machine.
 * mh_object_fill_symbol() will then be called to write the machine object
 * to the interpreter values */
extern void
mh_object_fill_symbol_if_appropriate (Symbol *name)
{
  if (symbol_get_mach_res (name))
    mh_object_fill_symbol (name);
}

/*
 * mh_object_restore_symbol()
 *
 * Using the values of NAME->VALUES; fill in the compiler's NAME->MACHINE
 * slot.  This is done unconditionally. */
extern void
mh_object_restore_symbol (Symbol *name)
{
  mh_object_t   result;

  switch (name->type)
    {
    case symtype_STRING:
      {
	unsigned int length = name->values_index;

	if (0 == length)
	  result = MH_OBJECT_EMPTY;
	else if (1 == length)
	  result = mh_object_read (name->values [0]);
	else
	  {
	    unsigned int count;
	    mh_vector_t  vector  = mh_vector_new (length);
	    mh_object_t *objects = MH_VECTOR_VALUES (vector);

	    for (count = 0; count < length; count++)
	      {
		objects [count] = 
		  mh_object_read (name->values [count]);
	      }
		    
	    result = MH_AS_OBJECT (vector);
	  }
      }
      break;

    case symtype_USERFUN:
      result = name->machine;
      break;

    default:
      /* Trouble in machine city - 
	 Use NULL for package-to-alist ... what?? */
      result = (void *) NULL;
      break;
    }

  /* This may be the first time that the machine has touch NAME.  If so we
     best be sure that the name->machine slot is a GC root.  Note that
     symbol_intern(), and its cousin(s), set name->machine to NULL upon
     symbol creation and that NULL has stood until now */
  if (result && ! name->machine)
    mh_memory_add_root ((mh_object_t *) & name->machine);

  name->machine = result;

  /* The INTERPRETER and MACHINE are now consistent */
  symbol_clear_modified (name);
  symbol_clear_mach_res (name);
}

/*
 * mh_object_{set,ref}_symbol ()
 *
 * The functions mh_object_set_symbol() and mh_object_ref_symbol() are
 * always called anytime the MACHINE needs to access (read or write) the
 * machine slot of SYMBOL.  */
static inline void
mh_object_set_symbol (Symbol     *symbol,
		      mh_object_t object)
{
  if (! symbol->machine)
    mh_memory_add_root ((mh_object_t *) & symbol->machine);

  symbol->machine = (void *) object;

  /* The SYMBOL is surely modified by the MACHINE.  Comparison of OBJECT to
     SYMBOL->MACHINE is inappropriate as a way to avoid setting this
     modified flag because substructure in SYMBOL->MACHINE might have
     changed.  */
  symbol_set_mach_res (symbol);

  /* SYMBOL->MACHINE has now overridden SYMBOL->VALUES so we can declare
     SYMBOL->VALUES as unmodified.  Normally it would be unmodified except
     in the case where the intepreter modified a value and then the machine 
     modified the same value *without* referencing it. */
  symbol_clear_modified (symbol);
}

static inline mh_object_t
mh_object_ref_symbol (Symbol *symbol)
{
  /* When the INTERPRETER has modified the SYMBOL values, the intepreter
     will have set the symbol's modified flag and the MACHINE must then,
     for consistency, get the new values into SYMBOL->MACHINE */
  if (symbol_get_modified (symbol))
    mh_object_restore_symbol (symbol);

#if 0
  if (symbol->machine && symbol->machine < (void*) 0x00001000)
    assert (0);
#endif

  /* symbol->machine is now safe to reference  */
  return MH_AS_OBJECT (symbol->machine);
}


/*
 * mh_object_set_name() // mh_object_set_name_in_package ()
 *
 * These two functions are used anytime the OBJECT/VALUE associated
 * with a NAME must be assigned by the machine.  They ensure that the
 * values used by the machine can remain in synch with the values used by
 * the interpreter. */
static inline void
mh_object_set_name (string_t    name,
		    mh_object_t object)
{
  mh_object_set_symbol
    (mh_symbol_intern_safely (name), object);
}

static inline void
mh_object_set_name_in_package (string_t    name,
			       Package    *package,
			       mh_object_t object)
{
  mh_object_set_symbol
    (mh_symbol_intern_in_package_safely (name, package), object);
}

/*
 * mh_object_ref_name() // mh_object_ref_name_in_package ()
 *
 * These two functions are used anytime the OBJECT/VALUE associated
 * with a NAME must be accessed.  As with the correspondingly named SET
 functions, these serve to maintain the INTEPRETER/MACHINE consistency. */
static inline mh_object_t
mh_object_ref_name (string_t name)
{
  return mh_object_ref_symbol
    (mh_symbol_intern_safely (name));
}

static inline mh_object_t
mh_object_ref_name_in_package (string_t name,
			       Package *package)
{
  return mh_object_ref_symbol
    (mh_symbol_intern_in_package_safely (name, package));
}


/*****************************************************************************
 *
 *
 * mh_package_to_alist () // mh_alist_to_package ()
 *
 *
 */
static mh_object_t
mh_package_to_alist (Package *package, boolean_t strip)
{
  Symbol     *symbol;
  Symbol    **symbols = symbols_of_package (package);

  mh_alist_t alist   = MH_AS_ALIST (mh_nil);

  if (symbols != NULL)
    while (NULL != (symbol = *symbols++))
      {
	mh_object_t name;
	mh_object_t value = mh_object_ref_symbol (symbol);
	
	if (true == strip || NULL == package->name)
	  name = MH_AS_OBJECT (mh_string_new (symbol->name));
	else
	  name = MH_AS_OBJECT
	    (mh_string_3_new (package->name, "::", symbol->name));

	if (NULL == value)
	  value = mh_nil;

	alist = mh_alist_new (alist, name, value);
      }
  return MH_AS_OBJECT (mh_alist_reverse (alist));
}

static void
mh_alist_to_package (mh_alist_t alist,
		     Package    *package)
{
  for (; ! MH_NIL_P (alist); alist = MH_ALIST_TAIL (alist))
    {
      /* Do something with '::' prefixes */

      /* Get ALIST into a package X */

      /* Copy symbols from X into PACKAGE */

      /* If any symbol name has '::', then rename the symbol */

    }

}


/************************************************************************
 *
 *
 * Continuations
 *
 */
typedef struct mh_cont *mh_cont_t;

struct mh_cont {
  mh_cont_t    next;
  mh_cont_t    parent;
  Package     *package;		/* MetaHTML proper */
  mh_tag_t     tag;
  mh_object_t  result;
  unsigned int size;		/* stack size >= 0 */
  int          cp_offset;
  int          sp_offset;
  mh_object_t  stack [1];	/* Actually stack[size] */
};

#define MH_CONT_PARENT( cont )    ((cont)->parent)
#define MH_CONT_NEXT( cont )      ((cont)->next)
#define MH_CONT_SIZE( cont )      ((cont)->size)
#define MH_CONT_TAG( cont )       ((cont)->tag)
#define MH_CONT_RESULT( cont )    ((cont)->result)
#define MH_CONT_PACKAGE( cont )   ((cont)->package)
#define MH_CONT_CODE( cont )      MH_TAG_CODE((cont)->tag)
#define MH_CONT_CP_OFFSET( cont ) ((cont)->cp_offset)
#define MH_CONT_SP_OFFSET( cont ) ((cont)->sp_offset)
#define MH_CONT_STACK_SIZE( cont )((cont)->size)
#define MH_CONT_STACK( cont )     ((cont)->stack)

#define MH_CONT_CP( cont )       			\
  (MH_CONT_CODE (cont) + MH_CONT_CP_OFFSET (cont))

#define MH_CONT_SP( cont )       			\
  (MH_CONT_STACK (cont) + MH_CONT_SP_OFFSET (cont))

#define MH_CONT_ALLOC_SIZE( size )				\
  MH_OBJECT_CHUNK (sizeof (struct mh_cont) + 			\
		   MAX (0, (size)-1) * sizeof (mh_object_t))

/* Forward Declaration */
static void
mh_cont_transfer_args (mh_cont_t     cont,
		       mh_tag_t      tag,
		       mh_object_t  *args,
		       size_t        args_count);

/*
 * Continuation Caching
 *
 */
#define MH_CONT_BUCKET_SIZE   10
#define MH_CONT_BUCKET_COUNT  25

static mh_cont_t 
mh_cont_cache [MH_CONT_BUCKET_COUNT];

/*
 * mh_cont_alloc ()
 *
 * Allocate and return a new CONT with stack SIZE from C memory */
static mh_cont_t
mh_cont_alloc  (unsigned int size)
{
  mh_cont_t cont = (mh_cont_t)
    xmalloc (MH_CONT_ALLOC_SIZE (size));
  MH_CONT_STACK_SIZE (cont) = size;
  return cont;
}

/* 
 * mh_cont_init ()
 *
 * Initialize a new CONT by installing a subset of the mh_cont_t fields as
 * GC roots.  And, ensure the said roots have GC-able bindings */
static void
mh_cont_init (mh_cont_t cont)
{
  unsigned int count = MH_CONT_STACK_SIZE (cont);
  mh_object_t *sp    = MH_CONT_STACK (cont);

  while (count--)
    {
      *sp = MH_OBJECT_EMPTY;
      mh_memory_add_root (sp++);
    }

  mh_memory_add_root ((mh_object_t *) & MH_CONT_TAG (cont));
  mh_memory_add_root (& MH_CONT_RESULT (cont));

  MH_CONT_TAG       (cont) = (mh_tag_t) MH_OBJECT_EMPTY;
  MH_CONT_RESULT    (cont) = MH_OBJECT_EMPTY;
  MH_CONT_SP_OFFSET (cont) = 0;
  MH_CONT_CP_OFFSET (cont) = 0;
}

/*
 * mh_cont_free ()
 *
 * Return a CONT to the C memory subsystem.  This is more (much more?) than 
 * just free() because the previously added GC roots must be removed */
static void
mh_cont_free (mh_cont_t cont)
{
  unsigned int count = MH_CONT_STACK_SIZE (cont);
  mh_object_t *sp    = MH_CONT_STACK (cont);

  while (count--)
    mh_memory_rem_root (sp++);

  mh_memory_rem_root ((mh_object_t *) & MH_CONT_TAG (cont));
  mh_memory_rem_root (& MH_CONT_RESULT (cont));

  free (cont);
}

/*
 * mh_cont_lookup ()
 *
 * Return a CONT either by looking it up in the cont cache or by creating a 
 * new one.  When a new one is created it is initialized. */
static mh_cont_t
mh_cont_lookup (unsigned int size)
{
  unsigned int cont_size = size;
  unsigned int bucket    = size / MH_CONT_BUCKET_SIZE;
  mh_cont_t cont = NULL;

  assert (cont_size >= 0);
  
  if (bucket < MH_CONT_BUCKET_COUNT)
    {
      cont = mh_cont_cache [bucket];
      if (cont)
	mh_cont_cache[bucket] = cont->next;
      else
	cont_size = (1+bucket) * MH_CONT_BUCKET_SIZE;
    }

  /* If unable to find a suitably size CONT in the cache then we are force
     to create (allocate, initialize and then free on return).  This is
     relatively expensive owing to GC issues; which is, after all, why we
     cache in the first place. */
  if (! cont)
    mh_cont_init ((cont = mh_cont_alloc (cont_size)));

  /* Needed whether or not inited */
  MH_CONT_NEXT   (cont) = (mh_cont_t) NULL;
  MH_CONT_PARENT (cont) = (mh_cont_t) NULL;

  return cont;
}

static inline void
mh_cont_return (mh_cont_t cont)
{
  unsigned int bucket;

  /* If the TAG didn't execute in the current package, then the tag has
     it's own package which must be discarded relevently */
  if (false == MH_TAG_CURRENT_PACKAGE_P (MH_CONT_TAG (cont)))
    {
      Package *package = MH_CONT_PACKAGE (cont);

      /* This pop had best match the preceeding PUSH otherwise trouble */ 
      symbol_pop_package ();

      /* For those cases where the package had no name we are required to
	 destroy the package - required because the package was created
	 when the tag was called. */
      if (package && package->name == NULL)
	symbol_destroy_package (package);
    }

  /* Clear out the stack contents.  This ensure that the GC, when tracing
     the mh_cont_t's roots, won't hold on to a lot of garbage. */
  {
    unsigned int count = MH_CONT_STACK_SIZE (cont);
    mh_object_t *sp    = MH_CONT_STACK (cont);

    while (count--)
      *sp++ = MH_OBJECT_EMPTY;

    MH_CONT_TAG    (cont) = (mh_tag_t) MH_OBJECT_EMPTY;
    MH_CONT_RESULT (cont) = MH_OBJECT_EMPTY;

  }

  /* Having finished with CONT we return it to the cache or if we allocated 
     it explicity (because of an excessive stack size) then just free it */

  /* Note there is an inconsistency in this bucket calculation.  We subtact 
     one here but not above, possibly. */
  bucket = (cont->size-1) / MH_CONT_BUCKET_SIZE;

  if (bucket < MH_CONT_BUCKET_COUNT)
    {
      MH_CONT_NEXT (cont)   = mh_cont_cache[bucket];
      mh_cont_cache[bucket] = cont;
    }
  else
    mh_cont_free (cont);
}

static inline void
mh_cont_fill (mh_cont_t    cont, 
	      mh_cont_t    parent, 
	      mh_tag_t     tag,
	      mh_object_t *args,
	      size_t       args_count)
{
  MH_CONT_PARENT   (cont) = parent;
  MH_CONT_TAG      (cont) = tag;
  MH_CONT_RESULT   (cont) = MH_OBJECT_EMPTY;

  MH_CONT_PACKAGE  (cont) = 
    (true == MH_TAG_CURRENT_PACKAGE_P (tag)
     ? CurrentPackage
     : symbol_get_package (MH_TAG_PACKNAME (tag)));

  MH_CONT_CP_OFFSET (cont) =  0;
  MH_CONT_SP_OFFSET (cont) = -1; /* Yes */

  assert (MH_CONT_SIZE (cont) >= MH_TAG_STACK_SIZE (tag));

  /* MetaHMTL */
  if (false == MH_TAG_CURRENT_PACKAGE_P (tag))
    symbol_push_package (MH_CONT_PACKAGE (cont));

  mh_cont_transfer_args (cont, tag, args, args_count);
}

static void
mh_cont_transfer_args (mh_cont_t    cont,
		       mh_tag_t     tag,
		       mh_object_t *params,
		       size_t       params_count)
{
  mh_object_t    obj;
  mh_argument_t  arg;
  mh_argument_t *args       = MH_TAG_ARGS (tag);
  unsigned int   args_count =  MH_TAG_ARGS_COUNT (tag);
  unsigned int   args_index = 0;

  Package *package = MH_CONT_PACKAGE (cont);
  
  if (args_count != params_count)
    assert (0);
    
  /* Augment PACKAGE with all ARGUMENTS defaulted to MH_EMPTY */
  for (args_index = 0; args_index < args_count; args_index++)
    {
      arg = args [args_index];
      obj = (args_index < params_count
	     ? params [args_index]
	     : MH_OBJECT_EMPTY);

      if (MH_ARGUMENT_REST  == MH_ARGUMENT_TYPE  (arg) &&
	  MH_ARGUMENT_ARRAY != MH_ARGUMENT_ARRAY (arg))
	obj = MH_AS_OBJECT (mh_object_to_string_object (obj, false));

      mh_object_set_name_in_package 
	(MH_ARGUMENT_NAME (arg), package, obj);
    }
}

/*****************************************************************************
 *
 *
 * BC Engine Stepping, Tracing and Performance Monitoring
 *
 */
boolean_t mh_machine_step_p      = false;
boolean_t mh_machine_perfmon_p   = false;
boolean_t mh_machine_externmon_p = false;
boolean_t mh_machine_memorymon_p = false;
boolean_t mh_machine_trace_p     = false;
unsigned int mh_machine_trace_depth = 0;

#if defined (MH_MACH_DEBUG)

/*
 * Tracing
 *
 */
extern void
mh_machine_whitespace (unsigned int count)
{
  while (count-- > 0)
    fputc (' ', stdout);
}

extern void
mh_machine_trace_call (mh_tag_t      tag,
		      unsigned int  args_count,
		      mh_object_t  *args)
{
  unsigned int count;
  unsigned int tag_name_size;

  for (count = 0; count < mh_machine_trace_depth; count++)
    fputs ("| ", stdout);

  /* The tag */
  fputc ('<', stdout);
  fputs (MH_TAG_NAME (tag), stdout);
  tag_name_size = strlen (MH_TAG_NAME (tag));
 
  /* Its args */
  for (count = 0; count < args_count; count++)
    {
      fputc (' ', stdout);
      mh_object_to_file (args[count], true, stdout);
    }
  fputs (">\n", stdout);
  
  mh_machine_trace_depth++;
}

extern void
mh_machine_trace_return (mh_object_t return_value)
{
  unsigned int count;

  mh_machine_trace_depth--;
  for (count = 0; count < mh_machine_trace_depth; count++)
    fputs ("| ", stdout);

  fputs ("---> ", stdout);
  mh_object_to_file (return_value, true, stdout);
  fputc ('\n', stdout);
}

/*
 * (Single) Stepping
 *
 */
static void
mh_machine_step (mh_object_t   *stack, 
		 mh_object_t   *sp,
		 mh_byte_code_t *cp)
{
  unsigned int offset = 0;

  printf ("Stack\n");
  for (; stack + offset <= sp; offset++)
    {
      printf ("  [%d]: ", offset);
      mh_object_to_file (stack[offset], true, stdout);
      printf ("\n");
    }
  
  mh_byte_code_instr_disassemble (cp, stdout);
  printf (" >");
  getchar();
}

/*
 * Performance Monitoring
 *
 */
typedef struct mh_machine_perf 
{
  unsigned int pair_call_count [MH_NUMBER_OF_BYTEOPS];
  unsigned int   call_count;
  struct timeval call_time;
} *mh_machine_perf_t;

static struct mh_machine_perf
mh_machine_perf_data [MH_NUMBER_OF_BYTEOPS];

static void
mh_machine_perf_update (mh_byte_code_t  this_op,
		       mh_byte_code_t  next_op,
		       struct timeval *interval)
{
  mh_machine_perf_t this_perf = &mh_machine_perf_data[this_op];

#if 0
  mh_byte_code_spec_t this_spec = &mh_byte_code_spec_table[this_op];
  mh_byte_op_t next_op = (*codes == MH_RETURN_OP ?
			  MH_RETURN_OP : *(codes + this_spec->length));
#endif

  this_perf->call_count++;
  this_perf->call_time.tv_sec  += interval->tv_sec;
  this_perf->call_time.tv_usec += interval->tv_usec;

  /* Recoord pairwise count */
  if (this_op != MH_RETURN_OP)
    this_perf->pair_call_count [next_op]++;
}

#if 0
static void
mh_machine_perf_clear (void)
{
  memset ((void *) mh_machine_perf_data, 0,
	  MH_NUMBER_OF_BYTEOPS * sizeof (mh_machine_perf_t));
}
#endif

extern void
mh_machine_perf_report (void)
{
  mh_byte_op_t op;
  printf
    ("\n\nByteCode Perf Report:  OP     COUNT      ELAPSED   PER_CALL (ms)\n");
  for (op = 0; op < MH_NUMBER_OF_BYTEOPS; op++)
    {
      mh_machine_perf_t    perf = & mh_machine_perf_data [op];
      mh_byte_code_spec_t spec = & mh_byte_code_spec_table [op];
      
      if (perf->call_count)
	{
	  printf ("%26s: %6d   %12.3f   %8.3f\n",
		  spec->name,
		  perf->call_count,
		  1000 * timeval_to_seconds (&perf->call_time),
		  (1000 * timeval_to_seconds (&perf->call_time)
		   / perf->call_count));
#if 0
	  if (op != MH_RETURN_OP)
	    for (next_op = 0; next_op < MH_NUMBER_OF_BYTEOPS; next_op++)
	      printf ("    %s: %d\n",
		      mh_byte_code_spec_table[next_op].name,
		      perf->pair_call_count [next_op]);
#endif
	  memset ((void *) perf, 0, sizeof (struct mh_machine_perf));
	}
    }
}


typedef struct mh_machine_perf_extern *mh_machine_perf_extern_t;

struct mh_machine_perf_extern
{
  string_t       expression;
  unsigned int   call_count;
  struct timeval call_time;
  mh_machine_perf_extern_t  next;
};

mh_machine_perf_extern_t mh_machine_perf_extern_list = NULL;

static mh_machine_perf_extern_t
mh_machine_perf_extern_lookup (string_t expression)
{
  mh_machine_perf_extern_t this = mh_machine_perf_extern_list;

  for (; this; this = this->next)
    if (0 == strcmp (expression, this->expression))
      return this;

  this = xcalloc (1, sizeof (struct mh_machine_perf_extern));
  this->expression = strdup (expression);
  this->next = mh_machine_perf_extern_list;
  mh_machine_perf_extern_list = this;
  return this;
}

static void
mh_machine_perf_extern_update (string_t expression,
			     struct timeval *interval)
{
  mh_machine_perf_extern_t this =
    mh_machine_perf_extern_lookup (expression);

  this->call_count++;
  this->call_time.tv_sec  += interval->tv_sec;
  this->call_time.tv_usec += interval->tv_usec;
}

static void
mh_machine_perf_extern_clear (void)
{
  mh_machine_perf_extern_t next, this = mh_machine_perf_extern_list;

  while (this)
    {
      next = this->next;
      xfree (this->expression);
      xfree (this);
      this = next;
    }

  mh_machine_perf_extern_list = NULL;
}

static void
mh_machine_perf_extern_report (void)
{
  struct timeval total = {0, 0};
  mh_machine_perf_extern_t this = mh_machine_perf_extern_list;

  if (mh_machine_perf_extern_list)
    printf
      ("\n
Extern Perf Report
  COUNT      ELAPSED   PER_CALL (ms)   EXPRESSION\n");

  for (; this; this = this->next)
    {
      printf (" %5d %12.3f   %8.3f\n     %s\n\n",
	      this->call_count,
	      1000 * timeval_to_seconds (&this->call_time),
	      (1000 * timeval_to_seconds (&this->call_time)
	       / this->call_count),
	      this->expression);
      total.tv_sec  += this->call_time.tv_sec;
      total.tv_usec += this->call_time.tv_usec;
    }
  if (mh_machine_perf_extern_list)
    printf ("Extern Pref Report Completed\n  Total Elapsed: %12.3f\n",
	    1000 * timeval_to_seconds (&total));
}

#endif /* defined (MH_MACH_DEBUG) */
  

#if defined (NEVER_USED)
static mh_object_t
mh_machine_format (mh_object_t *sp, 
		  unsigned int count)
{
  BPRINTF_BUFFER *buffer;
  unsigned int offset;
  unsigned int not_empty_count = 0;
  mh_string_t result;

  for (offset = 0; offset < count; offset++)
    if (! MH_EMPTY_P (sp[offset]))
      not_empty_count++;
  
  if (0 == not_empty_count)
    return MH_OBJECT_EMPTY;

  else if (1 == not_empty_count)
    for (offset = 0; offset < count; offset++)
      if (! MH_EMPTY_P (sp[offset]))
	return sp[offset];

  /* else */
  buffer =  bprintf_create_buffer ();
  for (offset = 0; offset < count; offset++)
    if (! MH_EMPTY_P (sp[offset]))
      mh_object_to_buffer (sp[offset], false, buffer);

  result = mh_string_new (buffer->buffer);
  bprintf_free_buffer (buffer);
  return MH_AS_OBJECT (result);
}
#endif

/*
 *
 * MH_WELCOME_TO_THE_MACHINE
 *
 */
static unsigned int mh_welcome_entry_count = 0;

static mh_object_t
mh_welcome_to_the_machine_internal (mh_tag_t      tag,
				    mh_object_t  *args,
				    size_t        args_count)
{
  /* STACK */
  mh_object_t *stack;		/* Stack Base */
  mh_object_t *sp;		/* Stack Pointer */
#define STACK_PUSH(o)      (*++sp = (o))
#define STACK_POP()        (*sp--)
#define STACK_REF(n)       (*(sp+(n)))
#define STACK_SET(o,n)     (*(sp+(n)) = (o))
#define STACK_TOP()        (*sp)
#define STACK_SET_TOP(o)   (*sp  = (o))
#define STACK_ADDR(n)      ( sp  + (n))
#define STACK_POP_N(n)     ( sp -= (n))
#define STACK_IS_EMPTY_P() ( sp  < stack )

  /* CODE */
  mh_byte_code_t *code;		/* Code Vector */
  mh_byte_code_t *cp;		/* Code Pointer */
  unsigned int    cp_offset;
#define OPERATOR()     (cp[0])
#define OPERAND(n)     (cp[(n)])
#define NEXTOP(code)    cp += MH_##code##_OP_LEN; break
#define NEXTOFF(offset) cp += (offset); break

  /* Code Constants */
  mh_object_t *constants;
#define CONSTANT(n)  (constants[(n)])	/* skip '&' */

  /* Current continuation */
  mh_cont_t cont;

#define MH_CONT_SAVE( cont_to_save )					\
  MH_CONT_CP_OFFSET (cont_to_save) = cp - MH_CONT_CODE  (cont_to_save);	\
  MH_CONT_SP_OFFSET (cont_to_save) = sp - MH_CONT_STACK (cont_to_save);

#define MH_CONT_RESTORE( cont_to_restore )	\
  cp        = MH_CONT_CP (cont_to_restore);	\
  sp        = MH_CONT_SP (cont_to_restore);	\
  stack     = MH_CONT_STACK (cont_to_restore);	\
  tag       = MH_CONT_TAG (cont_to_restore);	\
  code      = MH_TAG_CODE (tag);		\
  constants = MH_TAG_CONSTANTS (tag);		\
  cont      = cont_to_restore
  
#if defined (MH_MACH_DEBUG)
  struct timeval last_time;
  struct timeval this_time;
  mh_byte_code_t last_op;

  struct timeval extern_last_time;
  struct timeval extern_this_time;
  boolean_t      extern_seen_p;
#endif

  /* Shorthand */
  boolean_t   raw_p;
  string_t    string;
  mh_string_t name;
  mh_object_t object, object1, object2;

  Symbol *symbol = NULL;

  /* OFFSET/INDEX into CONSTANTS/STACK; COUNT of arguments */
  unsigned int offset, iter, count = 0;

  /* Axe when integrated with MHTML */
  static boolean_t randomized_p = false;

  mh_welcome_entry_count++;

#if 0
  mh_memory_gc_disable ();
#endif
  /* Get a continuation with the required stack size */
  cont = mh_cont_lookup (MH_TAG_STACK_SIZE (tag));

  /* Fill out the continuation */
  mh_cont_fill (cont, (mh_cont_t) NULL, tag, args, args_count);

  /* Restore CONT the first time to initialize all the locals */
  MH_CONT_RESTORE (cont);

  /* Initialize a random number generator */
  if (randomized_p == false)
    {
      randomized_p = true;
      srandom ((unsigned int) getpid ());
    }

#if defined (MH_MACH_DEBUG)
  /* Prime the LAST_TIME structure */
  last_time.tv_sec = last_time.tv_usec = 0;
  this_time.tv_sec = this_time.tv_usec = 0;
  last_op = 0;

  if (mh_machine_perfmon_p)
    gettimeofday (&last_time, NULL);

  /* Delay until after following (! tag) conditional */
  if (mh_machine_trace_p)
    mh_machine_trace_call (tag, args_count, args);

  extern_last_time.tv_sec = extern_last_time.tv_usec = 0;
  extern_this_time.tv_sec = extern_this_time.tv_usec = 0;
  extern_seen_p    = false;

  if (1 == mh_welcome_entry_count)
    mh_machine_perf_extern_clear ();

  if (mh_machine_memorymon_p && 1 == mh_welcome_entry_count)
    mh_memory_summarize ();

#endif

  while (1)
    {

#if defined (MH_MACH_DEBUG)
      if (mh_machine_step_p)
	mh_machine_step (stack, sp, cp);

      /* Figure that this ought to be after STEP_P */
      if (mh_machine_perfmon_p)
	last_op = OPERATOR();

#endif /* defined (MH_MACH_DEBUG) */
	  
      /* Tracing, Monitoring, */

      switch (OPERATOR())
	{

	case MH_CALL_OP:
	LABEL_call:
	  /* Argument count for the call */
	  count  = OPERAND (1);

	  tag =
	    MH_AS_TAG (STACK_REF (- count));

	  args = STACK_ADDR (1 - count);

	  /* Check validity of TAG */

	  /* mh_object_restore_symbol () needs to NULL machine
	     (which causes root trouble) */

	  /* Make a tag call */
	  {
	    /* Get a continuation with the proper stack size */
	    unsigned int call_count = count;
	    unsigned int call_size  = MH_TAG_STACK_SIZE (tag);
	    mh_cont_t    call_cont  = mh_cont_lookup (call_size);

#if defined (MH_MACH_DEBUG)
	    /* Delay until after following (! tag) conditional */
	    if (mh_machine_trace_p)
	      mh_machine_trace_call (tag, count, STACK_ADDR (1 - count));
#endif /* defined (MH_MACH_DEBUG) */

	    /* Fill out CALL_CONT with TAG and ARGS */
	    mh_cont_fill (call_cont, cont, tag, args, call_count);

	    /* Pop all the args and tag from the stack 
	       Variable AP still points to the args */
	    STACK_POP_N (1 + call_count);

	    /* Increment the CP for eventual return */
	    cp += MH_CALL_OP_LEN;

	    /* Record the stack and code pointers in the 'parent'
	       for eventual return. After this point there are no C
	       pointers within mh_welcome_to_the_machine() to any GC
	       allocated objects.  That is, afterall, the definition of a
	       GC safe point. */
	    MH_CONT_SAVE (cont);

	    /* Change the stack, args and code pointers and the
	       constants vector to reflect the new continuation */

#if 0
	    {
	      extern void mh_memory_gc_force (void);
		
	      mh_memory_gc_enable ();
	      mh_memory_gc ();
	      mh_memory_gc_disable ();
	    }
#endif

	    /* Goto the new continuation */
	    MH_CONT_RESTORE (call_cont);

	  }
	  break;

	case MH_CALL_TAIL_OP:
	  /* Unimplmented; just revert to MH_CALL_OP */
	  goto LABEL_call;
	  
	case MH_RETURN_OP:
	  /* The CONT will return OBJECT; either by pushing it onto the
	     stack or by returning it from mh_welcome_to_the_machine(). */
	  object = MH_CONT_RESULT (cont);

	  {
	    mh_cont_t   
	      child  = cont,
	      parent = MH_CONT_PARENT (child);

	    /* Discard the current continuation.  This is safe because
	       nothing in CHILD is referenced. */
	    mh_cont_return (child);

	    /* If there is no PARENT, then CONT is a continuation created
	       on entry to mh_welcome_...() and thus when returning from
	       CONT we actually return from mh_welcome_to_the_machine(). */
	    if (parent == (mh_cont_t) NULL)
	      goto LABEL_mh_exit;

	    /* Restore the parent continuation */
	    MH_CONT_RESTORE (parent);

	  }

	  /* Push the RETURN_VALUE */
	  STACK_PUSH (object);

#if defined (MH_MACH_DEBUG)
	  if (mh_machine_trace_p)
	    mh_machine_trace_return (object);
#endif /* defined (MH_MACH_DEBUG) */

	  /* Critically NEXTOP is not used because MH_CONT_RESTORE and the
	     setup in MH_CALL_OP ensure that CP points to the right code */
	  break;

	case MH_SHIFT_OP:
	  offset = OPERAND (1);
	  object = STACK_TOP ();
	  STACK_POP_N (offset);
	  STACK_PUSH (object);
	  NEXTOP (SHIFT);
	 
	case MH_EVAL_OP:
	  cp_offset = MH_EVAL_OP_LEN;
	  name      = MH_AS_STRING (STACK_POP ());

#if defined (MH_MACH_DEBUG)
	  /* Prime the EXTERN_LAST_TIME structure */
	  if (mh_machine_externmon_p && !extern_seen_p)
	    {
	      gettimeofday (&extern_last_time, NULL);
	      extern_seen_p = true;
	    }
#endif

	  /* Call MetaHTML interpreter's evaluate */
	  string =
	    mhtml_evaluate_string (MH_STRING_CHARS (name));

	  /* Get the returned STRING - as a string*/
	  object = MH_AS_OBJECT (mh_string_new (string));

#if defined (MH_MACH_DEBUG)
	  if (mh_machine_externmon_p)
	    {
	      gettimeofday (&extern_this_time, NULL);
	      timersub (&extern_this_time,
			&extern_last_time,
			&extern_last_time);
	      mh_machine_perf_extern_update
		(MH_STRING_CHARS (name),
		 &extern_last_time);
	      gettimeofday (&extern_last_time, NULL);
	    }
#endif

	  /* Done with string */
	  xfree (string);

	  /* Place on stack */
	  STACK_PUSH (object);
	  NEXTOFF (cp_offset);

	case MH_JUMP_OP:
	  cp = code + 256 * OPERAND (1) + OPERAND (2);
	  break;

	case MH_JUMP_IF_FALSE_OP:
	  object = STACK_POP ();
	  if (MH_EMPTY_P (object)) /* MH_FALSE_P (object) */
	    {
	      cp = code + 256 * OPERAND (1) + OPERAND (2);
	      break;
	    }
	  NEXTOP (JUMP_IF_FALSE);

	case MH_JUMP_IF_TRUE_OP:
	  object = STACK_POP ();
	  if (MH_NOT_EMPTY_P (object)) /* MH_NOT_FALSE_P (object) */
	    {
	      cp = code + 256 * OPERAND (1) + OPERAND (2);
	      break;
	    }
	  NEXTOP (JUMP_IF_TRUE);

	case MH_JUMP_IF_EQ_OP:
	  object1 = STACK_POP ();
	  object2 = STACK_POP ();

	  if (object1 == object2) /* no, not quite */
	    {
	      cp = code + 256 * OPERAND (1) + OPERAND (2);
	      break;
	    }
	  NEXTOP (JUMP_IF_EQ);

	case MH_ID_OP:
	  object1 = STACK_POP ();
	  object2 = STACK_TOP ();
	  STACK_SET_TOP
	    (MH_AS_OBJECT (mh_object_equal (object1, object2)
			   ? MH_TRUE
			   : MH_EMPTY));
	  NEXTOP (ID);
	  
	case MH_POP_OP:
	  object = STACK_POP (); /* ignored */
	  NEXTOP (POP);

	case MH_POP_N_OP:
	  offset = OPERAND (1);
	  STACK_POP_N (offset);
	  NEXTOP (POP_N);

	case MH_DUP_OP:
	  object = STACK_TOP ();
	  STACK_PUSH (object);
	  NEXTOP (DUP);

	case MH_DATA_OP:
	  offset = OPERAND (1);
	  object = CONSTANT (offset);
	  STACK_PUSH (object);
	  NEXTOP (DATA);

	case MH_DATA_LONG_OP:
	  offset = 256 * OPERAND (1) + OPERAND (2);
	  object = CONSTANT (offset);
	  STACK_PUSH (object);
	  NEXTOP (DATA_LONG);
	  
	case MH_OUT_OP:
	  object = STACK_POP ();
	  if (! MH_EMPTY_P (object))
	    MH_CONT_RESULT (cont) =
	      (MH_EMPTY_P (MH_CONT_RESULT (cont))
	       ? object
	       : mh_string_concat (MH_CONT_RESULT (cont), object));
	  NEXTOP (OUT);

	case MH_CAT_OP:
	  object2 = STACK_POP ();
	  object1 = STACK_TOP ();
	  STACK_SET_TOP (mh_string_concat (object1, object2));
	  NEXTOP (CAT);

	case MH_CAT_N_OP:
	  count = OPERAND (1);
	  switch (count)
	    {
	    case 0: object = MH_OBJECT_EMPTY; break;
	    case 1: object = STACK_POP ();    break;
	    case 2: 
	      object2 = STACK_POP ();
	      object1 = STACK_POP ();
	      object = mh_string_concat (object1, object2);
	      break;
	      
	    default:
	      object = MH_OBJECT_EMPTY;
	      while (count--)
		object = mh_string_concat
		  (object, STACK_REF (- count));
	      count = OPERAND (1);
	      STACK_POP_N (count);
	      break;
	    }
	  STACK_PUSH (object);
	  NEXTOP (CAT_N);

	  /*
	   *
	   * GET, GET_LONG, GET_VAR, GET_RAW, GET_VAR_RAW
	   *
	   */
	case MH_GET_OP:
	  cp_offset = MH_GET_OP_LEN;
	  raw_p     = false;
	  offset    = OPERAND (1);
	  name      = MH_AS_STRING (CONSTANT (offset));
	LABEL_get:		/* NAME, CP_OFFSET, and RAW_P */
	  object = mh_object_ref_name (MH_STRING_CHARS (name));
	  STACK_PUSH
	    (NULL == object
	     ? MH_OBJECT_EMPTY
	     : (!raw_p && MH_VECTOR_P (object)
		? MH_VECTOR_REF (MH_AS_VECTOR (object), 0)
		: MH_AS_OBJECT (mh_object_to_vector (object))));
	  NEXTOFF (cp_offset);

	case MH_GET_LONG_OP:
	  cp_offset = MH_GET_LONG_OP_LEN;
	  raw_p     = false;
	LABEL_get_long:
	  offset    = 256 * OPERAND (1) + OPERAND (2);
	  name      = MH_AS_STRING (CONSTANT (offset));
	  goto LABEL_get;

	case MH_GET_RAW_OP:
	  cp_offset = MH_GET_RAW_OP_LEN;
	  raw_p     = true;
	  goto LABEL_get_long;

	case MH_GET_VAR_OP:
	  cp_offset = MH_GET_VAR_OP_LEN;
	  raw_p     = false;
	LABEL_get_var:
	  name      = MH_AS_STRING (STACK_POP ());
	  if (MH_STRING_P (name))
	    goto LABEL_get;
	  STACK_PUSH (MH_OBJECT_EMPTY);
	  NEXTOFF (cp_offset);
	  
	case MH_GET_VAR_RAW_OP:
	  cp_offset = MH_GET_VAR_RAW_OP_LEN;
	  raw_p     = true;
	  goto LABEL_get_var;

	  /*
	   *
	   * SET, SET_LONG, SET_VAR, SET_RAW, SET_VAR_RAW
	   *
	   */
	case MH_SET_OP:
	  cp_offset = MH_SET_OP_LEN;
	  raw_p     = false;
	  offset    = OPERAND (1);
	  object    = STACK_TOP ();
	  name      = MH_AS_STRING (CONSTANT (offset));
	LABEL_set:
	  /* CP_OFFSET, NAME, OBJECT, and RAW_P,  */
	  mh_object_set_name
	    (MH_STRING_CHARS (name), 
	     (NULL == object
	      ? MH_OBJECT_NULL
	      : (!raw_p && MH_VECTOR_P (object)
		 ? MH_VECTOR_REF (MH_AS_VECTOR (object), 0)
		 : MH_AS_OBJECT (mh_object_to_vector (object)))));
	  STACK_SET_TOP (MH_OBJECT_EMPTY);
	  NEXTOFF (cp_offset);

	case MH_SET_LONG_OP:
	  cp_offset = MH_SET_LONG_OP_LEN;
	  raw_p     = false;
	  /* ... */
	LABEL_set_long:
	  offset    = 256 * OPERAND (1) + OPERAND (2);
	  object    = STACK_TOP ();
	  name      = MH_AS_STRING (CONSTANT (offset));
	  goto LABEL_set;

	case MH_SET_RAW_OP:
	  cp_offset = MH_SET_RAW_OP_LEN;
	  raw_p     = true;
	  goto LABEL_set_long;

	case MH_SET_VAR_OP:
	  cp_offset = MH_SET_VAR_OP_LEN;
	  raw_p     = false;
	  /* ... */
	LABEL_set_var:
	  object    = STACK_POP ();
	  name      = MH_AS_STRING (STACK_TOP ());
	  if (MH_STRING_P (name))
	    goto LABEL_set;
	  STACK_SET_TOP (MH_OBJECT_EMPTY);
	  NEXTOFF (cp_offset);

	case MH_SET_VAR_RAW_OP:
	  cp_offset = MH_SET_VAR_RAW_OP_LEN;
	  raw_p     = true;
	  goto LABEL_set_var;

	  /*
	   *
	   * FGET // FSET - Tag
	   *
	   */
	case MH_FGET_OP:
	  offset    = OPERAND (1);
	  cp_offset = MH_FGET_OP_LEN;
	  /* ... */
	LABEL_fget:
	  name  = MH_AS_STRING (CONSTANT (offset));
	  assert (MH_STRING_P (name));

	  symbol = mhtml_find_user_function_symbol
	    (MH_STRING_CHARS (name));

	  assert (symbol && MH_TAG_P (symbol->machine));

	  /* Was (MH_AS_OBJECT (symbol->machine)) */
	  STACK_PUSH (mh_object_ref_symbol (symbol));
	  NEXTOFF (cp_offset);

	case MH_FGET_LONG_OP:
	  offset    = 256 * OPERAND (1) + OPERAND (2);
	  cp_offset = MH_FGET_LONG_OP_LEN;
	  goto LABEL_fget;

	case MH_FSET_OP:
	  offset    = OPERAND (1);
	  cp_offset = MH_FGET_OP_LEN;
	LABEL_fset:

	  /* The TAG itself */
	  object = CONSTANT (offset);

	  /* Find the Symbol that will hold the TAG. */
	  symbol = mhtml_find_user_function_symbol
	    (MH_TAG_NAME (MH_AS_TAG (object)));

	  /* If there is no Symbol, then create one and proceed */
	  if (!symbol)
	    {
	      mhtml_add_user_function
		(user_DEFUN, 
		 MH_TAG_NAME (MH_AS_TAG (object)),
		 "",
		 NULL);

	      symbol = mhtml_find_user_function_symbol
		(MH_TAG_NAME (MH_AS_TAG (object)));

	      /* There had better by a Symbol by now */
	    }

	  /* Finally, associate the Symbol with the TAG */

	  /* Was symbol->machine = (void *) MH_AS_TAG (object); */
	  mh_object_set_symbol (symbol, object);
	  STACK_PUSH (MH_OBJECT_EMPTY);
	  NEXTOFF (cp_offset);

	case MH_FSET_LONG_OP:
	  offset    = 256 * OPERAND (1) + OPERAND (2);
	  cp_offset = MH_FSET_LONG_OP_LEN;
	  goto LABEL_fset;

	case MH_DATA_EMPTY_OP:
	  STACK_PUSH (MH_OBJECT_EMPTY);
	  NEXTOP (DATA_EMPTY);

	case MH_DATA_TRUE_OP:
	  STACK_PUSH (MH_OBJECT_TRUE);
	  NEXTOP (DATA_TRUE);

	case MH_DATA_FALSE_OP:
	  STACK_PUSH (MH_OBJECT_EMPTY);
	  NEXTOP (DATA_FALSE);

	case MH_DATA_ZERO_OP:
	  STACK_PUSH (MH_AS_OBJECT (MH_NUMBER_ZERO));
	  NEXTOP (DATA_ZERO);

	case MH_DATA_ONE_OP:
	  STACK_PUSH (MH_AS_OBJECT (MH_NUMBER_ONE));
	  NEXTOP (DATA_ONE);

	case MH_NOT_OP:
	  object = STACK_TOP ();
	  STACK_SET_TOP (MH_EMPTY_P (object)
			 ? MH_AS_OBJECT (MH_TRUE)
			 : MH_AS_OBJECT (MH_OBJECT_EMPTY));
	  NEXTOP (NOT);

	/*
	 *
	 * VAR_EXISTS / VAR_UNSET 
	 *
	 */
	case MH_VAR_EXISTS_OP:
	  name   = MH_AS_STRING (STACK_TOP ());
	  STACK_SET_TOP
	    (MH_STRING_P (name) && symbol_lookup (MH_STRING_CHARS (name))
	     ? MH_OBJECT_TRUE
	     : MH_OBJECT_EMPTY);
	  NEXTOP (VAR_EXISTS);

	case MH_VAR_UNSET_OP:
	  name   = MH_AS_STRING (STACK_TOP ());
	  if (MH_STRING_P (name))
	    {
	      symbol = symbol_lookup (MH_STRING_CHARS (name));
	      if (symbol)
		{
		  if (symbol->notifier)
		    {
		      *symbol->notifier = 0;
		      /* Clear out all values */
		    }
		  else if (!symbol_get_flag (symbol, sym_READONLY))
		    {
		      symbol_free (symbol_remove (MH_STRING_CHARS (name)));
		    }
		}
	    }
	  STACK_SET_TOP (MH_OBJECT_EMPTY);
	  NEXTOP (VAR_UNSET);
	  

	  /* These should come from a template - they should be coded
	     properly besides! */
	     
#define MH_BIN_OP( op, func )				\
	case MH_##op##_OP:				\
	  object2 = STACK_POP ();			\
	  object1 = STACK_TOP ();			\
	  STACK_SET_TOP (MH_AS_OBJECT			\
			 (func				\
			  (MH_AS_NUMBER (object1),	\
			   MH_AS_NUMBER (object2))));	\
	  NEXTOP (op)

	MH_BIN_OP (ADD, mh_number_add);
	MH_BIN_OP (SUB, mh_number_sub);
	MH_BIN_OP (MUL, mh_number_mul);
	MH_BIN_OP (DIV, mh_number_div);

#define MH_UN_OP( op, func )				\
	case MH_##op##_OP:				\
	  object = STACK_TOP ();			\
	  STACK_SET_TOP (MH_AS_OBJECT			\
			 (func				\
			  (MH_AS_NUMBER (object))));	\
	  NEXTOP (op)

	MH_UN_OP (INC, mh_number_inc);
	MH_UN_OP (DEC, mh_number_dec);
	MH_UN_OP (EQZ, mh_number_eqz);
	MH_UN_OP (NEZ, mh_number_nez);
	MH_UN_OP (INTEGER_P, mh_number_integer_p);
	MH_UN_OP (REAL_P,    mh_number_real_p);
	MH_UN_OP (SQRT,      mh_number_sqrt);
	MH_UN_OP (RANDOM,    mh_number_random);

#define MH_REL_OP( op, func )					\
	case MH_##op##_OP:					\
	  object2 = STACK_POP ();				\
	  object1 = STACK_TOP ();				\
	  STACK_SET_TOP (MH_AS_OBJECT				\
			 (func (MH_AS_NUMBER (object1),		\
				MH_AS_NUMBER (object2))));	\
	  NEXTOP (op)

	MH_REL_OP (EQ, mh_number_eq);
	MH_REL_OP (NE, mh_number_ne);
	MH_REL_OP (LT, mh_number_lt);
	MH_REL_OP (LE, mh_number_le);
	MH_REL_OP (GT, mh_number_gt);
	MH_REL_OP (GE, mh_number_ge);

	case MH_RANDOMIZE_OP:
	  object = STACK_TOP ();
  	  mh_number_randomize (MH_AS_NUMBER (object));
	  STACK_SET_TOP (MH_OBJECT_EMPTY);
	  NEXTOP (RANDOMIZE);

	case MH_INTEGER_OP:
	  object = STACK_TOP ();
	  STACK_SET_TOP (MH_AS_OBJECT
			 (mh_number_integer (MH_AS_NUMBER (object))));
	  NEXTOP (INTEGER);

	  /*
	   *
	   *  STRING
	   *
	   */
	case MH_STR_COMP_OP:
	  object2 = STACK_POP ();
	  object1 = STACK_TOP ();
	  STACK_SET_TOP (MH_AS_OBJECT
			 (mh_string_compare
			  ((MH_STRING_P (object1)
			    ? MH_AS_STRING (object1)
			    : mh_object_to_string_object (object1, false)),
			   (MH_STRING_P (object2)
			    ? MH_AS_STRING (object2)
			    : mh_object_to_string_object (object2, false)))));
	  NEXTOP (STR_COMP);

	case MH_STR_EQ_OP:
	  object  = STACK_POP (); /* caseless */
	  object2 = STACK_POP ();
	  object1 = STACK_TOP ();
	  STACK_SET_TOP (MH_AS_OBJECT
			 (mh_string_equal
			  ((MH_STRING_P (object1)
			    ? MH_AS_STRING (object1)
			    : mh_object_to_string_object (object1, false)),
			   (MH_STRING_P (object2)
			    ? MH_AS_STRING (object2)
			    : mh_object_to_string_object (object2, false)),
			   MH_EMPTY_P (object) ? false : true)));
	  NEXTOP (STR_COMP);

	case MH_STR_NEQ_OP:
	  object  = STACK_POP (); /* caseless */
	  object2 = STACK_POP (); /* string2 */
	  object1 = STACK_TOP (); /* string1 */
	  STACK_SET_TOP (MH_AS_OBJECT
			 (mh_string_not_equal
			  ((MH_STRING_P (object1)
			    ? MH_AS_STRING (object1)
			    : mh_object_to_string_object (object1, false)),
			   (MH_STRING_P (object2)
			    ? MH_AS_STRING (object2)
			    : mh_object_to_string_object (object2, false)),
			   MH_EMPTY_P (object) ? false : true)));
	  NEXTOP (STR_COMP);

	case MH_STR_LEN_OP:
	  object = STACK_TOP ();
	  STACK_SET_TOP (MH_AS_OBJECT
			 (mh_number_new
			  ((double) mh_string_length
			   ((MH_STRING_P (object)
			     ? MH_AS_STRING (object)
			     : mh_object_to_string_object (object, false))))));
	  NEXTOP (STR_LEN);

	case MH_STR_SUB_OP:
	  object2 = STACK_POP ();
	  object1 = STACK_POP ();
	  object  = STACK_TOP ();
	  STACK_SET_TOP (MH_AS_OBJECT
			 (mh_string_sub
			  ((MH_STRING_P (object)
			    ? MH_AS_STRING (object)
			    : mh_object_to_string_object (object, false)),
			   MH_AS_NUMBER (object1),
			   MH_AS_NUMBER (object2))));
	  NEXTOP (STR_SUB);

#define MH_STR_CASE_OP( op, func )		\
	case MH_##op##_OP:			\
	  object = STACK_TOP ();		\
	  if (MH_VECTOR_P (object))		\
	  object = MH_VECTOR_REF (MH_AS_VECTOR (object), 0); \
	  assert (MH_STRING_P (object));	\
	  func (MH_AS_STRING (object),		\
		MH_AS_STRING (object));		\
	  NEXTOP (op)

	MH_STR_CASE_OP (STR_DOWN,   mh_string_downcase);
	MH_STR_CASE_OP (STR_UP,     mh_string_upcase);
	MH_STR_CASE_OP (STR_CAP,    mh_string_capitalize);

	  /*
	   *
	   *  ALIST
	   *
	   */
	case MH_ALIST_OP:
	  count  = OPERAND (1);
	  object = MH_AS_OBJECT
	    (mh_alist_fill_pairs (STACK_ADDR (1 - 2 * count), count));
	  STACK_POP_N (2 * count);
	  STACK_PUSH (object);
	  NEXTOP (ALIST);

	case MH_ALIST_P_OP:
	  object = STACK_TOP ();
	  STACK_SET_TOP
	    (mh_alist_p (object)
	     ? MH_OBJECT_TRUE
	     : MH_OBJECT_EMPTY);
	  NEXTOP (ALIST_P);

	case MH_ALIST_NEXT_OP:
	  object = STACK_TOP ();
	  assert (mh_alist_p (object));
	  STACK_SET_TOP 
	    (MH_ALIST_P (object)
	     ? MH_AS_OBJECT (MH_ALIST_TAIL (MH_AS_ALIST (object)))
	     : MH_NIL);
	  NEXTOP (ALIST_NEXT);

	case MH_ALIST_GET_OP:
	  object2 = STACK_POP (); /* NAME  */
	  object1 = STACK_TOP (); /* ALIST */
	  if (! mh_alist_p (object1))
	    object1 = mh_object_to_alist (object1);
	  object  = mh_alist_get (MH_AS_ALIST (object1), object2);
	  STACK_SET_TOP
	    (NULL == object
	     ? MH_OBJECT_EMPTY
	     : (MH_VECTOR_P (object)
		? MH_VECTOR_REF (MH_AS_VECTOR (object), 0)
		: object));
	  NEXTOP (ALIST_GET);

	case MH_ALIST_GET_RAW_OP:
	  object2 = STACK_POP (); /* NAME  */
	  object1 = STACK_TOP (); /* ALIST */
	  if (! mh_alist_p (object1))
	    object1 = mh_object_to_alist (object1);
	  object  = mh_alist_get (MH_AS_ALIST (object1), object2);
	  STACK_SET_TOP (object ? object : MH_OBJECT_EMPTY);
	  NEXTOP (ALIST_GET);

	case MH_ALIST_SET_OP:
	  count  = OPERAND (1);
	  object = STACK_REF (- 2 * count); /* ALIST */

	  if (! mh_alist_p (object))
	    object = mh_object_to_alist (object);

	  for (iter = 0; iter < count; iter++)
	    {
	      mh_object_t value = STACK_POP ();
	      mh_object_t namex = STACK_POP ();

	      object = MH_AS_OBJECT
		(mh_alist_set (MH_AS_ALIST (object), namex, value));
	    }
	  /* Replace ALIST */
	  STACK_SET_TOP (object);
	  NEXTOP (ALIST_SET);

	case MH_ALIST_MOD_OP:
	  {
	    mh_object_t ref_value = STACK_POP();
	    mh_object_t ref_index = STACK_POP();
	    mh_object_t ref_name  = STACK_POP();
	    mh_object_t alist     = STACK_TOP();

	    if (!mh_alist_p (alist))
	      alist = mh_object_to_alist (alist);

	    /* Assume REF_INDEX is actually a number */
	    if (! MH_EMPTY_P (ref_index) && ! MH_NUMBER_P (ref_index))
	      ref_index = MH_AS_OBJECT (mh_object_to_number (ref_index));

	    if (MH_NUMBER_P (ref_index))
	      {
		object = mh_alist_get (MH_AS_ALIST (alist), ref_name);

		/* What about 
		   <alist-set-var <make-alist foo=0> foo[10]=1> */

		/* Turn object into a vector */
		if (! object)
		  object = MH_AS_OBJECT (mh_vector_new (0));
		else if (! MH_VECTOR_P (object))
		  object = MH_AS_OBJECT
		    (mh_object_to_vector (object));

		/* Extend OBJECT with REF_VALUE at REF_INDEX */
		object = MH_AS_OBJECT
		  (mh_vector_extend
		   (MH_AS_VECTOR (object), ref_index, ref_value));
	      }
	    else
	      /* <ALIST-SET-VAR ALIST NAME[garbage]=VALUE> */
	      object = ref_value; /* mh_vector_fill (&ref_value, 1); */

	    STACK_SET_TOP (object);
	  }
	  NEXTOP (ALIST_MOD);

	case MH_ALIST_REM_OP:
	  object2 = STACK_POP (); /* NAME */
	  object1 = STACK_TOP (); /* ALIST */

	  if (! mh_alist_p (object1))
	    object1 = mh_object_to_alist (object1);

	  STACK_SET_TOP
	    (MH_AS_OBJECT (mh_alist_rem (MH_AS_ALIST (object1), object2)));
	  NEXTOP (ALIST_REM);

	case MH_ALIST_HAS_OP:
	  object2  = STACK_POP (); /* NAME */
	  object1  = STACK_TOP (); /* ALIST */

	  if (! mh_alist_p (object1))
	    object1 = mh_object_to_alist (object1);

	  STACK_SET_TOP
	    (mh_alist_has (MH_AS_ALIST (object1), object2)
	     ? MH_OBJECT_TRUE
	     : MH_OBJECT_EMPTY);
	  NEXTOP (ALIST_HAS);

	case MH_ALIST_LEN_OP:
	  object = STACK_TOP ();

	  if (! mh_alist_p (object))
	    object = mh_object_to_alist (object);

	  STACK_SET_TOP
	    (MH_AS_OBJECT (mh_number_new
			   (mh_alist_length (MH_AS_ALIST (object)))));
	  NEXTOP (ALIST_LEN);

	case MH_ALIST_MERGE_OP:
	  object2 = STACK_POP ();
	  object1 = STACK_TOP ();

	  if (! mh_alist_p (object1))
	    object1 = mh_object_to_alist (object1);

	  if (! mh_alist_p (object2))
	    object2 = mh_object_to_alist (object2);

	  STACK_SET_TOP
	    (MH_AS_OBJECT (mh_alist_merge
			   (MH_AS_ALIST (object1),
			    MH_AS_ALIST (object2))));

	  NEXTOP (ALIST_MERGE);

	case MH_ALIST_NAMES_OP:
	  object = STACK_TOP ();

	  if (! mh_alist_p (object))
	    object = mh_object_to_alist (object);

	  STACK_SET_TOP
	    (MH_AS_OBJECT (mh_alist_names (MH_AS_ALIST (object))));
	  NEXTOP (ALIST_NAMES);

	case MH_ALIST_VALUES_OP:
	  object = STACK_TOP ();

	  if (! mh_alist_p (object))
	    object = mh_object_to_alist (object);

	  STACK_SET_TOP
	    (MH_AS_OBJECT (mh_alist_values (MH_AS_ALIST (object))));
	  NEXTOP (ALIST_VALUES);

	case MH_PACKAGE_TO_ALIST_OP:
	  object2 = STACK_POP (); /* strip */
	  object1 = STACK_TOP (); /* packname */
	  {
	    Package *package = MH_EMPTY_P (object1)
	      ? CurrentPackage
	      : symbol_lookup_package (MH_STRING_CHARS
				       (MH_AS_STRING (object1)));

	    STACK_SET_TOP
	      (NULL != package
	       ? MH_AS_OBJECT (mh_package_to_alist
			       (package, 
				MH_NOT_EMPTY_P (object2)))
	       : MH_NIL);
	  }
	  NEXTOP (PACKAGE_TO_ALIST);

	case MH_ALIST_TO_PACKAGE_OP:
	  object2 = STACK_POP (); /* packname */
	  object1 = STACK_TOP (); /* alist */
	  {
	    Package *package = MH_EMPTY_P (object1)
	      ? CurrentPackage
	      : symbol_lookup_package (MH_STRING_CHARS
				       (MH_AS_STRING (object1)));

	    if (! mh_alist_p (object1))
	      object1 = mh_object_to_alist (object1);

	    mh_alist_to_package (MH_AS_ALIST (object1), package);
	    STACK_SET_TOP (MH_OBJECT_EMPTY);
	  }
	  NEXTOP (ALIST_TO_PACKAGE);

	  /*
	   *
	   *  ARRAY
	   *
	   */ 
	case MH_ARRAY_OP:
	  count  = OPERAND (1);
	  {
	    mh_vector_t array = mh_vector_new (count);
	    while (count--)
	      MH_VECTOR_VALUES (array) [count] = STACK_POP ();
	    STACK_PUSH (MH_AS_OBJECT (array));
	  }
	  NEXTOP (ARRAY);

	case MH_ARRAY_REF_OP:
	  object2 = STACK_POP (); /* index */
	  object1 = STACK_TOP (); /* array */

	  if (! MH_VECTOR_P (object1))
	    object1 = MH_AS_OBJECT (mh_object_to_vector (object1));

	  STACK_SET_TOP
	    (mh_vector_ref (MH_AS_VECTOR (object1), object2));
	  NEXTOP (ARRAY_REF);

	case MH_ARRAY_SET_OP:
	  object  = STACK_POP (); /* value */
	  object2 = STACK_POP (); /* index */
	  object1 = STACK_TOP (); /* array */

	  {
	    mh_vector_t vector = MH_VECTOR_P (object1)
	      ? MH_AS_VECTOR (object1)
	      : mh_vector_new (0); /* ZERO, temporarily */

	    vector = mh_vector_extend
	      (MH_AS_VECTOR (vector),
	       object2,
	       object);
	  
	  STACK_SET_TOP
	    (0 == MH_VECTOR_LENGTH (vector)
	     ? MH_OBJECT_EMPTY
	     : (1 == MH_VECTOR_LENGTH (vector)
		? MH_VECTOR_REF (vector, 0)
		: MH_AS_OBJECT (vector)));
	  }
	  NEXTOP (ARRAY_SET);
	  
	case MH_ARRAY_LEN_OP:
	  object = STACK_TOP ();
	  STACK_SET_TOP 
	    (MH_VECTOR_P (object)
	     ? mh_array_length (MH_AS_VECTOR (object))
	     /* Don't coerce, GET_RAW did already */
	     : MH_AS_OBJECT (MH_NUMBER_ZERO));
	  NEXTOP (ARRAY_LEN);
	  
	case MH_ARRAY_REV_OP:
	  object = STACK_TOP ();
	  STACK_SET_TOP 
	    (MH_AS_OBJECT
	     (MH_VECTOR_P (object)
	      ? mh_vector_copy (MH_AS_VECTOR (object))
	      : mh_object_to_vector (object)));
	  mh_vector_reverse_inplace (MH_AS_VECTOR (STACK_TOP ()));
	  NEXTOP (ARRAY_REV);

	  /* <array-member item arrayvar> */
	case MH_ARRAY_MEM_OP:
	  object  = STACK_POP (); /* caseless */
	  object2 = STACK_POP (); /* array */
	  object1 = STACK_TOP (); /* item */

	  STACK_SET_TOP 
	    (MH_AS_OBJECT
	     (mh_vector_member
	      (MH_AS_VECTOR (mh_object_to_vector (object2)),
	       object1,
	       MH_EMPTY_P (object) ? false : true)));
	  NEXTOP (ARRAY_MEM);

	case MH_ARRAY_APP_OP:
	  object2 = STACK_POP (); /* array */
	  object1 = STACK_TOP (); /* item */

	  STACK_SET_TOP 
	    (MH_AS_OBJECT
	     (mh_vector_append
	      (MH_AS_VECTOR (mh_object_to_vector (object2)), object1)));
	  NEXTOP (ARRAY_APP);

	case MH_ARRAY_COPY_OP:
	  object = STACK_TOP ();

	  if (! MH_VECTOR_P (object))
	    object = MH_AS_OBJECT (mh_object_to_vector (object));
	  
	  STACK_SET_TOP
	    (MH_AS_OBJECT
	     (mh_vector_copy (MH_AS_VECTOR (object))));

	  NEXTOP (ARRAY_COPY);

	case MH_ARRAY_FORCE_OP:
	  object = STACK_TOP ();
	  STACK_SET_TOP
	    (MH_VECTOR_P (object)
	     ? object
	     : MH_AS_OBJECT (mh_object_to_vector (object)));
	  NEXTOP (ARRAY_FORCE);

	case MH_ARRAY_NEW_AT_OP:
	  object2 = STACK_POP (); /* value */
	  object1 = STACK_TOP (); /* index */
	  STACK_SET_TOP
	    (MH_AS_OBJECT
	     (mh_vector_extend (NULL, object1, object2)));
	  NEXTOP (ARRAY_NEW_AT);
	  
	default:
	  {
	    mh_byte_op_t        this_op = OPERATOR();
	    mh_byte_code_spec_t this_spec = 
	      & mh_byte_code_spec_table [this_op];

	    mh_tag_disassemble (tag, stdout);

	    if (this_op >= MH_NUMBER_OF_BYTEOPS)
	      mh_machine_fail ("", ENGINE_ERR_NOT_A_BYTEOP);
	    else
	      mh_machine_fail (this_spec->name,
			      ENGINE_ERR_UNIMPLEMENTED_BYTEOP);
	    break;
	  }
       }

#if defined (MH_MACH_DEBUG)
      if (mh_machine_perfmon_p)
	{
	  gettimeofday (&this_time, NULL);
	  timersub (&this_time, &last_time, &last_time);
	  mh_machine_perf_update (last_op, *cp, &last_time);
	  gettimeofday (&last_time, NULL);
	}
#endif /* defined (MH_MACH_DEBUG) */

      /* Back for another OPERATOR */
    }

LABEL_mh_exit:
  /* OBJECT must be relevent here and beyond */

#if defined (MH_MACH_DEBUG)
  if (mh_machine_trace_p)
    mh_machine_trace_return (object);

  if (mh_machine_perfmon_p && 1 == mh_welcome_entry_count)
    mh_machine_perf_report ();

  if (mh_machine_externmon_p && 1 == mh_welcome_entry_count)
    mh_machine_perf_extern_report ();

  if (mh_machine_memorymon_p && 1 == mh_welcome_entry_count)
    mh_memory_summarize ();
#endif

  assert (object);
#if 0
  mh_memory_gc_enable ();
#endif

  mh_welcome_entry_count--;

  /* End of the MACHINE */
  return (object);
}

extern mh_object_t
mh_welcome_to_the_machine (mh_tag_t tag,
			   mh_object_t  *args,
			   size_t        args_count)
{
  engine_err_type_t err_type = 0;

  if (0 != (err_type = setjmp (engine_err_jmp_buf)))
    {
      printf ("Machine Error: %s",
	      engine_err_type_name [err_type]);
      printf (engine_err_message ? " \"%s\"\n" : "\n", engine_err_message);
      return MH_OBJECT_EMPTY;
    }
  
  return mh_welcome_to_the_machine_internal (tag, args, args_count);
}

extern mh_object_t
mh_welcome_to_the_machine_with_time (mh_tag_t   tag,
				     mh_object_t  *args,
				     size_t        args_count,
				     struct timeval *tm)
{
  mh_object_t object;
  struct timeval start_time;

  gettimeofday (&start_time, NULL);
  
  object = mh_welcome_to_the_machine (tag, args, args_count);

  gettimeofday (tm, NULL);
  timersub (tm, &start_time, tm);

  return (object);
}

/* Thu Oct 10 22:53:12 1996.  */

