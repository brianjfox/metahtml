/* cryptfuncs.c: -*- C -*-  Produce encrypted/decrypted versions of data. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Mon Aug  5 11:30:41 1996.  */

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

#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

#if !defined (HEADER_DES_LOCL_H)
#define HEADER_DES_LOCL_H 1
#endif
#include <des.h>

#define DES_VERSION 4
#define DES_TRUE_VERSION 4.04b

static void pf_encrypt (PFunArgs);
static void pf_decrypt (PFunArgs);

static PFunDesc func_table[] =
{
  { "ENCRYPT",		0, 0, pf_encrypt },
  { "DECRYPT",		0, 0, pf_decrypt },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

DOC_SECTION (VARIABLES)
PACKAGE_INITIALIZER (initialize_crypt_functions)

#if !defined (DES_ENCRYPT)
#  define DES_ENCRYPT 1
#  define DES_DECRYPT 0
#endif

unsigned char *
triple_des (unsigned char *srcbuff, char *key, int *len, int encrypt_p)
{
  register int i;
  int output_len = (((*len) + 7) / 8) * 8;
  unsigned char *result = (unsigned char *)xmalloc (1 + output_len);
  unsigned char *buffer = (unsigned char *)xmalloc (1 + output_len);
  int interim_result = 0;
  des_cblock deskey1, deskey2;
  des_key_schedule sched1, sched2;

  memset (result, 0, output_len);
  memcpy (buffer, srcbuff, *len);
  memset (buffer + *len, 0, output_len - *len);
#if (DES_VERSION < 2)
  interim_result =
#endif
    des_string_to_2keys (key, &deskey1, &deskey2);

  interim_result = des_key_sched (&deskey1, sched1);
  interim_result = des_key_sched (&deskey2, sched2);

  for (i = 0; i < *len; i += 8)
#if (DES_VERSION < 2)
    interim_result =
      des_3ecb_encrypt ((des_cblock *)(buffer + i),
			(des_cblock *)(result + i),
			sched1, sched2, encrypt_p);
#else
      des_ecb3_encrypt ((des_cblock *)(buffer + i),
			(des_cblock *)(result + i),
			sched1, sched2, sched1, encrypt_p);
#endif

  /* Handle last left-over byte. */
  if (i < output_len)
    {
      register int j;
      unsigned char input[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

      for (j = 0; j < output_len - i; j++)
	input[j] = buffer[i + j];

#if (DES_VERSION < 2)
      interim_result =
	des_3ecb_encrypt ((des_cblock *)input, (des_cblock *)(result + i),
			  sched1, sched2, encrypt_p);
#else
      des_ecb3_encrypt ((des_cblock *)input, (des_cblock *)(result + i),
			sched1, sched2, sched1, encrypt_p);
#endif
    }

  *len = output_len;
  free (buffer);
  return (result);
}

static char *
encode_data (unsigned char *data, int length)
{
  register int i;
  char *result;

  result = (char *)xmalloc (1 + (2 * length));
  for (i = 0; i < length; i++)
    sprintf (result + (2 * i), "%02x", data[i]);

  result[2 * i] = '\0';

  return (result);
}

static unsigned char *
decode_data (char *data)
{
  register int i;
  unsigned char *result;

  result = (unsigned char *)xmalloc (1 + (strlen (data) / 2));

  for (i = 0; data[i] != '\0'; i+= 2)
    {
      unsigned char value = 0;

      if (isdigit (data[i]))
	value = data[i] - '0';
      else
	value = data[i] - 'a' + 10;

      value *= 16;

      if (isdigit (data[i + 1]))
	value += data[i + 1] - '0';
      else
	value += data[i + 1] - 'a' + 10;

      result[i / 2] = value;
    }

  result[i / 2] = '\0';
  return (result);
}

DEFUN (pf_encrypt, varname key &key algorithm=[3des],
"Encrypts the contents of <var varname> using <var algorithm>\n\
(defaults to 3des).\n\
\n\
The contents of <var varname> are replaced with an encrypted version.\n\
<var key> is the cleartext key to use for encrypting the data.\n\
\n\
Example:\n\
<example>\n\
<set-var foo=\"Hello\">\n\
<encrypt foo \"secret password\">\n\
<get-var foo> --> 3b743366228f5f37\n\
</example>")
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *key = mhtml_evaluate_string (get_positional_arg (vars, 1));
  Symbol *sym = symbol_lookup (name);

  if ((sym != (Symbol *)NULL) && (!empty_string_p (key)) &&
      (sym->values != (char **)NULL) &&
      (((sym->type == symtype_BINARY) &&
	(((Datablock *)(sym->values))->length != 0)) ||
       ((sym->type == symtype_STRING) &&
	(sym->values_index != 0))))
    {
      unsigned char *data = (unsigned char *)NULL;
      char *final = (char *)NULL;
      int length = 0;

      if (sym->type == symtype_BINARY)
	{
	  Datablock *block = (Datablock *)sym->values;
	  data = (unsigned char *)block->data;
	  length = block->length;
	}
      else
	{
	  data = (unsigned char *)sym->values[0];
	  length = 1 + strlen ((char *)data);
	}

      /* Do the encryption. */
      {
	int result_length = length;
	unsigned char *result = 
	  triple_des (data, key, &result_length, DES_ENCRYPT);

	/* Now make the data be all ASCII. */
	final = encode_data (result, result_length);
	free (result);
      }

      /* Put the result back in the variable. */
      if (sym->type == symtype_BINARY)
	{
	  Datablock *block = (Datablock *)xmalloc (sizeof (Datablock));
	  block->length = 1 + strlen (final);
	  block->data = final;
	  datablock_free ((Datablock *)sym->values);
	  sym->values = (char **)block;
	}
      else
	{
	  free (sym->values[0]);
	  sym->values[0] = final;
	}
    }
}

DEFUN (pf_decrypt, varname key &key algorithm=[3des],
"Decrypts the contents of <var varname> using <var algorithm>\n\
(defaults to 3des).\n\
\n\
The contents of <var varname> are replaced with a decrypted version.\n\
<var key> is the cleartext key to use for decrypting the data.\n\
\n\
Example:\n\
<example>\n\
<set-var foo=\"Hello\">\n\
<encrypt foo \"secret password\">\n\
<get-var foo> --> 3b743366228f5f37\n\
<decrypt foo \"secret password\">\n\
<get-var foo> --> Hello\n\
</example>")
{
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *key = mhtml_evaluate_string (get_positional_arg (vars, 1));
  Symbol *sym = symbol_lookup (name);

  if ((sym != (Symbol *)NULL) && (!empty_string_p (key)) &&
      (sym->values != (char **)NULL) &&
      (((sym->type == symtype_BINARY) &&
	(((Datablock *)(sym->values))->length != 0)) ||
       ((sym->type == symtype_STRING) &&
	(sym->values_index != 0))))
    {
      unsigned char *data = (unsigned char *)NULL;
      unsigned char *final = (unsigned char *)NULL;
      unsigned char *result = (unsigned char *)NULL;
      int length = 0;

      if (sym->type == symtype_BINARY)
	{
	  Datablock *block = (Datablock *)sym->values;
	  data = (unsigned char *)block->data;
	  length = block->length / 2;
	}
      else
	{
	  data = (unsigned char *)sym->values[0];
	  length = strlen ((char *)data) / 2;
	}

      /* Decode the hex bits. */
      result = decode_data ((char *)data);

      /* Do the decryption. */
      final = triple_des (result, key, &length, DES_DECRYPT);
      free (result);

      /* Put the result back in the variable. */
      if (sym->type == symtype_BINARY)
	{
	  Datablock *block = (Datablock *)xmalloc (sizeof (Datablock));
	  block->length = length;
	  block->data = (char *)final;
	  datablock_free ((Datablock *)sym->values);
	  sym->values = (char **)block;
	}
      else
	{
	  free (sym->values[0]);
	  sym->values[0] = (char *)final;
	}
    }
}

#if defined (__cplusplus)
}
#endif
