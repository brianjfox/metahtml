/*  bprintf.c: Functions in bprintf work like fprintf, but to a "buffer". */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Thu Apr 20 12:44:17 1995.  */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <ctype.h>
#include <xmalloc/xmalloc.h>

#include "bprintf.h"

/* The value which is printed when %s is called with a null pointer. */
#if !defined (NULL_STRING_VALUE)
#define NULL_STRING_VALUE ""
#endif

#if !defined (whitespace)
#  define whitespace(x) \
	(((x) == ' ') || ((x) == '\t') || ((x) == '\r') || ((x) == '\n'))
#endif

/* Create a new empty bprintf buffer. */
BPRINTF_BUFFER *
bprintf_create_buffer (void)
{
  BPRINTF_BUFFER *buffer;

  buffer = (BPRINTF_BUFFER *)xmalloc (sizeof (BPRINTF_BUFFER));
  memset (buffer, 0, sizeof (BPRINTF_BUFFER));

  return (buffer);
}

/* Free the contents of a bprintf buffer. */
void
bprintf_free_buffer (BPRINTF_BUFFER *buffer)
{
  if (buffer)
    {
      if (buffer->buffer)
	free (buffer->buffer);
      free (buffer);
    }
}

/* Create a new buffer which is a copy of INPUT. */
BPRINTF_BUFFER *
bprintf_copy_buffer (BPRINTF_BUFFER *input)
{
  BPRINTF_BUFFER *copy;

  copy = bprintf_create_buffer ();
  copy->bindex = input->bindex;
  copy->bsize = input->bsize;
  if (copy->bsize)
    {
      copy->buffer = (char *)xmalloc (1 + copy->bsize);
      memcpy (copy->buffer, input->buffer, copy->bsize);
      copy->buffer[copy->bsize] = '\0';
    }

  return (copy);
}

/* Resize BUFFER so that it has enough space to store LEN more bytes. */
static void
resize_buffer (BPRINTF_BUFFER *buffer, int len)
{
  if ((buffer->bindex + len + 2) > buffer->bsize)
    buffer->buffer = (char *)xrealloc
      (buffer->buffer, (buffer->bsize += (len + 100)));
}

/* Add STRING to BUFFER. */
static void
add_string_to_buffer (char *string, BPRINTF_BUFFER *buffer)
{
  if (string)
    {
      int len = strlen (string);

      resize_buffer (buffer, len);
      strcpy (buffer->buffer + buffer->bindex, string);
      buffer->bindex += len;
    }
}

/* The list of characters which terminate a printf specification. */
static char *printf_arg_specifiers = "diouxXDOUeEfgcspnm%";

/* The functions which actually format a small amount of information. */

/* Only used to format numbers, or strings which are guaranteed to fit. */
#define FORMAT_BUFF_SIZE 512
static char format_buff[FORMAT_BUFF_SIZE];
static int min_field_width = 0;
static int precision = 0;

/* Parse the MIN_FIELD_WIDTH and PRECISION specifiers in SPEC.  SPEC is
   the complete printf specification of what to display, such as "%6.2f". */
static void
parse_field_specifiers (char *spec)
{
  register int start, end;
  int parsing_width = 1;
  char numbuff[20];

  min_field_width = 0;
  precision = 0;

  /* If no field width or precision specifiers, then we are all done. */
  if (strlen (spec) == 2)
    return;

  /* Skip non-digit characters. */
  for (start = 0; spec[start] != '\0' && !isdigit (spec[start]); start++)
    if (spec[start] == '.')
      parsing_width = 0;

  /* If no more characters left, no field width or precision specifiers. */
  if (spec[start] == '\0')
    return;

  /* Gather all of the digits up to a decimal point. */
  for (end = start; spec[end] != '\0' && isdigit (spec[end]); end++);

  strncpy (numbuff, spec + start, end - start);
  numbuff[end - start] = '\0';

  sscanf (numbuff, "%d", parsing_width ? &min_field_width : &precision);

  if (!parsing_width)
    return;

  /* Time to parse the precision. */
  for (start = end; spec[start] != '\0' && !isdigit (spec[start]); start++);

  if (!isdigit (spec[start]))
    return;

  for (end = start; spec[end] != '\0' && isdigit (spec[end]); end++);

  strncpy (numbuff, spec + start, end - start);
  numbuff[end - start] = '\0';

  sscanf (numbuff, "%d", &precision);
}

static char *
format_long (char *spec, long value)
{
  sprintf (format_buff, spec, value);
  return (format_buff);
}

static char *
format_short (char *spec, short value)
{
  sprintf (format_buff, spec, value);
  return (format_buff);
}

static char *
format_int (char *spec, int value)
{
  sprintf (format_buff, spec, value);
  return (format_buff);
}

