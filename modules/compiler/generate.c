/* generate.c: -*- C -*-  */

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

#include "compiler/compile.h"
#include "compiler/corex.h"

/* #include "machine/code.h" */

/*********************************************************************
 *
 *
 * MH_LAP_T
 *
 */
typedef struct mh_lap {
  mh_byte_code_t instr [MH_MAXIMUM_BYTEOP_LEN];
  struct mh_lap *next;
} *mh_lap_t;

#define MH_LAP_OPERATOR( lap )     ((lap)->instr[0])
#define MH_LAP_OPERAND( lap, n )   ((lap)->instr[(n)]) /* ONE based */
#define MH_LAP_NEXT( lap )         ((lap)->next)
#define MH_LAP_INSTR_LENGTH( lap ) \
  MH_BYTE_CODE_SPEC_LENGTH (& mh_byte_code_spec_table [ MH_LAP_OPERATOR (lap)])

#define FOR_LAP( lap, lap_head ) \
  for ( lap = (lap_head); lap ; lap = lap->next)

/* Could cache these allocations */

static mh_lap_t
mh_lap_new (void)
{
  mh_lap_t lap = (mh_lap_t) xmalloc (sizeof (struct mh_lap));
  lap->next = (mh_lap_t) NULL;
  return (lap);
}

static void
mh_lap_free_special (mh_lap_t lap,
		     boolean_t recurse)
{
  if (lap == NULL) return;

  if (recurse)
    mh_lap_free_special (lap->next, recurse);

  free (lap);
}

static inline void
mh_lap_free (mh_lap_t lap)
{ mh_lap_free_special (lap, true); }

static boolean_t
mh_lap_equal (mh_lap_t lap_1,
	      mh_lap_t lap_2,
	      unsigned int operand_count)
{
  unsigned int offset = 0;
  for (; offset <= operand_count; offset++)
    if (lap_1->instr[offset] != lap_2->instr[offset])
      return false;
  return true;
}

/*****************************************************************************
 *
 * MH_LAP_QUEUE_T
 *
 * Record the head and tail of a lap sequence into the LAP_QUEUE.
 * This allows order (1) append operations.
 */
typedef struct mh_lap_queue {
  mh_lap_t head;
  mh_lap_t tail;
} mh_lap_queue_t;

static inline void
mh_lap_queue_init (mh_lap_queue_t *queue,
		   mh_lap_t head,
		   mh_lap_t tail)
{
  queue->head = head;
  queue->tail = tail;
}

static inline void
mh_lap_queue_clear (mh_lap_queue_t *queue)
{
  queue->head =
    queue->tail = (mh_lap_t) NULL;
}

static inline void
mh_lap_queue_explode (mh_lap_queue_t *queue,
		      mh_lap_t *head,
		      mh_lap_t *tail)
{
  *head = queue->head;
  *tail = queue->tail;
}

/* Need to handle NULL cases for TAIL and HEAD */

static void
mh_lap_queue_append_2 (mh_lap_queue_t *queue_1,
		       mh_lap_queue_t *queue_2)
{
  if (! queue_1->head)
    {
      queue_1->head = queue_2->head;
      queue_1->tail = queue_2->tail;
    }
  else
    {
      queue_1->tail->next = queue_2->head;
      queue_1->tail = queue_2->tail;
    }
}

#if defined (NOT_USED)
static void
mh_lap_queue_append_3 (mh_lap_queue_t *queue_1,
		       mh_lap_queue_t *queue_2,
		       mh_lap_queue_t *queue_3)
{
  mh_lap_queue_append_2 (queue_2, queue_3);
  mh_lap_queue_append_2 (queue_1, queue_2);
}

static void
mh_lap_queue_append_4 (mh_lap_queue_t *queue_1,
		       mh_lap_queue_t *queue_2,
		       mh_lap_queue_t *queue_3,
		       mh_lap_queue_t *queue_4)
{
  mh_lap_queue_append_3 (queue_2, queue_3, queue_4);
  mh_lap_queue_append_2 (queue_1, queue_2);
}
#endif

static void
mh_lap_queue_extend (mh_lap_queue_t *queue,
		     mh_lap_t lap)
{
  if (lap) assert (lap->next == (mh_lap_t) NULL);

  if (! queue->head)
    queue->head =
      queue->tail = lap;
  else
    {
      queue->tail->next = lap;
      queue->tail = lap;
    }
}
				 


/*****************************************************************************
 *
 *
 * ASSEMBLE
 *
 * Remember JUMP targets can be ZERO */
static void
mh_assemble (mh_lap_t lap,
	     unsigned int jump_label,
	     mh_byte_code_t **code_ptr_addr,
	     unsigned int   *code_count_addr)
{
  mh_byte_code_t *code, *cp;
  mh_lap_t lap_save = lap;
  unsigned int count = 0;

  unsigned int *jump_table = (unsigned int *)xmalloc
    ((1 + jump_label) * sizeof (unsigned int));

  /* Construct the JUMP_TABLE and determine the overall
     length of the byte_code vector*/
  FOR_LAP (lap, lap_save)
    {
       if (MH_LAP_OPERATOR (lap) == MH_LABEL_OP)
	{
	  unsigned int dex = 
	    MH_LAP_OPERAND (lap, 1) * 256 + MH_LAP_OPERAND (lap, 2);
	  jump_table [dex] = count;

	  /* Implementation restriction, (< count (ash 1 16)) */

	}
      else
	count += MH_LAP_INSTR_LENGTH (lap);
    }

  /* Patch the JUMP byte_ops with values from JUMP_TABLE */
  FOR_LAP (lap, lap_save)
    switch (MH_LAP_OPERATOR (lap))
      {
      case MH_JUMP_OP:
      case MH_JUMP_IF_FALSE_OP:
      case MH_JUMP_IF_TRUE_OP:
      case MH_JUMP_IF_EQ_OP:
	{
	  unsigned int dex = 
	    MH_LAP_OPERAND (lap, 1) * 256 + MH_LAP_OPERAND (lap, 2);
	  unsigned int cnt = jump_table [dex];
	  MH_LAP_OPERAND (lap, 1) = cnt / 256;
	  MH_LAP_OPERAND (lap, 2) = cnt % 256;
	  break;
	}
      default:
	break;
      }

  free (jump_table);
  lap = lap_save;

  cp = code = (mh_byte_code_t *) malloc (count * sizeof (mh_byte_code_t));
  if (! code)
    fail ();

  /* Move the lap instructions into the byte_code vector */
  FOR_LAP (lap, lap_save)
    if (MH_LAP_OPERATOR (lap) != MH_LABEL_OP)
      {
	mh_byte_code_t *instr = lap->instr;
	unsigned int dex, cnt;

	for (dex = 0, cnt = MH_LAP_INSTR_LENGTH (lap);
	     dex < cnt;
	     dex++)
	  *cp++ = *instr++;
      }

  *code_ptr_addr   = code;
  *code_count_addr = count;
  return;
}

/********************************************************************
 * 
 * PEEPHOLE OPTIMIZER
 *
 *
 */

/* Peephole optimization is a sign of a weak optimizer and it can be
   dangerous (for example, [DUP POP] => [] reduces the stack
   requirement by one but that one cannot be feed back to the
   generator). 

   Destructively modify LAP */
