/* modules.h: -*- C -*-  Header file for all modules. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jun 27 11:25:42 1998.  */

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

#if !defined (_MODULES_H_)
#define _MODULES_H_

#include "language.h"

#if defined (__cplusplus)
extern "C" {
#endif

#if !defined (MODULE_INITIALIZER_EXTRA_CODE)
#   define MODULE_INITIALIZER_EXTRA_CODE
#endif

#define MODULE_INITIALIZE(packstring, ftab)				\
void									\
module_initialize (void)						\
{									\
  static int called = 0;						\
  if (!called)								\
    {									\
      register int i;							\
      Symbol *sym, *funcnames;						\
      char symname[256];						\
									\
      called++;								\
      sprintf (symname, "modules::syms-of-%s", packstring);		\
      funcnames = symbol_intern (symname);				\
									\
      /* Install the names and pointers. */				\
      for (i = 0; ftab[i].tag != (char *)NULL; i++)			\
	{								\
	  sym = symbol_intern_in_package				\
		(mhtml_function_package, ftab[i].tag);			\
	  symbol_add_value (funcnames, ftab[i].tag);			\
	  sym->type = symtype_FUNCTION;					\
	  sym->values = (char **)(&ftab[i]);				\
	}								\
       MODULE_INITIALIZER_EXTRA_CODE					\
    }									\
}

extern void module_initialize (void);

#if !defined (__cplusplus) && !defined (Solaris)
void _init (void) { module_initialize (); }
#endif

  /* #if defined (__CYGWIN32__)
#  include <cygwin32/cygwin_dll.h>
int WINAPI dllEntry(HANDLE hDll, DWORD reason, LPVOID reserved)
{
  return (TRUE);
}
#endif / * __CYGWIN32__ */

#if defined (__cplusplus)
}
#endif

#endif /* MODULES_H */
