/* mathfuncs.h: -*- C -*-  Externally visible functions in mathfuncs.c. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Tue May  9 11:00:51 2000. */

/* This file is part of <Meta-HTML>(tm), a system for the rapid
   deployment of Internet and Intranet applications via the use
   of the Meta-HTML language.

   Copyright (c) 1995, 2000, Brian J. Fox (bfox@ai.mit.edu).

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

	http://www.metahtml.com/COPYING
*/


#if !defined (_MATHFUNCS_H_)
#define _MATHFUNCS_H_

extern int mhtml_get_output_radix (void);
extern void mhtml_set_output_radix (int radix);
extern char *mhtml_output_to_radix (long num, int radix);

/* Return a string which is the printed representation of a number.
   DOT_PRESENT, if non-zero, says that the number should be printed with
   mhtml_decimal_places decimal places. */
extern char *mhtml_double_to_string (double arith_result, int dot_present);

#endif /*! _MATHFUNCS_H_ */
