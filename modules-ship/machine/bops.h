/* bops.h: -*- C -*- */

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

/* Byte Operators (BOPS)
 *
 * Byte operators enumerate the fundamental, lowest-level commands in the
 * 'byte engine'.  An operator and its operands comprise a single
 * instruction.  The listing below is used in various ways to construct the
 * enumerated types for bc_byte_op_t (see code.h), bc_byte_op_len_t (see
 * code.h) and bc_byte_code_spec_t (see engine.c). 
 *
 * The fields below are:
 *
 *    OPERATOR           NAME                LENGTH */
BYTEOP (LABEL,		"label",		3)
BYTEOP (CALL,		"call",			2)
BYTEOP (CALL_TAIL,	"call-tail",		2)
BYTEOP (RETURN,		"ret",		        1)
BYTEOP (SHIFT,		"shift",	        2)

     /* External Calls */
BYTEOP (EVAL,           "eval",                 1)

     /* Jumps */
BYTEOP (JUMP,		"jump",			3)
BYTEOP (JUMP_IF_FALSE,	"jump-if-false",	3)
BYTEOP (JUMP_IF_TRUE,	"jump-if-true",		3)
BYTEOP (JUMP_IF_EQ,	"jump-if-eq",		3)

BYTEOP (ID,		"id",			1)

     /* Data */
BYTEOP (POP,		"pop",			1)
BYTEOP (POP_N,		"popn",			2)
BYTEOP (DUP,		"dup",			1)
BYTEOP (DATA,		"data",			2)
BYTEOP (DATA_LONG,	"data-long",		3)

BYTEOP (OUT,		"out",			1)
BYTEOP (CAT,            "cat",		        1)
BYTEOP (CAT_N,          "catn",		        2)

  /* Variable Access */
BYTEOP (SET,		"set",			2)
BYTEOP (SET_LONG,	"set-long",		3)
BYTEOP (SET_RAW,	"set-raw",		3)
BYTEOP (GET,		"get",			2)
BYTEOP (GET_LONG,	"get-long",		3)
BYTEOP (GET_RAW,	"get-raw",		3)

  /* Function Access */
BYTEOP (FSET,		"fset",			2)
BYTEOP (FSET_LONG,	"fset-long",		3)
BYTEOP (FGET,		"fget",			2)
BYTEOP (FGET_LONG,	"fget-long",		3)

     /* Literals */
BYTEOP (DATA_EMPTY,     "data-empty",           1)
BYTEOP (DATA_TRUE,      "data-true",            1)
BYTEOP (DATA_FALSE,     "data-false",           1)
BYTEOP (DATA_ZERO,      "data-zero",            1)
BYTEOP (DATA_ONE,       "data-one",             1)

/*
 * Primitive Operators
 *
 * A Primitive Operator is a Byte Operator used for the implementation
 * of a function (a 'primitive' function).  Primitive operators map
 * between the Meta-HTML function and the byte operator used to
 * implement the function within the byte engine.  In order to use a
 * byte operator a Meta-HTML function call must be a call to one of
 * the primitive operators and the arguments in the call must be of a
 * number appropriate for the primitive operator.  The listing below,
 * like those above, is used to construct various types including
 * bc_prim_op_spec_t (see code.h, expand.c and optimize.c). */
#if ! defined (PRIMOP)
#define PRIMOP( operator, name, length, min_args, max_args )	\
  BYTEOP (operator, name, length)
#define PRIMOP_DEFINED
#endif /* ! defined (PRIMOP) */

/*
 *
 * The fields below are:
 *
 *     OPERATOR         NAME                LENGTH MIN MAX  */
PRIMOP (NOT,		"not",			1,  1,  1)

   /* Numbers */
PRIMOP (ADD,		"add",			1,  2,  2)
PRIMOP (SUB,		"sub",			1,  2,  2)
PRIMOP (MUL,		"mul",			1,  2,  2)
PRIMOP (DIV,		"div",			1,  2,  2)

PRIMOP (DEC,		"dec",			1,  1,  1)
PRIMOP (INC,		"inc",			1,  1,  1)

PRIMOP (EQ,             "eq",                   1,  2,  2)
PRIMOP (NE,             "neq",                  1,  2,  2)
PRIMOP (LT,             "lt",                   1,  2,  2)
PRIMOP (LE,             "le",                   1,  2,  2)
PRIMOP (GT,             "gt",                   1,  2,  2)
PRIMOP (GE,             "ge",                   1,  2,  2)

