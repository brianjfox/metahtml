/* session_data.c: -*- C -*-  Manipulation of session data. */

/* Author: Brian J. Fox (bfox@ua.com) Thu Jul  6 13:39:02 1995.  */

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

#include "session_data.h"
#include <wisper/wisp.h>

#if defined (__cplusplus)
extern "C"
{
#endif

/* Given the ASCII representation of an alist in INFO->data,
   store that data in the indicated package. */
void
sd_info_to_package (SESSION_INFO *info, Package *package)
{
  WispObject *list;

  /* The data is stored as the ASCII representation of an alist. */
  list = wisp_from_string ((char *)info->data);

  if (list != (WispObject *)NULL)
    {
      while (list != NIL)
	{
	  WispObject *pair;

	  pair = CAR (list);
	  list = CDR (list);

	  if (CONS_P (pair) && STRING_P (CAR (pair)))
	    {
	      char *tag;

	      tag = strdup (STRING_VALUE (CAR (pair)));

	      if (STRING_P (CDR (pair)))
		{
		  Symbol *sym;

		  sym = symbol_intern_in_package (package, tag);
		  symbol_add_value (sym, STRING_VALUE (CDR (pair)));
		}
	      else
		{
		  WispObject *values = CDR (pair);
		  Symbol *sym = symbol_intern_in_package (package, tag);

		  while (CONS_P (values) && STRING_P (CAR (values)))
		    {
		      symbol_add_value (sym, STRING_VALUE (CAR (values)));
		      values = CDR (values);
		    }
		}
	      free (tag);
	    }
	}
    }
  gc_wisp_objects ();
}

/* Given a package with symbols in it, store an  ASCII representation of
   a Lisp alist containing those symbols and values in INFO. */
void
sd_package_to_info (SESSION_INFO *info, Package *package)
{
  BPRINTF_BUFFER *data;
  Symbol **symbols = symbols_of_package (package);

  data = bprintf_create_buffer ();
  bprintf (data, "(");

  if (symbols != (Symbol **)NULL)
    {
      register int i;
      Symbol *sym;

      for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	{
	  if (sym->type == symtype_STRING)
	    {
	      register int j;
	      char **values = sym->values;
	      char *item_name = strdup (wisp_readable (sym->name));

	      switch (sym->values_index)
		{
		case 0:
		  bprintf (data, "(%s . \"\")", item_name);
		  break;

		case 1:
		  bprintf (data, "(%s . %s)",
			   item_name, wisp_readable (values[0]));
		  break;

		default:
		  bprintf (data, "(%s", item_name);
		  for (j = 0; values[j] != (char *)NULL; j++)
		    bprintf (data, " %s", wisp_readable (values[j]));
		  bprintf (data, ")");
		}
	      free (item_name);
	    }
	}
      free (symbols);
    }

  bprintf (data, ")");

  if (info->data)
    free (info->data);

  info->data = (unsigned char *)data->buffer;
  info->length = data->bindex;

  free (data);
}

#if defined (__cplusplus)
}
#endif