static void mh_peephole (mh_lap_t *lap_ptr)
{
  mh_lap_t lap = *lap_ptr;
  mh_lap_t lap_save = lap;
  mh_lap_t lap_next;

  FOR_LAP (lap, lap_save)
    switch (MH_LAP_OPERATOR (lap))
      {
	/* These are quite rare */
      case MH_GET_OP:
      case MH_GET_LONG_OP:
      case MH_GET_RAW_OP:
      case MH_GET_VAR_OP:
      case MH_FGET_OP:
      case MH_DATA_OP:
	for (lap_next = MH_LAP_NEXT (lap);
	     lap_next;
	     lap_next = MH_LAP_NEXT (lap_next))
	  /* Replace ... {SGET n} {SGET n} by {SGET n} {DUP} */
	  if (mh_lap_equal (lap, lap_next, 1))
	    {
	      MH_LAP_OPERATOR (lap_next)    = MH_DUP_OP;
	      MH_LAP_OPERAND  (lap_next, 1) = 0;
	    }
	  else
	    break /* from FOR */ ;

	break /* from CASE */ ;

	/* These are quite common */
      case MH_SET_OP:
      case MH_SET_LONG_OP:
      case MH_SET_RAW_OP:
      case MH_SET_VAR_OP:
      case MH_FSET_OP:
	lap_next = MH_LAP_NEXT (lap);
	if (lap_next && 
	    (MH_CAT_OP == MH_LAP_OPERATOR (lap_next) ||
	     MH_OUT_OP == MH_LAP_OPERATOR (lap_next)))
	  {
	    MH_LAP_OPERATOR (lap_next) = MH_POP_OP;
	  }
	break;

      default:
	break;
      }
  return;
}

/***********************************************************************
 *
 * MH_MACHINE_T
 *
 *
 */
#define MH_MACHINE_DEFAULT_NUMBER_OF_CONSTANTS 256
#define MH_MACHINE_DEFAULT_NUMBER_OF_LOCALS    256

typedef struct mh_machine_marker *mh_machine_marker_t;

struct mh_machine_marker
{
  mh_machine_marker_t next;
  
  int stack_size;
  int stack_limit;

  unsigned int jump_label;
};

static mh_machine_marker_t
mh_machine_marker_new (int stack_size,
		       int stack_limit,
		       unsigned int jump_label,
		       mh_machine_marker_t next)	       
{
  mh_machine_marker_t marker = xmalloc (sizeof (struct mh_machine_marker));
  marker->next = next;
  marker->stack_size  = stack_size;
  marker->stack_limit = stack_limit;
  marker->jump_label  = jump_label;
  return marker;
}

static void
mh_machine_marker_free (mh_machine_marker_t marker)
{
  memset ((void *) marker, 0, sizeof (struct mh_machine_marker));
  xfree (marker);
}



typedef struct mh_machine
{
  mh_vector_t  arguments;

  mh_object_t *constants;	/* constants */
  unsigned int constants_count;
  unsigned int constants_limit;

  /* Jump label - then functions have their own labels
     all starting at zero and easy to patch.  */
  unsigned int jump_label;

  /* ... */
  mh_object_t *stack_map;	/* locals to stack location */
  int stack_size;
  int stack_limit;

  mh_machine_marker_t marker;
} *mh_machine_t;

static mh_machine_t
mh_machine_new (void)
{
  mh_machine_t machine = (mh_machine_t) xmalloc (sizeof (struct mh_machine));

  machine->arguments = (mh_vector_t) NULL;

  machine->constants = (mh_object_t *) NULL;

  machine->constants_count = 
    machine->constants_limit = 0;

  machine->jump_label = 0;

  machine->stack_map = (mh_object_t *) NULL;

  machine->stack_size =
    machine->stack_limit = 0;

  machine->marker = (mh_machine_marker_t) NULL;

  return machine;
}

static void
mh_machine_free (mh_machine_t machine)
{
  if (machine->constants)
    free (machine->constants);

  /* Memory Manage ... */

  /* Clear */
  free (machine);
}

static unsigned int
mh_machine_new_label (mh_machine_t machine)
{
  return (machine->jump_label++);
}


static unsigned int
mh_machine_new_constant (mh_machine_t machine,
			 mh_object_t  constant)
{
  unsigned int offset = 0;

  /* Look for a pre-existing CONSTANT */ 
  for (; offset < machine->constants_count; offset++)
    if (mh_object_equal_with_case (constant, machine->constants [offset]))
      return offset;
  
  /* Increase the CONSTANTS is required */
  if (machine->constants_count == machine->constants_limit)
    {
      /* Increase the constants_limit */
      machine->constants_limit += MH_MACHINE_DEFAULT_NUMBER_OF_CONSTANTS / 5;
      if (machine->constants)
	machine->constants =
	  xrealloc (machine->constants, 
		    machine->constants_limit * 
		    sizeof (mh_object_t));
      else
	machine->constants =
	  xcalloc (machine->constants_limit,
		   sizeof (mh_object_t));
	
    }
  machine->constants [machine->constants_count] = constant;
  return (machine->constants_count++);
}

static void
mh_machine_stack_adjust (mh_machine_t machine,
			 int          count)
{
  machine->stack_size += count;
  if (machine->stack_size > machine->stack_limit)
    machine->stack_limit = machine->stack_size;
}

static void
mh_machine_push_marker (mh_machine_t machine,
			unsigned int jump_label)
{
  machine->marker = mh_machine_marker_new
    (machine->stack_size,
     machine->stack_limit,
     jump_label,
     machine->marker);
}

static void
mh_machine_pop_marker (mh_machine_t machine)
{
  mh_machine_marker_t marker = machine->marker;

  if (marker)
    {
      machine->marker = marker->next;
      mh_machine_marker_free (marker);
    }
}

static inline mh_machine_marker_t
mh_machine_top_marker (mh_machine_t machine)
{ return machine->marker; }
  

#if defined (NEVER_USED)
static void
mh_machine_args (mh_machine_t machine,
		 mh_vector_t  args)
{
  machine->arguments = args;
}
#endif

/********************************************************************
 * 
 *
 *  GENERATE
 *
 * Need a way to compare constants (particularly of mh_object_t
 * includes things besides mh_string_t).  Or just put all the objects
 * into the constants vector and forget duplicates?  Don't we already
 * have functions somewhere...
 *
 * General Lap Generators - by OPERAND count
 *
 */
static mh_lap_t 
mh_gen_op_ZERO (mh_machine_t machine,
		int stack_offset,
		mh_byte_op_t operator)
{
  mh_lap_t lap = mh_lap_new ();
  MH_LAP_OPERATOR (lap) = operator;
  mh_machine_stack_adjust (machine, stack_offset);
  return (lap);
}

static mh_lap_t 
mh_gen_op_ONE (mh_machine_t machine,
	       int stack_offset,
	       mh_byte_op_t operator,
	       mh_byte_op_t operand1)
{
  mh_lap_t lap = mh_lap_new ();
  MH_LAP_OPERATOR (lap)    = operator;
  MH_LAP_OPERAND  (lap, 1) = operand1;
  mh_machine_stack_adjust (machine, stack_offset);
  return (lap);
}

static mh_lap_t 
mh_gen_op_TWO (mh_machine_t machine,
	       int stack_offset,
	       mh_byte_op_t operator,
	       mh_byte_op_t operand1,
	       mh_byte_op_t operand2)
{
  mh_lap_t lap = mh_lap_new ();
  MH_LAP_OPERATOR (lap)    = operator;
  MH_LAP_OPERAND  (lap, 1) = operand1;
  MH_LAP_OPERAND  (lap, 2) = operand2;
  mh_machine_stack_adjust (machine, stack_offset);
  return (lap);
}

