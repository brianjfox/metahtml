/* modsession.c: -*- C -*-  Session DB TCP/IP based server functions.. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sun Aug 15 13:51:05 1999.

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

#include "modules.h"
#include "sessiondb.h"
#include "tcp_session.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_read_dbobj (PFunArgs);
static void pf_write_dbobj (PFunArgs);
static void pf_read_command (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "READ-COMMAND",	0,	0,	pf_read_command },
  { "READ-DBOBJ",	0,	0,	pf_read_dbobj },
  { "WRITE-DBOBJ",	0,	0,	pf_write_dbobj },
  { (char *)NULL,	0,	0,	(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("modsession", ftab)

DEFINE_SECTION (SESSION-SERVER-MODULE, session;server;TCP,
"Helper functions for the TCP/IP based session server.", "")

DEFUN (pf_read_command,, "Read a command from the incoming connection.")
{
  int fd = mhtml_stdin_fileno;
  int command = -1;
  char *result = "UNKNOWN-COMMAND!";

  read (fd, &command, sizeof (command));

  switch (command)
    {
    case _tcpdb_OPEN: result = "OPEN"; break;
    case _tcpdb_STORE: result = "STORE"; break;
    case _tcpdb_FETCH: result = "FETCH"; break;
    case _tcpdb_DELETE: result = "DELETE"; break;
    case _tcpdb_CLOSE: result = "CLOSE"; break;
    case _tcpdb_FIRSTKEY: result = "FIRSTKEY"; break;
    case _tcpdb_NEXTKEY: result = "NEXTKEY"; break;
    default:
      {
	static char check[16];
	/* Text command? */
	memcpy (&check[0], &command, sizeof (command));
	check[sizeof (command)] = '\0';
	result = check;
      }
    }

  bprintf_insert (page, start, "%s", result);
  *newstart += strlen (result);
}

DEFUN (pf_read_dbobj, &optional objectvar,
"Read an object from the incoming port, and optionally store it in OBJECTVAR.
If OBJECTVAR is specified, this function returns the length of the read
object.  If OBJECTVAR is not specified, this function returns the object.")
{
  int fd = mhtml_stdin_fileno;
  int length = 0;
  char *buffer = (char *)NULL;
  char *symname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;
  char digits[20];

  read (fd, &length, sizeof (length));

  if (length > 0)
    {
      int bytes_read = 0;
      buffer = (char *)xmalloc (1 + length);
      buffer[length] = '\0';

      while (bytes_read < length)
	{
	  int amount = read (fd, buffer + bytes_read, length - bytes_read);
	  if (amount < 0)
	    break;
	  else
	    bytes_read += amount;
	}

      if (empty_string_p (symname))
	{
	  result = buffer;
	}
      else
	{
	  Symbol *sym = symbol_remove (symname);
	  Datablock *block;

	  symbol_free (sym);
	  sym = symbol_intern (symname);
	  block = datablock_create (buffer, length);
	  sym->type = symtype_BINARY;
	  sym->values = (char **)block;
	  result = digits;
	  sprintf (digits, "%d", length);
	}
    }

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }

  xfree (buffer);
  xfree (symname);
}

DEFUN (pf_write_dbobj, objectvar,
"Write the data in OBJECTVAR to the outgoing port.")
{
  char *symname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;
  char digits[20];

  if (!empty_string_p (symname))
    {
      int fd = mhtml_stdout_fileno;
      char *data = (char *)NULL;
      int length = 0;
      Symbol *sym = symbol_lookup (symname);

      if (sym != (Symbol *)NULL)
	{
	  switch (sym->type)
	    {
	    case symtype_STRING:
	      if (sym->values_index)
		{
		  data = sym->values[0];
		  length = data ? strlen (data) : 0;
		}
	      break;

	    case symtype_FUNCTION:
	      break;

	    case symtype_BINARY:
	      data = ((Datablock *)sym->values)->data;
	      length = ((Datablock *)sym->values)->length;
	      break;
	    }
	}

      write (fd, &length, sizeof (length));
      write (fd, data, length);
      sprintf (digits, "%d", length);
      result = digits;
    }

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }

  xfree (symname);
}

#if defined (__cplusplus)
}
#endif
