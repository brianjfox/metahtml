/* corex.c: -*- C -*- */

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

static mh_core_t mh_core_empty_value = MH_CORE_NULL;

extern mh_core_t
mh_core_empty ()
{
  if (MH_CORE_NULL == mh_core_empty_value)
    mh_core_empty_value =
      mh_core_data_new ("");
  return mh_core_empty_value;
}

extern boolean_t
mh_core_is_type (mh_core_t    core,
		 mh_core_op_t op)
{
  return (op == MH_CORE_OP (core));
}

extern void
mh_core_copy (mh_core_t dest,
	      mh_core_t source)
{
  memcpy ((void *) dest,
	  (const void *) source,
	  sizeof (struct mh_core));
}

extern mh_core_t
mh_core_data_new (string_t string)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_DATA;
  MH_CORE_DATA_STRING (core) = strdup (string);
  MH_CORE_DATA_OBJECT (core) = NULL;
  return (core);
}

extern boolean_t
mh_core_data_is_empty (mh_core_t core)
{
  assert (mh_core_is_type (core, MH_CORE_DATA));
  return 0 == strcmp ("", MH_CORE_DATA_STRING (core));
}

extern boolean_t
mh_core_data_is_string (mh_core_t core)
{
  assert (mh_core_is_type (core, MH_CORE_DATA));
#if 0
  return MH_STRING_P (MH_CORE_DATA_OBJECT (core))
    ? true
    : false;
#endif
  return true;
}

extern mh_core_t
mh_core_data_merge_strings (mh_core_t core1,
			    mh_core_t core2)
{
  mh_core_t core;
  string_t str1 = MH_CORE_DATA_STRING (core1);
  string_t str2 = MH_CORE_DATA_STRING (core2);
  string_t result = (string_t) xmalloc (1 + strlen (str1) + strlen (str2));
  strcpy (result, str1);
  strcat (result, str2);
  core = mh_core_data_new (result);
  free (result);
  return core;
}

extern mh_core_t
mh_core_get_new (mh_core_t location)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_GET;
  MH_CORE_GET_LOC (core) = location;
  return (core);
}

extern mh_core_t
mh_core_set_new (mh_core_t   *keys,
		 unsigned int keys_count)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_SET;
  MH_CORE_SET_KEYS (core)       = keys;
  MH_CORE_SET_KEYS_COUNT (core) = keys_count;
  return (core);
}

extern mh_core_t
mh_core_key_new (mh_core_t location,
		 mh_core_t value)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_KEY;
  MH_CORE_KEY_LOC   (core) = location;
  MH_CORE_KEY_VALUE (core) = value;
  return (core);
}

extern mh_core_t
mh_core_array_new (mh_core_t location,
		   mh_core_t indx)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_ARRAY;
  MH_CORE_ARRAY_LOC   (core) = location;
  MH_CORE_ARRAY_INDEX (core) = indx;
  return (core);
}

extern mh_core_t
mh_core_app_new (/* mh_format_t format, */
		 mh_core_t   operator,
		 mh_core_t  *operands,
		 int         operands_count)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_APP;
  /*  MH_CORE_APP_FORMAT   (core)  = format; */
  MH_CORE_APP_OPERATOR (core)  = operator;
  MH_CORE_APP_OPERANDS (core)  = operands;
  MH_CORE_APP_OPERANDS_COUNT (core)  = operands_count;
  return (core);
}

extern mh_core_t
mh_core_if_new (mh_core_t pred,
		mh_core_t cons,
		mh_core_t alt)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_IF;
  MH_CORE_IF_PRED (core)  = pred;
  MH_CORE_IF_CONS (core)  = cons;
  MH_CORE_IF_ALT  (core)  = alt;
  return (core);
}

extern mh_core_t
mh_core_or_new (mh_core_t *cores,
		int        cores_count)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_OR;
  MH_CORE_OR_CORES (core)  = cores;
  MH_CORE_OR_CORES_COUNT (core)  = cores_count;
  return (core);
}

extern mh_core_t
mh_core_fmt_new (mh_core_t  *cores,
		 int         cores_count)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_FMT;
  MH_CORE_FMT_CORES (core)   = cores;
  MH_CORE_FMT_CORES_COUNT (core)  = cores_count;
  return (core);
}