#if defined (NOT_USED)
static mh_lap_t
mh_gen_op_THREE (mh_machine_t machine,
		 int stack_offset,
		 mh_byte_op_t operator,
		 mh_byte_op_t operand1,
		 mh_byte_op_t operand2,
		 mh_byte_op_t operand3)
{
  mh_lap_t lap = mh_lap_new ();
  MH_LAP_OPERATOR (lap)    = operator;
  MH_LAP_OPERAND  (lap, 1) = operand1;
  MH_LAP_OPERAND  (lap, 2) = operand2;
  MH_LAP_OPERAND  (lap, 3) = operand3;
  mh_machine_stack_adjust (machine, stack_offset);
  return (lap);
}
#endif

/*
 *
 * Lap Generators for Byte Operators
 *
 */
static inline mh_lap_t 
mh_gen_op_LABEL (mh_machine_t machine,
		 unsigned long count)
{ 
  return (mh_gen_op_TWO (machine, 0, MH_LABEL_OP,
			 count / 256,
			 count % 256));
}

static inline mh_lap_t
mh_gen_op_CALL (mh_machine_t machine,
		unsigned int operand_count)
{
  /* Return one value, pop COUNT arguments and one function. */
  return (mh_gen_op_ONE (machine, - operand_count, MH_CALL_OP, 
			 operand_count));
}

static inline mh_lap_t
mh_gen_op_RETURN (mh_machine_t machine)
{ return (mh_gen_op_ZERO (machine, 0, MH_RETURN_OP)); }

static inline mh_lap_t
mh_gen_op_SHIFT (mh_machine_t machine,
		 unsigned long offset)
{ return mh_gen_op_ONE (machine, 1 - offset, MH_SHIFT_OP, offset); }

static inline mh_lap_t
mh_gen_op_JUMPING (mh_machine_t machine,
		   int           stack_offset,
		   unsigned long count,
		   mh_byte_op_t operator)
{
  return (mh_gen_op_TWO (machine, stack_offset, operator,
			 count / 256,
			 count % 256));
}

static inline mh_lap_t
mh_gen_op_JUMP (mh_machine_t machine,
		unsigned long count)
{ return (mh_gen_op_JUMPING (machine, 0, count, MH_JUMP_OP)); }

static inline mh_lap_t
mh_gen_op_JUMP_IF_FALSE (mh_machine_t machine,
			 unsigned long count)
{ return (mh_gen_op_JUMPING (machine, -1, count, MH_JUMP_IF_FALSE_OP)); }

static inline mh_lap_t
mh_gen_op_JUMP_IF_TRUE (mh_machine_t machine,
			 unsigned long count)
{ return (mh_gen_op_JUMPING (machine, -1, count, MH_JUMP_IF_TRUE_OP)); }

static inline mh_lap_t
mh_gen_op_JUMP_IF_EQ (mh_machine_t machine,
		      unsigned long count)
{ return (mh_gen_op_JUMPING (machine, -2, count, MH_JUMP_IF_EQ_OP)); }

static inline mh_lap_t
mh_gen_op_OUT (mh_machine_t machine)
{ return (mh_gen_op_ZERO (machine, -1, MH_OUT_OP)); }

static inline mh_lap_t
mh_gen_op_CAT_N (mh_machine_t machine,
	       unsigned int count)
{ return (mh_gen_op_ONE (machine, 1 - count, MH_CAT_N_OP, count)); }

static inline mh_lap_t
mh_gen_op_CAT (mh_machine_t machine)
{ return (mh_gen_op_ZERO (machine, -1, MH_CAT_OP)); }

static inline mh_lap_t
mh_gen_op_POP (mh_machine_t machine)
{ return (mh_gen_op_ZERO (machine, -1, MH_POP_OP)); }

static inline mh_lap_t
mh_gen_op_POP_N (mh_machine_t machine,
		 unsigned int count)
{ return mh_gen_op_ONE (machine, - count, MH_POP_N_OP, count); }

static inline mh_lap_t
mh_gen_op_DUP (mh_machine_t machine)
{ return (mh_gen_op_ZERO (machine, 1, MH_DUP_OP)); }

static inline mh_lap_t
mh_gen_op_DATA (mh_machine_t machine,
		unsigned int offset)
{ return offset < 256
    ? mh_gen_op_ONE (machine, 1, MH_DATA_OP, offset)
    : mh_gen_op_TWO (machine, 1, MH_DATA_LONG_OP,
		     offset / 256,
		     offset % 256); }

static inline mh_lap_t
mh_gen_op_DATA_SP (mh_machine_t machine,
		   mh_byte_op_t operator)
{ return (mh_gen_op_ZERO (machine, 1, operator)); }

static inline mh_lap_t
mh_gen_op_EVAL (mh_machine_t machine)
{ return mh_gen_op_ZERO (machine, 0, MH_EVAL_OP); }

/* Variable GET // Variable SET */
static inline mh_lap_t
mh_gen_op_GET (mh_machine_t machine,
	       unsigned int offset,
	       boolean_t    raw_p)
{
  return raw_p
    ? mh_gen_op_TWO (machine, 1, MH_GET_RAW_OP,
		     offset / 256,
		     offset % 256)
    : (offset < 256
       ? mh_gen_op_ONE (machine, 1, MH_GET_OP, offset)
       : mh_gen_op_TWO (machine, 1, MH_GET_LONG_OP,
			offset / 256,
			offset % 256));
}

static inline mh_lap_t
mh_gen_op_SET (mh_machine_t machine,
	       unsigned int offset,
	       boolean_t    raw_p)
{
  return raw_p
    ? mh_gen_op_TWO (machine, 0, MH_SET_RAW_OP,
		     offset / 256,
		     offset % 256)
    : (offset < 256
       ? mh_gen_op_ONE (machine, 0, MH_SET_OP, offset)
       : mh_gen_op_TWO (machine, 0, MH_SET_LONG_OP,
			offset / 256,
			offset % 256));
}

/* Function GET // Variable SET */
static inline mh_lap_t
mh_gen_op_FGET (mh_machine_t machine,
		unsigned int offset)
{ return offset < 256
    ? mh_gen_op_ONE (machine, 1, MH_FGET_OP, offset)
    : mh_gen_op_TWO (machine, 1, MH_FGET_LONG_OP,
		     offset / 256,
		     offset % 256); }

static inline mh_lap_t
mh_gen_op_FSET (mh_machine_t machine,
		unsigned int offset)
{ return offset < 256
    ? mh_gen_op_ONE (machine, 1, MH_FSET_OP, offset)
    : mh_gen_op_TWO (machine, 1, MH_FSET_LONG_OP,
		     offset / 256,
		     offset % 256); }

static inline mh_lap_t
mh_gen_op_SET_VAR (mh_machine_t machine,
		   boolean_t    raw_p)
{ 
  return mh_gen_op_ZERO (machine, -1,
			 (raw_p
			  ? MH_SET_VAR_RAW_OP
			  : MH_SET_VAR_OP));
}

static inline mh_lap_t
mh_gen_op_GET_VAR (mh_machine_t machine,
		   boolean_t    raw_p)
{ 
  return mh_gen_op_ZERO (machine, 0,
			 (raw_p
			  ? MH_GET_VAR_RAW_OP
			  : MH_GET_VAR_OP));
}