static char *
format_double (char *spec, double value)
{
  sprintf (format_buff, spec, value);
  return (format_buff);
}

static char *
format_character (char *spec, int character)
{
  format_buff[0] = character;
  format_buff[1] = '\0';
  return (format_buff);
}

static char *
format_string (char *spec, char *string)
{
  parse_field_specifiers (spec);
  if (!string)
    string = NULL_STRING_VALUE;

  if (min_field_width)
    {
      if ((min_field_width + strlen (string)) < FORMAT_BUFF_SIZE)
	{
	  sprintf (format_buff, spec, string);
	  return (format_buff);
	}
      else
	{
	  static char *fb = (char *)NULL;

	  if (fb) free (fb);
	  fb = (char *)xmalloc (20 + min_field_width + strlen (string));
	  sprintf (fb, spec, string);
	  return (fb);
	}
    }
  return (string);
}

/* The main function in this library.  Print to BUFFER with FORMAT and
   any additional args. */
void
bprintf (BPRINTF_BUFFER *buffer, char *format, ...)
{
  va_list args;

  va_start (args, format);
  vbprintf (buffer, format, args);
  va_end (args);
}

void
vbprintf (BPRINTF_BUFFER *buffer, char *format, va_list args)
{
  register int i, c;
  char *accum = (char *)NULL;
  int accum_index = 0;
  int accum_size = 0;
  int recent_errno = errno;

  for (i = 0; (c = format[i]) != '\0'; i++)
    {
      if ((c != '%') || (format[i + 1] == '\0'))
	{
	  if ((accum_index + 3) > accum_size)
	    accum = (char *)xrealloc (accum, (accum_size += 100));

	  accum[accum_index++] = c;
	  accum[accum_index] = '\0';
	}
      else
	{
	  int start = i;
	  int long_format = 0;
	  int short_format = 0;
	  char *result = (char *)NULL;
	  char *spec;

	  if (accum_index)
	    {
	      add_string_to_buffer (accum, buffer);
	      accum_index = 0;
	      accum[0] = '\0';
	    }

	  while ((c = format[++i]) != '\0')
	    {
	      if (c == 'l')
		long_format++;
	      else if (c == 'h')
		short_format++;
	      else if (strchr (printf_arg_specifiers, c) != (char *)NULL)
		break;
	    }

	  spec = (char *)xmalloc (2 + (i - start));
	  memcpy (spec, format + start, ((i - start) + 1));
	  spec[(i - start) + 1] = '\0';

	  /* Handle the specific formatter. */
	  switch (c)
	    {
	    case 'd':
	    case 'i':
	    case 'o':
	    case 'u':
	    case 'x':
	    case 'X':
	      {
		if (long_format)
		  {
		    long value;

		    value = va_arg (args, long);
		    result = format_long (spec, value);
		  }
		else if (short_format)
		  {
		    short value;

		    value = (short) va_arg (args, int);
		    result = format_short (spec, value);
		  }
		else
		  {
		    int value;

		    value = va_arg (args, int);
		    result = format_int (spec, value);
		  }
	      }
	      break;

	    case 'O':
	    case 'U':
	    case 'p':
	      {
		unsigned long value;

		value = va_arg (args, long);
		result = format_long (spec, value);
	      }
	      break;

	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
	    case 'G':
	      {
		double value;

		value = va_arg (args, double);

		result = format_double (spec, value);
	      }
	      break;

	    case 'c':
	      {
		int value;

		value = va_arg (args, int);
		result = format_character (spec, value);
	      }
	      break;

	    case 's':
	      {
		char *value;

		value = va_arg (args, char *);
		result = format_string (spec, value);
	      }
	      break;

	    case '%':
	      result = "%";
	      break;

	      /* The number of characters written so far is stored into the
		 integer indicated by the ``int *'' (or variant) pointer
		 argument.  No argument is converted. */
	    case 'n':
	      {
		int *value;

		value = va_arg (args, int *);
		*value = buffer->bindex;
	      }
	      break;

	    case 'm':
	      result = (char *)strerror (recent_errno);
	      break;

	    default:
	      {
		static char bad_idea[3];

		bad_idea[0] = '%';
		bad_idea[1] = c;
		bad_idea[2] = '\0';
	      
		result = bad_idea;
	      }
	    }

	  if (result)
	    add_string_to_buffer (result, buffer);

	  free (spec);
	}
    }
  /* If there are any accumulated characters left over, add them to the
     buffer as well. */
  if (accum_index)
    add_string_to_buffer (accum, buffer);

  if (accum != (char *)NULL)
    free (accum);
}

/* A few helpful functions for manipulating the insides of bprintf buffers. */

