/* tcp_session.h: -*- C -*-  Client/Server communication protocol. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Aug 13 22:41:41 1999.  */

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

#if !defined (_TCP_SESSION_H_)
#  define _TCP_SESSION_H_

#if defined (__cplusplus)
extern "C"
{
#endif

#define _tcpdb_OPEN   0
#define _tcpdb_STORE  1
#define _tcpdb_FETCH  2
#define _tcpdb_DELETE 3
#define _tcpdb_CLOSE  4
#define _tcpdb_FIRSTKEY 5
#define _tcpdb_NEXTKEY 6

#define _tcpdb_opname_OPEN	"OPEN"
#define _tcpdb_opname_STORE	"STORE"
#define _tcpdb_opname_FETCH	"FETCH"
#define _tcpdb_opname_DELETE	"DELETE"
#define _tcpdb_opname_CLOSE	"CLOSE"
#define _tcpdb_opname_FIRSTKEY	"FIRSTKEY"
#define _tcpdb_opname_NEXTKEY	"NEXTKEY"

extern SessionDBFuncs tcp_session_table;

#if defined (__cplusplus)
}
#endif

#endif /* _TCP_SESSION_H_ */