/*
 *
 * Lap Queue Generators - Helpers for Byte Core Ops
 *
 *
 */
typedef enum {
  MH_USAGE_FOR_VALUE,
  MH_USAGE_FOR_EFFECT,
  MH_USAGE_FOR_OUTPUT,
  MH_USAGE_FOR_RETURN
} mh_usage_t;

/* Forward Definition for recursive calls */
static void
mh_gen (mh_machine_t    machine,
	mh_lap_queue_t *queue,
	mh_core_t       core,
	mh_usage_t      usage);

static inline void
mh_gen_return (mh_machine_t    machine,
	       mh_lap_queue_t *queue)
{ mh_lap_queue_extend (queue, mh_gen_op_RETURN (machine)); }

static inline void
mh_gen_pop (mh_machine_t    machine,
	    mh_lap_queue_t *queue)
{ mh_lap_queue_extend (queue, mh_gen_op_POP (machine)); }

static inline void
mh_gen_out (mh_machine_t    machine,
	    mh_lap_queue_t *queue)
{ mh_lap_queue_extend (queue, mh_gen_op_OUT (machine)); }

static void 
mh_gen_tail (mh_machine_t    machine,
	     mh_lap_queue_t *queue,
	     mh_usage_t      usage)
{
  switch (usage)
    {
    case MH_USAGE_FOR_RETURN:
      mh_gen_out (machine, queue);
      mh_gen_return (machine, queue);
      break;

    case MH_USAGE_FOR_OUTPUT:
      mh_gen_out (machine, queue);
      break;

    case MH_USAGE_FOR_VALUE:
      break;

    case MH_USAGE_FOR_EFFECT:
      mh_gen_pop (machine, queue);
      break;
    }
}

static void
mh_gen_constant (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 mh_object_t     constant,
		 mh_usage_t      usage)
{
  if (usage != MH_USAGE_FOR_EFFECT) 
    {
      if (MH_EMPTY_P (constant))
	mh_lap_queue_extend
	  (queue, mh_gen_op_DATA_SP (machine, MH_DATA_EMPTY_OP));
      else if (MH_TRUE_P (constant))
	mh_lap_queue_extend
	  (queue, mh_gen_op_DATA_SP (machine, MH_DATA_TRUE_OP));
#if 0
      else if (MH_FALSE_P (constant))
	mh_lap_queue_extend
	  (queue, mh_gen_op_DATA_SP (machine, MH_DATA_FALSE_OP));
#endif
      else if (MH_NUMBER_P (constant) && 
	       0 == MH_NUMBER_VALUE (MH_AS_NUMBER (constant)))
	mh_lap_queue_extend
	  (queue, mh_gen_op_DATA_SP (machine, MH_DATA_ZERO_OP));
      else if (MH_NUMBER_P (constant) && 
	       1 == MH_NUMBER_VALUE (MH_AS_NUMBER (constant)))
	mh_lap_queue_extend
	  (queue, mh_gen_op_DATA_SP (machine, MH_DATA_ONE_OP));

      else 
	{
	  unsigned int offset = 
	    mh_machine_new_constant (machine, constant);

	  mh_lap_queue_extend (queue, mh_gen_op_DATA (machine, offset));
	}
      mh_gen_tail (machine, queue, usage);
    }
}

static void
mh_gen_stacked (mh_machine_t    machine,
		mh_lap_queue_t *queue,
		mh_core_t      *cores,
		unsigned int    cores_count)
{
  while (cores_count--)
    mh_gen (machine, queue, *cores++, MH_USAGE_FOR_VALUE);
}

static void
mh_gen_fun_ref (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 string_t        name,
		 mh_usage_t      usage)
{
  switch (usage)
    {
    case MH_USAGE_FOR_RETURN:
      /* Can't return a function */
      break;

    case MH_USAGE_FOR_OUTPUT:
      /* Can't output a function */
      mh_gen_out (machine, queue);
      break;

    case MH_USAGE_FOR_VALUE:
      {
	unsigned int offset = mh_machine_new_constant
	  (machine, MH_AS_OBJECT (mh_string_new (name)));

	mh_lap_queue_extend (queue, mh_gen_op_FGET (machine, offset));
	mh_gen_tail (machine, queue, usage);
      }
      break;

    case MH_USAGE_FOR_EFFECT:
      /* Nothing */
      break;
    }
}

static void
mh_gen_get (mh_machine_t    machine,
	    mh_lap_queue_t *queue,
	    boolean_t       raw_p,
	    mh_core_t       location,
	    mh_usage_t      usage);

static void
mh_gen_get_name (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 boolean_t       raw_p,
		 string_t        name,
		 mh_usage_t      usage)
{
  unsigned int offset;

  if (usage == MH_USAGE_FOR_EFFECT) 
    return;

  offset =  mh_machine_new_constant
    (machine, MH_AS_OBJECT (mh_string_new (name)));

  /* RAW_P */
  mh_lap_queue_extend (queue, mh_gen_op_GET (machine, offset, raw_p));

  mh_gen_tail (machine, queue, usage);
}

static void
mh_gen_get_array (mh_machine_t    machine,
		  mh_lap_queue_t *queue,
		  boolean_t       raw_p,
		  mh_core_t       loc,
		  mh_core_t       indx,
		  mh_usage_t      usage)
{
  /* A RAW get always does ARRAY_FORCE [which can't guarantee an array] */
  mh_gen_get (machine, queue, true, loc, MH_USAGE_FOR_VALUE);

  if (MH_CORE_NULL != indx)
    {
      mh_gen (machine, queue, indx, MH_USAGE_FOR_VALUE);
      mh_lap_queue_extend
	(queue, mh_gen_op_ZERO (machine, -1, MH_ARRAY_REF_OP));
    }
#if 0
  else
    mh_lap_queue_extend
      (queue, mh_gen_op_ZERO (machine, 0, MH_ARRAY_FORCE_OP));
#endif

  mh_gen_tail (machine, queue, usage);
}

static void
mh_gen_get (mh_machine_t    machine,
	    mh_lap_queue_t *queue,
	    boolean_t       raw_p,
	    mh_core_t       location,
	    mh_usage_t      usage)
{
  switch (MH_CORE_OP (location))
    {
    case MH_CORE_DATA:
      mh_gen_get_name (machine, queue, raw_p,
		       MH_CORE_DATA_STRING (location),
		       usage);
      break;

    case MH_CORE_ARRAY:
      mh_gen_get_array (machine, queue, raw_p,
			MH_CORE_ARRAY_LOC   (location),
			MH_CORE_ARRAY_INDEX (location),
			usage);
      break;

    default:
      /* Use RAW_P */
      mh_gen (machine, queue, location, MH_USAGE_FOR_VALUE);
      mh_lap_queue_extend
	(queue, mh_gen_op_GET_VAR (machine, raw_p));
      mh_gen_tail (machine, queue, usage);
    }
}


static void
mh_gen_set_name (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 string_t        name,
		 mh_core_t       value)
{
  unsigned int offset;

  mh_gen (machine, queue, value, MH_USAGE_FOR_VALUE);
	  
  offset =  mh_machine_new_constant
    (machine, MH_AS_OBJECT (mh_string_new (name)));

  mh_lap_queue_extend (queue, mh_gen_op_SET (machine, offset, false));
}

