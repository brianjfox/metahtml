/*  forms.c: The code which reads data posted from an HTML form over HTTP. */

/* Author: Brian J. Fox (bfox@ua.com) Sat May 20 11:38:02 1995.  */

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

#if defined (__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#if defined (__unix)
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "forms.h"
#include <xmalloc/xmalloc.h>

#undef whitespace
#define whitespace(c) \
     (((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n'))

static void cleanup (char *string);
static int parse_hex_pair (char *pair_start);

extern char **environ;

static Package *posted_variables = (Package *)NULL;

/* Return the package containing only those items which were found in
   the posted material.  This excludes environment variables.  You
   must have first called forms_input_data () before you can get
   anything useful. */
Package *
forms_posted_data (void)
{
  return (posted_variables);
}

/* Read the input data from all of the available sources.  This means
   the environment variables PATH_INFO and QUERY_STRING, the contents
   of standard input, if there is any, and the arguments passed into
   the CGI program.  Nothing is returned, the symbols and values are
   simply interned. The program arguments are returned in the item
   PROGRAM-ARGUMENTS. */
void
forms_input_data (int argc, char *argv[])
{
  register int i;
  int content_length = 0;
  char *env_item;
  char *data_string = (char *)NULL;
  int data_string_size = 0, data_string_index = 0;
  Symbol *symbol;
  Package *package;

  posted_variables = symbol_get_package ("POSTED");

  /* First, get the program arguments if there are any. */
  if (argc != 1)
    {
      symbol = symbol_intern ("PROGRAM-ARGUMENTS");
      symbol->values = (char **)xmalloc (argc * sizeof (char *));
      symbol->values_slots = argc;
      symbol->values_index = 0;

      for (i = 1; i < argc; i++)
	symbol->values[symbol->values_index++] = strdup (argv[i]);

      symbol->values[symbol->values_index] = (char *)NULL;
    }

  /* Now get all of the environment variables. */
  package = symbol_get_package ("ENV");
  for (i = 0; environ != (char **)NULL && environ[i] != (char *)NULL; i++)
    {
      char *name, *value;

      name = strdup (environ[i]);
      value = strchr (name, '=');

      if (value)
	{
	  *value = '\0';
	  value++;
	  value = strdup (value);
	}

      symbol = symbol_intern_in_package (package, name);

      if (value)
	{
	  symbol->values = (char **)xmalloc
	    ((symbol->values_slots = 2) * sizeof (char *));
	  symbol->values[0] = value;
	  symbol->values[1] = (char *)NULL;
	  symbol->values_index = 1;
	}

      free (name);
    }

  /* If there is any input available from standard input, then read it
     now. */
  if (((env_item = getenv ("CONTENT_LENGTH")) != (char *)NULL) &&
      ((content_length = atoi (env_item)) != 0))
    {
      int offset = 0;

      data_string = (char *)xmalloc (1 + (data_string_size = content_length));

      while (content_length != 0)
	{
	  int amount_read =
	    read (fileno (stdin), data_string + offset, content_length);

	  if (amount_read == -1)
	    abort ();

	  content_length -= amount_read;
	  offset += amount_read;
	}

      data_string[offset] = '\0';
      data_string_index = offset;
    }

  /* If any input is coming from QUERY_STRING or PATH_INFO, get it now. */
  {
    char *names[3] = { "QUERY_STRING", "PATH_INFO", (char *)NULL };

    for (i = 0; names[i]; i++)
      {
	if (((env_item = getenv (names[i])) != (char *)NULL) &&
	    (*env_item != '\0'))
	  {
	    int max_len;

	    if ((strcmp (names[i], "PATH_INFO") == 0) &&
		*env_item == '/')
	      env_item++;

	    max_len = data_string_index + strlen (env_item) + 3;

	    if (max_len > data_string_size)
	      data_string = (char *)xrealloc
		(data_string, (data_string_size = max_len));

	    if (data_string_index != 0)
	      data_string[data_string_index++] = '&';

	    strcpy (data_string + data_string_index, env_item);
	    data_string_index += strlen (env_item);
	    data_string[data_string_index] = '\0';
	  }
      }
  }

  /* DATA_STRING may contain any number of name/value pairs, including
     none.  If there are any, intern them now. */
  if (data_string)
    {
      package = posted_variables;

      forms_parse_data_string (data_string, package);

      /* Copy the parsed symbols into the default package. */
      {
	Package *default_pack = 
	  symbol_get_package_hash (DEFAULT_PACKAGE_NAME, 577);
	Symbol **symbols = symbol_package_symbols ("POSTED");

	if (symbols != (Symbol **)NULL)
	  {
	    for (i = 0; symbols[i] != (Symbol *)NULL; i++)
	      symbol_copy (symbols[i], default_pack);
	  }
      }

      free (data_string);
    }
}

/* Read name/value pairs from BUFFER, and intern the symbols in PACKAGE.
   The pairs are delimited with ampersand (`&') or end of data.  The name
   is separated from the value by an equals sign (`=').  Space characters
   are encoded as plus signs (`+').  A percent sign (`%') is used to
   introduce two hex digits, which when coerced to an ASCII character is
   the result.  This mechanism is used to get plus signs into the name or
   value string, for example. */
void
forms_parse_data_string (const char *input, Package *package)
{
  register int i, start, length;
  char *buffer = strdup (input);
  Symbol *symbol;

  /* Strip out the name/value pairs, and add them to the package. */
  length = strlen (buffer);
  start = 0;

  while (start < length)
    {
      int equal = -1;
      char *name, *value;

      for (i = start; buffer[i] != '\0' && buffer[i] != '&'; i++)
	if (buffer[i] == '=')
	  equal =  i;

      /* If there was no equal sign found, we are done parsing the pairs. */
      if (equal < 0)
	break;

      /* `EQUAL' is the offset of the name/value separator.
	 `I' is the offset of the delimiter between pairs. */
      buffer[equal] = '\0';
      buffer[i] = '\0';

      name = strdup (buffer + start);
      value = strdup (buffer + equal + 1);

      cleanup (name);
      cleanup (value);

      /* Get the symbol. */
      symbol = symbol_intern_in_package (package, name);

      /* Add this value. */
      symbol_add_value (symbol, value);
      start = i + 1;
    }

  free (buffer);
}

static char hexchars[]="0123456789ABCDEF";

#define hi_nibble(c) hexchars[(((c & 0x000000FF) & 0xF0) >> 4)]
#define lo_nibble(c) hexchars[(((c & 0x000000FF) & 0X0F) >> 0)]

/* Turn SYMBOLS into a string suitable for appending onto a URL.
   This means that we encode special characters, and write name
   value pairs into a new string.
   A newly allocated string is returned. */
char *
forms_unparse_items (Symbol **symbols)
{
  register int i;
  char *result = (char *)NULL;
  int result_index = 0;
  int result_size = 0;
  Symbol *symbol;

  if (symbols == (Symbol **)NULL)
    return ((char *)NULL);

  for (i = 0; (symbol = symbols[i]) != (Symbol *)NULL; i++)
    {
      register int j;
      char *name = symbol_full_name (symbol);
      int name_len = strlen (name);

      for (j = 0; j < symbol->values_index; j++)
	{
	  register int from, to, c;
	  char *pair;

	  pair = (char *)xmalloc
	    (1 + (3 * (name_len + strlen (symbol->values[j]))));

	  for (from = 0, to = 0; (c = (name[from])) != '\0'; from++)
	    {
	      if ((isalnum (c)) || (strchr (".-_@:", c) != (char *)NULL))
		pair[to++] = c;
	      else if (c == ' ')
		pair[to++] = '+';
	      else
		{
		  pair[to++] = '%';
		  pair[to++] = hi_nibble (c);
		  pair[to++] = lo_nibble (c);
		}
	    }

	  pair[to++] = '=';

	  for (from = 0; (c = symbol->values[j][from]) != '\0'; from++)
	    {
	      if ((isalnum (c)) || (strchr (".-_@:", c) != (char *)NULL))
		pair[to++] = c;
	      else if (c == ' ')
		pair[to++] = '+';
	      else
		{
		  pair[to++] = '%';
		  pair[to++] = hi_nibble (c);
		  pair[to++] = lo_nibble (c);
		}
	    }

	  pair[to] = '\0';

	  /* Add this pair to our string. */
	  if ((result_index + strlen (pair) + 2) > result_size)
	    result = (char *)xrealloc
	      (result, (result_size += 100 + strlen (pair)));
	  
	  /* If there is already a pair present, separate it from this
	     one with an ampersand. */
	  if (result_index != 0)
	    result[result_index++] = '&';

	  strcpy (result + result_index, pair);
	  result_index += strlen (pair);
	  result[result_index] = '\0';
	  free (pair);
	}

      free (name);
    }
  return (result);
}

/* Do the `%FF' and `+' hacking on string.  We can do this hacking in
   place, since the resultant string cannot be longer than the input
   string. */
static void
cleanup (char *string)
{
  register int i, j, len;
  char *dest = string;

  len = strlen (string);

  for (i = 0, j = 0; i < len; i++)
    {
      switch (string[i])
	{
	case '%':
	  dest[j++] = parse_hex_pair (string + i + 1);
	  i += 2;
	  break;

	case '+':
	  dest[j++] = ' ';
	  break;

	default:
	  dest[j++] = string[i];
	}
    }

  dest[j] = '\0';
}

static int
parse_hex_pair (char *pair_start)
{
  int value = 0;
  int char1, char2;

  char1 = char2 = 0;

  char1 = *pair_start;

  if (char1)
    char2 = (pair_start[1]);

  if (isupper (char1))
    char1 = tolower (char1);

  if (isupper (char2))
    char2 = tolower (char2);

  if (isdigit (char1))
    value = char1 - '0';
  else if ((char1 <= 'f') && (char1 >= 'a'))
    value = 10 + (char1 - 'a');

  if (isdigit (char2))
    value = (value * 16) + (char2 - '0');
  else if ((char2 <= 'f') && (char2 >= 'a'))
    value = (value * 16) + (10 + (char2 - 'a'));

  return (value);
}

/* Get the value in PACKAGE of the variable named by TAG.
   Passing a PACKAGE of NULL is the same as calling forms_get_tag_value. */
char *
forms_get_tag_value_in_package (Package *package, char *tag)
{
  register int i, j;
  char *name = tag;
  int value_index = 0;
  int unarray = 0;
  char *value = (char *)NULL;
  Symbol *symbol;

  /* Does this variable ref contain an array indicator? */
  for (i = 0; tag[i] != '\0'; i++)
    if (tag[i] == '[')
      {
	/* Find out if this is a real array reference. */
#if MUST_BE_DIGITS
	for (j = i + 1; isdigit (tag[j]); j++);
	if (tag[j] == ']')
	  {
	    if (j == i + 1)
	      unarray = 1;
	    else
	      value_index = atoi (tag + i + 1);

	    name = strdup (tag);
	    name[i] = '\0';
	  }
#else /* !MUST_BE_DIGITS */
	int all_digits = 1;

	for (j = i + 1; tag[j] != '\0' && tag[j] != ']'; j++)
	  if (!isdigit (tag[j]))
	    all_digits = 0;

	if (tag[j] == ']')
	  {
	    if (j == i + 1)
	      unarray = 1;
	    else
	      {
		if (all_digits)
		  value_index = atoi (tag + i + 1);
		else
		  {
		    char *tv, *nv;

		    j--;
		    tv = (char *)xmalloc (1 + (j - i));
		    strncpy (tv, tag + i + 1, j - i);
		    tv[j - i] = '\0';
		    nv = forms_get_tag_value (tv);
		    if (nv)
		      value_index = atoi (nv);
		    free (tv);
		  }
	      }
	    name = strdup (tag);
	    name[i] = '\0';
	  }
#endif /* !MUST_BE_DIGITS */
	break;
      }

  /* Get the symbol for this name. */
  if (package != (Package *)NULL)
    symbol = symbol_lookup_in_package (package, name);
  else
    symbol = symbol_lookup (name);

  if (symbol != (Symbol *)NULL)
    {
      char **values = symbol->values;

      /* Now find the appropriate variable in that realm. */
      if ((values != (char **)NULL) && (symbol->type == symtype_STRING))
	{
	  if (unarray)
	    {
	      int length = 0;
	      char *temp;

	      for (i = 0; values[i] != (char *)NULL; i++)
		length += (2 + strlen (values[i]));

	      value = (char *)xmalloc (1 + length);
	      temp = value;
	      *value = '\0';

	      for (i = 0; values[i] != (char *)NULL; i++)
		{
		  strcpy (temp, values[i]);
		  temp += strlen (values[i]);
		  if (values[i + 1] != (char *)NULL)
		    {
		      *temp = '\n';
		      temp++;
		    }
		}
	      forms_gc_remember (value);
	    }
	  else
	    {
	      if ((value_index < symbol->values_index) &&
		  (value_index > -1))
		value = symbol->values[value_index];
	    }
	}
    }

  if (name != tag) free (name);
  return (value);
}

/* Get the value of the variable named by TAG.  Tag may contain an
   array index referent, in which case, that value is returned.
   The magic referent "tag[]" refers to each element of the array,
   separated by newlines. The current package is used unless TAG
   contains a package part, as in "foo::bar". */
char *
forms_get_tag_value (char *tag)
{
  return (forms_get_tag_value_in_package ((Package *)NULL, tag));
}

/* Free the individual pointers in ARRAY, and the ARRAY itself. */
static void
free_array (char **array)
{
  if (array != (char **)NULL)
    {
      register int i;

      for (i = 0; array[i] != (char *)NULL; i++)
	free (array[i]);

      free (array);
    }
}

/* Give TAG VALUE as one of the values.
   Special syntax allows you to create an array from newline separated
   strings in VALUE, or to set a specific array element of TAG.
   The syntax "foo:tag[3]" sets the third array element of TAG in the
   package FOO, while the syntax "tag[]" arrayifies VALUE. */
void
forms_set_tag_value (char *tag, char *value)
{
  forms_set_tag_value_in_package ((Package *)NULL, tag, value);
}

void
forms_set_tag_value_in_package (Package *package, char *tag, char *value)
{
  register int i, j;
  char *name = tag;
  int value_index = 0;
  int arrayify = 0;
  Symbol *symbol;

  /* Does this variable ref contain an array indicator? */
  for (i = 0; tag[i] != '\0'; i++)
    if (tag[i] == '[')
      {
	/* Find out if this is a real array reference. */
#if MUST_BE_DIGITS
	for (j = i + 1; isdigit (tag[j]); j++);
	if (tag[j] == ']')
	  {
	    if (j == i + 1)
	      arrayify = 1;
	    else
	      value_index = atoi (tag + i + 1);

	    name = strdup (tag);
	    name[i] = '\0';
	  }
#else /* !MUST_BE_DIGITS */
	int all_digits = 1;

	for (j = i + 1; tag[j] != '\0' && tag[j] != ']'; j++)
	  if (!isdigit (tag[j]))
	    all_digits = 0;

	if (tag[j] == ']')
	  {
	    if (j == i + 1)
	      arrayify = 1;
	    else
	      {
		if (all_digits)
		  value_index = atoi (tag + i + 1);
		else
		  {
		    char *tv, *nv;

		    j--;
		    tv = (char *)xmalloc (1 + (j - i));
		    strncpy (tv, tag + i + 1, j - i);
		    tv[j - i] = '\0';
		    nv = forms_get_tag_value (tv);
		    if (nv)
		      value_index = atoi (nv);
		    free (tv);
		  }
	      }
	    name = strdup (tag);
	    name[i] = '\0';
	  }
#endif /* !MUST_BE_DIGITS */
	break;
      }

  /* Get the symbol for this name. */
  if (package == (Package *)NULL)
    symbol = symbol_intern (name);
  else
    symbol = symbol_intern_in_package (package, name);

  /* If this symbol is readonly, then we cannot manipulate it. */
  if (!symbol_get_flag (symbol, sym_READONLY))
    {
      /* If this symbol is not of type STRING, then delete it. */
      if (symbol->type != symtype_STRING)
	{
	  Symbol *r = symbol_remove_in_package (symbol->package, symbol->name);
	  symbol_free (r);

	  if (package == (Package *)NULL)
	    symbol = symbol_intern (name);
	  else
	    symbol = symbol_intern_in_package (package, name);
	}

      /* If the value is empty, then add it as the empty string. */
      if (value == (char *)NULL)
	value = "";

      if (symbol->notifier)
	*(symbol->notifier) = 1;

      /* If the index to store at is larger than the number of items in the
	 list, then make a bunch of blank items to fill in the space. */
      if (value_index >= symbol->values_index)
	{
	  symbol->values = (char **)xrealloc
	    (symbol->values,
	     (symbol->values_slots = 2 + value_index) * sizeof (char *));

	  while (symbol->values_index <= value_index)
	    symbol->values[symbol->values_index++] = strdup ("");

	  symbol->values[symbol->values_index] = (char *)NULL;
	}

      /* Store the value.  If we are supposed to arrayify this variable,
	 do it now. */
      if (arrayify)
	{
	  char **values = (char **)NULL;
	  int values_index = 0;
	  int values_slots = 0;
	  int start = 0;

	  while (value[start])
	    {
	      /* Skip all whitespace between items. */
	      for (i = start; whitespace (value[i]); i++);

	      start = i;
	      if (value[i] == '\0')
		break;

	      for (; value[i] != '\0' && value[i] != '\n'; i++);

	      if (values_index + 2 > values_slots)
		values = (char **)xrealloc
		(values, (values_slots += 10) * sizeof (char *));

	      values[values_index] = (char *)xmalloc (1 + (i - start));
	      strncpy (values[values_index], value + start, i - start);
	      values[values_index][i - start] = '\0';
	      values_index++;
	      values[values_index] = (char *)NULL;
	      start = i;
	    }

	  free_array (symbol->values);
	  symbol->values = values;
	  symbol->values_index = values_index;
	  symbol->values_slots = values_slots;
	}
      else
	{
	  if (value_index > -1)
	    {
	      free (symbol->values[value_index]);
	      symbol->values[value_index] = strdup (value);
	    }
	}
      symbol_set_modified (symbol);
    }

  if (name != tag)
    free (name);
}

/* This is a bit of cruft.  You can put pointers in this list which, for
   one reason or the other, just *had* to be malloc'ed, but which normally
   would not have to be (e.g., forms_get_tag_value of an array reference).
   Then, later, when you think it is safe, you can gc them all.  Ugh. */
static char **forms_gc_list = (char **)NULL;
static int forms_gc_list_index = 0;
static int forms_gc_list_slots = 0;

void
forms_gc_remember (char *pointer)
{
  if (forms_gc_list_index + 2 > forms_gc_list_slots)
    forms_gc_list = (char **)xrealloc
      (forms_gc_list, (forms_gc_list_slots += 10) * sizeof (char *));

  forms_gc_list[forms_gc_list_index++] = pointer;
  forms_gc_list[forms_gc_list_index] = (char *)NULL;
}

void
forms_gc_pointers (void)
{
  register int i;

  for (i = 0; i < forms_gc_list_index; i++)
    free (forms_gc_list[i]);

  forms_gc_list_index = 0;
}

#if defined (__cplusplus)
}
#endif
