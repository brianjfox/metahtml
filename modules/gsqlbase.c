/* gsqlbase.c: -*- C -*-  The basis for all SQL DB modules. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Thu May 28 22:03:49 1998.

    This file is part of <Meta-HTML>(tm), a system for the rapid
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

	 http://www.metahtml.com/COPYING  */

#define COMPILING_MODULE 1

#include "gsql.c"

#if defined (__cplusplus)
extern "C"
{
#endif

void
module_initialize (void)
{
  static int called = 0;

  if (!called)
    {
      register int i;
      Symbol *sym, *funcnames;
      char *symsof = "modules::syms-of-database";

#if defined (COMPILING_MYSQLFUNCS)
      symsof = "modules::syms-of-mysql";
#endif

#if defined (COMPILING_MYSQLPERFUNCS)
      symsof = "modules::syms-of-mysqlper";
#endif

#if defined (COMPILING_MSQLFUNCS)
      symsof = "modules::syms-of-msql";
#endif

#if defined (COMPILING_ODBCFUNCS)
      symsof = "modules::syms-of-odbc";
#endif

#if defined (COMPILING_IODBCFUNCS)
      symsof = "modules::syms-of-iodbc";
#endif

#if defined (COMPILING_ODBCPERFUNCS)
      symsof = "modules::syms-of-odbcper";
#endif

#if defined (COMPILING_IODBCPERFUNCS)
      symsof = "modules::syms-of-iodbcper";
#endif

      called++;
      funcnames = symbol_intern (symsof);

      /* Install the names and pointers. */
      for (i = 0; func_table[i].tag != (char *)NULL; i++)
	{
	  sym = symbol_intern_in_package
	    (mhtml_function_package, func_table[i].tag);
	  symbol_add_value (funcnames, func_table[i].tag);
	  sym->type = symtype_FUNCTION;
	  sym->values = (char **)(&func_table[i]);
	}
    }
}

#if defined (__CYGWIN32__)
#  include <cygwin32/cygwin_dll.h>
int WINAPI dllEntry(HANDLE hDll, DWORD reason, LPVOID reserved)
{
  return (TRUE);
}
#endif /* __CYGWIN32__ */

#if !defined (__cplusplus)
#  if !defined (Solaris)
     void _init (void) { module_initialize (); }
#  endif
#else
}
#endif