static void
mh_gen_set_array (mh_machine_t    machine,
		  mh_lap_queue_t *queue,
		  mh_core_t       loc,
		  mh_core_t       indx,
		  mh_core_t       value)
{
  if (MH_CORE_NULL != indx)
    {
      /* Fingers crossed */

      /* Generate the location - that is, a name */
      mh_gen (machine, queue, loc, MH_USAGE_FOR_VALUE);

      /* Duplicate it */
      mh_lap_queue_extend
	(queue, mh_gen_op_DUP (machine));
      
      /* Get its value */
      mh_lap_queue_extend
	(queue, mh_gen_op_GET_VAR (machine, true));

      mh_gen (machine, queue, indx,  MH_USAGE_FOR_VALUE);
      mh_gen (machine, queue, value, MH_USAGE_FOR_VALUE);
      mh_lap_queue_extend
	(queue, mh_gen_op_ZERO (machine, -2, MH_ARRAY_SET_OP));
      /* Should be assigned back to LOC for those cases where the
	 ARRAY_SET is forced to grow the array. Except there is no LOC 
	 that is easily accessible? */

      /* Set It */
      mh_lap_queue_extend
	(queue, mh_gen_op_SET_VAR (machine, true));
    }
  else
    {
      switch (MH_CORE_OP (loc))
	{
	case MH_CORE_DATA:
	  {
	    unsigned int offset =  mh_machine_new_constant
	      (machine, 
	       MH_AS_OBJECT (mh_string_new
			     (MH_CORE_DATA_STRING (loc))));

	    mh_gen (machine, queue, value, MH_USAGE_FOR_VALUE);

#if 0 /* mh_gen_op_SET with RAW_P of true */
	    mh_lap_queue_extend
	      (queue, mh_gen_op_ZERO (machine, 0, 
				      MH_ARRAY_FORCE_OP));
#endif
	    mh_lap_queue_extend
	      (queue, mh_gen_op_SET (machine, offset, true));
	  }
	  break;

	default:
	  mh_gen (machine, queue, loc,   MH_USAGE_FOR_VALUE);
	  mh_gen (machine, queue, value, MH_USAGE_FOR_VALUE);
#if 0
	  mh_lap_queue_extend
	    (queue, mh_gen_op_ZERO (machine, 0, 
				    MH_ARRAY_FORCE_OP));
#endif
	  mh_lap_queue_extend
	    (queue, mh_gen_op_SET_VAR (machine, true));
	  break;
	}
    }
}

static void
mh_gen_set  (mh_machine_t    machine,
	     mh_lap_queue_t *queue,
	     mh_core_t       loc,
	     mh_core_t       value)
{
  switch (MH_CORE_OP (loc))
    {
    case MH_CORE_DATA:
      mh_gen_set_name (machine, queue,
		       MH_CORE_DATA_STRING (loc),
		       value);
      break;

    case MH_CORE_ARRAY:
      mh_gen_set_array (machine, queue,
			MH_CORE_ARRAY_LOC   (loc),
			MH_CORE_ARRAY_INDEX (loc),
			value);
      break;

    default:
      mh_gen (machine, queue, loc,   MH_USAGE_FOR_VALUE);
      mh_gen (machine, queue, value, MH_USAGE_FOR_VALUE);
      mh_lap_queue_extend (queue, mh_gen_op_SET_VAR (machine, false));
      break;
    }
}


/**************************************************************************
 *
 *
 * Lap Queue Generators for Byte Core Ops
 *
 *
 */
static void
mh_gen_core_data (mh_machine_t    machine,
		  mh_lap_queue_t *queue,
		  mh_core_t       core,
		  mh_usage_t      usage)
{
  mh_gen_constant (machine, queue, 
		   MH_CORE_DATA_OBJECT (core),
		   usage);
}

static void
mh_gen_core_get (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 mh_core_t       core,
		 mh_usage_t      usage)
{
  mh_gen_get (machine, queue, false,
	      MH_CORE_GET_LOC (core),
	      usage);
}

static void
mh_gen_core_set (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 mh_core_t       core,
		 mh_usage_t      usage)
{
  mh_core_t   *cores       = MH_CORE_SET_KEYS (core);
  unsigned int cores_count = MH_CORE_SET_KEYS_COUNT (core);
  unsigned int cores_index = 0;

  for (; cores_index < cores_count; cores_index++)
    {
      switch (MH_CORE_OP (cores [cores_index]))
	{
	case MH_CORE_KEY:
	  mh_gen_set 
	    (machine, queue,
	     MH_CORE_KEY_LOC   (cores [cores_index]),
	     MH_CORE_KEY_VALUE (cores [cores_index]));
	  break;

	case MH_CORE_PRIM:
	  mh_gen (machine, queue, 
		  cores[cores_index],
		  MH_USAGE_FOR_EFFECT);
	  break;

	default:
	  assert (mh_core_is_type (cores[cores_index], MH_CORE_KEY));
	}

      if (cores_index + 1 < cores_count)
	mh_gen_tail (machine, queue, MH_USAGE_FOR_EFFECT);
    }

  mh_gen_tail (machine, queue, usage);
}

/*
 * mh_gen_core_key ()
 *
 * The KEY core should only be generated here when passing keyword
 * arguments. */
static void
mh_gen_core_key (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 mh_core_t       core,
		 mh_usage_t      usage)
{
  mh_core_t name   = MH_CORE_KEY_LOC   (core);
  mh_core_t value  = MH_CORE_KEY_VALUE (core);

  mh_gen (machine, queue, name,  MH_USAGE_FOR_VALUE);
  mh_gen (machine, queue, value, MH_USAGE_FOR_VALUE);

  /* mh_lap_queue_extend
     (queue, mh_gen_op_CONS (machine)); */

  assert (0);

  mh_gen_tail (machine, queue, usage);
}

/* ARRAY REFERENCE - ARRAY Assignment handled in SET */
static void
mh_gen_core_array (mh_machine_t    machine,
		   mh_lap_queue_t *queue,
		   mh_core_t       core,
		   mh_usage_t      usage)
{
  assert (0);
}

static void
mh_gen_core_app (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 mh_core_t       core,
		 mh_usage_t      usage)
{
  mh_core_t    operator = MH_CORE_APP_OPERATOR (core);
  mh_core_t   *operands = MH_CORE_APP_OPERANDS (core);
  unsigned int operand_count =
    MH_CORE_APP_OPERANDS_COUNT (core);

  /* OPERATOR must be a symbol as per MHTML and thus expand.c */
  /* Should code that into core.h */
  string_t name = MH_CORE_DATA == MH_CORE_OP (operator)
    ? MH_CORE_DATA_STRING (operator)
    : (fail(), "");

  mh_gen_fun_ref (machine, queue, name,     MH_USAGE_FOR_VALUE);
  mh_gen_stacked (machine, queue, operands, operand_count);

  switch (usage)
    {
#if defined (NOT_YET)
    case MH_USAGE_FOR_RETURN:
      mh_lap_queue_extend
	(queue, mh_gen_op_CALL_TAIL (machine, operand_count));
      break;
#endif      
    default:
      mh_lap_queue_extend
	(queue, mh_gen_op_CALL (machine, operand_count));
      mh_gen_tail (machine, queue, usage);
      break;
    }

}

