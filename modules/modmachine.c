/* modmachine.c: -*- C -*-  */

/*  Copyright (c) 2000, SupplySolution, Inc.
    Author: E. B. Gamble Jr. (ed.gamble@alum.mit.edu) */

#define MODULE_INITIALIZER_EXTRA_CODE mh_module_initialize ();

#include "modules.h"
#include "machine/machine.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static int machine_version_major = 0;
static int machine_version_minor = 90;

static void pf_machine_version (PFunArgs);
static void pf_disable_gc (PFunArgs);
static void pf_enable_gc (PFunArgs);
static void pf_gc_immediately (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag	                    complex? debug_level    code    */
  { "MACHINE::VERSION",	        0,	0,	pf_machine_version },
  { "MACHINE::DISABLE-GC",	0,	0,	pf_disable_gc },
  { "MACHINE::ENABLE-GC",	0,	0,	pf_enable_gc },
  { "MACHINE::GC-IMMEDIATELY",	0,	0,	pf_gc_immediately },
  { (char *)NULL,		0,	0,	(PFunHandler *)NULL }
};

static void mh_module_initialize (void);


MODULE_INITIALIZE ("modmachine", ftab)

static double
timeval_to_seconds (struct timeval *tm)
{
  return tm->tv_sec + tm->tv_usec / 1e6;
}

static mh_object_t *
mh_machine_apply_process_args (mh_tag_t tag,
			       Package *vars,
			       char    *body,
			       char    *attr,
			       unsigned int *result_args_count)
{
#if 0
  Symbol *params         = symbol_lookup_in_package (vars, "*pvars*");
  Symbol *params_if_name = symbol_lookup_in_package (vars, "*pvals*");
#endif
  
  char   *body_eval = NULL;

  mh_argument_t  arg;
  mh_argument_t *args       = MH_TAG_ARGS (tag);
  unsigned int   args_count = MH_TAG_ARGS_COUNT (tag);
  unsigned int   args_index = 0;
  unsigned int   args_pos_index = 0;

  mh_object_t *objects;
  mh_object_t  object;
  string_t     string;

  /* Settle this right up front */
  *result_args_count = args_count;
  
  if (0 == args_count) return (mh_object_t *) NULL;

  /* Leading WHITESPACE is axed from body */
  while (*body && whitespace (*body)) body++;

  /* Construct the OBJECTS returned */
  objects = xcalloc (args_count, sizeof (mh_object_t));

  for (args_index = 0; args_index < args_count; args_index++)
    {
      arg    = args [args_index];
      object = NULL;

      switch (MH_ARGUMENT_TYPE (arg))
	{
	case MH_ARGUMENT_BODY:
	  /* Evaluate the BODY if required */
	  {
	    boolean_t body_eval_p =
	      MH_ARGUMENT_EVALLED == MH_ARGUMENT_EVAL (arg);

	    if (body_eval_p && !body_eval)
	      body_eval = mhtml_evaluate_string (body);

	    object = MH_AS_OBJECT
	      (mh_string_new (body_eval_p ? body_eval : body));
	  }
	  break;

	case MH_ARGUMENT_ATTRIBUTES:
	  object = MH_AS_OBJECT (mh_string_new (attr));
	  break;

	  
	case MH_ARGUMENT_REQUIRED:
	case MH_ARGUMENT_OPTIONAL:
	  string = get_positional_arg (vars, args_pos_index++);
	  break;

	case MH_ARGUMENT_REST:
	  {
	    mh_vector_t  vector;
	    unsigned int args_rest_index = args_pos_index;
	    unsigned int rest_index = 0;
	    unsigned int rest_count = 10;
	    boolean_t    rest_evalled_p = 
	      MH_ARGUMENT_EVALLED == MH_ARGUMENT_EVAL (arg);

	    string_t     next;
	    string_t    *rest = (string_t *) 
	      xcalloc (rest_count, sizeof (string_t));

	    /* Gather */
	    while ((next = get_positional_arg (vars, args_rest_index++)))
	      {
		if (rest_index >= rest_count)
		  rest = xrealloc (rest, (rest_count += 10));
		rest [rest_index++] = next;
	      }

	    rest_count = rest_index;
	    vector = mh_vector_new (rest_count);

	    /* Place into VECTOR */
	    for (rest_index = 0; rest_index < rest_count; rest_index++)
	      MH_VECTOR_REF (vector, rest_index) = mh_object_read
		(rest_evalled_p
		 ? mhtml_evaluate_string (rest [rest_index])
		 : rest [rest_index]);

	    xfree (rest);
	    object = MH_AS_OBJECT (vector);
	  }
	  break;

	case MH_ARGUMENT_KEY:
	  string = get_value (vars, MH_ARGUMENT_NAME (arg));
	  break;
	}

      if (! string) string = "";

      objects [args_index] = object 
	? object
	: mh_object_read (MH_ARGUMENT_EVALLED == MH_ARGUMENT_EVAL (arg)
			  ? mhtml_evaluate_string (string)
			  : string);
    }

  if (body_eval) xfree (body_eval);

  return objects;
}

