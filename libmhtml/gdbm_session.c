/* gdbm_session.c: -*- C -*-  The functions which handle sessioning thru the
   standard Meta-HTML database mechanism (i.e., GDBM). */

/* Author: Brian J. Fox (bfox@ua.com) Fri Aug 13 09:08:58 1999.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1995, 2001, Brian J. Fox (bfox@ai.mit.edu).

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

#include "mhtmlstd.h"
#include "sessiondb.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* ************************************************************ */
/*								*/
/*	  Forward Declarations for Private Implementation	*/
/*								*/
/* ************************************************************ */

extern int generic_session_reaper (void);

SessionDBFuncs gdbm_session_table =
{
  database_open,
  database_close,
  database_firstkey,
  database_nextkey,
  database_fetch,
  database_store,
  database_delete,
  database_setkey,
  generic_session_reaper,
  database_strerror,
  database_had_error_p,
  database_reset_error
};

#if defined (__cplusplus)
}
#endif