static void
mh_gen_core_if (mh_machine_t    machine,
		mh_lap_queue_t *queue,
		mh_core_t       core,
		mh_usage_t      usage)
{
  /* This is way too simple and wrong... but good enough for now 
     Standing errors: 
       failed to merge CONS and ALT stacks,
         partial implementation does not duplicate machines
	 which allows ALT stack to compute locals based on CONS stack
	 and that causes lots of potential trouble.

	 On the positive side, this compiler doesn't use locals - so
	 maybe, just maybe we are ok? 
     */
  unsigned long 
    else_label = mh_machine_new_label (machine),
    exit_label = mh_machine_new_label (machine);

  unsigned int 
    stack_size_pred,
    stack_limit_pred,
    stack_limit_cons,
    stack_limit_alt;

  /* pred */
  mh_gen (machine, queue,
	  MH_CORE_IF_PRED (core),
	  MH_USAGE_FOR_VALUE);

  /* (JUMP_IF_FALSE else) */
  mh_lap_queue_extend
    (queue, mh_gen_op_JUMP_IF_FALSE (machine, else_label));

  /* Preserve the Stack */
  stack_size_pred  = machine->stack_size;
  stack_limit_pred = machine->stack_limit;
  
  /* cons */
  mh_gen (machine, queue,
	  MH_CORE_IF_CONS (core),
	  usage);

  if (MH_USAGE_FOR_RETURN != usage)
    /* (JUMP exit) */
    mh_lap_queue_extend
      (queue, mh_gen_op_JUMP (machine, exit_label));

  /* Stack: Record the CONS limit */
  stack_limit_cons = machine->stack_limit;

  /* Stack: Restore to PRED */
  machine->stack_size  = stack_size_pred;
  machine->stack_limit = stack_limit_pred;

  /* (LABEL else) */
  mh_lap_queue_extend
    (queue, mh_gen_op_LABEL (machine, else_label));

  /* alt */
  mh_gen (machine, queue,
	  MH_CORE_IF_ALT (core),
	  usage);

  if (MH_USAGE_FOR_RETURN != usage)
    /* (LABEL exit) */
    mh_lap_queue_extend
      (queue, mh_gen_op_LABEL (machine, exit_label));
  
  /* Stack: Record the ALT limit */
  stack_limit_alt = machine->stack_limit;

  /* Stack: Munge Limit to avoid CONS adding with ALT */
  machine->stack_limit = MAX (stack_limit_cons, stack_limit_alt);
}

static void
mh_gen_core_or (mh_machine_t    machine,
		mh_lap_queue_t *queue,
		mh_core_t       core,
		mh_usage_t      usage)
{
  unsigned long 
    done_label = mh_machine_new_label (machine);

  mh_core_t   *cores       = MH_CORE_OR_CORES (core);
  unsigned int cores_count = MH_CORE_OR_CORES_COUNT (core);
  
  while (cores_count--)
    {
      /* exp */
      mh_gen (machine, queue, *cores++, MH_USAGE_FOR_VALUE);

      if (cores_count)
	{
	  /*  DUP */
	  mh_lap_queue_extend
	    (queue, mh_gen_op_DUP (machine));
      
	  /* (JUMP_IF_TRUE else) */
	  mh_lap_queue_extend
	    (queue, mh_gen_op_JUMP_IF_TRUE (machine, done_label));

	  /*  POP */
	  mh_lap_queue_extend
	    (queue, mh_gen_op_POP (machine));
	}
    }

  /* (LABEL else) */
  mh_lap_queue_extend
    (queue, mh_gen_op_LABEL (machine, done_label));

  mh_gen_tail (machine, queue, usage);
}

static void
mh_gen_core_fmt (mh_machine_t    machine,
		 mh_lap_queue_t *queue,
		 mh_core_t       core,
		 mh_usage_t      usage)
{
  mh_core_t   *cores       = MH_CORE_FMT_CORES (core);
  unsigned int cores_count = MH_CORE_FMT_CORES_COUNT (core);
  unsigned int cores_index = 0;
  boolean_t    for_return  = (usage == MH_USAGE_FOR_RETURN);

#if defined (FMT_IS_BREAK_BLOCK)
  unsigned long 
    exit_label  = mh_machine_new_label (machine);
#endif

  if (for_return) 
    usage = MH_USAGE_FOR_VALUE;

#if defined (FMT_IS_BREAK_BLOCK)
  mh_machine_push_marker (machine, exit_label);
#endif

  switch (cores_count)
    {
    case 0: break;

    case 1:
      /* Last one: (for_return ? MH_USAGE_FOR_RETURN : usage) */
      mh_gen (machine, queue, cores[0], usage);
      break;

    default:
      mh_gen (machine, queue, cores[cores_index++], usage);
      for (; cores_index < cores_count; cores_index++)
	{
	  mh_gen (machine, queue, cores[cores_index], usage);
	  if (MH_USAGE_FOR_VALUE == usage)
	    mh_lap_queue_extend
	      (queue, mh_gen_op_CAT (machine));
	}

      /* Last one: (for_return ? MH_USAGE_FOR_RETURN : usage) */
      break;
    }

#if defined (FMT_IS_BREAK_BLOCK)
  mh_machine_pop_marker (machine);

  /* (LABEL exit) */
  mh_lap_queue_extend
    (queue, mh_gen_op_LABEL (machine, exit_label));
#endif

  if (for_return)
    mh_gen_tail (machine, queue, MH_USAGE_FOR_RETURN);
}

static void
mh_gen_core_while (mh_machine_t    machine,
		   mh_lap_queue_t *queue,
		   mh_core_t       core,
		   mh_usage_t      usage)
{
  /* The expansion and resulting core for WHILE always ensures that
     WHILE is surrounded by FMT.  Thus a BUFFER is in place. */
  unsigned long 
    entry_label = mh_machine_new_label (machine),
    exit_label  = mh_machine_new_label (machine);

  boolean_t for_return = (usage == MH_USAGE_FOR_RETURN);

  if (for_return) 
    usage = MH_USAGE_FOR_OUTPUT;

  if (MH_USAGE_FOR_VALUE == usage)
    mh_lap_queue_extend
      (queue, mh_gen_op_DATA_SP (machine, MH_DATA_EMPTY_OP));

  /* (LABEL entry) */
  mh_lap_queue_extend
    (queue, mh_gen_op_LABEL (machine, entry_label));

  /* test */
  mh_gen (machine, queue,
	  MH_CORE_WHILE_TEST (core),
	  MH_USAGE_FOR_VALUE);

  /* (JUMP_IF_FALSE exit) */
  mh_lap_queue_extend
    (queue, mh_gen_op_JUMP_IF_FALSE (machine, exit_label));
  
  mh_machine_push_marker (machine, exit_label);

  /* body */
  mh_gen (machine, queue,
	  MH_CORE_WHILE_BODY (core),
	  usage);		/* was VALUE followed below by OUT */

  mh_machine_pop_marker (machine);

  if (MH_USAGE_FOR_VALUE == usage)
    mh_lap_queue_extend
      (queue, mh_gen_op_CAT (machine));

  /* (JUMP entry) */
  mh_lap_queue_extend
    (queue, mh_gen_op_JUMP (machine, entry_label));

  /* (LABEL exit) */
  mh_lap_queue_extend
    (queue, mh_gen_op_LABEL (machine, exit_label));

  if (for_return)
    mh_gen_return (machine, queue);
}