/* 
 * MH_MACHINE_APPLY
 *
 * Every compiled function installs a mh_machine_apply() in the PFunDesc
 * structure on the function name's symbol-value.  That way when MetaHTML
 * sees the function name in a tag, the evaluation results in a call to
 * mh_machine_apply().  As shown below, mh_machine_apply then uses FNAME to
 * determine what byte-coded function should be executed by
 * mh_welcome_to_the_machine(). */
static void
mh_machine_apply (mh_tag_t tag, PFunArgs, char *attr)
{
  /* Need to parse properly to handle pack::name */
  mh_object_t   object;
  char *result = (char *)NULL;

  struct timeval machine_time;

  if (debug_level > 5)
    page_debug ("Machine Apply  : <%s ...>\n", fname);

  if (MH_TAG_P (tag))
    {
      /* Number of ARGUMENTS in the upcoming MACHINE invocation */
      unsigned int args_count;

      /* The ARGUMENTS themselves */
      mh_object_t *args =mh_machine_apply_process_args
	(tag, vars, 
	 (empty_string_p (body->buffer) ? "" : body->buffer),
	 attr,
	 & args_count);

      object = mh_welcome_to_the_machine_with_time
	(tag, args, args_count, &machine_time);

      xfree (args);

      /* Memory Manage ARGS_VECTOR */
    }
  else
    {
      page_debug ("No mh_tag_t for %s!\n", fname);
      object = MH_AS_OBJECT (tag);
    }

  if (debug_level > 5)
    page_debug ("Machine Time: %.3f (ms)\n", 
		1000 * timeval_to_seconds (&machine_time));

  result = mh_object_to_string (object, false);
  bprintf_insert (page, start, "%s", result);
  *newstart += strlen (result);
  xfree (result);
}


/* Given a Symbol, get the machine slot of the symbol into a datablock
   representation, and return that consed datablock. */
static Datablock *
mh_machine_symbol_get_hook (Symbol *sym)
{
  BPRINTF_BUFFER *buffer;
  Datablock *block = datablock_create ((char *)NULL, 0);

  buffer = mh_object_serialize_to_buffer ((mh_object_t)sym->machine);
  block->length = buffer->bindex;
  block->data = buffer->buffer;
  free (buffer);
  return (block);
}

/* Given SYM and BLOCK, unpack the serialized representation of
   the machine slot of that symbol found in BLOCK. This is the first
   time SYM has been seen and we assume the SYM->VALUES are setup
   consistently with what we place in SYM->MACHINE.  Be careful to add 
   SYM->MACHINE as a root. */
static void
mh_machine_symbol_fill_hook (Symbol *sym, Datablock *block)
{
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();

  buffer->bindex = buffer->bsize = block->length;
  buffer->buffer = (char *)block->data;

  /* This is a whole bunch like mh_object_set_symbol() ! */
  sym->machine = (void *)mh_buffer_serialize_to_object (buffer);
  mh_memory_add_root ((mh_object_t *) & sym->machine);
  symbol_clear_modified (sym);
  symbol_clear_mach_res (sym);

  free (buffer);
}

