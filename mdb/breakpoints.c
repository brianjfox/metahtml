/* breakpoints.c: Setting, inserting, and deleting breakpoints in MDB. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Oct  1 20:30:09 1995.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
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

#include "language.h"
#include "mdb.h"
#include "commands.h"
#include "breakpoints.h"


#if defined (__cplusplus)
extern "C"
{
#endif

/* An array of breakpoints. */
static MDB_Breakpoint **mdb_breakpoints = (MDB_Breakpoint **)NULL;
static int mdb_breakpoints_slots = 0;
static int mdb_breakpoints_index = 0;

/* The breakpoint used internally, e.g., for doing "next". */
static MDB_Breakpoint *mdb_internal_bp = (MDB_Breakpoint *)NULL;

/* Return the list of our breakpoints. */
MDB_Breakpoint **
mdb_breakpoint_list (void)
{
  return (mdb_breakpoints);
}

/* Return the indicated breakpoint structure. */
MDB_Breakpoint *
mdb_this_breakpoint (int which)
{
  MDB_Breakpoint *bp = (MDB_Breakpoint *)NULL;

  if ((which < mdb_breakpoints_index) && (which > -1))
    bp = mdb_breakpoints[which];
  else if (which == -1)
    bp = mdb_internal_bp;

  return (bp);
}

static void
mdb_skip_to_start_of_sexp (char *buffer, int *position)
{
  register int pos = *position;
  int done = 0;

  /* Skip leading whitespace and comments. */
  while (!done)
    {
      while (whitespace (buffer[pos])) pos++;

      if ((buffer[pos] == ';') &&
	  (buffer[pos + 1] == ';') &&
	  (buffer[pos + 2] == ';'))
	while ((buffer[pos] != '\0') && (buffer[pos] != '\n')) pos++;
      else
	done = 1;
    }
  *position = pos;
}

int
mdb_skip_sexp (char *string)
{
  register int i;
  int quoted = 0;
  int brace_depth = 0;
  char c;

  /* Skip leading whitespace. */
  for (i = 0; string[i] && whitespace (string[i]); i++);

  for (; (c = string[i]) != '\0'; i++)
    {
      switch (c)
	{
	case ';':
	  if ((string[i + 1] == ';') && (string[i + 2] == ';'))
	    {
	      while ((string[i] != '\0') && (string[i] != '\n')) i++;
	      while (whitespace (string[i])) i++;
	      i--;
	      continue;
	    }
	  break;

	case '"':
	  quoted = !quoted;
	  break;

	case '<':
	  if (!quoted)
	    brace_depth++;
	  break;

	case '>':
	  if (!quoted)
	    brace_depth--;

	  if (!brace_depth)
	    return (i + 1);
	  break;

	case ' ':
	  if (!brace_depth && !quoted)
	    return (i);
	  break;
	}
    }

  return (i);
}

int
mdb_step_into (char *string)
{
  register int i;
  int quoted = 0;
  int brace_depth = 0;
  char c;

  /* Skip leading whitespace. */
  for (i = 0; string[i] && whitespace (string[i]); i++);

  for (; (c = string[i]) != '\0'; i++)
    {
      switch (c)
	{
	case ';':
	  if ((string[i + 1] == ';') && (string[i + 2] == ';'))
	    {
	      while ((string[i] != '\0') && (string[i] != '\n')) i++;
	      while (whitespace (string[i])) i++;
	      i--;
	      continue;
	    }
	  break;

	case '"':
	  quoted = !quoted;
	  break;

	case '<':
	  if (!quoted)
	    brace_depth++;

	  if (brace_depth == 2)
	    return (i);
	  break;

	case '>':
	  if (!quoted)
	    brace_depth--;
	  if (!brace_depth)
	    return (i + 1);
	  break;

	case ' ':
	  if (!brace_depth && !quoted)
	    return (i);
	  break;
	}
    }

  return (i);
}

static void
mdb_insert_bp_1 (PAGE *page, int which, MDB_Breakpoint *bp, int position)
{
  int expression_length;
  BPRINTF_BUFFER *insertion;

  if (position == -1)
    return;

  /* The breakpoint surrounds the area to be broken.  We have to parse
     a single sexp forward. */
  insertion = bprintf_create_buffer ();
  bprintf  (insertion, "<*MDB*::BREAK %d ", which);

  /* Now skip until we have read an entire expression. */
  expression_length = mdb_skip_sexp (page->buffer + position);

  /* Insert the breakpoint. */
  bprintf_insert (page, position, "%s", insertion->buffer);
  bprintf_insert (page, position + insertion->bindex + expression_length, ">");

  /* Remember the page. */
  bp->code = page;
}

