/* optimize.c: -*- C -*-  */

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

#include "compiler/corex.h"
#include "compiler/parse.h"

/*****************************************************************************
 *
 *
 * BC OPTIMIZE
 *
 */
static void
mh_opt (mh_core_t core);

static void
mh_opt_sequence (mh_core_t    *cores,
		 unsigned int  count)
{
  while (count--)
    mh_opt (*cores++);
}

/* mh_opt_core_data() has the critical responsibility of filling in
   MH_CORE_DATA_OBJECT() based on the contents of
   MH_CORE_DATA_STRING(). */
static void
mh_opt_core_data (mh_core_t core)
{
  string_t    string = MH_CORE_DATA_STRING (core);
  mh_object_t object;

  if (0 == strcmp ("", string))
    object = MH_AS_OBJECT (MH_EMPTY);
  else if (0 == strcmp ("true", string))
    object = MH_AS_OBJECT (MH_TRUE);
  else
    {
#if defined (OPTIMZE_WITH_DATA_READ)
      /* mh_object_read() will delete leading whitespace which is highly
	 undesireable.  Particularly since we have likely returned the
	 whitespace based on the coded request.  Also mh_object_read()
	 interprets a leading semicolon as a comment and returns
	 nothing; which is highly undesireable! */
      if (0 != index ("\n\r\t\f ;", *string))
	object = MH_AS_OBJECT (mh_string_new (string));
      else 
	/* There is significantly more to do here; in particular to
	   produce numbers, alists and the like. */ 
	object = mh_object_read (string);
#else
	object = MH_AS_OBJECT (mh_string_new (string));
#endif
    }

  MH_CORE_DATA_OBJECT (core) = object;
  return;
}

static void
mh_opt_core_if (mh_core_t core)
{
  mh_core_t if_pred = MH_CORE_IF_PRED (core);
  mh_core_t if_cons = MH_CORE_IF_CONS (core);
  mh_core_t if_alt  = MH_CORE_IF_ALT  (core);
  
  mh_opt (if_pred);

  switch (MH_CORE_OP (if_pred))
    {
    case MH_CORE_DATA:
      if (! mh_core_data_is_empty (if_pred))
	{
	  mh_opt (if_cons);
	  mh_core_copy (core, if_cons);
	  mh_core_free_special (if_cons, false);
	  mh_core_free (if_alt);
	}
      else
	{
	  mh_opt (if_alt);
	  mh_core_copy (core, if_alt);
	  mh_core_free_special (if_alt, false);
	  mh_core_free (if_cons);
	}
      mh_core_free (if_pred);
      break;

    case MH_CORE_SET:
      {
	mh_core_t new_core = mh_core_fmt_new
	  (mh_core_2 (if_pred, if_alt), 2);
	
	mh_core_copy (core, new_core);
	mh_core_free (if_cons);
	mh_opt (core);
      }
      break;
      
      /* Surely some others are guaranteed to return true or false ?? */
    default:
      mh_opt (if_cons);
      mh_opt (if_alt);
      break;
    }
}

static void
mh_opt_core_or (mh_core_t core)
{
  /* Distructively modify CORES */
  mh_core_t   *cores = MH_CORE_OR_CORES (core);
  unsigned int count = MH_CORE_OR_CORES_COUNT (core);
  unsigned int offset;

  for (offset = 0; offset < count; offset++)
    if (mh_core_is_type (cores[offset], MH_CORE_DATA))
      {
	/* If *CORES is not empty, then we will never continue with
	   the remaining cores */
	if (! mh_core_data_is_empty (cores[offset]))
	  {
	    MH_CORE_OR_CORES_COUNT (core) = ++offset;	      
	    break; /* FOR */
	  }
	else
	  {
	    /* If *CORES is empty, then ignore this CORE.  
	       For now, do nothing. */
	    ;
	  }
      }

  /* Free what is left */
  if (offset < count)
    {
      for (; offset < count; offset++)
	mh_core_free (cores [offset]);

      switch (MH_CORE_OR_CORES_COUNT (core))
	{
	case 0:
	  xfree (cores);
	  MH_CORE_OR_CORES (core) = NULL;
	  break;

	case 1:
	  mh_core_copy (core, cores[0]);
	  mh_core_free_special (cores[0], false);
	  xfree (cores);
	  break;

	default:
	  /* Realloc */
	  MH_CORE_OR_CORES (core) =	(mh_core_t *)
	    xrealloc (MH_CORE_OR_CORES (core),
		      MH_CORE_OR_CORES_COUNT (core) * sizeof (mh_core_t));
	  break;
	}
    }
}