static void
mh_module_initialize (void)
{
  Symbol *sym;

  mh_object_init ();
  
  mhtml_machine_apply_function_hook = 
    (COMPILER_APPLY_FUNCTION *) mh_machine_apply;
  symbol_retrieve_hook =
    (SYMBOL_FUNCTION *) mh_object_fill_symbol_if_appropriate;
  symbol_free_hook =
    (SYMBOL_FUNCTION *) mh_object_free_symbol;

  symbol_machfill_hook = mh_machine_symbol_fill_hook;
  symbol_machget_hook  = mh_machine_symbol_get_hook;

  sym = symbol_intern ("*MACHINE*::VERBOSE-DEBUGGING");
  symbol_notify_value (sym, (void*) &mh_machine_verbose_debugging);

  sym = symbol_intern ("*MACHINE*::PERFORMANCE-MONITOR-P");
  symbol_notify_value (sym, (void*) &mh_machine_perfmon_p);

  sym = symbol_intern ("*MACHINE*::EXTERN-MONITOR-P");
  symbol_notify_value (sym, (void*) &mh_machine_externmon_p);

  sym = symbol_intern ("*MACHINE*::MEMORY-MONITOR-P");
  symbol_notify_value (sym, (void*) &mh_machine_memorymon_p);

  sym = symbol_intern ("*MACHINE*::TRACE-P");
  symbol_notify_value (sym, (void*) &mh_machine_trace_p);

  sym = symbol_intern ("*MACHINE*::TRACE-DEPTH");
  symbol_notify_value (sym, (void*) &mh_machine_trace_depth);

  sym = symbol_intern ("*MACHINE*::STEP-P");
  symbol_notify_value (sym, (void*) &mh_machine_step_p);
}


DEFINE_SECTION (MACHINE-MODULE, compiler; internals,
"The <Meta-HTML> machine (implemented in modmachine.so) is the
dynamically loadable code segment that implements the byte code engine 
part of the compiler/engine suite.

The user-visible functions in this module are simply for controlling
and viewing the operation of the machine (if it is present).", "")

DEFVAR (*MACHINE*::VERBOSE-DEBUGGING,
	"Non-empty causes verboseness in the output of stepping through code")

DEFVAR (*MACHINE*::PERFORMANCE-MONITOR-P,
	"Non-empty means turn on the performance monitor -- this
 allows one to evaluate the performance of the machine as it executes
 a specific bit of code")

DEFVAR (*MACHINE*::EXTERN-MONITOR-P,
	"I don't really know what this does")

DEFVAR (*MACHINE*::MEMORY-MONITOR-P,
	"Non-empty means monitor memory usage, report results")
DEFVAR (*MACHINE*::TRACE-P,
	"Non-empty means to trace entry and exit from each byte op")

DEFVAR (*MACHINE*::TRACE-DEPTH, "Don't trace past this depth...")

DEFVAR (*MACHINE*::STEP-P,
	"When non-empty, you must hit the spacebar before each
 instruction will be executed")

DEFUNX (machine::version, ,
	"Return the version of the machine")
static void
pf_machine_version (PFunArgs)
{
  bprintf_insert (page, start, "%d.%d",
		  machine_version_major, 
		  machine_version_minor);
}

DEFUNX (machine::enable-gc, ,
	"Allow the machine to garbage collect when it sees fit to.");
static void
pf_enable_gc (PFunArgs)
{
  mh_memory_gc_enable ();
}

DEFUNX (machine::disable-gc, ,
	"Don't allow the machine to garbage collect when it sees fit to.");
static void
pf_disable_gc (PFunArgs)
{
  mh_memory_gc_disable ();
}

DEFUNX (machine::gc-immediately, ,
	"Start a garbage collection of machine objects right now!");
static void
pf_gc_immediately (PFunArgs)
{
  mh_memory_gc_force ();
}

#if defined (__cplusplus)
}
#endif

