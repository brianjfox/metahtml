/* code.h: -*- C -*- */

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

/* Interface declarations and definitions for byte_ops and prim_ops. */

#if !defined(_MH_CODE_H_)
#define _MH_CODE_H_ 1

/* 
 * MH_BYTEOP_T (Byte Operator and byte Operand)
 *
 * Enumerate the byte_code operators.  We limit the number of byte_ops to
 * 256 (including '0' which is not a byte_op).  
 */
typedef enum mh_byte_op
{
# define BYTEOP( operand, name, size )     MH_ ## operand ## _OP,
# include "bops.h"
  MH_NUMBER_OF_BYTEOPS
# undef BYTEOP
} mh_byte_op_t;

#if MH_NUMBER_OF_BYTEOPS >= 256
#error Too many byte_ops defined
#endif

/*
 * MH_BYTEOP_LEN_T
 *
 * Byte operators have an arbitrary number of byte operands.  The
 * byte_op JUMP, for example, has two operands to encode the 'index'
 * (in the byte code vector) to jump to.  The byte_op length includes
 * the operator itself.
 */

typedef enum
{
# define BYTEOP( operand, name, size )   MH_ ## operand ## _OP_LEN = size,
# include "bops.h"
  MH_IGNORE_BYTEOP_LEN
# undef BYTEOP
} mh_byte_op_len_t;

/* Get this specification correct by looking over bops.h */
#define MH_MAXIMUM_BYTEOP_LEN 3

/*
 * MH_BYTE_CODE_SPEC_T
 *
 * A byte_code spec summarized byte_op and is often used for printing
 * the byte vector.
 *
 */
typedef struct mh_byte_code_spec
{
  mh_byte_op_t     operator;
  mh_byte_op_len_t length;
  char             *name;
} *mh_byte_code_spec_t;

#define MH_BYTE_CODE_SPEC_OPERATOR( spec ) ((spec)->operator)
#define MH_BYTE_CODE_SPEC_LENGTH( spec )   ((spec)->length)
#define MH_BYTE_CODE_SPEC_NAME( spec )     ((spec)->name)

/* Table with one byte code spec for each byte operator */
extern struct mh_byte_code_spec
mh_byte_code_spec_table [1 + MH_NUMBER_OF_BYTEOPS];

#define MH_BYTE_CODE_SPEC_LOOKUP( code )	\
   (& mh_byte_code_spec_table [(code)])

/* <byte_code <code-vector> <code-constants> <code-stack> ...> */


#endif /* ! _MH_CODE_H_ */

/* Thu Oct 10 22:29:23 1996.  */
