/* acconfig.h: -*- C -*-  For "autoheader".. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Thu Aug 22 18:09:27 1996.  */

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

/* Defined every time by configure. */
#undef MHTML_SYSTEM_TYPE

/* Defined on those systems for which ODBC compilation with the UDBC client
   is desired. */
#undef OPENLINK_UDBC_CLIENT

/* Defined only when compiing on some brand of Solaris system. */
#undef Solaris

/* Defined when your system has a define or typedef for sig_t. */
#undef HAVE_TYPE_SIG_T


/* Defined when your system has a define or typedef for time_t. */
#undef HAVE_TYPE_TIME_T

@BOTTOM@
#if defined (Solaris)
#  include <arch/solaris/prototypes.h>
#endif

#if defined (HAVE_TIME_H)
#  define METAHTML_PROFILER 1
#endif