extern mh_core_t
mh_core_while_new (mh_core_t test,
		   mh_core_t body)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_WHILE;
  MH_CORE_WHILE_TEST (core)  = test;
  MH_CORE_WHILE_BODY (core)  = body;
  return (core);
}

extern mh_core_t
mh_core_break_new (mh_core_t value)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_BREAK;
  MH_CORE_BREAK_VALUE (core)  = value;
  return (core);
}

extern mh_core_t
mh_core_with_new (mh_core_t set,
		  mh_core_t body)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_WITH;
  MH_CORE_WITH_SET  (core) = set;
  MH_CORE_WITH_BODY (core) = body;
  return (core);
}

extern mh_core_t
mh_core_extern_new (string_t form)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_EXTERN;
  MH_CORE_EXTERN_FORM (core) = strdup (form);
  return (core);
}

extern mh_core_t
mh_core_extern_merge_strings (mh_core_t core1,
			      mh_core_t core2)
{
  mh_core_t core;
  string_t str1 = MH_CORE_EXTERN_FORM (core1);
  string_t str2 = MH_CORE_EXTERN_FORM (core2);
  string_t result = (string_t) xmalloc (1 + strlen (str1) + strlen (str2));
  strcpy (result, str1);
  strcat (result, str2);
  core = mh_core_extern_new (result);
  free (result);
  return core;
}


extern mh_core_t
mh_core_func_new (mh_tag_t  tag,
		  mh_core_t body)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_FUNC;
  /* ?? Get by without copying?? */
  MH_CORE_FUNC_TAG  (core)  = tag;
  MH_CORE_FUNC_BODY (core)  = body;
  return (core);
}

extern mh_core_t
mh_core_prim_new (mh_byte_op_t operator,
		  mh_core_t   *operands,
		  int          operands_count)
{
  mh_core_t core = (mh_core_t) xcalloc (MH_CORE_SIZE,  1);
  MH_CORE_OP (core) = MH_CORE_PRIM;
  MH_CORE_PRIM_OPERATOR (core)  = operator;
  MH_CORE_PRIM_OPERANDS (core)  = operands;
  MH_CORE_PRIM_OPERANDS_COUNT (core)  = operands_count;
  return (core);
}

extern void
mh_core_free_special (mh_core_t core,
		      boolean_t recurse)
{
}

extern mh_core_t *
mh_core_1 (mh_core_t core_1)
{
  mh_core_t *cores = (mh_core_t *) xmalloc (1 * sizeof (mh_core_t));

  cores[0] = core_1;

  return cores;
}


extern mh_core_t *
mh_core_2 (mh_core_t core_1,
	   mh_core_t core_2)
{
  mh_core_t *cores = (mh_core_t *) xmalloc (2 * sizeof (mh_core_t));

  cores[0] = core_1;
  cores[1] = core_2;

  return cores;
}

extern mh_core_t *
mh_core_3 (mh_core_t core_1,
	   mh_core_t core_2,
	   mh_core_t core_3)
{
  mh_core_t *cores = (mh_core_t *) xmalloc (3 * sizeof (mh_core_t));

  cores[0] = core_1;
  cores[1] = core_2;
  cores[2] = core_3;

  return cores;
}

extern mh_core_t *
mh_core_4 (mh_core_t core_1,
	   mh_core_t core_2,
	   mh_core_t core_3,
	   mh_core_t core_4)
{
  mh_core_t *cores = (mh_core_t *) xmalloc (4 * sizeof (mh_core_t));

  cores[0] = core_1;
  cores[1] = core_2;
  cores[2] = core_3;
  cores[3] = core_4;

  return cores;
}


/****************************************************************************
 *
 * MH_CORE_VAR_IS_BOUND ()
 *
 */
extern boolean_t
mh_core_var_is_bound (mh_core_t core,
		      string_t  this)
{
  /* Recurse or not */

  return false;
}

/****************************************************************************
 *
 * MH_CORE_VAR_SUBST ()
 *
 */
static void
mh_core_var_subst_many (mh_core_t   *cores,
			unsigned int cores_count,
			string_t     this,
			mh_core_t    that)
{
  unsigned int cores_index;
  for (cores_index = 0; cores_index < cores_count; cores_index++)
    mh_core_var_subst (cores[cores_index], this, that);
}
			