static void
mh_gen_core_break (mh_machine_t    machine,
		   mh_lap_queue_t *queue,
		   mh_core_t       core,
		   mh_usage_t      usage)
{
  mh_core_t value =
    MH_CORE_BREAK_VALUE (core);

  /* Check if a break */
  if (MH_CORE_NULL == value)
    {
      mh_machine_marker_t marker = mh_machine_top_marker (machine);

      /* (JUMP marker_jump_label) */
      if (marker && marker->jump_label)
	{
	  unsigned int stack_offset = 
	    machine->stack_size - marker->stack_size;

	  switch (stack_offset)
	    {
	    case 0: break;
	    case 1:  
	      mh_lap_queue_extend
		(queue, mh_gen_op_POP (machine));
	      break;
	    default:
	      mh_lap_queue_extend
		(queue, mh_gen_op_POP_N (machine, stack_offset));
	      break;
	    }

	  mh_lap_queue_extend
	    (queue, mh_gen_op_JUMP (machine, marker->jump_label));
	}
      else
	mh_lap_queue_extend
	  (queue, mh_gen_op_DATA_SP (machine, MH_DATA_EMPTY_OP));
    }
  else
    {
      /* unsigned int stack_limit = machine->stack_limit;*/
      /* Requires BUFFER/OUT - all functions in FMT */

      mh_gen (machine, queue, value, MH_USAGE_FOR_VALUE);
      mh_gen_tail (machine, queue, MH_USAGE_FOR_RETURN);

      /* machine->stack_limit = stack_limit; */
    }
}

static void
mh_gen_core_with (mh_machine_t    machine,
		  mh_lap_queue_t *queue,
		  mh_core_t       core,
		  mh_usage_t      usage)
{
  /* Bindings need to be to unique names... so that shadowed variables
     don't see their values reassigned.  As:
        <defun foo bar> <with bar=10>...</with><get-var bar></defun>
     What does FOO return, the passed value or 10.  Must be the passed 
     value. */

  unsigned int stack_size_result;

  mh_core_t set  = MH_CORE_WITH_SET  (core);
  mh_core_t body = MH_CORE_WITH_BODY (core);

  stack_size_result    = machine->stack_size;

  mh_gen_core_set (machine, queue, set, MH_USAGE_FOR_EFFECT);

  /* Generate the code for BODY (in the context of KEYS) */
  mh_gen (machine, queue, body, usage);

  if (usage == MH_USAGE_FOR_VALUE)
    mh_lap_queue_extend
      (queue, mh_gen_op_SHIFT (machine, 
			       machine->stack_size -
			       stack_size_result));

#if 0
  switch (usage)
    {
    case MH_USAGE_FOR_VALUE:
      /* SHIFT return value to STACK_SIZE_RESULT */
      mh_lap_queue_extend
	(queue, mh_gen_op_SHIFT (machine, 
				 machine->stack_size - stack_size_result));
      break;

    case MH_USAGE_FOR_EFFECT:
      mh_lap_queue_extend
	(queue, mh_gen_op_POP_N (machine, 
				 machine->stack_size - stack_size_result));
      break;

    case MH_USAGE_FOR_OUTPUT:
      mh_lap_queue_extend
	(queue, mh_gen_op_SHIFT (machine, 
				 machine->stack_size - stack_size_result));
      mh_gen_out (machine, queue);
      break;
      
    case MH_USAGE_FOR_RETURN:
      /* Forget the stack, RETURN directly */
      mh_gen_tail (machine, queue, usage);
      break;
    }
#endif

}

static void
mh_gen_core_extern (mh_machine_t    machine,
		    mh_lap_queue_t *queue,
		    mh_core_t       core,
		    mh_usage_t      usage)
{
  mh_string_t string = mh_string_new (MH_CORE_EXTERN_FORM (core));

  unsigned int offset = 
    mh_machine_new_constant (machine, MH_AS_OBJECT (string));

  mh_lap_queue_extend
    (queue, mh_gen_op_DATA (machine, offset));
  mh_lap_queue_extend
    (queue, mh_gen_op_EVAL (machine));
  mh_gen_tail (machine, queue, usage);
}

/* What is this generating?? FSET, FGET */
static void
mh_gen_core_func (mh_machine_t    machine,
		  mh_lap_queue_t *queue,
		  mh_core_t       core,
		  mh_usage_t      usage)
{
  /* Generate a 'top-level' function - core has been produced that way. */
  mh_tag_t tag = 
    mh_generate (core);

  /* Add TAG as a machine constant */
  unsigned int offset = 
    mh_machine_new_constant (machine, MH_AS_OBJECT (tag));

  /* And the TAG'S name symbol */

  /* Do the fset */
  mh_lap_queue_extend
    (queue, mh_gen_op_FSET (machine, offset));

  mh_gen_tail (machine, queue, usage);
}

static void
mh_gen_core_prim_alist (mh_machine_t    machine,
			mh_lap_queue_t *queue,
			mh_core_t       core,
			mh_usage_t      usage)
{
  mh_core_t   *operands       = MH_CORE_PRIM_OPERANDS (core);
  unsigned int operands_count = MH_CORE_PRIM_OPERANDS_COUNT (core);
  unsigned int count;

  /* All must be KEY - very late to check */
  for (count = 0; count < operands_count; count++)
    {
      mh_core_t key = *operands++;

      mh_gen (machine, queue, MH_CORE_KEY_LOC   (key),  MH_USAGE_FOR_VALUE);
      mh_gen (machine, queue, MH_CORE_KEY_VALUE (key),  MH_USAGE_FOR_VALUE);
    }

  /* mh_gen_stacked (machine, queue, operands, operands_count); */
		  
  /* Generate the OPERATOR byte_op */
  mh_lap_queue_extend
    (queue, mh_gen_op_ONE (machine, 1 - operands_count,
			   MH_CORE_PRIM_OPERATOR (core),
			   MH_CORE_PRIM_OPERANDS_COUNT (core)));

  mh_gen_tail (machine, queue, usage);
}

static void
mh_gen_core_prim_alist_set (mh_machine_t    machine,
			    mh_lap_queue_t *queue,
			    mh_core_t       core,
			    mh_usage_t      usage)
{
  mh_core_t   *operands       = MH_CORE_PRIM_OPERANDS (core);
  unsigned int operands_count = MH_CORE_PRIM_OPERANDS_COUNT (core);
  unsigned int count;

  /* OPERANDS_COUNT includes ALIST itself */

  /* ALIST */
  mh_gen (machine, queue, 
	  operands [0],
	  MH_USAGE_FOR_VALUE);

  /* Skip alist, first operand */
  operands++; operands_count--;

  /* All must be KEY - very late to check */
  for (count = 0; count < operands_count; count++)
    {
      mh_core_t key = *operands++;

      mh_gen (machine, queue, MH_CORE_KEY_LOC   (key),  MH_USAGE_FOR_VALUE);
      mh_gen (machine, queue, MH_CORE_KEY_VALUE (key),  MH_USAGE_FOR_VALUE);
    }

  /* mh_gen_stacked (machine, queue, operands, operands_count); */

  /* Generate the OPERATOR byte_op */
  mh_lap_queue_extend
    (queue, mh_gen_op_ONE (machine, - operands_count,
			   MH_CORE_PRIM_OPERATOR (core),
			   operands_count));

  mh_gen_tail (machine, queue, usage);
}

