/*  wisp.c: -*- C -*- Functions that implement a weak lisp interface. */

/* Author: bfox@ai.mit.edu, Jan 1990
   Copyright (c) 1990 Brian J. Fox (bfox@ai.mit.edu) */

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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "wisp.h"
#include <xmalloc/xmalloc.h>

#if defined (__cplusplus)
extern "C"
{
#endif
static void initialize_string_buffer (void);
static void print_object_to_string_buffer (WispObject *object);
static void print_string_to_string_buffer (char *string);
static void print_list_contents (WispObject *list);
static char *get_string_buffer (void);
static char *read_string (void);
static WispObject *read_symbol_or_number (void);
static WispObject *read_list (void);

static int  read_character (void);
static void unread_character (void);

/* Hey!  Here is NIL. */
WispObject wisp_nil_value = { LT_nil };

/* Here is the magic symbol DOT. */
WispObject wisp_dot_value = { LT_string, { "." } };

/* Here is an array of pointers to every rawly allocated object. */
static WispObject **obarray = (WispObject **)NULL;
static int obarray_index = 0;
static int obarray_size = 0;

static void
obarray_add (WispObject *object)
{
  if ((obarray_index + 2) > obarray_size)
    obarray = (WispObject **)xrealloc
      (obarray, (obarray_size += 1000) * sizeof (WispObject *));

  obarray[obarray_index++] = object;
  obarray[obarray_index] = (WispObject *)NULL;
}

static void
obarray_del (WispObject *object)
{
  register int i, j;

  for (i = 0; i < obarray_index; i++)
    if (obarray[i] == object)
      {
	for (j = i + 1;
	     (obarray[i] = obarray[j]) != (WispObject *)NULL;
	     j++, i++);

	if (obarray_index == j)
	  obarray_index--;

	break;
      }
}

void
gc_wisp_free (WispObject *object)
{
  switch (object->type)
    {
    case LT_string:
      free (object->val.string);
      free (object);
      break;

    case LT_number:
    case LT_cons:
    case LT_symbol:
      free (object);
      break;

    case LT_nil:
      break;
    }
}

void
gc_wisp_objects ()
{
  register int i;
  register WispObject *object;

  for (i = 0; i < obarray_index; i++)
    {
      object = obarray[i];

      gc_wisp_free (object);
    }
  obarray_index = 0;
}

static WispObject *
allocate_object (void)
{
  WispObject *object;

  object = (WispObject *)xmalloc (sizeof (WispObject));
  memset ((void *)object, 0, sizeof (WispObject));

  /* Remember the allocated object on the obarray. */
  obarray_add (object);

  return (object);
}

/* Steal OBJECT from the GC list so that it may persist.
   The caller is then responsible for freeing the object
   by calling gc_wisp_free () on it. */
void
gc_steal (WispObject *object)
{
  register int i;

  for (i = 0; i < obarray_index; i++)
    {
      if (obarray[i] == object)
	{
	  for (; i < obarray_index; i++)
	    obarray[i] = obarray[i + 1];

	  obarray[i] = (WispObject *)NULL;
	  obarray_index--;
	  break;
	}
    }
}
  
WispObject *
make_string_object (char *string)
{
  WispObject *object;

  object = allocate_object ();
  object->type = LT_string;
  object->val.string = strdup (string);

  return (object);
}

WispObject *
make_number_object (double number)
{
  WispObject *object;

  object = allocate_object ();
  object->type = LT_number;
  object->val.number = number;

  return (object);
}

WispObject *
make_cons (WispObject *car, WispObject *cdr)
{
  WispObject *object;

  object = allocate_object ();
  object->type = LT_cons;
  CAR (object) = car;
  CDR (object) = cdr;

  return (object);
}

WispObject *
make_list (WispObject *car, WispObject *cdr)
{
  WispObject *list;

  list = make_cons (cdr, NIL);
  list = make_cons (car, list);

  return (list);
}

char *
string_from_wisp (WispObject *object)
{
  initialize_string_buffer ();
  print_object_to_string_buffer (object);
  return (get_string_buffer ());
}

WispObject *
wisp_from_string (char *string)
{
  WispObject *object;

  /* The sentinel values EOF and empty string return the magical value
     of a NULL pointer to a Wisp Object, not the value NIL.  NIL can
     only be returned by "()". */
  if ((string == (char *)NULL) || (*string == '\0'))
    return ((WispObject *)NULL);

  wisp_push_input_string (string);
  object = wisp_read ();
  wisp_pop_input_string ();
  return (object);
}

WispObject *
wisp_read ()
{
  WispObject *object = (WispObject *)NULL;
  int reading_object = 1;

  while (reading_object)
    {
      int character;

      character = read_character ();

      switch (character)
	{
	  /* End of FILE seen at top level returns a NULL object.  This isn't
	     quite right, since things that call wisp_read () may have to
	     explicty check to see if we are at the end of file.  */
	case EOF:
	  reading_object = 0;
	  break;

	case '"':
	  {
	    char *string;

	    string = read_string ();
	    object = make_string_object (string);
	    free (string);
	    reading_object = 0;
	  }
	  break;

	case '\'':
	  object = wisp_read ();
	  reading_object = 0;
	  break;
	    
	case '(':
	  {
	    WispObject *list;

	    list = read_list ();

	    if (list == (WispObject *)NULL)
	      object = NIL;
	    else
	      object = list;

	    reading_object = 0;
	  }
	  break;

	  /* This works like magic.  If a list is being defined, this has
	     the desired effect of installing NIL in the cdr of the list.
	     If this symbol is seen at top level, it constitutes an error,
	     since the evaller cannot handle a NULL object. */
	case ')':
	  reading_object = 0;
	  break;

	  /* Ignore separating whitespace. */
	case '\t':
	case ' ':
	case '\r':
	case '\n':
	  break;

	  /* Gee, I forgot to handle comments? */
	case ';':
	  while (((character = read_character ()) != EOF) &&
		 (character != '\n'));
	  break;

	default:
	  {
	    /* This is a symbol or a number (or DOT). */
	    unread_character ();
	    object = read_symbol_or_number ();
	    reading_object = 0;
	    if (
#if !defined (HAVE_SYMBOLS)
		((STRING_P (object)) &&
		 (strcmp (STRING_VALUE (object), ".") == 0)) ||
#endif /* !HAVE_SYMBOLS */
		((SYMBOL_P (object)) &&
		 (strcmp (SYMBOL_PNAME (object), ".") == 0)))
	      {
		obarray_del (object);
		free (object->val.string);
		free (object);
		object = &wisp_dot_value;
	      }
	  }
	  break;
	}
    }

  return (object);
}

static char *
read_string_1 (int delimited)
{
  char *string;
  int string_index, string_size;
  int chars_to_read = 1;

  string = (char *)xmalloc (1);
  *string = '\0';
  string_index = 0;
  string_size = 1;

  while (chars_to_read)
    {
      int character;

      character = read_character ();

      if (character == EOF)
	{
	  chars_to_read = 0;
	  continue;
	}

      if (!delimited)
	{
	  if (self_delimiting (character))
	    unread_character ();

	  if (whitespace_or_newline (character) || self_delimiting (character))
	    {
	      chars_to_read = 0;
	      continue;
	    }
	}

      switch (character)
	{
	case '\\':
	  character = read_character ();
	  switch (character)
	    {
	    case 'n':
	    case 'r':
	      character = '\n';
	      break;
	    case 't':
	      character = '\t';
	      break;
	    case 'f':
	      character = '\f';
	      break;
	    case '\n':		/* Backslash followed by newline ignored. */
	    case '\r':
	      continue;
	    }

	  if (string_index + 1 >= string_size)
	    string = (char *)xrealloc (string, (string_size += 20));

	  string[string_index++] = character;
	  string[string_index] = '\0';
	  break;

	case '"':
	  chars_to_read = 0;
	  break;

	default:
	  if (string_index + 1 >= string_size)
	    string = (char *)xrealloc (string, (string_size += 20));

	  string[string_index++] = character;
	  string[string_index] = '\0';
	}
    }
  return (string);
}

/* Read a string object which is delimited by double quotes.  The double
   quote which introduces the string has already been read.  Strings end
   with a double quote seen at top level. */
static char *
read_string ()
{
  return (read_string_1 (1));
}

/* Read a symbol, fixnum or realnum, and return that object. */
static WispObject *
read_symbol_or_number (void)
{
  char *string;

  string = read_string_1 (0);

#if defined (NEVER)
  /* Check the contents of string.  If it is all digits, perhaps with a 
     leading sign, then this object is a number. */
  if (*string == '-' || *string == '+' || *string == '.' || isdigit (*string))
    {
      register int i;
      int dot_seen = (*string == '.');

      for (i = 1; string[i]; i++)
	{
	  if ((string[i] == '.'))
	    {
	      if (dot_seen)
		break;
	      else
		dot_seen++;
	    }
	  else if (!isdigit (string[i]))
	    break;
	}

      /* If STRING consists of only a leading sign or dot, then it is a
	 symbol.  Otherwise, it is a number. */
      if (!string[i] && (i != 1 || isdigit (string[0])))
	{
	  /* If the user specified a period as part of the number, then
	     this is a real. */
	  if (dot_seen)
	    {
	      extern double atof ();
	      Realnum *realnum;

	      realnum = (Realnum *)xmalloc (sizeof (Realnum));
	      realnum->value = atof (string);
	      return (make_object (RealnumObj, realnum));
	    }
	  else
	    {
	      long fixnum;
	      fixnum = atol (string);
	      return (make_object (FixnumObj, fixnum));
	    }
	}
    }
#endif /* NEVER */
  {
    WispObject *result;

    result = make_string_object (string);
    free (string);
    return (result);
  }
}

/* Read a list object which is delimited by `(' and `)'.  The `(' which
   introduces the list has already been read.  Lists end with a `)' seen
   at top level. */
static WispObject *
read_list (void)
{
  WispObject *list, *val_read;

  val_read = wisp_read ();

  /* Magic for reading a dotted pair.  If the item just read is the symbol
     `.', then we just read the next object, and make that the object to 
     return.  Make sure that we ind the end of the list, though. */
  if (val_read == &wisp_dot_value)
    {
      WispObject *next_val = (WispObject *)NULL;

      val_read = wisp_read ();

      if (val_read != (WispObject *)NULL)
	{
	  next_val = wisp_read ();

	  if (next_val == (WispObject *)NULL)
	    return (val_read);
	  else
	    {
	      WispObject *new_list = allocate_object ();
	      new_list->type = LT_cons;
	      CAR (new_list) = val_read;
	      CDR (new_list) = make_cons (next_val, read_list ());
	      return (new_list);
	    }
	}
    }

  if (val_read == (WispObject *)NULL)
    list = NIL;
  else
    {
      list = allocate_object ();
      list->type = LT_cons;
      CAR (list) = val_read;
      CDR (list) = read_list ();
    }
  return (list);
}

static void
print_object_to_string_buffer (WispObject *object)
{
  if (object == (WispObject *)NULL)
    return;

  switch (object->type)
    {
    case LT_string:
      {
	register int i, j;
	char *string, *pr;

	string = STRING_VALUE (object);
	pr = (char *)xmalloc ((2 * STRING_LENGTH (object)) + 1);

	j = 0;
	pr[j++] = '"';

	for (i = 0; (string != (char *)NULL) && (string[i] != '\0'); i++)
	  {
	    if (string[i] == '"')
	      pr[j++] = '\\';

	    pr[j++] = string[i];
	  }

	pr[j++] = '"';
	pr[j++] = '\0';

	print_string_to_string_buffer (pr);
	free (pr);
      }
      break;

    case LT_number:
      {
	char float_rep[100];

	sprintf (float_rep, "\"%.2f\"", NUMBER_VALUE (object));
	print_string_to_string_buffer (float_rep);
      }
      break;

    case LT_cons:
      if (CAR (object) == (WispObject *)NULL)
	return;
      else
	{
	  print_string_to_string_buffer ("(");
	  print_list_contents (object);
	  print_string_to_string_buffer (")");
	}
      break;

    case LT_symbol:
      print_string_to_string_buffer (SYMBOL_PNAME (object));
      break;

    case LT_nil:
      print_string_to_string_buffer ("()");
      break;
    }
}

static void
print_list_contents (WispObject *list)
{
  if (NIL_P (list))
    return;

  print_object_to_string_buffer (CAR (list));

  if (!NIL_P (CDR (list)))
    {
      print_string_to_string_buffer (" ");

      if (CONS_P (CDR (list)))
	print_list_contents (CDR (list));
      else
	{
	  print_string_to_string_buffer (". ");
	  print_object_to_string_buffer (CDR (list));
	}
    }
}

/*******************************************************************/
/*								   */
/*                  Output to a string buffer.			   */
/*								   */
/*******************************************************************/

static char *string_buffer = (char *)NULL;
static int sb_index = 0;
static int sb_size = 0;

static void
initialize_string_buffer ()
{
  sb_index = 0;
  if (string_buffer)
    string_buffer[0] = '\0';
}

static char *
get_string_buffer ()
{
  return (string_buffer);
}

#define EXPANSION 512

static void
print_string_to_string_buffer (char *string)
{
  int len = 0;

  if (string)
    len += strlen (string);

  if (!len)
    return;

  /* Get the buffer big enough to hold the data. */
  while ((sb_index + len) >= sb_size)
    string_buffer = (char *)xrealloc (string_buffer, (sb_size += EXPANSION));

  /* Copy the string data over. */
  strcpy (string_buffer + sb_index, string);
  sb_index += len;
}

/*******************************************************************/
/*								   */
/*                  Input from a string buffer.			   */
/*								   */
/*******************************************************************/

static char *the_input_string = (char *)NULL;
static int the_input_string_index = 0;

void
wisp_push_input_string (char *string)
{
  the_input_string = string;
  the_input_string_index = 0;
}

void
wisp_pop_input_string ()
{
  the_input_string = (char *)NULL;
  the_input_string_index = 0;
}

static int
read_character ()
{
  int character = EOF;

  if ((the_input_string != (char *)NULL) &&
      (the_input_string[the_input_string_index] != '\0'))
    character = the_input_string[the_input_string_index++] & 0x00ff;

  return (character);
}

static void
unread_character (void)
{
  if ((the_input_string == (char *)NULL) ||
      (the_input_string_index == 0))
    return;

  the_input_string_index--;
}

/* Return the wisp-readable representation of STRING.  That is, create a
   new string from STRING such that:

   strcmp (string_from_wisp (wisp_from_string (STRING)), STRING) == 0
*/
char *
wisp_readable (char *string)
{
  int newlen = (string == (char *)NULL) ? 1 : (3 + (2 * strlen (string)));
  static char *result = (char *)NULL;
  static int oldlen = -1;

  if (newlen > oldlen)
    result = (char *)xrealloc (result, newlen);

  oldlen = newlen;
  
  if (string != (char *)NULL)
    {
      register int i, j;

      j = 0;

      result[j++] = '"';

      for (i = 0; string[i] != '\0'; i++)
	{
	  if ((string[i] == '"') || (string[i] == '\\'))
	    result[j++] = '\\';

	  result[j++] = string[i];
	}

      result[j++] = '"';
      result[j] = '\0';
    }
  else
    {
      result[0] = '0';
    }

  return (result);
}

/*******************************************************************/
/*								   */
/*		  Lisp-like List Processing Primitives		   */
/*								   */
/*******************************************************************/

/* A great deal similar to (assoc), but not quite the same.  In this
   case, the key is a C string, and the list is a WispObject.
   Returns the part of the list whose car is a string which case
   independently matches KEY, or a NULL pointer if there is no match. */
WispObject *
assoc (char *key, WispObject *list)
{
  while (CONS_P (list) && CONS_P (CAR (list)))
    {
      WispObject *pair, *car;

      pair = CAR (list);
      car = CAR (pair);

      if (STRING_P (car) && (strcasecmp (STRING_VALUE (car), key) == 0))
	return (pair);

      list = CDR (list);
    }
  return ((WispObject *)NULL);
}

/* A heavily used function which returns the C string value of the cdr
   of the pair in LIST whose car matched KEY.  Arguments like assoc ()
   above. */
char *
sassoc (char *key, WispObject *list)
{
  WispObject *result;

  result = assoc (key, list);

  if ((result != (WispObject *)NULL) && 
      (CONS_P (result)) && (STRING_P (CAR (CDR (result)))))
    return (STRING_VALUE (CAR (CDR (result))));

  return ((char *)NULL);
}

WispObject *
wisp_append (WispObject *list, WispObject *object)
{
  WispObject *last_cons = list;

  /* Case of first argument NIL, means return the second arg. */
  if (NIL_P (list))
    return (object);

  /* First object must be a list, not a cons.  Find the last pair. */
  while (CONS_P (last_cons))
    {
      if (NIL_P (CDR (last_cons)))
	{
	  CDR (last_cons) = object; 
	  return (list);
	}
      last_cons = CDR (last_cons);
    }
  abort ();			/* Called incorrectly. */

  return ((WispObject *)NULL);
}

WispObject *
copy_object (WispObject *object)
{
  WispObject *copy = (WispObject *)NULL;

  if (object == (WispObject *)NULL)
    return (object);

  copy = allocate_object ();
  copy->type = object->type;

  switch (object->type)
    {
    case LT_string:
      copy->val.string = strdup (STRING_VALUE (object));
      break;

    case LT_number:
      copy->val.number = NUMBER_VALUE (object);
      break;

    case LT_cons:
      CAR (copy) = copy_object (CAR (object));
      CDR (copy) = copy_object (CDR (object));
      break;

    case LT_symbol:
      copy->val.symbol = object->val.symbol;
      break;

    case LT_nil:
      free (copy);
      copy = NIL;
    }
  return (copy);
}

/* Return the Nth element of LIST, counting 0 as the first element.
   If N is greater than the number of elements in the list, return
   NIL. */
WispObject *
wisp_nth (int n, WispObject *list)
{
  WispObject *result = NIL;

  while ((CONS_P (list)) && (n > 0))
    {
      --n;

      if (n == 0)
	{
	  result = CAR (list);
	  break;
	}
      else
	list = CDR (list);
    }

  return (result);
}

/* Return the length of OBJECT.
   For a list this is the number of elements, for a string or symbol,
   this is the number of characters in the string, for a number, this
   is the number of character in the printed representation of the
   number. */
int
wisp_length (WispObject *object)
{
  int result = 0;

  if (object != (WispObject *)NULL)
    {
      switch (object->type)
	{
	case LT_nil:
	  break;

	case LT_string:
	  result = strlen (STRING_VALUE (object));
	  break;

	case LT_cons:
	  {
	    while (CONS_P (object))
	      {
		result++;
		object = CDR (object);

		/* Handle left-over dotted pair. */
		if (!CONS_P (object) && !NIL_P (object))
		  result++;
	      }
	    break;

	case LT_number:
	    {
	      char buffer[100];
	      sprintf (buffer, "%f", NUMBER_VALUE (object));
	      result = strlen (buffer);
	    }
	    break;

	case LT_symbol:
	    result = strlen (SYMBOL_PNAME (object));
	    break;
	  }
	}
    }
  return (result);
}

#if defined (__cplusplus)
}
#endif