/*
 * Two or more FMT/PROG cores back-to-back optimize to one FMT core.
 *   Here in mh_opt_core_fmt() or more likely mh_opt_core_prog() or
 *   mh_opt_sequence().
 */


static void
mh_opt_core_with (mh_core_t core)
{
#if ! defined (DYNAMIC_SCOPING_IS_FORBIDDEN)
  return;
#else
  mh_core_t set  = MH_CORE_WITH_SET  (core);
  mh_core_t body = MH_CORE_WITH_BODY (core);

  mh_core_t   *keys       = MH_CORE_SET_KEYS (set);
  unsigned int keys_count = MH_CORE_SET_KEYS_COUNT (set);
  unsigned int keys_index;

  mh_core_t   *keys_that_remain;
  unsigned int keys_that_remain_index = 0;

  keys_that_remain = (mh_core_t *)
    xmalloc (keys_count * sizeof (mh_core_t *));
  
  /* Any NAME in KEYS that is bound to DATA gets substituted. */
  for (keys_index = 0; keys_index < keys_count; keys_index++)
    {
      mh_core_t key = keys [keys_index];
      mh_core_t key_loc   = MH_CORE_KEY_LOC   (key);
      mh_core_t key_value = MH_CORE_KEY_VALUE (key);
      
      
      if (mh_core_is_type (key_loc, MH_CORE_DATA) &&
	  (mh_core_is_type (key_value, MH_CORE_DATA) ||
	   mh_core_is_type (key_value, MH_CORE_VAR)))
	{
	  mh_core_var_subst (body, MH_CORE_DATA_STRING (key_loc), key_value);
	}
      else if (mh_core_is_type (key_loc, MH_CORE_DATA) &&
	       (mh_core_is_side_effecting (key_value) ||
		mh_core_var_is_referenced (body, 
					   MH_CORE_DATA_STRING (key_name))))
	{
	  keys_that_remain [keys_that_remain_index++] = key;
	}
      else
	/* ignore key */ ;
    }

  mh_opt (body);

  if (0 == keys_that_remain_index)
    {
      /* Replace CORE with BODY */
      mh_core_copy (core, body);

      /* Memory Manage KEYS, BODY */
      mh_core_free_special (body, false);
      mh_core_free (set);
    }
  else
    {
      MH_CORE_SET_KEYS (set)       = keys_that_remain;
      MH_CORE_SET_KEYS_COUNT (set) = keys_that_remain_index;

      /* Memory Manage KEYS */
      xfree (keys);
    }
#endif
}

static void
mh_opt_core_while (mh_core_t core)
{
  mh_core_t test = MH_CORE_WHILE_TEST (core);
  /* mh_core_t body = MH_CORE_WHILE_BODY (core); */

  /* If TEST is "", then ignore body and test. */
  if (mh_core_is_type (test, MH_CORE_DATA) &&
      mh_core_data_is_empty (test))
    {
      /* Memory Manage */
      MH_CORE_OP          (core) = MH_CORE_DATA;
      MH_CORE_DATA_STRING (core) = strdup ("");
      MH_CORE_DATA_OBJECT (core) = MH_AS_OBJECT (MH_EMPTY);
    }
}

/* Lift nested FMT core
   {FMT ... {FMT ,,,} ...}
   ==>> {FMT ... ,,, ...} */