/* Insert a breakpoint at bp->position. */
static void
mdb_insert_bp_at_pos (PAGE *page, int which, MDB_Breakpoint *bp)
{
  int position = bp->position;

  mdb_skip_to_start_of_sexp (page->buffer, &position);
  mdb_insert_bp_1 (page, which, bp, position);
}

/* Insert a single breakpoint. */
static void
mdb_insert_bp_at_line (PAGE *page, int which, MDB_Breakpoint *bp)
{
  int position = mdb_position_of_line (page->buffer, bp->line_number);

  mdb_skip_to_start_of_sexp (page->buffer, &position);
  mdb_insert_bp_1 (page, which, bp, position);
}

/* Insert BPS into PAGE (which came from FILE. */
void
mdb_insert_breakpoints (MDB_File *file, PAGE *page, MDB_Breakpoint **bps)
{
  register int i;
  MDB_Breakpoint *bp;

  if (!bps)
    return;

  for (i = 0; (bp = bps[i]) != (MDB_Breakpoint *)NULL; i++)
    if ((bp->file == file) && (bp->type != break_DELETED))
      mdb_insert_bp_at_line (page, i, bp);
}

/* Report the total number of breakpoints in FILE. */
int
mdb_count_breakpoints (MDB_File *file)
{
  register int i;
  int counter = 0;

  for (i = 0; i < mdb_breakpoints_index; i++)
    if ((mdb_breakpoints[i]->file == file) &&
	(mdb_breakpoints[i]->type != break_DELETED))
      counter++;

  return (counter);
}

static int
mdb_what_line (char *string, int pos)
{
  register int i;
  int line = 1;

  for (i = 0; string[i] && i < pos; i++)
    if (string[i] == '\n')
      line++;

  return (line);
}

/* You can only call this with an active breakpoint that has already been
   run through pf_break_handler ().  If you don't understand this, don't
   call this function. */
void
mdb_set_next_breakpoint (MDB_Breakpoint *bp)
{
  MDB_Breakpoint *new_bp;
  int pos;

  new_bp = (MDB_Breakpoint *)xmalloc (sizeof (MDB_Breakpoint));
  new_bp->type = break_INTERNAL;
  new_bp->file = bp->file;
  new_bp->fname = bp->fname;
  new_bp->code = bp->code;
  new_bp->fname = (char *)NULL;

  /* Get the line number in the execution page of this breakpoint. */
  pos = bp->position + mdb_skip_sexp (bp->code->buffer + bp->position);
  new_bp->position = pos;
  new_bp->line_number = mdb_what_line (bp->code->buffer, pos);

  /* Get the line number in the original file of this breakpoint. */
  pos = mdb_position_of_line (bp->file->contents->buffer, bp->line_number);
  while (1)
    {
      pos += mdb_skip_sexp (bp->file->contents->buffer + pos);
      mdb_skip_to_start_of_sexp (bp->file->contents->buffer, &pos);
      if ((bp->file->contents->buffer[pos + 0] != '<') ||
	  (bp->file->contents->buffer[pos + 1] != '/'))
	break;
    }

  if (pos != bp->file->contents->bindex)
    {
      /* Set this breakpoint in the page. */
      mdb_insert_bp_at_pos (new_bp->code, -1, new_bp);

      new_bp->line_number = mdb_what_line (bp->file->contents->buffer, pos);

      /* Remeber this breakpoint... */
      mdb_internal_bp = new_bp;
    }
  else
    free (new_bp);
}

/* You can only call this with an active breakpoint that has already been
   run through pf_break_handler ().  If you don't understand this, don't
   call this function. */
void
mdb_set_step_breakpoint (MDB_Breakpoint *bp)
{
  MDB_Breakpoint *new_bp;
  int pos, line;

  new_bp = (MDB_Breakpoint *)xmalloc (sizeof (MDB_Breakpoint));
  new_bp->type = break_INTERNAL;
  new_bp->file = bp->file;
  new_bp->fname = bp->fname;
  new_bp->code = bp->code;
  new_bp->fname = (char *)NULL;

  /* Get the line number in the execution page of this breakpoint. */
  pos = bp->position + mdb_step_into (bp->code->buffer + bp->position);
  new_bp->position = pos;
  new_bp->line_number = mdb_what_line (bp->code->buffer, pos);

  /* Get the line number in the original file of this breakpoint. */
  line = mdb_position_of_line (bp->file->contents->buffer, bp->line_number);
  while (1)
    {
      line += mdb_skip_sexp (bp->file->contents->buffer + line);
      mdb_skip_to_start_of_sexp (bp->file->contents->buffer, &line);
      if ((bp->file->contents->buffer[pos + 0] != '<') ||
	  (bp->file->contents->buffer[pos + 1] != '/'))
	break;
    }

  if (pos != bp->file->contents->bindex)
    {
      /* Set this breakpoint in the page. */
      mdb_insert_bp_at_pos (new_bp->code, -1, new_bp);

      new_bp->line_number = mdb_what_line (bp->file->contents->buffer, line);

      /* Remeber this breakpoint... */
      mdb_internal_bp = new_bp;
    }
  else
    free (new_bp);
}