extern void
mh_core_var_subst (mh_core_t core,
		   string_t  this,
		   mh_core_t that)
{
#define recurse( core )       mh_core_var_subst (core, this, that)
#define recurse_many( cores, cores_count )			\
   mh_core_var_subst_many (cores, cores_count, this, that)

  switch (MH_CORE_OP (core))
    {
    case MH_CORE_DATA:
      break;

    case MH_CORE_GET:
      {
	mh_core_t location = MH_CORE_GET_LOC (core);

	if (! mh_core_is_type (location, MH_CORE_DATA))
	  recurse (location);
	else if (0 == strcmp (this, MH_CORE_DATA_STRING (location)))
	  mh_core_copy (core, that);
      }
      break;

    case MH_CORE_SET:
      /* Careful with scoping */
      recurse_many (MH_CORE_SET_KEYS (core),
		    MH_CORE_SET_KEYS_COUNT (core));
      break;

    case MH_CORE_KEY:
#if defined (WHAT_IS_CORRECT)
      if (this != MH_CORE_KEY_LOC (core))
	recurse (MH_CORE_KEY_VALUE (core));
#endif
      recurse (MH_CORE_KEY_LOC   (core));
      recurse (MH_CORE_KEY_VALUE (core));
      break;

    case MH_CORE_ARRAY:
#if defined (WHAT_IS_CORRECT)
      if (this != MH_CORE_ARRAY_LOC (core))
	recurse (MH_CORE_ARRAY_INDEX (core));
#endif
      recurse (MH_CORE_ARRAY_LOC   (core));
      recurse (MH_CORE_ARRAY_INDEX (core));
      break;

    case MH_CORE_APP:
      recurse (MH_CORE_APP_OPERATOR (core));
      recurse_many (MH_CORE_APP_OPERANDS (core),
		    MH_CORE_APP_OPERANDS_COUNT (core));
      break;

    case MH_CORE_IF:
      recurse (MH_CORE_IF_PRED (core));
      recurse (MH_CORE_IF_CONS (core));
      recurse (MH_CORE_IF_ALT  (core));
      break;

    case MH_CORE_OR:
      recurse_many (MH_CORE_OR_CORES (core),
		    MH_CORE_OR_CORES_COUNT (core));
      break;

    case MH_CORE_FMT:
      recurse_many (MH_CORE_FMT_CORES (core),
		    MH_CORE_FMT_CORES_COUNT (core));
      break;

    case MH_CORE_WHILE:
      recurse (MH_CORE_WHILE_TEST (core));
      recurse (MH_CORE_WHILE_BODY (core));
      break;

    case MH_CORE_BREAK:
      recurse (MH_CORE_BREAK_VALUE (core));
      break;

    case MH_CORE_WITH:
      /* Careful with scoping */
      recurse (MH_CORE_WITH_SET  (core));

      /* Only if THIS is not bound in SET above */
      recurse (MH_CORE_WITH_BODY (core));
      break;
      
    case MH_CORE_EXTERN:
      break; /* always */

    case MH_CORE_FUNC:
      recurse (MH_CORE_FUNC_BODY (core));
      break;

    case MH_CORE_PRIM:
      recurse_many (MH_CORE_PRIM_OPERANDS (core),
		    MH_CORE_PRIM_OPERANDS_COUNT (core));
      break;
    }
#undef recurse
#undef recurse_many
}

/*
 * MH_CORE_VAR_IS_REFERENCED ()
 *
 */
static boolean_t
mh_core_var_is_referenced_many (mh_core_t   *cores,
				unsigned int cores_count,
				string_t     this)
{
  unsigned int cores_index;
  for (cores_index = 0; cores_index < cores_count; cores_index++)
    if (mh_core_var_is_referenced (cores[cores_index], this))
      return true;
  return false;
}

