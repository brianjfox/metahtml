/* corex.h: -*- C -*-  */

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

#if !defined (_MH_CORE_H_)
#define _MH_CORE_H_ 1

#include "compiler/compile.h"

/* Needs ARRAY */
typedef enum
{
  MH_CORE_DATA,			/* string */
  MH_CORE_GET,			/* get-var */
  MH_CORE_SET,			/* set-var */
  MH_CORE_KEY,			/* foo=bar */
  MH_CORE_ARRAY,		/* foo[bar] */
  MH_CORE_APP,			/* <tag arg ...> */
  MH_CORE_IF,			/* <if pred cons alt> */
  MH_CORE_OR,			/* Gonzo! */
  MH_CORE_FMT,
  MH_CORE_WHILE,		/* <while test> ... </while> */
  MH_CORE_BREAK,		/* <break/return val ...> */
  MH_CORE_WITH,			/* <with ...> ... </with> */
  MH_CORE_EXTERN,
  MH_CORE_FUNC,			/* <defun ...> */
  MH_CORE_PRIM
} mh_core_op_t;

#define MH_NUMBER_OF_CORE_OPS (1 + MH_CORE_PRIM)

/* Forward Declaration - Only Slightly */
/* typedef struct mh_core *mh_core_t; */

struct mh_core
{
  mh_core_op_t op;
#define MH_CORE_OP( core )  ((core)->op)
  
  union
  {
    struct
    {
      string_t    string;
      mh_object_t object;	/* NULL until mh_opt()  */
    } data;
#define MH_CORE_DATA_STRING( core )    ((core)->u.data.string)
#define MH_CORE_DATA_OBJECT( core )    ((core)->u.data.object)
      
    struct
    {
      mh_core_t loc;
    } var;
#define MH_CORE_GET_LOC( core )   ((core)->u.var.loc)

    struct
    {
      mh_core_t   *keys;
      unsigned int keys_count;
    } set;
#define MH_CORE_SET_KEYS( core )        ((core)->u.set.keys)
#define MH_CORE_SET_KEYS_COUNT( core )  ((core)->u.set.keys_count)

    struct
    {
      mh_core_t loc;		/* NAME or expression evaluating to NAME */
      mh_core_t value;
    } key;
#define MH_CORE_KEY_LOC( core )     ((core)->u.key.loc)
#define MH_CORE_KEY_VALUE( core )   ((core)->u.key.value)

    struct
    {
      mh_core_t loc;		/* NAME or expression evaluating to NAME */
      mh_core_t index;
    } array;
#define MH_CORE_ARRAY_LOC( core )     ((core)->u.array.loc)
#define MH_CORE_ARRAY_INDEX( core )   ((core)->u.array.index)

    struct
    {
      mh_core_t   operator;
      mh_core_t  *operands;
      int         operands_count;
    } app;
#define MH_CORE_APP_OPERATOR( core )        ((core)->u.app.operator)
#define MH_CORE_APP_OPERANDS( core )        ((core)->u.app.operands)
#define MH_CORE_APP_OPERANDS_COUNT( core )  ((core)->u.app.operands_count)
   
    struct
    {
      mh_core_t pred;
      mh_core_t cons;
      mh_core_t alt;
    } core_if;
#define MH_CORE_IF_PRED( core )   ((core)->u.core_if.pred)
#define MH_CORE_IF_CONS( core )   ((core)->u.core_if.cons)
#define MH_CORE_IF_ALT(  core )   ((core)->u.core_if.alt)

    /* Should be:  <or exp1 exp2 ...>
     *             <with tmp=exp1><if tmp tmp <or exp2 ...>></with>
     *
     * modulo any quirky semantics for WITH. */
    struct
    {
      mh_core_t *cores;
      int        cores_count;
    } or;
#define MH_CORE_OR_CORES( core )       ((core)->u.or.cores)
#define MH_CORE_OR_CORES_COUNT( core ) ((core)->u.or.cores_count)

    struct
    {
      mh_core_t *cores;
      int        cores_count;
    } fmt;
#define MH_CORE_FMT_CORES( core )       ((core)->u.fmt.cores)
#define MH_CORE_FMT_CORES_COUNT( core ) ((core)->u.fmt.cores_count)