PRIMOP (EQZ,            "eqz",                  1,  1,  1)
PRIMOP (NEZ,            "nez",                  1,  1,  1)
PRIMOP (INTEGER_P,	"integer?",		1,  1,  1)
PRIMOP (REAL_P,	        "real?",		1,  1,  1)

PRIMOP (SQRT,           "sqrt",                 1,  1,  1)

PRIMOP (RANDOMIZE,	"randomize",		1,  1,  1)
PRIMOP (RANDOM, 	"random",		1,  1,  1)

PRIMOP (INTEGER,	"integer",		1,  1,  1)

  /* Strings */
PRIMOP (STR_COMP,	"string-compare",	1,  2,  2)
PRIMOP (STR_EQ,	        "string-eq",         	1,  3,  3)
PRIMOP (STR_NEQ,        "string-neq",    	1,  3,  3)
PRIMOP (STR_LEN,	"string-length",	1,  1,  1)
PRIMOP (STR_SUB,	"substring",    	1,  3,  3)
PRIMOP (STR_DOWN,	"downcase",		1,  1,  1)
PRIMOP (STR_UP,		"upcase",		1,  1,  1)
PRIMOP (STR_CAP,	"capitalize",		1,  1,  1)

  /* Symbols / Varibles */
PRIMOP (GET_VAR,	"get-var",		1,  1,  1)
PRIMOP (GET_VAR_RAW,	"get-var-raw",		1,  1,  1)
PRIMOP (SET_VAR,	"set-var",		1,  2,  2)
PRIMOP (SET_VAR_RAW,	"set-var-raw",		1,  2,  2)
PRIMOP (VAR_EXISTS,	"var-exists",		1,  1,  1)
PRIMOP (VAR_UNSET,	"var-unset",		1,  1,  1)

PRIMOP (ALIST,  	"alist",		2,  0,  -1)
PRIMOP (ALIST_P,	"alist?",		1,  1,  1)
PRIMOP (ALIST_NEXT,	"alist-next",	        1,  1,  1)
PRIMOP (ALIST_GET,	"alist-get-var",	1,  2,  2)
PRIMOP (ALIST_SET,	"alist-set-var",	2,  2,  -1)
PRIMOP (ALIST_GET_RAW,	"alist-get-var-raw",	1,  2,  2)
PRIMOP (ALIST_MOD,	"alist-mod-var",	1,  4,  4)
PRIMOP (ALIST_REM,	"alist-unset-var",      1,  2,  2)
PRIMOP (ALIST_HAS,	"alist-has-var",        1,  2,  2)
PRIMOP (ALIST_LEN,	"alist-length",         1,  1,  1)
PRIMOP (ALIST_MERGE,	"alist-merge",          1,  2,  2)
PRIMOP (ALIST_NAMES,	"alist-names",          1,  1,  1)
PRIMOP (ALIST_VALUES,	"alist-values",         1,  1,  1)

PRIMOP (ARRAY,  	"array",		2,  0, -1)
PRIMOP (ARRAY_REF,	"array-ref",            1,  2,  2)
PRIMOP (ARRAY_SET,	"array-set",            1,  3,  3)
PRIMOP (ARRAY_LEN,	"array-size",           1,  1,  1)
PRIMOP (ARRAY_REV,	"array-reverse",        1,  1,  1)
PRIMOP (ARRAY_MEM,	"array-member",         1,  3,  3)
PRIMOP (ARRAY_APP,	"array-append",         1,  2,  2)
PRIMOP (ARRAY_COPY,	"array-copy",           1,  1,  1)
PRIMOP (ARRAY_FORCE,    "array-force",          1,  1,  1)
PRIMOP (ARRAY_NEW_AT,  	"array-new-at",		1,  2,  2)

PRIMOP (PACKAGE_TO_ALIST,	"package-to-alist",	1,  2,  2)
PRIMOP (ALIST_TO_PACKAGE,	"alist-to-package",	1,  1,  1)


#if defined (PRIMOP_DEFINED)
#undef PRIMOP
#undef PRIMOP_DEFINED
#endif /* defined (PRIMOP_DEFINED) */

/*
 *
 * Primitive Operators do not appear as symbol/variable values; rather
 * they are 'optimizations' when expanding.  But, primops do have
 * functions and those functions need to be values and those functions
 * need to be produced.  Either by defprim or explicit 'C code' to
 * produce a function.
 *
 * <defprim add a b>
 *    <add a b>
 * </defprim>
 *
 * The above looks recursive but the call to <add a b> would be
 * 'inlined' as a byte-op for BC_ADD_OP.
 *
 */


/* Date: Wed Nov  6 16:37:48 1996 */