extern boolean_t
mh_core_var_is_referenced (mh_core_t core,
			   string_t  this)
{
#define recurse( core )       mh_core_var_is_referenced (core, this)
#define recurse_many( cores, cores_count )		\
  mh_core_var_is_referenced_many (cores, cores_count, this)

  /* */
  switch (MH_CORE_OP (core))
    {
    case MH_CORE_DATA:
      return false;

    case MH_CORE_GET:
      {
	mh_core_t location = MH_CORE_GET_LOC (core);

	if (mh_core_is_type (location, MH_CORE_DATA))
	  return 0 == strcmp (this, MH_CORE_DATA_STRING (location));
	else
	  return recurse (location);
      }

    case MH_CORE_SET:
      return recurse_many
	(MH_CORE_SET_KEYS (core),
	 MH_CORE_SET_KEYS_COUNT (core));

    case MH_CORE_KEY:
      /* Careful about scope */
#if defined (WHAT_IS_CORRECT)
      return recurse (MH_CORE_KEY_VALUE (core));
#endif
      return
	recurse (MH_CORE_KEY_VALUE (core)) ||
	recurse (MH_CORE_KEY_LOC   (core));

    case MH_CORE_ARRAY:
      /* Careful about scope */
#if defined (WHAT_IS_CORRECT)
      return recurse (MH_CORE_ARRAY_INDEX (core));
#endif
      return 
	recurse (MH_CORE_ARRAY_INDEX (core)) ||
	recurse (MH_CORE_ARRAY_LOC   (core));

    case MH_CORE_APP:
      return
	recurse (MH_CORE_APP_OPERATOR (core)) ||
	recurse_many
	(MH_CORE_APP_OPERANDS (core),
	 MH_CORE_APP_OPERANDS_COUNT (core));

    case MH_CORE_IF:
      return 
	recurse (MH_CORE_IF_PRED (core)) ||
	recurse (MH_CORE_IF_CONS (core)) ||
	recurse (MH_CORE_IF_ALT  (core));

    case MH_CORE_OR:
      return recurse_many
	(MH_CORE_OR_CORES (core),
	 MH_CORE_OR_CORES_COUNT (core));

    case MH_CORE_FMT:
      return recurse_many
	(MH_CORE_FMT_CORES (core),
	 MH_CORE_FMT_CORES_COUNT (core));

    case MH_CORE_WHILE:
      return 
	recurse (MH_CORE_WHILE_TEST (core)) ||
	recurse (MH_CORE_WHILE_BODY (core));

    case MH_CORE_BREAK:
      return 
	recurse (MH_CORE_BREAK_VALUE (core));

    case MH_CORE_WITH:
      /* Careful about scope */
      return
	recurse (MH_CORE_WITH_SET  (core)) ||
	recurse (MH_CORE_WITH_BODY (core));
      
    case MH_CORE_EXTERN:
      return false;

    case MH_CORE_FUNC:
      return recurse (MH_CORE_FUNC_BODY (core));

    case MH_CORE_PRIM:
      return recurse_many
	(MH_CORE_PRIM_OPERANDS (core),
	 MH_CORE_PRIM_OPERANDS_COUNT (core));
    }

  /* Never here */
  return false;

#undef recurse
#undef recurse_many
}

/*
 *
 *
 */
static boolean_t
mh_core_is_side_effecting_many (mh_core_t   *cores,
				unsigned int cores_count)
{
  unsigned int cores_index;
  for (cores_index = 0; cores_index < cores_count; cores_index++)
    if (mh_core_is_side_effecting (cores[cores_index]))
      return true;
  return false;
}

extern boolean_t
mh_core_is_side_effecting (mh_core_t core)
{
#define recurse( core )      mh_core_is_side_effecting (core)

  switch (MH_CORE_OP (core))
    {
    case MH_CORE_DATA:
      return false;

    case MH_CORE_GET:
      return recurse (MH_CORE_GET_LOC (core));

    case MH_CORE_IF:
      return 
	recurse (MH_CORE_IF_PRED (core)) ||
	recurse (MH_CORE_IF_CONS (core)) ||
	recurse (MH_CORE_IF_ALT  (core));

    case MH_CORE_OR:
      return mh_core_is_side_effecting_many
	(MH_CORE_OR_CORES (core),
	 MH_CORE_OR_CORES_COUNT (core));

    default:
      return true;
    }
#undef recurse
}


/*
 * MH_CORE_IS_EXTERN_FREE ()
 *
 */
static boolean_t
mh_core_is_extern_free_many (mh_core_t   *cores,
			     unsigned int cores_count)
{
  unsigned int cores_index;
  for (cores_index = 0; cores_index < cores_count; cores_index++)
    if (mh_core_is_extern_free (cores[cores_index]))
      return true;
  return false;
}