/* Return a string describing the current state of breakpoints. */
char *
mdb_breakpoint_info (void)
{
  register int i;
  BPRINTF_BUFFER *listing = (BPRINTF_BUFFER *)NULL;
  char *result;

  if (mdb_breakpoints_index != 0)
    {
      for (i = 0; i < mdb_breakpoints_index; i++)
	{
	  if (mdb_breakpoints[i]->type != break_DELETED)
	    {
	      if (listing == (BPRINTF_BUFFER *)NULL)
		{
		  listing = bprintf_create_buffer ();
		  bprintf (listing, "Num         In File      \tAt Line\n");
		}

	      if (mdb_breakpoints[i]->file != (MDB_File *)NULL)
		{
		  bprintf (listing, "%3d  %20s\t%5d\n", i + 1,
			   mdb_breakpoints[i]->file->nameonly,
			   mdb_breakpoints[i]->line_number);
		}
	      else
		{
		  bprintf (listing, "%3d  %20s\t%5d\n", i + 1,
			   mdb_breakpoints[i]->fname ?
			   mdb_breakpoints[i]->fname : "#undefined#", 0);
		}
	    }
	}
    }

  if (listing == (BPRINTF_BUFFER *)NULL)
    result = strdup ("There are no breakpoints.");
  else
    {
      result = listing->buffer;
      free (listing);
    }
  return (result);
}

/* Locate the breakpoint index for FNAME. */
int
mdb_find_breakpoint_function (char *fname)
{
  register int i;

  for (i = 0; i < mdb_breakpoints_index; i++)
    if ((mdb_breakpoints[i]->fname != (char *)NULL) &&
	(strcasecmp (mdb_breakpoints[i]->fname, fname) == 0))
      return (i);

  return (-1);
}

/* Locate the breakpoint structure for FILE, LINE and TYPE. */
MDB_Breakpoint *
mdb_find_breakpoint (MDB_File *file, int line, int type)
{
  register int i;
  MDB_Breakpoint *result;

  result = (MDB_Breakpoint *)NULL;

  for (i = 0; i < mdb_breakpoints_index; i++)
    if ((mdb_breakpoints[i]->file == file) &&
	(mdb_breakpoints[i]->type == type) &&
	(mdb_breakpoints[i]->line_number == line))
      {
	result = mdb_breakpoints[i];
	break;
      }

  return (result);
}

/* Add a breakpoint to the list of breakpoints. */
void
mdb_add_breakpoint (MDB_File *file, int *at_line, int type, char *fname)
{
  MDB_Breakpoint *bp = (MDB_Breakpoint *)NULL;
  int the_line = *at_line;

  if (fname != (char *)NULL)
    {
      int which = mdb_find_breakpoint_function (fname);
      if (which != -1)
	bp = mdb_breakpoints[which];
    }
  else
    {
      /* Adjust the_line to reflect the actual location of the breakpoint. */
      int pos = mdb_position_of_line (file->contents->buffer, the_line);
      mdb_skip_to_start_of_sexp (file->contents->buffer, &pos);
      the_line = mdb_what_line (file->contents->buffer, pos);
    }

  *at_line = the_line;

  if (type != break_CONDITION)
    bp = mdb_find_breakpoint (file, the_line, type);

  if (!bp)
    {
      bp = (MDB_Breakpoint *)xmalloc (sizeof (MDB_Breakpoint));
      bp->type = type;
      bp->line_number = the_line;
      bp->file = file;
      bp->fname = fname;
      bp->code = (PAGE *)NULL;
      bp->position = -1;

      if (mdb_breakpoints_index + 2 > mdb_breakpoints_slots)
	mdb_breakpoints = (MDB_Breakpoint **)xrealloc
	  (mdb_breakpoints, ((mdb_breakpoints_slots += 10) * sizeof (char *)));
      mdb_breakpoints[mdb_breakpoints_index++] = bp;
      mdb_breakpoints[mdb_breakpoints_index] = (MDB_Breakpoint *)NULL;
    }
}

int
mdb_position_of_line (char *string, int which)
{
  if (string)
    {
      register int i;
      int line = 1;

      if (which == 1)
	return (0);

      for (i = 0; string[i]; i++)
	{
	  if (string[i] == '\n')
	    {
	      line++;

	      if (line == which)
		return (i + 1);
	    }
	}
    }

  return (-1);
}

#if defined (__cplusplus)
}
#endif