static void
mh_opt_core_fmt_fmt (mh_core_t core)
{
  mh_core_t   *cores       = MH_CORE_FMT_CORES (core);
  unsigned int cores_count = MH_CORE_FMT_CORES_COUNT (core);
  unsigned int nested_fmt_count   = 0;
  unsigned int nested_cores_count = 0;

  unsigned int cores_index = 0;

  for (; cores_index < cores_count; cores_index++)
    if (mh_core_is_type (cores [cores_index], MH_CORE_FMT))
      {
	nested_fmt_count++;
	nested_cores_count += 
	  MH_CORE_FMT_CORES_COUNT (cores [cores_index]);
      }

  if (nested_fmt_count)
    {
      unsigned int new_cores_index = 0;
      unsigned int new_cores_count =
	cores_count + nested_cores_count - nested_fmt_count;
      mh_core_t *new_cores = (mh_core_t *)
	xcalloc (new_cores_count, sizeof (mh_core_t));

      for (cores_index = 0; cores_index < cores_count; cores_index++)
	if (mh_core_is_type (cores [cores_index], MH_CORE_FMT))
	  {
	    mh_core_t sub_fmt_core = cores [cores_index];

	    memcpy (& new_cores [new_cores_index],
		    MH_CORE_FMT_CORES (sub_fmt_core),
		    (MH_CORE_FMT_CORES_COUNT (sub_fmt_core) * 
		     sizeof (mh_core_t)));

	    new_cores_index += MH_CORE_FMT_CORES_COUNT (sub_fmt_core);
	    mh_core_free_special (sub_fmt_core, false);
	  }
	else
	  new_cores [new_cores_index++] =
	    cores [cores_index];

      xfree (cores);
      MH_CORE_FMT_CORES (core) = new_cores;
      MH_CORE_FMT_CORES_COUNT (core) = new_cores_count;
    }
}

/* Merge consecutive DATA cores
   {FMT ... {DATA "abc"} {DATA "def"} ...}
   ==>> {FMT ... {DATA "abcdef"} ...} */
static void
mh_opt_core_fmt_data (mh_core_t core)
{
  mh_core_t   *cores       = MH_CORE_FMT_CORES (core);
  unsigned int cores_count = MH_CORE_FMT_CORES_COUNT (core);

  switch (cores_count)
    {
    case 0:
      return;

    case 1:
      {
	mh_core_t one_core = cores [0];
	mh_core_copy (core, one_core);
	xfree (cores);
	return;
      }

    default:
      {
	unsigned int cores_old = 0;
	unsigned int cores_new = 0;
      
	boolean_t core_was_data_p = false;

	for (; cores_old < cores_count; cores_old++)
	  if (mh_core_is_type (cores [cores_old], MH_CORE_DATA))
	    {
	      if (false == core_was_data_p)
		{
		  core_was_data_p = true;
		  cores [cores_new] = cores [cores_old];
		}

	      else
		{
		  /* Merge DATA at cores_new with DATA at cores_old */
		  mh_core_t merged =
		    mh_core_data_merge_strings (cores [cores_new],
						cores [cores_old]);
		  mh_core_free (cores [cores_new]);
		  cores [cores_new] = merged;
		}
	    }
	  else
	    {
	      if (true == core_was_data_p)
		cores_new++;

	      cores [cores_new++] = cores [cores_old];
	      core_was_data_p = false;
	    }

	if (true == core_was_data_p)
	  cores_new++;

	if (cores_new != cores_count)
	  {
	    MH_CORE_FMT_CORES_COUNT (core) = cores_new;
	    MH_CORE_FMT_CORES       (core) =
	      xrealloc (MH_CORE_FMT_CORES (core), 
			MH_CORE_FMT_CORES_COUNT (core) * sizeof (mh_core_t));
	    mh_opt (core);
	  }

	return;
      }
    }
}


/* Merge consecutive EXTERN cores
   {FMT ... {EXTERN "abc"} {EXTERN "def"} ...}
   ==>> {FMT ... {EXTERN "abcdef"} ...} */
