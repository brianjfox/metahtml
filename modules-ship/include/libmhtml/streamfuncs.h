/* streamfuncs.h: -*- C -*-  Header file for stream functions in Meta-HTML. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Tue Sep 26 21:50:11 1995. */

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

#if !defined (_STREAMFUNCS_H_)
#define _STREAMFUNCS_H_

#if defined (__cplusplus)
extern "C"
{
#endif

typedef enum
  { mhtml_stream_FILE, mhtml_stream_TCP, mhtml_stream_FD,
    mhtml_stream_PROG, mhtml_stream_CLOSED }
MHTMLStreamType;

#define mhtml_stream_MUST_SEEK 0x01
#define mhtml_stream_NOTIMEOUT 0x02
#define mhtml_stream_op_NONE  0
#define mhtml_stream_op_READ  1
#define mhtml_stream_op_WRITE 2

typedef struct
{
  char *name;		/* The name used to open this stream. */
  MHTMLStreamType type;	/* The type of stream: stream_FILE, stream_TCP, etc. */
  int fd;		/* The file descriptor that describes this stream. */
  int ifd;		/* For PROG pipes, the input channel to the stream. */
  pid_t pid;		/* For PROG pipes, the pid of the child. */
  int mode;		/* Access mode. */
  int flags;		/* Special flags indicate changes in behaviour. */
  int timeout;		/* Default value for IO timer. */
  int last_op;		/* The last operation done on this stream. */
} MHTMLStream;

#define HAVE_MHTML_STREAM_TYPE 1

extern int mhtml_stdin_fileno;
extern int mhtml_stdout_fileno;
extern int mhtml_stderr_fileno;

/* Return an alist representing the stream referenced by the first
   positional argument in VARS. */
extern char *mhtml_stream_reference_to_alist (Package *vars);

/* Return a pointer to the MHTMStream structure representing the stream
   passed in VARS.  In order to manipulate a stream from a library, you
   should call this function in the following way:

   static void
   pf_muck_with_stream (PFunArgs)
    {
      MHTMLStream *stream = mhtml_get_stream_reference (vars);
      if (stream != (MHTMLStream *)NULL)
        muck_with_stream (stream);
    }
*/
extern MHTMLStream *mhtml_get_stream_reference (Package *vars);

/* Returns the internal stream type matching NAME. */
extern MHTMLStreamType mhtml_stream_type (char *type_name);

#if defined (__cplusplus)
}
#endif

#endif /* _STREAMFUNCS_H_ */