    struct
    {
      mh_core_t test;
      mh_core_t body;
    } core_while;
#define MH_CORE_WHILE_TEST( core )     ((core)->u.core_while.test)
#define MH_CORE_WHILE_BODY( core )     ((core)->u.core_while.body)

    struct
    {
      mh_core_t value;		/* Non-NULL means 'return' */
    } core_break;
#define MH_CORE_BREAK_VALUE( core )     ((core)->u.core_break.value)

    struct
    {
      mh_core_t set;
      mh_core_t body;
    } core_with;
#define MH_CORE_WITH_SET( core )            ((core)->u.core_with.set)
#define MH_CORE_WITH_BODY( core )           ((core)->u.core_with.body)

    struct
    {
      string_t form;
    } core_extern;
#define MH_CORE_EXTERN_FORM( core )     ((core)->u.core_extern.form)

    struct
    {
      mh_tag_t     tag;		/* Prototype */
      mh_core_t    body;
    } func;
#define MH_CORE_FUNC_TAG( core )         ((core)->u.func.tag)
#define MH_CORE_FUNC_BODY( core )        ((core)->u.func.body)


    struct
    {
      mh_byte_op_t  operator;
      mh_core_t    *operands;
      int           operands_count;
    } prim;
#define MH_CORE_PRIM_OPERATOR( core )        ((core)->u.prim.operator)
#define MH_CORE_PRIM_OPERANDS( core )        ((core)->u.prim.operands)
#define MH_CORE_PRIM_OPERANDS_COUNT( core )  ((core)->u.prim.operands_count)

  } u;
};

#define MH_CORE_SIZE   (sizeof (struct mh_core))

extern mh_core_t
mh_core_empty (void);

extern boolean_t
mh_core_is_type (mh_core_t    core,
		 mh_core_op_t op);

extern void
mh_core_copy (mh_core_t dest,
	      mh_core_t source);

extern mh_core_t *
mh_core_1 (mh_core_t core_1);

extern mh_core_t *
mh_core_2 (mh_core_t core_1,
	   mh_core_t core_2);

extern mh_core_t *
mh_core_3 (mh_core_t core_1,
	   mh_core_t core_2,
	   mh_core_t core_3);

extern mh_core_t *
mh_core_4 (mh_core_t core_1,
	   mh_core_t core_2,
	   mh_core_t core_3,
	   mh_core_t core_4);

extern boolean_t
mh_core_data_is_empty (mh_core_t core);

extern boolean_t
mh_core_data_is_string (mh_core_t core);

extern mh_core_t
mh_core_data_merge_strings (mh_core_t core1,
			    mh_core_t core2);

extern mh_core_t
mh_core_data_new (string_t string);

extern mh_core_t
mh_core_get_new (mh_core_t location);

static inline mh_core_t
mh_core_get_name_new (string_t name)
{ return mh_core_get_new (mh_core_data_new (name)); }

extern mh_core_t
mh_core_key_new (mh_core_t location,
		 mh_core_t value);

extern mh_core_t
mh_core_set_new (mh_core_t   *keys,
		 unsigned int keys_count);

static inline mh_core_t
mh_core_set_1_new (mh_core_t name,
		   mh_core_t value)
{ 
  return mh_core_set_new
    (mh_core_1 (mh_core_key_new (name, value)),
     1);
}
		
static inline mh_core_t
mh_core_set_1_key_new (mh_core_t key)
{ 
  return mh_core_set_new (mh_core_1 (key), 1);
}
		
static inline mh_core_t
mh_core_set_2_key_new (mh_core_t key1,
		       mh_core_t key2)
{ 
  return mh_core_set_new (mh_core_2 (key1, key2), 2);
}
		
static inline mh_core_t
mh_core_set_3_key_new (mh_core_t key1,
		       mh_core_t key2,
		       mh_core_t key3)
{ 
  return mh_core_set_new (mh_core_3 (key1, key2, key3), 3);
}
		
static inline mh_core_t
mh_core_set_4_key_new (mh_core_t key1,
		       mh_core_t key2,
		       mh_core_t key3,
		       mh_core_t key4)
{ 
  return mh_core_set_new (mh_core_4 (key1, key2, key3, key4), 4);
}
		