static void
mh_opt_core_fmt_extern (mh_core_t core)
{
  mh_core_t   *cores       = MH_CORE_FMT_CORES (core);
  unsigned int cores_count = MH_CORE_FMT_CORES_COUNT (core);

  switch (cores_count)
    {
    case 0:
      return;

    case 1:
      {
	mh_core_t one_core = cores [0];
	xfree (MH_CORE_FMT_CORES (core));
	mh_core_copy (core, one_core);
	return;
      }

    default:
      {
	unsigned int cores_old = 0;
	unsigned int cores_new = 0;
      
	boolean_t core_was_data_p = false;

	for (; cores_old < cores_count; cores_old++)
	  if (mh_core_is_type (cores [cores_old], MH_CORE_EXTERN))
	    {
	      if (false == core_was_data_p)
		{
		  core_was_data_p = true;
		  cores [cores_new] = cores [cores_old];
		}

	      else
		{
		  /* Merge DATA at cores_new with DATA at cores_old */
		  mh_core_t merged =
		    mh_core_extern_merge_strings (cores [cores_new],
						  cores [cores_old]);
		  mh_core_free (cores [cores_new]);
		  cores [cores_new] = merged;
		}
	    }
	  else
	    {
	      if (true == core_was_data_p)
		cores_new++;

	      cores [cores_new++] = cores [cores_old];
	      core_was_data_p = false;
	    }

	if (true == core_was_data_p)
	  cores_new++;

	if (cores_new != cores_count)
	  {
	    MH_CORE_FMT_CORES_COUNT (core) = cores_new;
	    MH_CORE_FMT_CORES       (core) =
	      xrealloc (MH_CORE_FMT_CORES (core), 
			MH_CORE_FMT_CORES_COUNT (core) * sizeof (mh_core_t));
	    mh_opt (core);
	  }

	return;
      }
    }
}

static void
mh_opt_core_fmt (mh_core_t core)
{
  mh_opt_core_fmt_fmt    (core);

  if (mh_core_is_type (core, MH_CORE_FMT))
    mh_opt_core_fmt_data (core);

  if (mh_core_is_type (core, MH_CORE_FMT))
    mh_opt_core_fmt_extern (core);
}