/* Delete the characters from START to END from buffer.  The END'th
   character is not deleted, but the START'th character is.
   Return the number of characters deleted. */
int
bprintf_delete_range (BPRINTF_BUFFER *buffer, int start, int end)
{
  /* Values passed as -1 indicate the terminating end of the buffer. */
  if (end < 0)   end = buffer->bindex;
  if (start < 0) start = 0;

  /* Deleting backwards? */
  if (end < start)
    {
      int temp = start;
      start = end;
      end = temp;
    }

  /* Deleting past the end of the buffer? */
  if (end > buffer->bindex) end = buffer->bindex;

  /* If nothing in the buffer to delete, return. */
  if ((buffer->bindex == 0) || (end == start))
    return (0);

  /* Move the characters from END to the end of the buffer to START. */
  if (end != buffer->bindex)
    memmove
      (buffer->buffer + start, buffer->buffer + end, buffer->bindex - end);
  buffer->bindex -= (end - start);
  buffer->buffer[buffer->bindex] = '\0';
  return (end - start);
}

/* Insert into BUFFER at POINT the string TEXT.
   Return the number of characters inserted, or -1 if there was an error. */
int
bprintf_insert_text (BPRINTF_BUFFER *buffer, int point, char *text)
{
  int text_len;

  if (point > buffer->bindex)
    return (-1);

  text_len = text ? strlen (text) : 0;

  if (text_len == 0)
    return (0);

  /* Make sure there is enough space to insert TEXT. */
  resize_buffer (buffer, 1 + text_len);

  /* Move the text in buffer from POINT to BINDEX to POINT + TEXT_LEN. */
  memmove (buffer->buffer + point + text_len, buffer->buffer + point,
	   (buffer->bindex + 1) - point);

  /* Put TEXT right at point. */
  memmove (buffer->buffer + point, text, text_len);
  buffer->bindex += text_len;

  return (text_len);
}

/* Insert into BUFFER at POINT the results of printing FORMAT with args.
   Return the number of characters inserted, or -1 if there was an error. */
int
bprintf_insert (BPRINTF_BUFFER *buffer, int point, char *format, ...)
{
  va_list args;
  BPRINTF_BUFFER *temp_buffer;
  int result;

  temp_buffer = bprintf_create_buffer ();
  va_start (args, format);
  vbprintf (temp_buffer, format, args);
  va_end (args);
  result = bprintf_insert_text (buffer, point, temp_buffer->buffer);
  bprintf_free_buffer (temp_buffer);
  return (result);
}

/* Insert info BUFFER at POINT LEN bytes of DATA. */
void
bprintf_insert_binary (BPRINTF_BUFFER *buffer, int point, char *data, int len)
{
  if ((len + buffer->bindex) >= buffer->bsize)
    buffer->buffer = (char *)xrealloc
      (buffer->buffer, (buffer->bsize += (len + 100)));

  memmove (buffer->buffer + point + len, buffer->buffer + point,
	   (buffer->bindex + 1) - point);

  memcpy (buffer->buffer + point, data, len);
  buffer->bindex += len;
}

/* Append to BUFFER LEN bytes of DATA. */
void
bprintf_append_binary (BPRINTF_BUFFER *buffer, char *data, int len)
{
  if ((len + buffer->bindex) >= buffer->bsize)
    buffer->buffer = (char *)xrealloc
      (buffer->buffer, (buffer->bsize += (len + 100)));

  memcpy (buffer->buffer + buffer->bindex, data, len);
  buffer->bindex += len;
}

/* Word wrap the text in BUFFER making lines no longer than WIDTH. */
void
bprintf_word_wrap (BPRINTF_BUFFER *buffer, int width)
{
  register int i;
  int col = 0;
  int last_space = 0;

  /* Make sure that the last word is wrapped. */
  bprintf (buffer, " ");

  for (i = 0; i < buffer->bindex; i++)
    {
      col++;

      if (buffer->buffer[i] == '\n')
	{
	  if (((i + 1) < buffer->bindex) &&
	      (buffer->buffer[i + 1] == '\n'))
	    {
	      col = 0;
	      i++;
	      continue;
	    }
	  else
	    buffer->buffer[i] = ' ';
	}

      if (buffer->buffer[i] == ' ')
	{
	  if (col >= width)
	    {
	      int start;

	      if (last_space) i = last_space;
	      buffer->buffer[i++] = '\n';
	      start = i;
	      while (whitespace (buffer->buffer[i]) && i < buffer->bindex) i++;
	      if (start != i)
		{
		  bprintf_delete_range (buffer, start, i);
		  i = start - 1;
		}
	      last_space = 0;
	      col = 0;
	    }
	  else
	    last_space = i;
	}
    }
}