extern boolean_t
mh_core_is_extern_free (mh_core_t core)
{
#define recurse( core )               mh_core_is_extern_free (core)
#define recurse_many( cores, count )  		\
  mh_core_is_extern_free_many (cores, count)

  /* */
  switch (MH_CORE_OP (core))
    {
    case MH_CORE_DATA:
    case MH_CORE_GET:
      return recurse (MH_CORE_GET_LOC (core));

    case MH_CORE_SET:
      return recurse_many
	(MH_CORE_SET_KEYS (core),
	 MH_CORE_SET_KEYS_COUNT (core));

    case MH_CORE_KEY:
      /* Careful about scope */
      return
	recurse (MH_CORE_KEY_VALUE (core)) &&
	recurse (MH_CORE_KEY_LOC   (core));

    case MH_CORE_ARRAY:
      /* Careful about scope */
      return 
	recurse (MH_CORE_ARRAY_INDEX (core)) &&
	recurse (MH_CORE_ARRAY_LOC   (core));

    case MH_CORE_APP:
      return
	recurse      (MH_CORE_APP_OPERATOR (core)) &&
	recurse_many (MH_CORE_APP_OPERANDS (core),
		      MH_CORE_APP_OPERANDS_COUNT (core));

    case MH_CORE_IF:
      return 
	recurse (MH_CORE_IF_PRED (core)) &&
	recurse (MH_CORE_IF_CONS (core)) &&
	recurse (MH_CORE_IF_ALT  (core));

    case MH_CORE_OR:
      return recurse_many
	(MH_CORE_OR_CORES (core),
	 MH_CORE_OR_CORES_COUNT (core));

    case MH_CORE_FMT:
      return recurse_many
	(MH_CORE_FMT_CORES (core),
	 MH_CORE_FMT_CORES_COUNT (core));

    case MH_CORE_WHILE:
      return 
	recurse (MH_CORE_WHILE_TEST (core)) &&
	recurse (MH_CORE_WHILE_BODY (core));

    case MH_CORE_BREAK:
      return 
	recurse (MH_CORE_BREAK_VALUE (core));

    case MH_CORE_WITH:
      /* Careful about scope */
      return
	recurse (MH_CORE_WITH_SET  (core)) &&
	recurse (MH_CORE_WITH_BODY (core));
      
    case MH_CORE_EXTERN:
      return false;

    case MH_CORE_FUNC:
      return recurse (MH_CORE_FUNC_BODY (core));

    case MH_CORE_PRIM:
      return recurse_many
	(MH_CORE_PRIM_OPERANDS (core),
	 MH_CORE_PRIM_OPERANDS_COUNT (core));
    }

  /* Never here */
  return false;
}


/*
 *
 *
 *
 */
static mh_core_t *
mh_core_dup_multiple (mh_core_t   *cores,
		      unsigned int cores_count)
{
  
  mh_core_t *result = (mh_core_t *)
    (cores_count == 0
     ? NULL
     : xmalloc (cores_count * sizeof (mh_core_t)));

  unsigned int cores_index = -1;

  while (++cores_index < cores_count)
    result [cores_index] = mh_core_dup (cores [cores_index]);

  return result;
}