static void
mh_opt_core_prim (mh_core_t core)
{
  mh_byte_op_t operator       = MH_CORE_PRIM_OPERATOR (core);
  mh_core_t   *operands       = MH_CORE_PRIM_OPERANDS (core);

  /* Memory management is wrong */

  switch (operator)
    {
    case MH_ADD_OP:
      /* <add x 1>  => <inc x>
	 <add 1 x>  => <inc x>
	 <add x -1> => <dec x> */
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[0])) &&
	  1 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				(MH_CORE_DATA_OBJECT (operands[0]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_INC_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[1];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])) &&
	       1 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				     (MH_CORE_DATA_OBJECT (operands[1]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_INC_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[0];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])) &&
	       -1 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				      (MH_CORE_DATA_OBJECT (operands[1]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_DEC_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[0];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	       mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[0])) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])))
	{
	  mh_number_t num0 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [0]));
	  mh_number_t num1 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [1]));
	  mh_number_t res  = mh_number_add (num0, num1);

	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) = 
	    mh_object_to_string (MH_AS_OBJECT (res), false);
	  MH_CORE_DATA_OBJECT (core) = MH_AS_OBJECT (res);
	}
      break;

    case MH_SUB_OP:
      /* <sub x  1> => <dec x>
	 <sub x -1> => <inc x> */
      if (mh_core_is_type (operands[1], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])) &&
	  1 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				(MH_CORE_DATA_OBJECT (operands[1]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_DEC_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[0];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])) &&
	       -1 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				      (MH_CORE_DATA_OBJECT (operands[1]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_INC_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[0];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	       mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[0])) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])))
	{
	  mh_number_t num0 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [0]));
	  mh_number_t num1 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [1]));
	  mh_number_t res  = mh_number_sub (num0, num1);

	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) = 
	    mh_object_to_string (MH_AS_OBJECT (res), false);
	  MH_CORE_DATA_OBJECT (core) = MH_AS_OBJECT (res);
	}
      break;

    case MH_NE_OP:
      /* <ne x 0> => <nez x>
	 <ne 0 x> => <nez x> */
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[0])) &&
	  0 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				(MH_CORE_DATA_OBJECT (operands[0]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_NEZ_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[1];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])) &&
	       0 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				     (MH_CORE_DATA_OBJECT (operands[1]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_NEZ_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[0];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	       mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[0])) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])))
	{
	  mh_number_t num0 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [0]));
	  mh_number_t num1 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [1]));
	  
	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) =
	    (MH_NUMBER_VALUE (num0) != MH_NUMBER_VALUE (num1)
	     ? "true"
	     : "");
	  mh_opt_core_data (core);
	}
      break;

    case MH_EQ_OP:
      /* <eq x 0> => <eqz x>
	 <eq 0 x> => <eqz x> */
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[0])) &&
	  0 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				(MH_CORE_DATA_OBJECT (operands[0]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_EQZ_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[1];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])) &&
	       0 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				     (MH_CORE_DATA_OBJECT (operands[1]))))
	{
	  MH_CORE_PRIM_OPERATOR (core)  = MH_EQZ_OP;
	  MH_CORE_PRIM_OPERANDS (core)  = &operands[0];	/* Memory Manage */
	  MH_CORE_PRIM_OPERANDS_COUNT (core)  = 1;
	  mh_opt_core_prim (core);
	}
      else if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	       mh_core_is_type (operands[1], MH_CORE_DATA) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[0])) &&
	       MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands[1])))
	{
	  mh_number_t num0 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [0]));
	  mh_number_t num1 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [1]));
	  
	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) =
	    (MH_NUMBER_VALUE (num0) == MH_NUMBER_VALUE (num1)
	     ? "true"
	     : "");
	  mh_opt_core_data (core);
	}
      break;

    case MH_EQZ_OP:
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands [0])))
	{
	  /* Perform the comparison - Memory manage*/
	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) =
	    (0 == MH_NUMBER_VALUE (MH_AS_NUMBER 
				   (MH_CORE_DATA_OBJECT (operands [0])))
	     ? "true"
	     : "");

	  mh_opt_core_data (core);
	}
      break;

    case MH_NEZ_OP:
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands [0])))
	{
	  /* Perform the comparison - Memory manage*/
	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) =
	    (0 != MH_NUMBER_VALUE (MH_AS_NUMBER 
				   (MH_CORE_DATA_OBJECT (operands [0])))
	     ? "true"
	     : "");

	  mh_opt_core_data (core);
	}
      break;

    case MH_INC_OP:
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands [0])))
	{
	  mh_number_t num0 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [0]));
	  mh_number_t res  = mh_number_inc (num0);

	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) = 
	    mh_object_to_string (MH_AS_OBJECT (res), false);
	  MH_CORE_DATA_OBJECT (core) = MH_AS_OBJECT (res);
	}
      break;

    case MH_DEC_OP:
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  MH_NUMBER_P (MH_CORE_DATA_OBJECT (operands [0])))
	{
	  mh_number_t num0 = MH_AS_NUMBER (MH_CORE_DATA_OBJECT (operands [0]));
	  mh_number_t res  = mh_number_dec (num0);

	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) = 
	    mh_object_to_string (MH_AS_OBJECT (res), false);
	  MH_CORE_DATA_OBJECT (core) = MH_AS_OBJECT (res);
	}
      break;

    case MH_STR_EQ_OP:
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  mh_core_is_type (operands[1], MH_CORE_DATA) &&
	  mh_core_is_type (operands[2], MH_CORE_DATA))
	{
	  boolean_t caseless_p =
	    MH_EMPTY_P (MH_CORE_DATA_OBJECT (operands[2])) ? false : true;
	  
	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) =
	    (0 == ((caseless_p ? strcasecmp : strcmp)
		   (MH_CORE_DATA_STRING (operands[0]),
		    MH_CORE_DATA_STRING (operands[1])))
	     ? "true"
	     : "");
	  mh_opt_core_data (core);
	}
      break;

    case MH_STR_NEQ_OP:
      if (mh_core_is_type (operands[0], MH_CORE_DATA) &&
	  mh_core_is_type (operands[1], MH_CORE_DATA) &&
	  mh_core_is_type (operands[2], MH_CORE_DATA))
	{
	  boolean_t caseless_p =
	    MH_EMPTY_P (MH_CORE_DATA_OBJECT (operands[2])) ? false : true;
	  
	  MH_CORE_OP          (core) = MH_CORE_DATA;
	  MH_CORE_DATA_STRING (core) =
	    (0 != ((caseless_p ? strcasecmp : strcmp)
		   (MH_CORE_DATA_STRING (operands[0]),
		    MH_CORE_DATA_STRING (operands[1])))
	     ? "true"
	     : "");
	  mh_opt_core_data (core);
	}
      break;

    default:
      break;
    }
}

