/* breakpoints.h: -*- C -*-  Description of mdb breakpoint functionality. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Oct  1 20:29:07 1995. */

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

#if !defined (_BREAKPOINTS_H_)
#define _BREAKPOINTS_H_ 1

#if defined (__cplusplus)
extern "C"
{
#endif

#define break_USER      1
#define break_INTERNAL  2
#define break_CONDITION 3
#define break_DELETED   4
#define break_CALLBACK  5

typedef struct
{
  MDB_File *file;		/* The "Owner" of this breakpoint. */
  char *fname;			/* Used when MDB_File is NULL. */
  PAGE *code;			/* The execution page. */
  int type;			/* Type of breakpoint. */
  int line_number;		/* The line number of the original file. */
  int position;			/* The position of the breakpoint in the
				   original file. */
} MDB_Breakpoint;

/* Locate the breakpoint index for FNAME. */
extern int mdb_find_breakpoint_function (char *fname);

/* Return the list of our breakpoints. */
extern MDB_Breakpoint **mdb_breakpoint_list (void);

/* Return the indicated breakpoint structure. */
extern MDB_Breakpoint *mdb_this_breakpoint (int which);

/* Add a breakpoint to the list of breakpoints. */
extern void mdb_add_breakpoint (MDB_File *file, int *at_line, int type,
				char *fname);

/* Insert BPS into PAGE (which came from FILE. */
extern void mdb_insert_breakpoints (MDB_File *file, PAGE *page,
				    MDB_Breakpoint **bps);

/* Report the total number of breakpoints in FILE. */
extern int mdb_count_breakpoints (MDB_File *file);

/* Return a string describing the current state of breakpoints. */
extern char *mdb_breakpoint_info (void);

/* Locate the breakpoint structure for FILE, LINE and TYPE. */
extern MDB_Breakpoint *mdb_find_breakpoint (MDB_File *file, int line,
					    int type);

extern void mdb_set_next_breakpoint (MDB_Breakpoint *bp);
extern void mdb_set_step_breakpoint (MDB_Breakpoint *bp);

extern int mdb_skip_sexp (char *string);
extern int mdb_position_of_line (char *string, int which);

#if defined (__cplusplus)
}
#endif

#endif /* !_BREAKPOINTS_H_ */