extern mh_core_t
mh_core_dup (mh_core_t core)
{
  if (MH_CORE_NULL == core) return MH_CORE_NULL;

  switch (MH_CORE_OP (core))
    {
    case MH_CORE_DATA:
      return mh_core_data_new (MH_CORE_DATA_STRING (core));

    case MH_CORE_GET:
      return mh_core_get_new (MH_CORE_GET_LOC (core));

    case MH_CORE_SET:
      return mh_core_set_new
	(mh_core_dup_multiple (MH_CORE_SET_KEYS (core),
			       MH_CORE_SET_KEYS_COUNT (core)),
	 MH_CORE_SET_KEYS_COUNT (core));

    case MH_CORE_KEY:
      return mh_core_key_new
	(mh_core_dup (MH_CORE_KEY_LOC   (core)),
	 mh_core_dup (MH_CORE_KEY_VALUE (core)));

    case MH_CORE_ARRAY:
      return mh_core_array_new
	(mh_core_dup (MH_CORE_ARRAY_LOC   (core)),
	 mh_core_dup (MH_CORE_ARRAY_INDEX (core)));

    case MH_CORE_APP:
      return mh_core_app_new
	(mh_core_dup (MH_CORE_APP_OPERATOR (core)),
	 mh_core_dup_multiple (MH_CORE_APP_OPERANDS (core),
			       MH_CORE_APP_OPERANDS_COUNT (core)),
	 MH_CORE_APP_OPERANDS_COUNT (core));

    case MH_CORE_IF:
      return mh_core_if_new
	(mh_core_dup (MH_CORE_IF_PRED (core)),
	 mh_core_dup (MH_CORE_IF_CONS (core)),
	 mh_core_dup (MH_CORE_IF_ALT  (core)));

    case MH_CORE_OR:
      return mh_core_or_new
	(mh_core_dup_multiple (MH_CORE_OR_CORES (core),
			       MH_CORE_OR_CORES_COUNT (core)),
	 MH_CORE_OR_CORES_COUNT (core));

    case MH_CORE_FMT:
      return mh_core_fmt_new
	(mh_core_dup_multiple (MH_CORE_FMT_CORES (core),
			       MH_CORE_FMT_CORES_COUNT (core)),
	 MH_CORE_FMT_CORES_COUNT (core));

    case MH_CORE_WHILE:
      return mh_core_while_new
	(mh_core_dup (MH_CORE_WHILE_TEST (core)),
	 mh_core_dup (MH_CORE_WHILE_BODY (core)));

    case MH_CORE_BREAK:
      return mh_core_break_new
	(mh_core_dup (MH_CORE_BREAK_VALUE (core)));

    case MH_CORE_WITH:
      return mh_core_with_new
	(mh_core_dup (MH_CORE_WITH_SET (core)),
	 mh_core_dup (MH_CORE_WITH_BODY (core)));

    case MH_CORE_EXTERN:
      return mh_core_extern_new (MH_CORE_EXTERN_FORM (core));

    case MH_CORE_FUNC:
      return mh_core_func_new
	(MH_CORE_FUNC_TAG (core), /* duplicate() */
	 mh_core_dup (MH_CORE_FUNC_BODY (core)));

    case MH_CORE_PRIM:
      return mh_core_prim_new
	(MH_CORE_PRIM_OPERATOR (core),
	 mh_core_dup_multiple (MH_CORE_PRIM_OPERANDS (core),
			       MH_CORE_PRIM_OPERANDS_COUNT (core)),
	 MH_CORE_PRIM_OPERANDS_COUNT (core));

    default:
      abort ();
    }
}

/*
 *
 */
static void
mh_core_show_internal (mh_core_t core,
		       int start,
		       int level);

static void
mh_core_show_space (int level)
{
  while (level--)
    putchar (' ');
}

static void
mh_core_show_multiple_internal (mh_core_t *cores,
				int cores_count,
				int start,
				int level)
{
  while (cores_count--)
    {
      mh_core_show_internal (*cores++, start, level);
      if (cores_count)
	{
	  start = level;
	  putchar ('\n');
	}
    }
}