static void
mh_opt (mh_core_t core)
{
  if (MH_CORE_NULL == core) return;

  switch (MH_CORE_OP (core))
    {
    case MH_CORE_DATA:
      mh_opt_core_data (core);
      break;

    case MH_CORE_GET:
      mh_opt (MH_CORE_GET_LOC (core));
      break;

    case MH_CORE_SET:
      mh_opt_sequence
	(MH_CORE_SET_KEYS (core),
	 MH_CORE_SET_KEYS_COUNT (core));
      break;

    case MH_CORE_KEY:
      mh_opt (MH_CORE_KEY_LOC   (core));
      mh_opt (MH_CORE_KEY_VALUE (core));
      break;

    case MH_CORE_ARRAY:
      mh_opt (MH_CORE_ARRAY_LOC   (core));
      mh_opt (MH_CORE_ARRAY_INDEX (core));
      break;

    case MH_CORE_APP:
      mh_opt (MH_CORE_APP_OPERATOR (core));
      mh_opt_sequence
	(MH_CORE_APP_OPERANDS (core),
	 MH_CORE_APP_OPERANDS_COUNT (core));
      break;
      
    case MH_CORE_IF:
      mh_opt_core_if (core);
      break;

    case MH_CORE_OR:
      mh_opt_sequence
	(MH_CORE_OR_CORES (core),
	 MH_CORE_OR_CORES_COUNT (core));
      mh_opt_core_or (core);
      break;

    case MH_CORE_FMT:
      mh_opt_sequence
	(MH_CORE_FMT_CORES (core),
	 MH_CORE_FMT_CORES_COUNT (core));
      mh_opt_core_fmt (core);
      break;

    case MH_CORE_WHILE:
      mh_opt (MH_CORE_WHILE_TEST (core));
      mh_opt (MH_CORE_WHILE_BODY (core));
      mh_opt_core_while (core);
      break;

    case MH_CORE_BREAK:
      mh_opt (MH_CORE_BREAK_VALUE (core));;
      break;

    case MH_CORE_WITH:
      mh_opt (MH_CORE_WITH_SET  (core));
      mh_opt (MH_CORE_WITH_BODY (core));
      mh_opt_core_with (core);
      break;

    case MH_CORE_EXTERN:
      break;

    case MH_CORE_FUNC:
      mh_opt (MH_CORE_FUNC_BODY (core));
      break;

    case MH_CORE_PRIM:
      mh_opt_sequence
	(MH_CORE_PRIM_OPERANDS (core),
	 MH_CORE_PRIM_OPERANDS_COUNT (core));
      mh_opt_core_prim (core);
      break;
    }
}

extern void
mh_optimize (mh_core_t core)
{
  mh_opt (core);
}

/* Wed Nov  6 16:28:03 1996.  */
