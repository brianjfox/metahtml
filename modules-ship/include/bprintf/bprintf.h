/* bprintf.h: Declarations of functions defined in bprintf.c. */

/* Author: Brian J. Fox (bfox@ua.com) Thu Apr 20 19:34:51 1995. */

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

#if !defined (_BPRINTF_H_)
#define _BPRINTF_H_

#if defined (__cplusplus)
extern "C"
{
#endif

/* A structure used to hold onto a dynamically created buffer of text. */
typedef struct {
  char *buffer;
  int bindex;
  int bsize;
  void *attachment;		/* Anything you want to put here. */
} BPRINTF_BUFFER;

/* Create a new empty output buffer. */
extern BPRINTF_BUFFER *bprintf_create_buffer (void);

/* Create a new buffer which is a copy of INPUT. */
extern BPRINTF_BUFFER *bprintf_copy_buffer (BPRINTF_BUFFER *input);

/* Free the contents of a bprintf buffer. */
extern void bprintf_free_buffer (BPRINTF_BUFFER *buffer);

/* The main function in this library.  Print to BUFFER with FORMAT and
   any additional args. */
extern void bprintf (BPRINTF_BUFFER *buffer, char *format, ...);

/* Like bprintf, but after the va_list has been started. */
extern void vbprintf (BPRINTF_BUFFER *buffer, char *format, va_list args);

/* A few helpful functions for manipulating the insides of bprintf buffers. */

/* Delete the characters from START to END from buffer.  The END'th
   character is not deleted, but the START'th character is.
   Return the number of characters deleted. */
extern int bprintf_delete_range (BPRINTF_BUFFER *buffer, int start, int end);

/* Insert into BUFFER at POINT the results of printing FORMAT with args.
   Return the number of characters inserted, or -1 if there was an error. */
extern int bprintf_insert (BPRINTF_BUFFER *buffer, int point, char *fmt, ...);

/* Insert info BUFFER at POINT LEN bytes of DATA. */
extern void bprintf_insert_binary (BPRINTF_BUFFER *buffer, int point,
				   char *data, int len);

/* Append to BUFFER LEN bytes of DATA. */
extern void bprintf_append_binary (BPRINTF_BUFFER *buffer,
				   char *data, int len);

/* Insert into BUFFER at POINT the string TEXT.
   Return the number of characters inserted, or -1 if there was an error. */
extern int bprintf_insert_text (BPRINTF_BUFFER *buffer, int point, char *text);

/* Word wrap the text in BUFFER making lines no longer than WIDTH. */
extern void bprintf_word_wrap (BPRINTF_BUFFER *buffer, int width);

#if defined (__cplusplus)
}
#endif

#endif /* ! _BPRINTF_H_ */