static void
mh_core_show_internal (mh_core_t core,
		       int start,
		       int level)
{
  if (core == MH_CORE_NULL)
    {
      printf ("<<CORE is NULL>>");
      abort();
    }

  mh_core_show_space (start);
  switch (MH_CORE_OP (core))
    {
    case MH_CORE_DATA:
      printf ("<DATA \"%s\"", MH_CORE_DATA_STRING (core));
      if (MH_CORE_DATA_OBJECT (core))
	{
	  printf (" ");
	  mh_object_to_file (MH_CORE_DATA_OBJECT (core), true, stdout);
	}
      printf (">");
      break;
      
    case MH_CORE_GET:
      printf ("<GET ");
      mh_core_show_internal 
	(MH_CORE_GET_LOC (core),
	 0, 
	 level+5);
      printf (">");
      break;

    case MH_CORE_SET:
      printf ("<SET ");
      mh_core_show_multiple_internal
	(MH_CORE_SET_KEYS (core),
	 MH_CORE_SET_KEYS_COUNT (core),
	 0,
	 level+5);
      printf (">");
      break;

    case MH_CORE_KEY:
      printf ("<KEY ");
      mh_core_show_internal
	(MH_CORE_KEY_LOC (core),
	 0,
	 level + 5);
      printf ("\n");
      mh_core_show_internal
	(MH_CORE_KEY_VALUE (core),
	 level + 5,
	 level + 5);
      printf (">");
      break;

    case MH_CORE_ARRAY:
      printf ("<ARRAY ");
      mh_core_show_internal
	(MH_CORE_ARRAY_LOC  (core),
	 0,
	 level + 7);
      if (MH_CORE_NULL != MH_CORE_ARRAY_INDEX (core))
	{
	  printf ("\n");
	  mh_core_show_internal
	    (MH_CORE_ARRAY_INDEX (core),
	     level + 7,
	     level + 7);
	}
      printf (">");
      break;

    case MH_CORE_APP:
      printf ("<APP ");
      mh_core_show_internal
	(MH_CORE_APP_OPERATOR (core),
	 0,
	 level + 5);
      if (0 != MH_CORE_APP_OPERANDS_COUNT (core))
	{
	  printf ("\n");
	  mh_core_show_multiple_internal
	    (MH_CORE_APP_OPERANDS (core),
	     MH_CORE_APP_OPERANDS_COUNT (core),
	     level+5,
	     level+5);
	}
      printf (">");
      break;

    case MH_CORE_IF:
      printf ("<IF ");
      mh_core_show_internal
	(MH_CORE_IF_PRED (core),
	 0,
	 level + 4);
      putchar ('\n');
      mh_core_show_internal
	(MH_CORE_IF_CONS (core),
	 level + 4,
	 level + 4);
      putchar ('\n');
      mh_core_show_internal
	(MH_CORE_IF_ALT (core),
	 level + 4,
	 level + 4);
      printf (">");
      break;

    case MH_CORE_OR:
      printf ("<OR ");
      mh_core_show_multiple_internal
	(MH_CORE_OR_CORES (core),
	 MH_CORE_OR_CORES_COUNT (core),
	 0,
	 level+4);
      printf (">");
      break;

    case MH_CORE_FMT:
      printf ("<FMT ");
      if (0 != MH_CORE_FMT_CORES_COUNT (core))
	{
	  mh_core_show_multiple_internal
	    (MH_CORE_FMT_CORES (core),
	     MH_CORE_FMT_CORES_COUNT (core),
	     0,
	     level + 5);
	}
      printf (">");
      break;

    case MH_CORE_WHILE:
      printf ("<WHILE ");
      mh_core_show_internal
	(MH_CORE_WHILE_TEST (core),
	 0,
	 level + 7);
      putchar ('\n');
      mh_core_show_internal
	(MH_CORE_WHILE_BODY (core),
	 level + 3,
	 level + 3);
      printf (">");
      break;

    case MH_CORE_BREAK:
      if (MH_CORE_BREAK_VALUE (core))
	{
	  printf ("<RETURN ");
	  mh_core_show_internal
	    (MH_CORE_BREAK_VALUE (core),
	     0,
	     level + 4);
	  putchar ('>');
	}
      else
	printf ("<BREAK>");
      break;

    case MH_CORE_WITH:
      printf ("<WITH ");
      mh_core_show_internal
	(MH_CORE_WITH_SET (core),
	 0,
	 level + 6);
      putchar ('\n');
      mh_core_show_internal
	(MH_CORE_WITH_BODY (core),
	 level + 2,
	 level + 2);
      putchar ('>');
      break;

    case MH_CORE_EXTERN:
      {
	string_t form = mh_string_to_string_quoted
	  (MH_CORE_EXTERN_FORM (core));
	printf ("<EXTERN \"%s\">", form);
	xfree (form);
      }
      break;

    case MH_CORE_FUNC:
      printf ("<FUNC %s", MH_TAG_NAME (MH_CORE_FUNC_TAG (core)));

      /* printf (" &key "); */
      /* Show keys */

      putchar ('\n');
      mh_core_show_internal
	(MH_CORE_FUNC_BODY (core),
	 level+6,
	 level+6);
      printf (">");
      break;

    case MH_CORE_PRIM:
      printf ("<PRIM %s\n",
	      MH_BYTE_CODE_SPEC_NAME (MH_BYTE_CODE_SPEC_LOOKUP
				      (MH_CORE_PRIM_OPERATOR (core))));
      mh_core_show_multiple_internal
	(MH_CORE_PRIM_OPERANDS (core),
	 MH_CORE_PRIM_OPERANDS_COUNT (core),
	 level+3,
	 level+3);
      printf (">");
      break;

    default:
      printf ("Unknown MH_CORE_OP of %d in %#lx", 
	      (int) MH_CORE_OP (core), (long unsigned int) core);
      break;
    }
}

extern void
mh_core_show (mh_core_t core)
{
  printf ("\n");
  mh_core_show_internal (core, 0, 0);
  fflush (stdout);
}


/* Wed Nov  6 16:27:20 1996.  */
