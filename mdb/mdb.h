/* mdb.h: -*- C -*-  Fundamental data structures of mdb. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Oct  1 20:39:17 1995. */

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

#if defined (__cplusplus)
extern "C"
{
#endif
typedef struct
{
  char *filename;		/* Name of loaded file. */
  char *nameonly;		/* The filename part of the file. */
  PAGE *contents;		/* The contents of the file. */
  int line_number;		/* The current line number. */
  struct stat finfo;		/* Disk Information. */
} MDB_File;

/* In mdb.c */
extern PAGE *mdb_page;
extern void mdb_loop (void);
extern int mdb_loop_level;
extern void mdb_throw_to_top_level (void);
extern int MDB_Interrupted;
extern void mdb_signal_restart (int sig);

#if defined (__cplusplus)
}
#endif
