/* modmd5.c: -*- C -*-  DESCRIPTIVE TEXT. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sun Nov 14 11:06:57 1999.

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

#if defined (__cplusplus)
extern "C"
{
#endif

#include "md5.h"

static void pf_md5digest_var (PFunArgs);
static void pf_md5digest_string (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "MD5::DIGEST-VAR",	0,	0,	pf_md5digest_var },
  { "MD5::DIGEST-STRING", 0,	0,	pf_md5digest_string },
  { (char *)NULL,	0,	0,	(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("modmd5", ftab)

static char *
digest_to_string (unsigned char *digest)
{
  register int i;
  char *result = (char *)NULL;
  BPRINTF_BUFFER *output = bprintf_create_buffer ();
  char bin2hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
		       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

  for (i = 0; i < 16; i++)
    {
      unsigned char c = digest[i];
      char char1 = bin2hex[((c & 0xf0) >> 4)];
      char char2 = bin2hex[((c & 0x0f) >> 0)];

      bprintf (output, "%c%c", char1, char2);
    }

  result = output->buffer;
  free (output);
  return (result);
}

DEFINE_SECTION (MD5-MODULE, message digest; MD5; unique key,
"The functions in this module allow you to create an MD5 digest (i.e., a
16 byte unique key) from the contents of a variable or file.", "")

DEFUNX (md5::digest-var, var,
"Create an MD5 digest from the contents of <var var> and return the 32
character hex representation of it.")

static void
pf_md5digest_var (PFunArgs)
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (varname != (char *)NULL)
    {
      Symbol *sym = symbol_lookup (varname);

      if (sym != (Symbol *)NULL)
	{
	  unsigned char *input = (char *)NULL;
	  size_t input_len = 0;
	  unsigned char digest[17];

	  memset (&digest[0], 0, 16);

	  switch (sym->type)
	    {
	    case symtype_STRING:
	      if (sym->values_index)
		{
		  input = sym->values[0];
		  input_len = strlen (input);
		}
	      break;

		case symtype_BINARY:
		  {
		    Datablock *d = (Datablock *)(sym->values);
		    if (d->length)
		      {
			input = d->data;
			input_len = d->length;
		      }
		  }
		  break;

		default:
		  break;
	    }

	  if (input_len > 0)
	    {
	      md5_buffer ((const char *)input, input_len, (void *)digest);
	      result = digest_to_string (digest);
	    }
	}
    }

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }

  xfree (varname);
}

DEFUNX (md5::digest-string, string,
"Create an MD5 digest from <var string>, and return the 32
character hex representation of it.")

static void
pf_md5digest_string (PFunArgs)
{
  char *input = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (!empty_string_p (input))
    {
      size_t input_len = (size_t) strlen (input);
      unsigned char digest[17];

      memset (&digest[0], 0, 16);

      if (input_len > 0)
	{
	  md5_buffer ((const char *)input, input_len, (void *)digest);
	  result = digest_to_string (digest);
	}
    }

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }

  xfree (input);
}

#if defined (__cplusplus)
}
#endif