extern mh_core_t
mh_core_array_new (mh_core_t location,
		   mh_core_t indx);

extern mh_core_t
mh_core_app_new (/* mh_format_t format, */
		 mh_core_t   func,
		 mh_core_t  *exps,
		 int         exps_count);

extern mh_core_t
mh_core_if_new (mh_core_t pred,
		mh_core_t cons,
		mh_core_t alt);

extern mh_core_t
mh_core_or_new (mh_core_t *exps,
		int        exps_count);

static inline mh_core_t
mh_core_or_1_new (mh_core_t core)
{ 
  return mh_core_or_new (mh_core_1 (core), 1);
}

static inline mh_core_t
mh_core_or_2_new (mh_core_t core_1,
		  mh_core_t core_2)
{ 
  return mh_core_or_new (mh_core_2 (core_1, core_2), 2);
}

extern mh_core_t
mh_core_fmt_new (mh_core_t  *exps,
		 int         exps_count);

static inline mh_core_t
mh_core_fmt_1_new (mh_core_t core)
{
  return mh_core_fmt_new (mh_core_1 (core), 1);
}

static inline mh_core_t
mh_core_fmt_2_new (mh_core_t core_1,
		   mh_core_t core_2)
{
  return mh_core_fmt_new (mh_core_2 (core_1, core_2), 2);
}

static inline mh_core_t
mh_core_fmt_3_new (mh_core_t core_1,
		   mh_core_t core_2,
		   mh_core_t core_3)
{
  return mh_core_fmt_new (mh_core_3 (core_1, core_2, core_3), 3);
}

static inline mh_core_t
mh_core_fmt_4_new (mh_core_t core_1,
		   mh_core_t core_2,
		   mh_core_t core_3,
		   mh_core_t core_4)
{
  return mh_core_fmt_new (mh_core_4 (core_1, core_2, core_3, core_4), 4);
}

extern mh_core_t
mh_core_while_new (mh_core_t pred,
		   mh_core_t body);

extern mh_core_t
mh_core_break_new (mh_core_t value);

extern mh_core_t
mh_core_with_new (mh_core_t set,
		  mh_core_t body);

extern mh_core_t
mh_core_extern_new (string_t form);

extern mh_core_t
mh_core_extern_merge_strings (mh_core_t core1,
			      mh_core_t core2);

extern mh_core_t
mh_core_func_new (mh_tag_t     tag,
		  mh_core_t    body);

extern mh_core_t
mh_core_prim_new (mh_byte_op_t prim_op,
		  mh_core_t   *exps,
		  int          exps_count);

static inline mh_core_t
mh_core_prim_1_new (mh_byte_op_t prim_op,
		    mh_core_t    core)
{
  return mh_core_prim_new (prim_op, mh_core_1 (core), 1);
}

static inline mh_core_t
mh_core_prim_2_new (mh_byte_op_t prim_op,
		    mh_core_t    core_1,
		    mh_core_t    core_2)
{
  return mh_core_prim_new (prim_op, mh_core_2 (core_1, core_2), 2);
}

static inline mh_core_t
mh_core_prim_3_new (mh_byte_op_t prim_op,
		    mh_core_t    core_1,
		    mh_core_t    core_2,
		    mh_core_t    core_3)
{
  return mh_core_prim_new (prim_op, mh_core_3 (core_1, core_2, core_3), 3);
}

static inline mh_core_t
mh_core_prim_4_new (mh_byte_op_t prim_op,
		    mh_core_t    core_1,
		    mh_core_t    core_2,
		    mh_core_t    core_3,
		    mh_core_t    core_4)
{
  return mh_core_prim_new
    (prim_op, mh_core_4 (core_1, core_2, core_3, core_4), 4);
}

extern mh_core_t
mh_core_dup (mh_core_t core);

extern void
mh_core_var_subst (mh_core_t core,
		   string_t  this,
		   mh_core_t that);

extern boolean_t
mh_core_var_is_referenced (mh_core_t body,
			   string_t  this);

extern boolean_t
mh_core_is_side_effecting (mh_core_t core);

extern boolean_t
mh_core_is_extern_free (mh_core_t core);

#endif /* ! _MH_CORE_H_ */


