/* packdump.c: -*- C -*-  How to dump a package in binary format. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jul 20 23:52:57 1996.  */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include "symbols.h"
#include "pages.h"
#include "parser.h"

/* The Mac only defines "macintosh" in sys/types.h.  Sigh... */
#include <sys/types.h>

#if defined (macintosh)
#  include <mac_port.h>
#endif

/* Functions for reading from and writing to a string. */
#define STRING_FD_THRESHOLD 1024
#define FD_IS_STRING_BUFFER(fd) ((fd) >= STRING_FD_THRESHOLD)
#define SD_TO_BUFFER_INDEX(sd) ((sd) - STRING_FD_THRESHOLD)
#define BUFFER_INDEX_TO_SD(bi) ((bi) + STRING_FD_THRESHOLD)

#if defined (__cplusplus)
extern "C"
{
#endif

static BPRINTF_BUFFER **open_strings = (BPRINTF_BUFFER **)NULL;
static int open_strings_slots = 0;
static int open_strings_index = 0;

static BPRINTF_BUFFER *
buffer_of_sd (int sd)
{
  BPRINTF_BUFFER *result = (BPRINTF_BUFFER *)NULL;

  sd = SD_TO_BUFFER_INDEX (sd);

  if ((sd >= 0) && (sd < open_strings_index))
    result = open_strings[sd];

  return (result);
}

/* Read LEN bytes from SD into DATA. */
int
symdump_read_string_data (int sd, int len, unsigned char *data)
{
  BPRINTF_BUFFER *string_buffer = buffer_of_sd (sd);
  int result = 0;

  if (string_buffer != (BPRINTF_BUFFER *)NULL)
    {
      if (string_buffer->bindex + len <= string_buffer->bsize)
	{
	  register int i;

	  for (i = 0; i < len; i++)
	    data[i] = (unsigned char)
	      (string_buffer->buffer[string_buffer->bindex++]);
	  result = i;
	}
    }
  return (result);
}

/* Write LEN bytes to SD from DATA. */
int
symdump_write_string_data (int sd, int len, unsigned char *data)
{
  BPRINTF_BUFFER *string_buffer = buffer_of_sd (sd);
  int result = 0;

  if (string_buffer != (BPRINTF_BUFFER *)NULL)
    {
      register int i;

      if (len + string_buffer->bindex >= string_buffer->bsize)
	string_buffer->buffer = (char *)xrealloc
	  (string_buffer->buffer, (string_buffer->bsize += (len + 100)));

      for (i = 0; i < len; i++)
	((unsigned char *) (string_buffer->buffer))[string_buffer->bindex++] =
	  data[i];

      result = i;
    }

  return (result);
}

/* Set the current offset index in SD to OFFSET. */
void
symdump_seek_string_data (int sd, int offset)
{
  BPRINTF_BUFFER *string_buffer = buffer_of_sd (sd);

  if ((string_buffer != (BPRINTF_BUFFER *)NULL) &&
      ((offset >= 0) && (offset <= string_buffer->bsize)))
    string_buffer->bindex = offset;
}

/* Set the end of SD to be END. */
void
symdump_set_string_data_buffer_size (int sd, int end)
{
  BPRINTF_BUFFER *string_buffer = buffer_of_sd (sd);

  if ((string_buffer != (BPRINTF_BUFFER *)NULL) &&
      ((end >= 0) && (end <= string_buffer->bsize)))
    string_buffer->bsize = end;
}

/* Return an SD (like an FD, but for the string buffer pool).
   Newly allocates a buffer.
   The SD returned is suitable for passing to symbol_load_package ()
   and symbol_dump_package (). */
int
symdump_open_string_data (void)
{
  register int i;
  int sd = -1;

  if (open_strings_index + 2 > open_strings_slots)
    {
      open_strings = (BPRINTF_BUFFER **) xrealloc
	(open_strings, (open_strings_slots += 4) * sizeof (BPRINTF_BUFFER *));
      for (i = open_strings_index; i < open_strings_slots; i++)
	open_strings[i] = (BPRINTF_BUFFER *)NULL;
    }

  /* Find first unused slot. */
  for (i = 0; i < open_strings_slots; i++)
    {
      if (open_strings[i] == (BPRINTF_BUFFER *)NULL)
	{
	  sd = i + STRING_FD_THRESHOLD;
	  open_strings[i] = bprintf_create_buffer ();
	  if (i == open_strings_index)
	    open_strings_index++;
	  break;
	}
    }

  return (sd);
}

/* Return the BPRINTF_BUFFER corresponding to SD, and return the allocated
   slot back to the pool.  Does NOT free the BPRINTF_BUFFER. */
BPRINTF_BUFFER *
symdump_close_string_data (int sd)
{
  BPRINTF_BUFFER *string_buffer = buffer_of_sd (sd);

  if (string_buffer != (BPRINTF_BUFFER *)NULL)
    {
      /* Reset the end of the buffer. */
      string_buffer->bsize = string_buffer->bindex;

      /* Allow slot to be reused. */
      open_strings[SD_TO_BUFFER_INDEX (sd)] = (BPRINTF_BUFFER *)NULL;
    }
  return (string_buffer);
}

static int
write_data (int fd, int len, unsigned char *data)
{
  if (FD_IS_STRING_BUFFER (fd))
    return (symdump_write_string_data (fd, len, data));
  else
    return (write (fd, data, len));
}

static int
read_data (int fd, int len, unsigned char *data)
{
  if (FD_IS_STRING_BUFFER (fd))
    return (symdump_read_string_data (fd, len, data));
  else
    return (read (fd, data, len));
}

static int
write_integer (int fd, int integer)
{
  unsigned char byte;
  int amount = 0;

  byte = (unsigned char) ((integer & 0x000000ff) >> 0);
  amount += write_data (fd, 1, &byte);

  byte = (unsigned char) ((integer & 0x0000ff00) >> 8);
  amount += write_data (fd, 1, &byte);

  byte = (unsigned char) ((integer & 0x00ff0000) >> 16);
  amount += write_data (fd, 1, &byte);

  byte = (unsigned char) ((integer & 0xff000000) >> 24);
  amount += write_data (fd, 1, &byte);

  return (amount);
}

static int
read_integer (int fd)
{
  unsigned char b1, b2, b3, b4;
  int safety = 0;
  int integer = 0;

  safety += read_data (fd, 1, &b1);
  safety += read_data (fd, 1, &b2);
  safety += read_data (fd, 1, &b3);
  safety += read_data (fd, 1, &b4);

  if (safety != 4)
    integer = 0;
  else
    integer =
      (((int)b4) << 24) | (((int)b3) << 16) | (((int)b2) << 8) | ((int)b1);

  return (integer);
}

static void
write_symbol (Symbol *symbol, int fd, int no_source)
{
  register int i;
  unsigned char byte;

  if (symbol == (Symbol *)NULL) return;

  /* Start with the name. */
  byte = (unsigned char)symbol->name_len;
  write_data (fd, 1, &byte);
  write_data (fd, byte, (unsigned char *)symbol->name);

  /* Now the type. */
  byte = (unsigned char)symbol->type;
  write_data (fd, 1, &byte);

  /* Now any flags which this symbol may have. */
  {
    int flags = symbol->flags &= ~sym_FLAGGED;

#if defined (METAHTML_COMPILER)
    if ((symbol->machine != (void *)NULL) &&
	(symbol_machget_hook != (SYMBOL_MACHGET_FUNCTION *)NULL))
      flags |= sym_MACH_RES;
    else
#endif
      flags &= ~sym_MACH_RES;

    write_integer (fd, flags);
  }

  /* Different methods for different symbol types. */
  switch (symbol->type)
    {
    case symtype_STRING:
      /* Write the number of values that this symbol has. */
      write_integer (fd, symbol->values_index);

      /* Write the values themselves. */
      for (i = 0; i < symbol->values_index; i++)
	{
	  int len = symbol->values[i] ? strlen (symbol->values[i]) : 0;
	  char *data_pointer = symbol->values[i];

	  if (!len)
	    {
	      len = 1;
	      data_pointer = "";
	    }

	  write_integer (fd, len);
	  write_data (fd, len, (unsigned char *)symbol->values[i]);
	}
      break;

    case symtype_FUNCTION:
      /* We write no data for functions at this time. */
      break;

    case symtype_BINARY:
      /* Write out the datablock. */
      {
	Datablock *block = (Datablock *)symbol->values;
	write_integer (fd, block->length);
	write_data (fd, block->length, (unsigned char *)block->data);
      }
      break;

    case symtype_USERFUN:
      /* Write out the user function. */
      {
	UserFunction *fun = (UserFunction *)symbol->values;
	char *body_text = fun->body;
	int len;

	/* Write the type of function. */
	byte = (unsigned char)fun->type;
	write_data (fd, 1, &byte);

	/* Write the function's flags. */
	byte = (unsigned char)fun->flags;
	write_data (fd, 1, &byte);

	/* We don't have to write the name of this function.
	   It is the same as the name of the symbol that we already
	   wrote. */

	/* Write the name of the package to wrap around this function. */
	byte = (unsigned char) (fun->packname ? strlen (fun->packname) : 0);
	write_data (fd, 1, &byte);
	if (fun->packname)
	  write_data (fd, (int)byte, (unsigned char *)fun->packname);

	/* Write the number of named parameters that this function takes. */
	for (i = 0; fun->named_parameters && fun->named_parameters[i]; i++);
	byte = (unsigned char )i;
	write_data (fd, 1, &byte);

	/* If there were any, write them now. */
	for (i = 0; i < byte; i++)
	  {
	    unsigned char l = strlen (fun->named_parameters[i]);
	    write_data (fd, 1, &l);
	    write_data (fd, (int)l, (unsigned char *)fun->named_parameters[i]);
	  }

#if defined (METAHTML_COMPILER)
	/* If this function has a machine slot, and the special flag which
	   says not to save the textual function body along with the compiled
	   function is on, write a zero length body. */
	if ((no_source != 0) &&
	    (symbol->machine != (void *)NULL) &&
	    (symbol_machget_hook != (SYMBOL_MACHGET_FUNCTION *)NULL))
	  body_text = "";
#endif
	/* Write the function body. */
	len = body_text ? strlen (body_text) : 0;
	write_integer (fd, len);
	write_data (fd, len, (unsigned char *)body_text);
      }
      break;

    default:
      abort ();
    }

#if defined (METAHTML_COMPILER)
  if ((symbol->machine != (void *)NULL) &&
      (symbol_machget_hook != (SYMBOL_MACHGET_FUNCTION *)NULL))
    {
      Datablock *block = (*symbol_machget_hook) (symbol);

      write_integer (fd, block->length);
      write_data (fd, block->length, (unsigned char *)block->data);

      datablock_free (block);
    }
#endif
}

static Symbol *
read_symbol (int fd, Package *package)
{
  register int i;
  static char *buffer = (char *)NULL;
  static int buflen = 0;
  unsigned char byte;
  Symbol *sym = (Symbol *)NULL;
  int previously_defined_p = 0;

  /* Read the length of the symbol name. */
  read_data (fd, 1, &byte);

  if (byte + 1 > buflen)
    buffer = (char *)xrealloc (buffer, (buflen += (byte + 128)));

  /* Read the symbol name. */
  read_data (fd, (int)byte, (unsigned char *)buffer);
  buffer[(int)byte] = '\0';

  /* Trash the symbol if it already exists. */
  sym = symbol_remove_in_package (package, buffer);
  if (sym != (Symbol *)NULL)
    {
      if (package == mhtml_user_keywords)
	previously_defined_p = 1;
      symbol_free (sym);
    }

  /* Make the symbol. */
  sym = symbol_intern_in_package (package, buffer);

  /* Get its type. */
  read_data (fd, 1, &byte);
  sym->type = (int)byte;

  /* And the flags. */
  sym->flags = read_integer (fd);

  /* Do different things for each symbol type. */
  switch (sym->type)
    {
    case symtype_STRING:
      /* Find out how many values are stored here. */
      sym->values_index = read_integer (fd);

      /* Allocate space for that many. */
      sym->values_slots = sym->values_index + 1;
      sym->values = (char **)xmalloc ((sym->values_slots * sizeof (char *)));

      /* Fill in the slots from the data in the file. */
      for (i = 0; i < sym->values_index; i++)
	{
	  int temp = read_integer (fd);
	  sym->values[i] = (char *)xmalloc (1 + temp);
	  read_data (fd, temp, (unsigned char *)sym->values[i]);
	  sym->values[i][temp] = '\0';
	}
      sym->values[i] = (char *)NULL;
      break;

    case symtype_FUNCTION:
      /* We read no data for C code primitives at this time. */
      break;

    case symtype_BINARY:
      /* Read in and create a datablock value. */
      {
	Datablock *block = (Datablock *)xmalloc (sizeof (Datablock));
	block->length = read_integer (fd);
	block->data = (char *)xmalloc (block->length);
	read_data (fd, block->length, (unsigned char *)block->data);
      }
      break;

    case symtype_USERFUN:
      /* Read in the user function definition. */
      {
	UserFunction *fun = (UserFunction *)xmalloc (sizeof (UserFunction));

	memset (fun, 0, sizeof (UserFunction));
	/* Get the function type. */
	read_data (fd, 1, &byte);
	fun->type = (int)byte;

	/* Get the flags. */
	read_data (fd, 1, &byte);
	fun->flags = (int)byte;
	fun->debug_level = 0;

	/* Put the function's name in place.  It is the same as the name of
	   this symbol. */
	fun->name = strdup (sym->name);

	/* Get the name of the surrounding package. */
	read_data (fd, 1, &byte);

	if (byte != 0)
	  {
	    fun->packname = (char *)xmalloc (1 + (int) byte);
	    read_data (fd, (int)byte, (unsigned char *)fun->packname);
	    fun->packname[(int)byte] = '\0';
	  }
	else
	  fun->packname = 0;

	/* Get named parameters list. */
	read_data (fd, 1, &byte);

	if (byte != 0)
	  {
	    unsigned char l;

	    fun->named_parameters = 
	      (char **)xmalloc ((1 + byte) * sizeof (char *));

	    for (i = 0; i < byte; i++)
	      {
		read_data (fd, 1, &l);
		fun->named_parameters[i] = (char *)xmalloc (1 + l);
		read_data (fd, (int)l, (unsigned char *)fun->named_parameters[i]);
		fun->named_parameters[i][(int)l] = '\0';
	      }
	    fun->named_parameters[i] = (char *)NULL;
	  }
	else
	  fun->named_parameters = (char **)NULL;

	/* Get the function body. */
	i = read_integer (fd);
	fun->body = (char *)xmalloc (1 + i);
	read_data (fd, i, (unsigned char *)fun->body);
	fun->body[i] ='\0';

	/* Make this the value of this symbol. */
	sym->values = (char **)fun;

	if (((mhtml_warn_on_redefine_primitive || mhtml_warn_on_redefine) &&
	     (symbol_lookup_in_package (mhtml_function_package, sym->name)
	      != (Symbol *)NULL)) ||
	    (mhtml_warn_on_redefine && previously_defined_p))
	  {
	    char *definer = "primitive?";

	    switch (fun->type)
	      {
	      case user_DEFUN: definer = "defun"; break;
	      case user_SUBST: definer = "defsubst"; break;
	      case user_MACRO: definer = "defmacro"; break;
	      }

	    page_syserr ("WARNING: <%s %s...> in library redefines a function",
			 definer, sym->name);
	  }
      }
      break;

    default:
      abort ();
    }

#if defined (METAHTML_COMPILER)
  /* If this symbol has data associated with the machine slot, then
     that data immediately follows the symbol in the input stream.
     Get the data from the stream, and decide what to do with it then. */
  if ((sym != (Symbol *)NULL) && (symbol_get_mach_res (sym)))
    {
      Datablock *block = (Datablock *)xmalloc (sizeof (Datablock));
      block->length = read_integer (fd);
      block->data = (char *)xmalloc (block->length);
      read_data (fd, block->length, (unsigned char *)block->data);

      if (symbol_machfill_hook != (SYMBOL_MACHFILL_FUNCTION *)NULL)
	(*symbol_machfill_hook) (sym, block);
      else
	{
	  symbol_clear_mach_res (sym);

	  if (sym->type != symtype_USERFUN)
	    symbol_set_modified (sym);
	}

      datablock_free (block);
    }
#endif /* METAHTML_COMPILER */

  return (sym);
}

/* Write out PACKAGE to FD such that SYMBOL_LOAD_PACKAGE can read
   it in again.  If FLAGGED is non-zero, only write out symbols with
   the sym_FLAGGED flag set.  If NO_SOURCE is non-zero, zero out the
   body of the function before writing it.  This might make sense if
   you are saving a file of byte-code, and you don't need the source
   to it for correct operation. */
void
symbol_dump_package (int fd, Package *package, int flagged, int no_source)
{
  register int i, len = 0;
  unsigned char byte;
  Symbol **symbols = package ? symbols_of_package (package) : (Symbol **)NULL;

  if (symbols == (Symbol **)NULL)
    return;

  for (i = 0; symbols[i] != (Symbol *)NULL; i++)
    if (!flagged || (symbols[i]->flags & sym_FLAGGED))
      len++;;

  /* Start by writing the name of this package. */
  byte = (unsigned char) package->name_len;
  write_data (fd, 1, &byte);
  write_data (fd, (int)byte, (unsigned char *)package->name);

  /* Next, write the number of symbols present in this package. */
  write_integer (fd, len);

  /* Now write each symbol, and its data. */
  for (i = 0; symbols[i] != (Symbol *)NULL; i++)
    {
      if (!flagged || (symbols[i]->flags & sym_FLAGGED))
	write_symbol (symbols[i], fd, no_source);
    }

  /* That's all we wrote! */
}

/* Read a package from FD, installing the symbols and values found within. */
Package *
symbol_load_package (int fd)
{
  register int i;
  int num_syms = 0;
  int compiled_p = 0;
  unsigned char byte;
  static char *buffer = (char *)NULL;
  static int buflen = 0;
  Package *package = (Package *)NULL;

  /* Read the length of the package name. */
  if (read_data (fd, 1, &byte) < 1)
    return (package);

  if (byte + 1 > buflen)
    buffer = (char *)xrealloc (buffer, (buflen += (byte + 128)));

  /* Read the package name. */
  read_data (fd, (int)byte, (unsigned char *)buffer);
  buffer[(int)byte] = '\0';

  /* Get the number of symbols stored in this package. */
  num_syms = read_integer (fd);
  compiled_p = (num_syms & 0x0000000001000000);

  /* Get a pointer to the package, perhaps creating it if it didn't exist. */
  {
    int small_prime = 0;

    if (num_syms > 300)
      small_prime = 577;

    if (num_syms > 500)
      small_prime = 797;

    if (num_syms > 900)
      small_prime = 1013;

    package = symbol_get_package_hash (buffer, small_prime);
  }

  /* Read those symbols into this package. */
  for (i = 0; i < num_syms; i++)
    read_symbol (fd, package);

  return (package);
}

#if defined (__cplusplus)
}
#endif