static void
mh_gen_core_prim_n_to_1 (mh_machine_t    machine,
			 mh_lap_queue_t *queue,
			 mh_core_t       core,
			 mh_usage_t      usage)
{
  mh_core_t   *operands       = MH_CORE_PRIM_OPERANDS (core);
  unsigned int operands_count = MH_CORE_PRIM_OPERANDS_COUNT (core);
  unsigned int offset;

  for (offset = 0; offset < operands_count; offset++)
    mh_gen (machine, queue, operands [offset], MH_USAGE_FOR_VALUE);
  
  /* Generate the OPERATOR byte_op */
  mh_lap_queue_extend
    (queue, mh_gen_op_ONE (machine, 
			   1 - operands_count,
			   MH_CORE_PRIM_OPERATOR (core),
			   operands_count));

  mh_gen_tail (machine, queue, usage);
}


static void
mh_gen_core_prim (mh_machine_t    machine,
		  mh_lap_queue_t *queue,
		  mh_core_t       core,
		  mh_usage_t      usage)
{
  mh_byte_op_t operator       = MH_CORE_PRIM_OPERATOR (core);
  /* mh_core_t   *operands       = MH_CORE_PRIM_OPERANDS (core); */

  /* Do some optimization */
  switch (operator)
    {
    case MH_ALIST_OP:
      return mh_gen_core_prim_alist (machine, queue, core, usage);
    case MH_ALIST_SET_OP:
      return mh_gen_core_prim_alist_set (machine, queue, core, usage);
    case MH_ARRAY_OP:
      return mh_gen_core_prim_n_to_1 (machine, queue, core, usage);
    default:
      break;
    }

  mh_gen_stacked (machine, queue, 
		  MH_CORE_PRIM_OPERANDS (core),
		  MH_CORE_PRIM_OPERANDS_COUNT (core));

  /* Generate the OPERATOR byte_op */
  mh_lap_queue_extend
    (queue, mh_gen_op_ZERO (machine, 
			    1 - MH_CORE_PRIM_OPERANDS_COUNT (core),
			    MH_CORE_PRIM_OPERATOR (core)));

  mh_gen_tail (machine, queue, usage);
}

/*
 *
 *
 *
 *
 */
static void
mh_gen (mh_machine_t    machine,
	mh_lap_queue_t *queue,
	mh_core_t       core,
	mh_usage_t      usage)
{
  typedef void (*mh_gen_core_func_t) (mh_machine_t    x_machine,
				      mh_lap_queue_t *x_queue,
				      mh_core_t       x_core,
				      mh_usage_t      x_usage);

  static mh_gen_core_func_t mh_gen_core_funcs [MH_NUMBER_OF_CORE_OPS] =
  {
    mh_gen_core_data,
    mh_gen_core_get,
    mh_gen_core_set,
    mh_gen_core_key,
    mh_gen_core_array,
    mh_gen_core_app,
    mh_gen_core_if,
    mh_gen_core_or,
    mh_gen_core_fmt,
    mh_gen_core_while,
    mh_gen_core_break,
    mh_gen_core_with,
    mh_gen_core_extern,
    mh_gen_core_func,
    mh_gen_core_prim
  };

  mh_lap_queue_t queue_2;
  mh_lap_queue_clear (& queue_2);

  (* mh_gen_core_funcs [MH_CORE_OP (core)])
    (machine, & queue_2, core, usage);

  mh_lap_queue_append_2 (queue, & queue_2);
  return;
}

mh_tag_t 
mh_generate (mh_core_t core)
{
  mh_machine_t   machine = mh_machine_new ();
  mh_lap_queue_t queue;
  mh_tag_t  tag;
  mh_core_t body;
  
  mh_lap_queue_clear (& queue);

  /* Core had best be a tag */
  assert (MH_CORE_FUNC == MH_CORE_OP (core));
  
  tag  = MH_CORE_FUNC_TAG (core);
  body = MH_CORE_FUNC_BODY (core);

  /* mh_machine_args (machine, args); */

  /* Might generate argument code here.  In particular, when an
     argument is declared as an array, this is the time to force it to
     be an array.  As [GET x, ARRAY_FORCE, SET x] */ 

  /* Generate the code for body (in the context of ARGS */
  mh_gen (machine, & queue, body, MH_USAGE_FOR_OUTPUT);
  mh_gen_return (machine, & queue);
  
  /* Build a tag */
  {
    mh_lap_t lap = queue.head;

    mh_byte_code_t *code;
    unsigned int    code_count;

    /* Peephole optimization is a sign of a weak optimizer and it can be
       dangerous (for example, [DUP POP] => [] reduces the stack
       requirement by one but that one cannot be feed back to the
       generator).

       Destructively modify LAP */
    mh_peephole (& lap);
    
    /* Assemble LAP into the byte_code vector */
    mh_assemble (lap, machine->jump_label, & code, & code_count);

    /* Get the MACHINE constants into the tags.  Rather than
       copying the constants we hack our way there.  */
    mh_tag_install_machine
      (tag, code, code_count,
       machine->constants,
       machine->constants_count,
       machine->stack_limit);
	 
    /* Prevent upcoming mh_machine_free from freeing the constants */
    machine->constants = 0;

    /* Lose the LAP and the MACHINE */
    mh_lap_free (lap);
    mh_machine_free (machine);
  }

  return (tag);
}


#if defined (GEN_TEST)

#include "compiler/parse.h"
#include "compiler/compile.h"

extern mh_core_t mh_expand   (string_t  parse_source, mh_parse_t parse);
extern void      mh_optimize (mh_core_t core);
extern mh_tag_t  mh_generate (mh_core_t core);

extern int
main (int   argc,
      char *argv[])
{
#if 0
  string_t source = "<set-var foo=10 <get-var-once foo>=30 foo[1]=20 <get-var-once foo>[2]=40 <get-var-once foo[1]>[2]=50><get-var-once foo[1]><alist-get-var foo key><alist-set-var foo key=value><add foo 1><foo 1>";

string_t source =
  "<defsubst parser::canonicalize-var :pcv-var :pcv-pack whitespace=delete>\n\
  <defvar :pcv-pack \"^\">\n\
  <if <not <match <get-var-once <get-var-once :pcv-var>> \"::\">>\n\
      <set-var <get-var-once :pcv-var> =\n\
	<get-var-once :pcv-pack>::<get-var-once <get-var-once :pcv-var>>>>\n\
</defsubst>";

 string_t source = "<set-var foo=10 <get-var-once foo>=30 foo[1]=20 <get-var-once foo>[2]=40 <get-var-once foo[1]>[2]=50 foo[5]=200>";

string_t source =
  "<defun foo &rest bars[]><foreach bar bars><get-var-once bar></foreach></defun>";

string_t source =
  "<a href=\"<concat abc ?key  =   def>\"></a>";
#endif

string_t source =
  "<subst-in-string <get-var num> \"([0-9])\" \"\\\\1\n\">";
  
  mh_parse_t parse = mh_parse ("expand", source);
  mh_core_t  core;
  mh_tag_t   tag;

  printf ("STRING: %s\nPARSE :\n", source);
  mh_parse_show (parse);
  printf ("\n");

  /* expand */
  mh_object_init ();
  core = mh_expand ("main", parse);
  /* mh_parse_free (parse); */
  mh_core_show (core);
  printf ("\n");

  mh_optimize (core);
  tag = mh_generate (core);
  mh_tag_disassemble (tag, stdout);

  return 0;
}
#endif /* defined (PARSE_TEST) */

/* Wed Nov  6 16:28:50 1996.  */
