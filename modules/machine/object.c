/* object.c: -*- C -*-  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1996, 2000, E. B. Gamble (ebg@metahtml.com).

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

#include <sys/time.h>		/* gettimeofday() */
#include <math.h>		/* sqrt() */
#include <netinet/in.h>		/* htonl(), ntohl() */

#include "machine/machine.h"

void fail (void) { abort (); }

static mh_number_t
mh_object_read_number (mh_string_t string);

static unsigned char *
strndup (unsigned char *string, size_t size)
{
  char *result;

  result = xmalloc (size + 1);
  memcpy (result, string, size);
  result [size] = 0;
  return result;
}

/* 
 * mh_string_to_string_escaped ()
 *
 * Destructively modify STRING by removing escapes */
extern void
mh_string_to_string_escaped (string_t string)
{
  int c;
  int rdex = -1, sdex = -1;

  while (string [++sdex])
    {
      ++rdex;
      switch (string [sdex])
	{
	case '\\':
	  switch (string [++sdex])
	    {
	    case '\0': c = '\0'; break;
	    case '\\': c = '\\'; break;
	    case '"' : c = '"' ; break;
	    case 'n' : c = '\n'; break;
	    case 't' : c = '\t'; break;
	    case 'r' : c = '\r'; break;
	    case 'f' : c = '\f'; break;
	    case '0' : c = '\0'; break;
	    default:
	      string [rdex++] = '\\';
	      c = string [sdex];
	      break;
	    }
	  break;

	default:
	  c = string [sdex];
	  break;
	}
      string [rdex] = c;
      if (c == '\0') return;
    }
  string [++rdex] = '\0';
}

#define STRING_QUOTES_COUNT   6
static string_t string_quotes [STRING_QUOTES_COUNT] =
{
  "",				/* 0:  */
  "\"",				/* 1: " */
  "\\\"",			/* 2: \" */
  "\\\\\\\"",			/* 3: \\\" */
  "\\\\\\\\\\\\\"",		/* 4: */
  "\\\\\\\\\\\\\\\\\\\\\\\\\""	/* 5: */
};

static inline unsigned int
mh_string_backslashes_at_depth_count (unsigned int depth)
{ return (1 << depth) - 1; }

static inline unsigned int
mh_string_quotes_at_depth_length (unsigned int depth)
{ return 1 + mh_string_backslashes_at_depth_count (depth); }

extern string_t 
mh_string_quotes_at_depth (unsigned int depth)
{
  if (depth < 0)
    return strdup ("");
  else if (depth < STRING_QUOTES_COUNT)
    return strdup (string_quotes [depth]);
  else
    {
      unsigned int backslashes =
	mh_string_backslashes_at_depth_count (depth);

      string_t string = xmalloc (backslashes + 1 + 1);
      
      memset (string, '\\', backslashes);
      string[backslashes    ] = '\"';
      string[backslashes + 1] = '\0';

      return string;
    }
}


/* Return the count of the number of characters this string would have if
   quoting was added in order for it to be passed out of the machine. */
static unsigned int
mh_string_to_string_quoted_length (string_t string)
{
  unsigned int length = 0;

  while (*string)
    {
      switch (*string)
	{
	case '"':
	case '\\':
	  length++;
	  break;

	default:
	  break;
	}
      length++;
      string++;
    }

  return (length);
}

/* Increase the quoting by one */	
extern string_t
mh_string_to_string_quoted (string_t string)
{
  unsigned int length = mh_string_to_string_quoted_length (string);

  string_t result = xmalloc (1 + length);
  string_t temp   = result;

  for (; *string; string++, temp++)
    {
      char ch = *string;
      switch (*string)
	{
	case '"':
	  *temp++ = '\\';
	  ch = '"';
	  break;
	case '\\':
	  *temp++ = '\\';
	  ch = '\\';
	  break;

	default:
	  break;
	}
  
      *temp = ch;
    }
  *temp = '\0';

  return (result);
}

/* WRONG ! */
extern string_t
mh_string_to_string_quoted_to_depth (string_t string,
				     unsigned int depth)
{
  string_t temp   = string ? strdup (string) : "";
  string_t result = temp;
 
  /* Just quote it DEPTH times - Yuck! */
  while (depth-- > 0)
    {
      result = mh_string_to_string_quoted (temp);
      xfree (temp);
      temp = result;
    }
  return result;
}

/***************************************************************************
 *
 * MH_BYTE_CODE_T
 *
 */ 
struct mh_byte_code_spec
mh_byte_code_spec_table [1 + MH_NUMBER_OF_BYTEOPS] =
{
# define BYTEOP( code, name, length )		\
  {						\
    MH_##code##_OP,				\
    MH_##code##_OP_LEN,				\
    name					\
  },
# include "bops.h"
# undef BYTEOP
};

extern mh_object_t
mh_object_alloc (mh_type_t    type,
		 unsigned int size_in_bytes)
{
  unsigned int objects = OBJECT_SIZE (size_in_bytes);
  mh_object_t  object  = mh_memory_alloc (objects);

  object->marker = OBJECT_MARKER;
  object->tag    = type;
  return object;
}

/***************************************************************************
 *
 * MH_STRING_T
 *
 */ 
mh_string_t MH_EMPTY;
mh_string_t MH_TRUE;

extern mh_string_t
mh_string_new (string_t string)
{
  size_t length = strlen (string);

  return 0 == length
    ? MH_EMPTY
    : mh_string_buffer (string, length);
}

extern mh_string_t
mh_string_buffer (string_t string,
		  size_t   length)
{
  mh_string_t target;

  length = length > 0 ? length : strlen (string);

  if (0 == length)
    return MH_EMPTY;

  target = (mh_string_t) 
    mh_object_alloc (MH_STRING_TAG, MH_STRING_ALLOC_SIZE (length));

  MH_STRING_LENGTH (target) = length;
  MH_STRING_CHARS  (target)[0] = 0;
  strncat (MH_STRING_CHARS (target), string, length);
  return target;
}

extern mh_string_t
mh_string_2_new (string_t str1,
		 string_t str2)
{
  mh_string_t string = mh_string_buffer
    ("", strlen (str1) + strlen (str2));
  sprintf (MH_STRING_CHARS (string), "%s%s", str1, str2);
  return string;
}

extern mh_string_t
mh_string_3_new (string_t str1,
		 string_t str2,
		 string_t str3)
{
  mh_string_t string = mh_string_buffer
    ("", strlen (str1) + strlen (str2) + strlen (str3));
  sprintf (MH_STRING_CHARS (string), "%s%s%s", str1, str2, str3);
  return string;
}


extern mh_string_t
mh_string_dup (mh_string_t string)
{
  mh_string_t target =
    (mh_string_t) xmalloc (MH_STRING_ALLOC_SIZE (MH_STRING_LENGTH (string)));
  mh_string_copy (target, string);
  return target;
}

extern void
mh_string_copy (mh_string_t target,
		mh_string_t source)
{
  memcpy ((void *) target,
	  (const void *) source,
	  MH_STRING_ALLOC_SIZE (MH_STRING_LENGTH (source)));
}

/* Accessors */
extern size_t
mh_string_length (mh_string_t string)
{
  return MH_STRING_LENGTH (string);
}

extern char
mh_string_char_at (mh_string_t  string, 
		   unsigned int offset)
{
  return MH_STRING_CHARS(string)[offset];
}

/* Equality */
extern mh_bool_t
mh_string_equal (mh_string_t str1,
		 mh_string_t str2,
		 boolean_t   caseless_p)
{
  return 0 == ((caseless_p ? strcasecmp : strcmp) 
	       (MH_STRING_CHARS (str1), MH_STRING_CHARS (str2)))
    ? MH_TRUE
    : MH_EMPTY;
}

extern mh_bool_t
mh_string_not_equal (mh_string_t str1,
		     mh_string_t str2,
		     boolean_t   caseless_p)
{
  return 0 != ((caseless_p ? strcasecmp : strcmp)
	       (MH_STRING_CHARS (str1), MH_STRING_CHARS (str2)))
    ? MH_TRUE
    : MH_EMPTY;
}

extern mh_string_t
mh_string_compare (mh_string_t str1,
		   mh_string_t str2)
{
  int compare = strcmp (MH_STRING_CHARS (str1), MH_STRING_CHARS (str2));

  return mh_string_new (compare == 0
			? "equal"
			: (compare > 0
			   ? "greater"
			   : "less"));
}

extern mh_string_t
mh_string_sub (mh_string_t string,
	       mh_number_t start,
	       mh_number_t end)
{
  unsigned int start_index =
    MH_NUMBER_P (start) ? MH_NUMBER_VALUE (start) : 0;
  unsigned int end_index =
    MH_NUMBER_P (end) ? MH_NUMBER_VALUE (end) : MH_STRING_LENGTH (string);

  return mh_string_buffer (start_index + MH_STRING_CHARS (string),
			   end_index - start_index);
}
  

/* Case Conversions */
extern void
mh_string_downcase (mh_string_t target,
		    mh_string_t source)
{
  string_t tar = MH_STRING_CHARS (target);
  string_t src = MH_STRING_CHARS (source);
  for (; *src; src++, tar++)
    if (isupper (*tar)) *tar = tolower (*src);
  return;
}

static void
mh_string_upcase_internal (string_t tar,
			   string_t src)
{
  for (; *src; src++, tar++)
    if (islower (*tar)) *tar = toupper (*src);
  return;
}
extern void
mh_string_upcase (mh_string_t target,
		  mh_string_t source)
{
  mh_string_upcase_internal
    (MH_STRING_CHARS (target),
     MH_STRING_CHARS (source));
  return;
}

extern void
mh_string_capitalize (mh_string_t target,
		      mh_string_t source)
{
  string_t tar    = MH_STRING_CHARS (target);
  string_t src    = MH_STRING_CHARS (source);
  boolean_t upper = true;

  for (; *src; src++, tar++)
    {
      unsigned char ch = *src;

      if (ch > 127)
	continue;

      if (!isalpha (ch))
	upper = true;
      else
	{
	  if (upper)
	    {
	      if (islower (ch))
		ch = toupper (ch);

	      upper = false;
	    }
	  else
	    {
	      if (isupper (ch))
		ch = tolower (ch);
	    }
	}

      *tar = ch;
    }

  return;
}

extern void
mh_string_extend (mh_string_t buffer,
		  mh_string_t extra)
{
  strcat (MH_STRING_CHARS (buffer),
	  MH_STRING_CHARS (extra));
}

static mh_object_t
string_concat_to_object (string_t str1,
			 string_t str2)
{
  mh_string_t result = 
    mh_string_buffer (str1, strlen (str1) + strlen (str2));

  strcat (MH_STRING_CHARS (result), str2);

  return MH_AS_OBJECT (result);
}
  
extern mh_object_t
mh_string_concat (mh_object_t buffer,
		  mh_object_t extra)
{
  if (MH_STRING_P (buffer) && MH_STRING_P (extra))
    return string_concat_to_object
      (MH_STRING_CHARS (MH_AS_STRING (buffer)),
       MH_STRING_CHARS (MH_AS_STRING (extra)));
  
  return string_concat_to_object
    (mh_object_to_string (buffer, false),
     mh_object_to_string (extra,  false));
}

static inline boolean_t
mh_string_match_p (mh_string_t string,
		   string_t    chars)
{
  return (0 == strcmp (MH_STRING_CHARS (string), chars));
}


/***************************************************************************
 *
 * MH_NUMBER_T
 *
 */ 
mh_number_t MH_NUMBER_ZERO;
mh_number_t MH_NUMBER_ONE;
mh_number_t MH_NUMBER_TWO;

extern mh_number_t
mh_number_new (long double value)
{
  if (0.0 == value && MH_NUMBER_ZERO)
    return MH_NUMBER_ZERO;
  else if (1.0 == value && MH_NUMBER_ONE)
    return MH_NUMBER_ONE;
  else if (2.0 == value && MH_NUMBER_TWO)
    return MH_NUMBER_TWO;
  else
    {
      mh_number_t target = 
	(mh_number_t) mh_object_alloc (MH_NUMBER_TAG, MH_NUMBER_ALLOC_SIZE);
      MH_NUMBER_VALUE (target) = value;
      return target;
    }
}

extern long double
mh_number_value (mh_number_t number)
{
  return MH_NUMBER_VALUE (number);
}

extern mh_number_t
mh_object_to_number (mh_object_t object)
{
  if (MH_NUMBER_P (object))
    return MH_AS_NUMBER (object);

  if (! MH_EMPTY_P (object) && MH_STRING_P (object))
    {
#if defined (READ_NUMBERS_TOO)
      mh_object_t read_object =
	mh_object_read (MH_STRING_CHARS (MH_AS_STRING (object)));

      if (MH_NUMBER_P (read_object))
	return MH_AS_NUMBER (read_object);
#else
      mh_number_t number =
	mh_object_read_number (MH_AS_STRING (object));

      return number ? number : MH_NUMBER_ZERO;
#endif
    }

  return MH_NUMBER_ZERO;
}

#define DEF_MH_NUMBER_REL_OP( name, op )			\
extern mh_bool_t						\
name (mh_number_t num1,						\
      mh_number_t num2)						\
{								\
  if (MH_NUMBER_P (num1) && MH_NUMBER_P (num2))			\
    return MH_NUMBER_VALUE (num1) op MH_NUMBER_VALUE (num2)	\
       ? MH_TRUE						\
       : MH_EMPTY;						\
  return name (mh_object_to_number (MH_AS_OBJECT (num1)),	\
	       mh_object_to_number (MH_AS_OBJECT (num2)));	\
}

DEF_MH_NUMBER_REL_OP (mh_number_eq, ==);
DEF_MH_NUMBER_REL_OP (mh_number_ne, !=);
DEF_MH_NUMBER_REL_OP (mh_number_lt, < );
DEF_MH_NUMBER_REL_OP (mh_number_le, <=);
DEF_MH_NUMBER_REL_OP (mh_number_gt, > );
DEF_MH_NUMBER_REL_OP (mh_number_ge, >=);

extern mh_number_t
mh_number_sqrt  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return mh_number_new (sqrt (MH_NUMBER_VALUE (number)));
  return mh_number_sqrt (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_number_t
mh_number_inc  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return mh_number_new (MH_NUMBER_VALUE (number) + 1);
  return mh_number_inc (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_number_t
mh_number_dec  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return mh_number_new (MH_NUMBER_VALUE (number) - 1);
  return mh_number_dec (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_number_t
mh_number_integer  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return mh_number_new ((double) rint (MH_NUMBER_VALUE (number)));
  else if (MH_OBJECT_EMPTY == MH_AS_OBJECT (number))
    return MH_AS_NUMBER (MH_OBJECT_EMPTY);
  return mh_number_integer (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_bool_t
mh_number_eqz  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return 0 == MH_NUMBER_VALUE (number)
      ? MH_TRUE
      : MH_EMPTY;

  return mh_number_eqz (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_bool_t
mh_number_nez  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return 0 != MH_NUMBER_VALUE (number)
      ? MH_TRUE
      : MH_EMPTY;

  return mh_number_eqz (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_bool_t
mh_number_integer_p  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return MH_NUMBER_VALUE (number) == rint (MH_NUMBER_VALUE (number))
      ? MH_TRUE
      : MH_EMPTY;

  return mh_number_integer_p (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_bool_t
mh_number_real_p  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    return MH_NUMBER_VALUE (number) != rint (MH_NUMBER_VALUE (number))
      ? MH_TRUE
      : MH_EMPTY;

  return mh_number_real_p (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_number_t
mh_number_random  (mh_number_t number)
{
  if (MH_NUMBER_P (number))
    {
      double range = MH_NUMBER_VALUE (number);
      return mh_number_new (0 == range
			    ? (int) random ()
			    : (int) ((range * random()) / RAND_MAX));
    }
  return mh_number_random (mh_object_to_number (MH_AS_OBJECT (number)));
}

extern mh_number_t
mh_number_randomize  (mh_number_t number)
{
  return number;
}

/* Binary Ops */

#define DEF_MH_NUMBER_BIN_OP( name, op )			\
extern mh_number_t						\
name (mh_number_t num1,						\
      mh_number_t num2)						\
{								\
  if (MH_NUMBER_P (num1) && MH_NUMBER_P (num2))			\
    return mh_number_new (MH_NUMBER_VALUE (num1) op		\
			  MH_NUMBER_VALUE (num2));		\
  return name (mh_object_to_number (MH_AS_OBJECT (num1)),	\
	       mh_object_to_number (MH_AS_OBJECT (num2)));	\
}

DEF_MH_NUMBER_BIN_OP (mh_number_add, +);
DEF_MH_NUMBER_BIN_OP (mh_number_sub, -);
DEF_MH_NUMBER_BIN_OP (mh_number_mul, *);

extern mh_number_t
mh_number_div (mh_number_t num1,
	       mh_number_t num2)
{
  if (MH_NUMBER_P (num1) && MH_NUMBER_P (num2))
    return 0 == MH_NUMBER_VALUE (num2)
      ? num2
      : mh_number_new (MH_NUMBER_VALUE (num1) /
		       MH_NUMBER_VALUE (num2));
  
  return mh_number_div (mh_object_to_number (MH_AS_OBJECT (num1)),
			mh_object_to_number (MH_AS_OBJECT (num2)));
}

/***************************************************************************
 *
 * MH_FUNCTION_T
 *
 */ 
static string_t mh_argument_type_names [] =
{
  "&required",
  "&optional",
  "&key",
  "&rest",
  "&body",
  "&attributes"
};

static string_t mh_argument_eval_names [] =
{
  "&evalled",
  "&unevalled"
};

static string_t mh_argument_array_names [] =
{
  "&value",
  "&array"
};

extern mh_argument_t
mh_argument_new (string_t            name,
		 mh_argument_type_t  type,
		 mh_argument_eval_t  eval,
		 mh_argument_array_t array)
{
  mh_argument_t argument = (mh_argument_t)
    xmalloc (sizeof (struct mh_argument));

  /* Fillout the function */
  MH_ARGUMENT_NAME  (argument) = strdup (name);
  MH_ARGUMENT_TYPE  (argument) = type;
  MH_ARGUMENT_EVAL  (argument) = eval;
  MH_ARGUMENT_ARRAY (argument) = array;
  
  return argument; 
}

extern void
mh_argument_free (mh_argument_t arg)
{
  xfree (MH_ARGUMENT_NAME (arg));
  xfree (arg);
}

extern void
mh_tag_install_machine (mh_tag_t tag,
			mh_byte_code_t *code,
			unsigned int    code_count,
			mh_object_t    *constants,
			unsigned int    constants_count,
			unsigned int    stack_size)
{
  MH_TAG_CODE (tag)       = code;
  MH_TAG_CODE_COUNT (tag) = code_count;
  MH_TAG_CONSTANTS_VECTOR (tag) =
    MH_AS_OBJECT (mh_vector_fill (constants, constants_count));
  MH_TAG_STACK_SIZE (tag) = stack_size;
}

static void
mh_tag_summarize_args (mh_tag_t tag)
{
  MH_TAG_ARGS_HAS_REST_P       (tag) = false;
  MH_TAG_ARGS_HAS_REST_ARRAY_P (tag) = false;
  MH_TAG_ARGS_HAS_UNEVALLED_P  (tag) = false;
}

extern mh_tag_t
mh_tag_new (string_t        name,
	    mh_tag_type_t   type,
	    boolean_t       complex_p,
	    boolean_t       weak_p,
	    mh_white_type_t whitespace,
	    string_t        body,
	    string_t        packname,
	    string_t        documentation,
	    mh_argument_t  *args,
	    unsigned int    args_count)
{
  mh_tag_t tag = (mh_tag_t) 
    mh_object_alloc (MH_FUNCTION_TAG, MH_TAG_ALLOC_SIZE ());

  MH_TAG_NAME (tag)          = name;
  MH_TAG_TYPE (tag)          = type;
  MH_TAG_COMPLEX_P (tag)     = complex_p;
  MH_TAG_WEAK_P (tag)        = weak_p;
  MH_TAG_WHITESPACE(tag)     = whitespace;
  MH_TAG_BODY (tag)          = body;
  MH_TAG_PACKNAME (tag)      = packname;
  MH_TAG_DOCUMENTATION (tag) = documentation;
  MH_TAG_DEBUG_LEVEL (tag)   = 0;

  MH_TAG_ARGS       (tag) = args;
  MH_TAG_ARGS_COUNT (tag) = args_count;

  MH_TAG_CURRENT_PACKAGE_P (tag) = false;
  MH_TAG_CONSTANTS_VECTOR  (tag) = MH_OBJECT_EMPTY;

  mh_tag_summarize_args (tag);
  return (tag);
}

extern boolean_t
mh_tag_arguments_have_type_p (mh_tag_t tag,
			      mh_argument_type_t type)
{
  unsigned int   args_index;
  unsigned int   args_limit = MH_TAG_ARGS_COUNT (tag);
  mh_argument_t *args       = MH_TAG_ARGS (tag);

  for (args_index = 0; args_index < args_limit; args_index++)
    if (type == MH_ARGUMENT_TYPE (args [args_index]))
      return true;
  return false;
}

extern unsigned int
mh_tag_arguments_type_count  (mh_tag_t      tag,
			      mh_argument_type_t type)
{
  unsigned int   count = 0;
  unsigned int   args_index;
  unsigned int   args_limit = MH_TAG_ARGS_COUNT (tag);
  mh_argument_t *args       = MH_TAG_ARGS (tag);

  for (args_index = 0; args_index < args_limit; args_index++)
    if (type == MH_ARGUMENT_TYPE (args [args_index]))
      count++;
  return count;
}

extern mh_argument_t
mh_tag_keyword_argument (mh_tag_t    tag,
			 mh_object_t name)
{
  unsigned int   offset;
  unsigned int   count     = MH_TAG_ARGS_COUNT (tag);
  mh_argument_t *arguments = MH_TAG_ARGS (tag);

  string_t key =
    (MH_STRING_P (name)
     ? MH_STRING_CHARS (MH_AS_STRING (name))
     : "");

  for (offset = 0; offset < count; offset++)
    {
      string_t            argument_name  = 
	MH_ARGUMENT_NAME  (arguments [offset]);

      mh_argument_array_t argument_array =
	MH_ARGUMENT_ARRAY (arguments [offset]);
      
      if (0 == strcmp (key, argument_name))
	return arguments [offset];
      else if (argument_array == MH_ARGUMENT_ARRAY)
	{
	  /* Perform (key == argument_name []) 
	     How?  Ignoring whitespace?  Where */
	  string_t new_argument_name = (string_t)
	    xmalloc (1 + 2 + strlen (argument_name));
	  boolean_t match;

	  strcpy (new_argument_name, argument_name);
	  strcat (new_argument_name, "[]");

	  match = (0 == strcmp (key, new_argument_name));
	  xfree (new_argument_name);
	  if (match)
	    return arguments [offset];
	}
    }
  
  return (mh_argument_t) NULL;
}

extern void
mh_byte_code_instr_disassemble (mh_byte_code_t *opcodes, FILE *file)
{
  mh_byte_code_spec_t spec =
    & mh_byte_code_spec_table [*opcodes];
  
  mh_byte_op_len_t length   = spec->length;
  char *name = spec->name;
  unsigned int     count;

  fprintf (file, "%-5s", name);
  for (count = 1; count < length; count++)
    fprintf (file, (count == length - 1
		    ? " %2d"
		    : " %2d,"),
	     opcodes [count]);
}

extern void 
mh_tag_disassemble (mh_tag_t tag, FILE *file)
{
  if (! file) file = stdout;

  { /* Tags In Constants */
    mh_object_t *constants =
      MH_TAG_CONSTANTS (tag);
    unsigned int count, limit = 
      MH_TAG_CONSTANTS_COUNT (tag);
    
    for (count = 0; count < limit; count++)
      {
	mh_object_t object = constants [count];
	if (MH_TAG_P (object))
	  {
	    mh_tag_disassemble (MH_AS_TAG (object), file);
	    fprintf (file, ";;\n;;\n;;");
	  }
	else
	  {
	    ;
	  }
      }
  }

  fprintf (file, "\n");
  fprintf (file, ";; Name     : %s\n;; Address  : %8p\n;; Package  : %s\n",
	   MH_TAG_NAME (tag),
	   tag,
	   (MH_TAG_CURRENT_PACKAGE_P (tag)
	    ? "CurrentPackage"
	    : MH_TAG_PACKNAME (tag)));

  /* Args */
  {
    unsigned int   length    = MH_TAG_ARGS_COUNT (tag);
    mh_argument_t *arguments = MH_TAG_ARGS (tag);

    unsigned int offset;

    fprintf (file, ";; Args     : [");
    for (offset = 0; offset < length; offset++)
      {
	mh_argument_t argument = arguments[offset];
	fprintf (file, "%s %s %s %s",
		 MH_ARGUMENT_NAME (argument),
		 mh_argument_type_names  [MH_ARGUMENT_TYPE  (argument)],
		 mh_argument_eval_names  [MH_ARGUMENT_EVAL  (argument)],
		 mh_argument_array_names [MH_ARGUMENT_ARRAY (argument)]);
	if (offset + 1 < length)
	  fprintf (file, "\n;;          :  ");
      }
    fprintf (file, "]\n");
  }

  fprintf (file, ";; StackSize: %d", 
	   MH_TAG_STACK_SIZE (tag));

  /* Constants */
  {
    mh_object_t *constants    = MH_TAG_CONSTANTS (tag);
    unsigned int count, limit = MH_TAG_CONSTANTS_COUNT (tag);
    fprintf (file, "\n;; Constants:");
    for (count = 0; count < limit; count++)
      {
	fprintf (file, "\n;; %9d: ", count);
	mh_object_to_file (constants [count], true, file);
      }
  }

  /* Code */
  {
    mh_byte_code_t *opcodes = MH_TAG_CODE (tag);
    mh_byte_code_t *opcodes_limit =
      opcodes + MH_TAG_CODE_COUNT (tag);
    mh_byte_code_t *opcodes_base = opcodes;

    fprintf (file, "\n;; Code     :\n");
    while (opcodes < opcodes_limit)
      {
	mh_byte_code_spec_t spec =
	  & mh_byte_code_spec_table [*opcodes];
	
	mh_byte_op_len_t length  = spec->length;
	size_t offset  = opcodes - opcodes_base;

	{
	  char buf [64];

	  if (1 == length)
	    sprintf (buf, "%d", (int) offset);
	  else
	    sprintf (buf, "%d-%d", (int) offset, (int) (offset + length - 1));

	  fprintf (file, ";; %9s:  ", buf);
	}

	mh_byte_code_instr_disassemble (opcodes, file);
	fprintf (file, "\n");

	opcodes += length;
      }
  }
}

/***************************************************************************
 *
 * MH_CONS_T
 *
 */ 
mh_object_t mh_nil;

extern boolean_t
mh_alist_p (mh_object_t object)
{
  return (MH_ALIST_P (object) || MH_NIL_P (object));
}

extern unsigned int
mh_alist_length (mh_alist_t alist)
{
  unsigned int size = 0;
  for (; ! MH_NIL_P (alist); alist = MH_ALIST_TAIL (alist))
    size++;

  return size;
}

extern mh_alist_t
mh_alist_new (mh_alist_t  tail,
	      mh_object_t name,
	      mh_object_t value)
{
  mh_alist_t alist =
    (mh_alist_t) mh_object_alloc (MH_ALIST_TAG, MH_ALIST_ALLOC_SIZE);
  MH_ALIST_NAME  (alist) = name;
  MH_ALIST_VALUE (alist) = value;
  MH_ALIST_TAIL  (alist) = tail;
  return alist;
}

extern mh_alist_t
mh_alist_copy (mh_alist_t alist)
{
  return MH_NIL_P (alist)
    ? MH_AS_ALIST (mh_nil)
    : mh_alist_new (mh_alist_reverse (MH_ALIST_TAIL (alist)),
		    MH_ALIST_NAME  (alist),
		    MH_ALIST_VALUE (alist));
}

static mh_alist_t
mh_alist_reverse_1 (mh_alist_t alist,
		    mh_alist_t result)
{
  return MH_NIL_P (alist)
    ? result
    : mh_alist_reverse_1 (MH_ALIST_TAIL (alist),
			  mh_alist_new (result,
					MH_ALIST_NAME  (alist),
					MH_ALIST_VALUE (alist)));
}

extern mh_alist_t
mh_alist_reverse (mh_alist_t alist)
{ return mh_alist_reverse_1 (alist, MH_AS_ALIST (mh_nil)); }

extern mh_object_t
mh_alist_find (mh_alist_t  alist,
	       mh_object_t name)
{
  for (; ! MH_NIL_P (alist); alist = MH_ALIST_TAIL (alist))
    if (mh_object_equal (name, MH_ALIST_NAME (alist)))
      return MH_AS_OBJECT (alist);
  return mh_nil;
}

static mh_object_t
mh_alist_vector_merge (mh_object_t object1,
		       mh_object_t object2);

extern mh_alist_t
mh_alist_fill (mh_object_t *names,
	       mh_object_t *values,
	       unsigned int count)
{
  mh_alist_t alist = MH_AS_ALIST (mh_nil);

  while (count--)
    {
      mh_object_t name  = *names++;
      mh_object_t value = *values++;

      /* Look for a preexisting KEY in ALIST */
      mh_object_t find = mh_alist_find (alist, name);

      if (MH_NIL_P (find))
	alist = mh_alist_new (alist, name, value);
      else
	MH_ALIST_VALUE (MH_AS_ALIST (find)) = 
	  mh_alist_vector_merge (find, value);
    }
  return alist;
}

extern mh_alist_t
mh_alist_fill_pairs (mh_object_t *names_and_values,
		     unsigned int count)
{
  mh_alist_t alist = MH_AS_ALIST (mh_nil);

  while (count--)
    {
      mh_object_t name  = *names_and_values++;
      mh_object_t value = *names_and_values++;

      /* Look for a preexisting KEY in ALIST */
      mh_object_t find = mh_alist_find (alist, name);

      if (MH_NIL_P (find))
	alist = mh_alist_new (alist, name, value);
      else
	MH_ALIST_VALUE (MH_AS_ALIST (find)) = 
	  mh_alist_vector_merge (find, value);
    }
  return mh_alist_reverse (alist);
}

extern boolean_t
mh_alist_has (mh_alist_t  alist,
	      mh_object_t name)
{
  return ! MH_NIL_P (mh_alist_find (alist, name));
}

extern mh_alist_t
mh_alist_rem (mh_alist_t alist,
	      mh_object_t name)
{
  mh_alist_t the_alist = alist;

  if (MH_NIL_P (alist))		/* Errors if no ALIST */
    return alist;
  else if (name == MH_ALIST_NAME (alist))
    return MH_ALIST_TAIL (alist);
  else
    {
      while (! MH_NIL_P (MH_ALIST_TAIL (alist)))
	{
	  mh_alist_t tail = MH_ALIST_TAIL (alist);
	  if (mh_object_equal (name, MH_ALIST_NAME (tail)))
	    {
	      /* Splice it out */
	      MH_ALIST_TAIL (alist) = MH_ALIST_TAIL (tail);
		
	      break /* while */ ;
	    }
	  alist = MH_ALIST_TAIL (alist);
	}
      return the_alist;
    }
}

extern mh_object_t
mh_alist_get (mh_alist_t alist,
	      mh_object_t name)
{
  mh_object_t find = mh_alist_find (alist, name);
  return MH_NIL_P (find)
    ? MH_OBJECT_NULL
    : MH_ALIST_VALUE (MH_AS_ALIST (find));
}

extern mh_alist_t
mh_alist_set (mh_alist_t  alist,
	      mh_object_t name,
	      mh_object_t value)
{
  mh_alist_t the_alist = alist;

  for (; ! MH_NIL_P (alist); alist = MH_ALIST_TAIL (alist))
    if (mh_object_equal (name, MH_ALIST_NAME (alist)))
      {
	mh_object_t cons_value = MH_ALIST_VALUE (alist);

	/* When VALUE is either a VECTOR or CONS we assign VALUE
	   directly to NAME in ALIST. */
	if (MH_VECTOR_P (value) /* || MH_CONS_P (value) */ )
	  MH_ALIST_VALUE (alist) = value;

#if 0
	/* If CONS_VALUE is itself a list replace its CAR with VALUE */
	else if (MH_CONS_P (cons_value))
	  MH_CAR (cons_value) = value;
#endif
	/* If CONS_VALUE is itself a vector, replace item zero with value */

	/* Note all vectors are length TWO or greater */
	else if (MH_VECTOR_P (cons_value) &&
		 1 <= MH_VECTOR_LENGTH (MH_AS_VECTOR (cons_value)))
	  MH_VECTOR_REF (MH_AS_VECTOR (cons_value), 0) = value;
	else
	  MH_ALIST_VALUE (alist) = value;

	return the_alist;
      }
  return mh_alist_new (the_alist, name, value);
}

extern mh_alist_t
mh_alist_merge (mh_alist_t alist1,
		mh_alist_t alist2)
{
  for (; ! MH_NIL_P (alist1); alist1 = MH_ALIST_TAIL (alist1))
    if (! mh_alist_has (alist2, MH_ALIST_NAME (alist1))) /* Sharing CONS */
      alist2 = mh_alist_new 
	(alist2,
	 MH_ALIST_NAME  (alist1),
	 MH_ALIST_VALUE (alist1));
  return alist2;
}

extern mh_vector_t
mh_alist_names (mh_alist_t alist)
{
  unsigned int offset;
  unsigned int length = mh_alist_length (alist);
  mh_vector_t  vector =	mh_vector_new  (length);

  for (offset = 0; offset < length; offset++)
	{
	  MH_VECTOR_REF (vector, offset) = MH_ALIST_NAME (alist);
	  alist = MH_ALIST_TAIL (alist);
	}
  return vector;
}

extern mh_vector_t
mh_alist_values (mh_alist_t alist)
{
  unsigned int offset;
  unsigned int length = mh_alist_length (alist);
  mh_vector_t  vector =	mh_vector_new  (length);

  for (offset = 0; offset < length; offset++)
	{
	  MH_VECTOR_REF (vector, offset) = MH_ALIST_VALUE (alist);
	  alist = MH_ALIST_TAIL (alist);
	}
  return vector;
}

static mh_object_t
mh_alist_vector_merge (mh_object_t object1,
		       mh_object_t object2)
{
  if (MH_VECTOR_P (object1) && MH_VECTOR_P (object2))
    {
      unsigned int count = 0;
      unsigned int limit =
	MIN (MH_VECTOR_LENGTH (MH_AS_VECTOR (object1)),
	     MH_VECTOR_LENGTH (MH_AS_VECTOR (object2)));

      if (1 == limit)
	return mh_alist_vector_merge
	  ((1 == MH_VECTOR_LENGTH (MH_AS_VECTOR (object1))
	    ? MH_VECTOR_REF (MH_AS_VECTOR (object1), 0)
	    : object1),
	   (1 == MH_VECTOR_LENGTH (MH_AS_VECTOR (object2))
	    ? MH_VECTOR_REF (MH_AS_VECTOR (object2), 0)
	    : object2));
	   
      for (; count < limit; count++)
	if (MH_EMPTY_P (MH_VECTOR_REF (MH_AS_VECTOR (object2), count)))
	  MH_VECTOR_REF (MH_AS_VECTOR (object2), count) =
	    MH_VECTOR_REF (MH_AS_VECTOR (object1), count);

      return object2;
    }
  else if (MH_VECTOR_P (object2))
    {
      if (0 != MH_VECTOR_LENGTH (MH_AS_VECTOR (object2)) &&
	  MH_EMPTY_P (MH_VECTOR_REF (MH_AS_VECTOR (object2), 0)))
	MH_VECTOR_REF (MH_AS_VECTOR (object2), 0) = object1;
      return object2;
    }
  else if (MH_VECTOR_P (object1))
    {
      if (0 == MH_VECTOR_LENGTH (MH_AS_VECTOR (object1)))
	return object2;
      else
	{
	  MH_VECTOR_REF (MH_AS_VECTOR (object1), 0) = object2;
	  return object1;
	}
    }
  else 
    return object2;
}


extern mh_object_t 
mh_object_to_alist (mh_object_t object)
{
  if (mh_alist_p (object))
    return object;

#if defined (CONVERSION_TRACING)
  printf ("mh_object_to_alist()\n");
#endif
  switch (MH_OBJECT_TAG (object))
    {
    case MH_STRING_TAG:
      {
	string_t string = MH_STRING_CHARS (MH_AS_STRING (object));

	return string && 0 != strcmp (string, "")
	  ? mh_object_read (string)
	  : MH_NIL;
      }

    case MH_NUMBER_TAG:
    case MH_FUNCTION_TAG:
      return MH_NIL;

    case MH_VECTOR_TAG:
      return 0 != MH_VECTOR_LENGTH (MH_AS_VECTOR (object))
	? mh_object_to_alist (MH_VECTOR_REF (MH_AS_VECTOR (object), 0))
	: MH_NIL;

    case MH_NIL_TAG:
    case MH_ALIST_TAG:
      return object;

    default:
      return MH_NIL;
    }
}


/***************************************************************************
 *
 * MH_VECTOR_T
 *
 */ 
extern mh_vector_t
mh_vector_new (size_t length)
{
  unsigned int offset;
  mh_vector_t vector = (mh_vector_t) 
    mh_object_alloc (MH_VECTOR_TAG, MH_VECTOR_ALLOC_SIZE (length));
  MH_VECTOR_LENGTH (vector) = length;
  for (offset = 0; offset < length; offset++)
    vector->values[offset] = MH_OBJECT_EMPTY;
  return vector;
}

extern size_t
mh_vector_length (mh_vector_t vector)
{
  return MH_VECTOR_LENGTH (vector);
}

extern mh_object_t
mh_vector_ref (mh_vector_t vector,
	       mh_object_t offset)
{
  if (MH_NUMBER_P (offset))
    {
      int indx   = (int)
	MH_NUMBER_VALUE (MH_AS_NUMBER (offset));

      return indx >= 0 && indx < MH_VECTOR_LENGTH (vector)
	? MH_VECTOR_REF (vector, indx)
	: MH_OBJECT_EMPTY;
    }

  return mh_vector_ref (vector, MH_AS_OBJECT (mh_object_to_number (offset)));
}

extern void
mh_vector_set (mh_vector_t vector,
	       mh_object_t offset,
	       mh_object_t value)
{
  if (MH_NUMBER_P (offset))
    {
      mh_number_t number = MH_AS_NUMBER (offset);
      int         indx   = MH_NUMBER_VALUE (number);
	
      /* Check bounds */
      MH_VECTOR_REF (vector, indx) = value;
      return;
    }
  mh_vector_set (vector, MH_AS_OBJECT (mh_object_to_number (offset)), value);
}

static void
mh_vector_dup (mh_vector_t target,
	       mh_vector_t source)
{
  unsigned int indx  = 0;
  unsigned int count =
    MIN (MH_VECTOR_LENGTH (target),
	 MH_VECTOR_LENGTH (source));

  for (; indx < count; indx++)
    MH_VECTOR_REF (target, indx) =
      MH_VECTOR_REF (source, indx);

  return;
}


extern mh_vector_t
mh_vector_extend  (mh_vector_t vector,
		   mh_object_t offset,
		   mh_object_t value)
{
  if (MH_NUMBER_P (offset))
    {
      mh_number_t number = MH_AS_NUMBER (offset);
      int         indx   = MH_NUMBER_VALUE (number);

      if (!vector || indx >= MH_VECTOR_LENGTH (vector))
	{
	  mh_vector_t result = mh_vector_new (1 + indx);
	  if (vector) mh_vector_dup (result, vector);
	  vector = result;
	}

      MH_VECTOR_REF (vector, indx) = value;
      return (vector);
    }
  else 
    return (mh_vector_extend
	    (vector, MH_AS_OBJECT (mh_object_to_number (offset)), value));
}
  

extern mh_object_t
mh_vector_member (mh_vector_t vector,
		  mh_object_t item,
		  boolean_t   caseless_p)
{
  unsigned int count   = 0;
  unsigned int size    = MH_VECTOR_LENGTH (vector);
  mh_object_t *objects = MH_VECTOR_VALUES (vector);

  for (; count < size; count++)
    if ((caseless_p ? mh_object_equal : mh_object_equal_with_case)
	(item, objects [count]))
      return MH_AS_OBJECT (mh_number_new ((long double) count));
  return MH_OBJECT_EMPTY;
}

extern mh_object_t
mh_vector_append (mh_vector_t vector,
		  mh_object_t item)
{
  unsigned int size    = 1 + MH_VECTOR_LENGTH (vector);

  if (MH_OBJECT_EMPTY == item) item = MH_AS_OBJECT (mh_string_new (""));

  return (mh_vector_extend
	  (vector, MH_AS_OBJECT (mh_number_new ((long double) size)), item));
}

extern void
mh_vector_reverse_inplace (mh_vector_t vector)
{
  unsigned int size  = MH_VECTOR_LENGTH (vector);
  unsigned int count = 0;
  mh_object_t *objects = MH_VECTOR_VALUES (vector);

  while (count < size/2)
    {
      mh_object_t temp           = objects [count];
      objects [count]            = objects [size - 1 - count];
      objects [size - 1 - count] = temp;
      count++;
    }
  return;
}

extern mh_vector_t
mh_vector_reverse (mh_vector_t vector)
{
  unsigned int size   = MH_VECTOR_LENGTH (vector);
  unsigned int count  = 0;
  mh_vector_t  result = mh_vector_new (size);

  for (; count < size; count++)
    MH_VECTOR_REF (result, count) = 
      MH_VECTOR_REF (vector, size - 1 - count) ;

  return result;
}

extern mh_vector_t
mh_vector_fill (mh_object_t *objects,
		size_t       length)
{
  unsigned int offset;
  mh_vector_t vector = (mh_vector_t) 
    mh_object_alloc (MH_VECTOR_TAG, MH_VECTOR_ALLOC_SIZE (length));
  MH_VECTOR_LENGTH (vector) = length;
  for (offset = 0; offset < length; offset++)
    vector->values[offset] = *objects++;
  return vector;
}

extern mh_object_t
mh_object_to_vector (mh_object_t object)
{
  /* Only STRINGs can become vectors */
  return MH_STRING_P (object)
    ? mh_string_to_vector (MH_AS_STRING (object), "\n")
    : object;
}

extern mh_object_t
mh_array_length (mh_vector_t vector)
{
  return MH_AS_OBJECT (mh_number_new (MH_VECTOR_LENGTH (vector)));
}

extern mh_object_t
mh_vector_unop (mh_object_t object,
		mh_vector_unop_fun_t fun)
{
  mh_object_t vector_maybe = mh_object_to_vector (object);
  return MH_VECTOR_P (vector_maybe)
    ? (*fun) (MH_AS_VECTOR (vector_maybe))
    : vector_maybe;
}

/***************************************************************************
 *
 * MH_OBJECT_T
 *
 */ 
extern void
mh_object_init (void)
{
  if (! MH_EMPTY)
    {
      /* Fails when LENGTH argument is zero; use 1 */
      MH_EMPTY = mh_string_buffer ("", 1);
      mh_memory_add_root ((mh_object_t *) & MH_EMPTY);

      MH_TRUE  = mh_string_buffer ("true", 0);
      mh_memory_add_root ((mh_object_t *) & MH_TRUE);

      mh_nil = mh_object_alloc (MH_NIL_TAG, MH_OBJECT_ALLOC_SIZE);
      mh_memory_add_root ((mh_object_t *) & mh_nil);

      MH_NUMBER_ZERO = mh_number_new (0.0);
      mh_memory_add_root ((mh_object_t *) & MH_NUMBER_ZERO);

      MH_NUMBER_ONE = mh_number_new (1.0);
      mh_memory_add_root ((mh_object_t *) & MH_NUMBER_ONE);

      MH_NUMBER_TWO = mh_number_new (2.0);
      mh_memory_add_root ((mh_object_t *) & MH_NUMBER_TWO);
    }
}

static boolean_t
mh_object_equal_1 (mh_object_t object1,
		   mh_object_t object2,
		   boolean_t   caseless_p)
{
#define recurse( obj1, obj2)   mh_object_equal_1 (obj1, obj2, caseless_p)

  mh_type_t tag1, tag2;

  if (object1 == object2)
    return true;

  tag1 = MH_OBJECT_TAG (object1);
  tag2 = MH_OBJECT_TAG (object2);

#if defined (CONVERSION_TRACING)
  if (tag1 != tag2)
    printf ("mh_object_to_equal()\n");
#endif

  if (tag1 != tag2)
    {
      string_t  str1 = mh_object_to_string (object1, false);
      string_t  str2 = mh_object_to_string (object2, false);
      boolean_t res  =
	(0 == (caseless_p ? strcasecmp : strcmp) (str1, str2));
      
      xfree (str1);
      xfree (str2);
      return res;
    }

  switch (tag1)
    {
    case MH_STRING_TAG:
      return 0 == ((caseless_p ? strcasecmp : strcmp)
		   (MH_STRING_CHARS (MH_AS_STRING (object1)),
		    MH_STRING_CHARS (MH_AS_STRING (object2))));

    case MH_NUMBER_TAG:
      return (MH_NUMBER_VALUE (MH_AS_NUMBER (object1)) ==
	      MH_NUMBER_VALUE (MH_AS_NUMBER (object2)));

    case MH_FUNCTION_TAG:
      return false;

    case MH_VECTOR_TAG:
      {
	mh_vector_t vector1 = MH_AS_VECTOR (object1);
	mh_vector_t vector2 = MH_AS_VECTOR (object2);
	
	if (MH_VECTOR_LENGTH (vector1) != MH_VECTOR_LENGTH (vector2))
	  return false;

	{
	  mh_object_t *vals1 = MH_VECTOR_VALUES (vector1);
	  mh_object_t *vals2 = MH_VECTOR_VALUES (vector2);
	  unsigned int indx  = MH_VECTOR_LENGTH (vector1);
	
	  while (indx-- > 0)
	    if (false == recurse (vals1[indx], vals2[indx]))
	      return false;

	  return true;
	}
      }
    case MH_NIL_TAG:
      return true;

    case MH_ALIST_TAG:
      {
	mh_alist_t alist1 = MH_AS_ALIST (object1);
	mh_alist_t alist2 = MH_AS_ALIST (object2);

	return
	  recurse (MH_ALIST_NAME  (alist1), MH_ALIST_NAME  (alist2)) &&
	  recurse (MH_ALIST_VALUE (alist1), MH_ALIST_VALUE (alist2)) &&
	  recurse (MH_AS_OBJECT (MH_ALIST_TAIL (alist1)),
		   MH_AS_OBJECT (MH_ALIST_TAIL (alist2)));
      }

    default:
      return false;
    }
#undef recurse
}

extern boolean_t
mh_object_equal (mh_object_t object1,
		 mh_object_t object2)
{
  return mh_object_equal_1 (object1, object2, true);
}

extern boolean_t
mh_object_equal_with_case (mh_object_t object1,
			   mh_object_t object2)
{
  return mh_object_equal_1 (object1, object2, false);
}


/*********************************************************************
 *
 * string_to_X()
 *
 *
 */
static mh_object_t
mh_string_to_object (string_t *string_ptr);

/* Remove newlines and, critically, leading whitespace */
extern mh_object_t
mh_string_to_vector (mh_string_t string, char *delimiters)
{
  string_t start_chars = NULL;
  string_t chars = MH_STRING_CHARS (string);
  unsigned int length = 0;
  mh_vector_t vector;

  while (*chars)
    {
      if (index (delimiters, *chars))
	{
	  if (start_chars)
	    {
	      length++;
	      start_chars = NULL;
	    }
	}
      else
	{
	  if (! start_chars)
	    start_chars = chars;
	}
      chars++;
    }
  if (start_chars) length++;

  /* No vectors with a length of one or zero */
  if (0 == length)
    return MH_OBJECT_EMPTY;
  else if (1 == length)
    return MH_AS_OBJECT (string);

  /* else */
  vector = mh_vector_new (length);

  length = 0;
  start_chars = NULL;
  chars = MH_STRING_CHARS (string);

  while (*chars)
    {
      if (index (delimiters, *chars))
	{
	  if (start_chars)
	    {
	      MH_VECTOR_VALUES (vector) [length++] =
		MH_AS_OBJECT (mh_string_buffer (start_chars, 
						chars - start_chars));
	      start_chars = NULL;
	    }
	}
      else
	{
	  if (! start_chars)
	    start_chars = chars;
	}
      chars++;
    }
  if (start_chars)
    MH_VECTOR_VALUES (vector) [length] = 
      MH_AS_OBJECT (mh_string_new (start_chars));

  return MH_AS_OBJECT (vector);
}

static void
mh_string_to_alist_pair_name (string_t    *string_ptr,
			      mh_object_t *name)
{
  *name = mh_string_to_object (string_ptr);
}

static void
mh_string_to_alist_pair_value (string_t    *string_ptr,
			       mh_object_t *value)
{
  *value = mh_string_to_object (string_ptr);

  /* *VALUE might be null if first non-whitespace character is ')' */
  if (*value == NULL)
    *value = MH_OBJECT_EMPTY;

  if (**string_ptr != ')')
    {
      unsigned int value_indx =  0;
      unsigned int value_size = 10;
      mh_object_t *values = (mh_object_t *) 
	xmalloc (value_size * sizeof (mh_object_t));

      values[value_indx++] = *value;

      while (**string_ptr != ')')
	{
	  if (value_indx == value_size)
	    values = (mh_object_t *)
	      xrealloc (values, 
			(value_size += 10) * sizeof (mh_object_t));

	  values [value_indx++] = mh_string_to_object (string_ptr);
	}

      /* Never make a VECTOR of length one */
      *value = (1 == value_indx
		? values [0]
		: MH_AS_OBJECT (mh_vector_fill (values, value_indx)));

      xfree (values);
    }
}
  
static boolean_t
mh_string_to_alist_pair (string_t    *string_ptr,
			 mh_object_t *name,
			 mh_object_t *value)
{
  if (**string_ptr == '\0')
    return false;

  /* Look for '(' <name> <value1> <value2> ... ')' */

  if (**string_ptr != '(')
    return false;

  ++*string_ptr;

  mh_string_to_alist_pair_name (string_ptr, name);

  /* Skip whitespace, a period, and more whitespace */
  while (**string_ptr && 0 != index ("\n\r\t\f ", **string_ptr))
    ++*string_ptr;
  if (**string_ptr == '.')
    ++*string_ptr;

  mh_string_to_alist_pair_value (string_ptr, value);

  if (**string_ptr == ')')
    ++*string_ptr;

  return true;
}

static mh_alist_t
mh_string_to_alist_iter (string_t *string_ptr)
{
  mh_alist_t  result = MH_AS_ALIST (mh_nil);
  mh_object_t name, value;

  /* First character is guaranteed, actually */
  if (**string_ptr == '(')
    ++*string_ptr;

  /* Otherwise, read name-value pairs and extend */
  while (mh_string_to_alist_pair (string_ptr, &name, &value))
    result = mh_alist_new (result, name, value);

  if (**string_ptr == ')')
    ++*string_ptr;

  return mh_alist_reverse (result);
}
  
static mh_alist_t
mh_string_to_alist (string_t *string_ptr)
{
  return mh_string_to_alist_iter (string_ptr);
}

static mh_object_t
mh_string_to_string (string_t *string_ptr)
{
  string_t str = *string_ptr;
  string_t str_start = ++str;
  mh_string_t result;

  while (*str && *str != '"')
    if (*str++ == '\\')
      /* Skip next character, no matter what it is! */
      if (*str) str++;

  /* If there is no matching trailing doublequote then figure the
     first doublequote should not be skipped */
  if (! *str)
    str_start--;

  {
    static char *temp = (char *)NULL;
    static int temp_size = 0;
    int len = str - str_start;

    if (len + 2 > temp_size)
      temp = (char *)xrealloc (temp, temp_size = (len + 100));

    strncpy (temp, str_start, len);
    temp[len] = '\0';
    result = mh_string_buffer (temp, len);
  }

  mh_string_to_string_escaped (result->chars);

  /* Skip trailing doublequote */
  if (*str) str++;
  *string_ptr = str;

  return MH_AS_OBJECT (result);
}

static mh_object_t
mh_string_to_thing (string_t *string_ptr)
{
  /* Ignore quoting! */
  string_t delimiters = "\n\r\t\f ()\"";

  string_t str = *string_ptr;
  string_t str_start = str;

  while (0 == index (delimiters, *str)) str++;

  *string_ptr = str;

  /* Try to make a number */
  {
    mh_string_t string = mh_string_buffer (str_start, str - str_start);
#if defined (READ_NUMBERS_TOO)
    string_t str_end;
    double      value  =
      strtod (MH_STRING_CHARS (string), &str_end);

    if (str_end && 0 == *str_end)
      {
	/* Free string */
	return MH_AS_OBJECT (mh_number_new (value));
      }
    else
#endif
      return MH_AS_OBJECT (string);
  }
}

/* Modifies *string_ptr to point to the character just beyond the last
   character contributing to the returned OBJECT.  Thus if
   **string_ptr is '\0' the entire string was read; otherwise
   something remains to be read. */
static mh_object_t
mh_string_to_object (string_t *string_ptr)
{
  /* string_ptr points to a string that:
        has already been escaped,
	is not bounded by double quotes */
  string_t string = *string_ptr;

  switch (*string)
    {
    case '\000':
      return MH_OBJECT_EMPTY;

      /* Whitespace */
    case '\n':
    case '\r':
    case '\t':
    case '\f':
    case ' ':
      while (*string && 0 != index ("\n\r\t\f ", *string)) string++;
      *string_ptr = string;
      return mh_string_to_object (string_ptr);

      /* Comment */
    case ';':
      while (*string && '\n' != *string) string++;
      *string_ptr = string;
      return mh_string_to_object (string_ptr);

      /* String */
    case '\"':
      return mh_string_to_string (string_ptr);

      /* Alist */
    case '(':
      return MH_AS_OBJECT (mh_string_to_alist (string_ptr));

    case ')':
      /* error */
      return NULL;

      /* Symbol or Number ... (but no symbols!) */
    default:
      return mh_string_to_thing (string_ptr);
    }
}

extern mh_object_t
mh_object_read (string_t string)
{
  string_t    string_save = string;
  mh_object_t object;

  object = mh_string_to_object (&string);

  return *string
    ? MH_AS_OBJECT (mh_string_new (string_save)) /* Worst case */
    : object;
}

static mh_number_t
mh_object_read_number (mh_string_t string)
{
  /* Ignore quoting! */
  string_t delimiters = "\n\r\t\f ()\"";

  string_t str = MH_STRING_CHARS (string);
  string_t str_end;

  double value;

  /* Skip WHITESPACE and more ... */
  while (0 != index (delimiters, *str)) str++;

  /* Try to make a number */
  value = strtod (str, &str_end);

  return str_end && 0 == *str_end
    ? mh_number_new (value)
    : MH_AS_NUMBER (NULL);
}

/****************************************************************
 *
 * mh_object_to_{buffer,string,file} ()
 *
 *
 *
 */
typedef enum
{
  MH_FOP_STRING,
  MH_FOP_EMPTY,
  MH_FOP_NUMBER,
  MH_FOP_VECTOR,
  MH_FOP_NIL,
  MH_FOP_ALIST,
  MH_FOP_TAG
} mh_fop_tag_t;

static void
mh_object_serialize (mh_object_t object,
		     BPRINTF_BUFFER *b);

static mh_object_t
mh_object_restore (string_t *data);

static void
mh_char_serialize (char value,
		   BPRINTF_BUFFER *b)
{
  bprintf_append_binary (b, (char *) & value, 1);
}

static char
mh_char_restore (string_t *data)
{
  string_t str = *data;
  *data += 1;
  return (mh_fop_tag_t) str[0];
}


static inline void
mh_fop_serialize (mh_fop_tag_t fop,
		  BPRINTF_BUFFER *b)
{ mh_char_serialize ((char) fop, b); }

static inline mh_fop_tag_t
mh_fop_restore (string_t *data)
{ return (mh_fop_tag_t) mh_char_restore (data); }
  
static void
mh_int_serialize (int value,
		  BPRINTF_BUFFER *b)
{
  unsigned long int canonical =
    htonl ((unsigned long int) value);

  bprintf_append_binary (b,
			 (char *) & canonical,
			 sizeof (unsigned long int));
}

static int
mh_int_restore (string_t *data)
{
  string_t string = *data;

  unsigned long int *value = (unsigned long int *) string;

  *data += sizeof (unsigned long int);
  return ntohl (*value);
}

static void
mh_chars_serialize (char  *chars,
		    size_t chars_count,
		    BPRINTF_BUFFER *b)
{
  mh_int_serialize (chars_count, b);
  bprintf_append_binary (b, (char *) chars, chars_count);
}

static void
mh_chars_restore (string_t *chars,
		  size_t   *chars_count,
		  string_t *data)
{
  *chars_count = mh_int_restore (data);
  *chars = *data;
  *data += *chars_count;
}

static void
mh_double_serialize (double value,
		     BPRINTF_BUFFER *b)
{
  /* Yea, right */
  bprintf_append_binary (b, (char *) & value, sizeof (double));
}

static double
mh_double_restore (string_t *data)
{
  double *value = (double *) *data;
  *data += sizeof (double);
  return *value;
}

static void
mh_argument_serialize (mh_argument_t arg,
		       BPRINTF_BUFFER *b)
{
  string_t name = MH_ARGUMENT_NAME  (arg);
  mh_chars_serialize (name, strlen (name), b);

  mh_char_serialize ((char) MH_ARGUMENT_TYPE  (arg), b);
  mh_char_serialize ((char) MH_ARGUMENT_EVAL  (arg), b);
  mh_char_serialize ((char) MH_ARGUMENT_ARRAY (arg), b);
}

static mh_argument_t
mh_argument_restore (string_t *data)
{
  string_t name, name_chars;
  size_t   name_size;

  mh_argument_type_t  type;
  mh_argument_eval_t  eval;
  mh_argument_array_t array;

  mh_chars_restore (&name_chars, &name_size, data);
  name = strndup (name_chars, name_size);

  type  = (mh_argument_type_t)  mh_char_restore (data);
  eval  = (mh_argument_eval_t)  mh_char_restore (data);
  array = (mh_argument_array_t) mh_char_restore (data);

  return mh_argument_new
    (name, type, eval, array);
}

static void
mh_string_serialize (mh_string_t string,
		     BPRINTF_BUFFER *b)
{
  if (MH_EMPTY_P (string))
    mh_fop_serialize (MH_FOP_EMPTY, b);
  else
    {
      mh_fop_serialize   (MH_FOP_STRING, b);
      mh_chars_serialize (MH_STRING_CHARS  (string),
			  MH_STRING_LENGTH (string),
			  b);
    }
}

static mh_string_t
mh_empty_restore (string_t *data)
{ return MH_EMPTY; }

static mh_string_t
mh_string_restore (string_t *data)
{
  char  *string;
  size_t string_length;

  mh_chars_restore (&string, &string_length, data);

  return mh_string_buffer (string, string_length);
}
  
static void
mh_number_serialize (mh_number_t number,
		     BPRINTF_BUFFER *b)
{
  mh_fop_serialize    (MH_FOP_NUMBER, b);
  mh_double_serialize (MH_NUMBER_VALUE (number), b);
}

static mh_number_t
mh_number_restore (string_t *data)
{
  return mh_number_new (mh_double_restore (data));
}

static void
mh_vector_serialize (mh_vector_t vector,
		     BPRINTF_BUFFER *b)
{
  unsigned int count   = MH_VECTOR_LENGTH (vector);
  mh_object_t *objects = MH_VECTOR_VALUES (vector);
    

  mh_fop_serialize (MH_FOP_VECTOR, b);
  mh_int_serialize (count, b);

  while (count--)
    mh_object_serialize (*objects++, b);
}

static mh_vector_t
mh_vector_restore (string_t *data)
{
  unsigned int count   = mh_int_restore   (data);
  mh_vector_t  vector  = mh_vector_new    (count);
  mh_object_t *objects = MH_VECTOR_VALUES (vector);

  while (count--)
    *objects++ = mh_object_restore (data);

  return vector;
}

static void
mh_alist_serialize (mh_alist_t alist,
		   BPRINTF_BUFFER *b)
{
  mh_fop_serialize    (MH_FOP_ALIST,   b);
  mh_object_serialize (MH_ALIST_NAME  (alist), b);
  mh_object_serialize (MH_ALIST_VALUE (alist), b);
  mh_object_serialize (MH_AS_OBJECT (MH_ALIST_TAIL (alist)), b);
}

static mh_alist_t
mh_alist_restore (string_t *data)
{
  mh_object_t name  = mh_object_restore (data);
  mh_object_t value = mh_object_restore (data);
  mh_object_t tail  = mh_object_restore (data);
  return mh_alist_new (MH_AS_ALIST (tail), name, value);
}
  
static void
mh_nil_serialize (BPRINTF_BUFFER *b)
{
  mh_fop_serialize (MH_FOP_NIL, b);
}

static mh_object_t
mh_nil_restore (string_t *data)
{ return mh_nil; }

static void
mh_tag_serialize (mh_tag_t tag,
		  BPRINTF_BUFFER *b)
{
  mh_fop_serialize (MH_FOP_TAG, b);

  mh_char_serialize (MH_TAG_TYPE              (tag), b);
  mh_char_serialize (MH_TAG_COMPLEX_P         (tag), b);
  mh_char_serialize (MH_TAG_WEAK_P            (tag), b);
  mh_char_serialize (MH_TAG_WHITESPACE        (tag), b);
  mh_char_serialize (MH_TAG_CURRENT_PACKAGE_P (tag), b);

  mh_chars_serialize (MH_TAG_NAME (tag), 
		      strlen (MH_TAG_NAME (tag)),
		      b);

  {
    unsigned int   args_count = MH_TAG_ARGS_COUNT (tag);
    mh_argument_t *args       = MH_TAG_ARGS (tag);

    mh_int_serialize (args_count, b);

    while (args_count--)
      mh_argument_serialize (*args++, b);
  }

  mh_int_serialize (MH_TAG_STACK_SIZE (tag), b);
  
  mh_chars_serialize (MH_TAG_CODE (tag),
		      MH_TAG_CODE_COUNT (tag),
		      b);

  mh_object_serialize (MH_TAG_CONSTANTS_VECTOR (tag), b);
}

static mh_tag_t
mh_tag_restore (string_t *data)
{
  mh_tag_t        tag;
  mh_tag_type_t   type         = (mh_tag_type_t)   mh_char_restore (data);
  boolean_t       complex_p    = (boolean_t)       mh_char_restore (data);
  boolean_t       weak_p       = (boolean_t)       mh_char_restore (data);
  mh_white_type_t whitespace   = (mh_white_type_t) mh_char_restore (data);
  boolean_t       crnt_pack_p  = (boolean_t)       mh_char_restore (data);

  string_t name;
  string_t name_chars;
  size_t   name_length;

  size_t         args_count;
  mh_argument_t *args;

  size_t stack_size;

  size_t          byte_code_count;
  mh_byte_code_t *byte_code_chars;
  mh_byte_code_t *byte_code;

  mh_vector_t constants;

  mh_chars_restore (&name_chars, &name_length, data);
  name = strndup (name_chars, name_length);

  {
    unsigned int count = 0;

    args_count = mh_int_restore (data);
    args       = xcalloc (args_count, sizeof (mh_argument_t));

    for (; count < args_count; count++)
      args [count] = mh_argument_restore (data);
  }

  stack_size = mh_int_restore (data);

  mh_chars_restore ((string_t *) &byte_code_chars, &byte_code_count, data);
  byte_code = strndup (byte_code_chars, byte_code_count);

  constants = MH_AS_VECTOR (mh_object_restore (data));

  tag = mh_tag_new
    (name,
     type,
     complex_p,
     weak_p,
     whitespace,
     "No Body",
     NULL,
     "No Documentation",
     args,
     args_count);

  MH_TAG_CURRENT_PACKAGE_P (tag) = crnt_pack_p;

  mh_tag_install_machine (tag, 
			  byte_code, byte_code_count,
			  MH_VECTOR_VALUES (constants),
			  MH_VECTOR_LENGTH (constants),
			  stack_size);

  return tag;
}

static void
mh_object_serialize (mh_object_t object,
			      BPRINTF_BUFFER *b)
{
  switch (MH_OBJECT_TAG (object))
    {
    case MH_STRING_TAG:
      mh_string_serialize (MH_AS_STRING (object), b);
      break;

    case MH_NUMBER_TAG:
      mh_number_serialize (MH_AS_NUMBER (object), b);
      break;

    case MH_FUNCTION_TAG:
      mh_tag_serialize (MH_AS_TAG (object), b);
      break;

    case MH_VECTOR_TAG:
      mh_vector_serialize (MH_AS_VECTOR (object), b);
      break;

    case MH_NIL_TAG:
      mh_nil_serialize (b);
      break;

    case MH_ALIST_TAG:
      mh_alist_serialize (MH_AS_ALIST (object), b);
      break;

    default:
      break;
    }
}

static mh_object_t
mh_object_restore (char **data)
{
  mh_fop_tag_t fop = mh_fop_restore (data);

  switch (fop)
    {
    case MH_FOP_STRING: return MH_AS_OBJECT (mh_string_restore (data));
    case MH_FOP_EMPTY:  return MH_AS_OBJECT (mh_empty_restore  (data));
    case MH_FOP_NUMBER: return MH_AS_OBJECT (mh_number_restore (data));
    case MH_FOP_VECTOR: return MH_AS_OBJECT (mh_vector_restore (data));
    case MH_FOP_NIL:    return MH_AS_OBJECT (mh_nil_restore    (data));
    case MH_FOP_ALIST:  return MH_AS_OBJECT (mh_alist_restore  (data));
    case MH_FOP_TAG:    return MH_AS_OBJECT (mh_tag_restore    (data));
    default:
      return NULL;
    }
}
  
extern mh_object_t
mh_buffer_serialize_to_object (BPRINTF_BUFFER *b)
{
  string_t data = b->buffer;
  return mh_object_restore (&data);
}

extern mh_object_t
mh_file_serialize_to_object (FILE *file)
{
  return NULL;
}
		   

extern BPRINTF_BUFFER *
mh_object_serialize_to_buffer (mh_object_t object)
{
  BPRINTF_BUFFER *b = 
    bprintf_create_buffer ();

  mh_object_serialize (object, b);

  return b;
}

extern void
mh_object_serialize_to_file (mh_object_t object,
			     FILE *file)
{
  BPRINTF_BUFFER *b = 
    bprintf_create_buffer ();
  
  mh_object_serialize (object, b);

  fwrite (b->buffer, 1, b->bindex, file);

  return;
}

/****************************************************************
 *
 * mh_object_to_{buffer,string,file} ()
 *
 *
 *
 * Note that vector_spacer and quote_depth are potentially coupled. */
static void
mh_object_to_buffer_internal (mh_object_t  object,
			      unsigned int quote_depth,
			      BPRINTF_BUFFER *b,
			      string_t    vector_spacer,
			      string_t    alist_spacer,
			      boolean_t   str_upcase_p)
{
  string_t quotes =
    mh_string_quotes_at_depth (quote_depth);

  switch (MH_OBJECT_TAG (object))
    {
    case MH_STRING_TAG:
      {
	string_t result = mh_string_to_string_quoted_to_depth
	  (MH_STRING_CHARS (MH_AS_STRING (object)), quote_depth);

	if (str_upcase_p)
	  mh_string_upcase_internal (result, result);

	bprintf (b, "%s%s%s", quotes, result, quotes);

	xfree (result);
      }
      break;

    case MH_NUMBER_TAG:
      {
	double number = (double)
	  MH_NUMBER_VALUE (MH_AS_NUMBER (object));
	
	bprintf (b, "%s", quotes);
	if (number == rint (number) &&
	    number <= LONG_MAX &&
	    number >= LONG_MIN)
	  bprintf (b, "%ld", (long int) number);
	else
	  bprintf (b, "%f", number);
	bprintf (b, "%s", quotes);
      }
      break;

    case MH_FUNCTION_TAG:
      bprintf (b, "{func %s}", MH_TAG_NAME (MH_AS_TAG (object)));
      break;

    case MH_VECTOR_TAG:
      {
	unsigned int offset = 0;
	unsigned int count = MH_VECTOR_LENGTH (MH_AS_VECTOR (object));
	
	for (; offset < count; offset++)
	  {
	    mh_object_t item =
	      MH_VECTOR_REF (MH_AS_VECTOR (object), offset);

	    if (! MH_VECTOR_P (item))
	      mh_object_to_buffer_internal
		(item, quote_depth, b, vector_spacer, alist_spacer, false);
	    else
	      {
		bprintf (b, "%s", quotes);
		/* Quote depth is not zero!  But not quote_depth either! */
		mh_object_to_buffer_internal
		  (item, 0, b, "\n", alist_spacer, false);
		bprintf (b, "%s", quotes);
	      }

	    if (offset + 1 < count)
	      bprintf (b, "%s", vector_spacer);
	  }
      }
      break;

    case MH_NIL_TAG:
      bprintf (b, "%s()%s", quotes, quotes);
      break;

    case MH_ALIST_TAG:
      {
	mh_alist_t alist = MH_AS_ALIST (object);

	/* This is an ALIST and only an ALIST. */
	bprintf (b, "%s(", quotes);

	while (MH_ALIST_P (alist))
	  {
	    bprintf (b, "(");
	    mh_object_to_buffer_internal
	      (MH_ALIST_NAME (alist), quote_depth + 1, b, " ", "", true);

	    if (MH_VECTOR_P (MH_ALIST_VALUE (alist)))
	      {
		mh_vector_t vector = MH_AS_VECTOR (MH_ALIST_VALUE (alist));

		/* Avoid this space if VECTOR length is zero */
#if 0
		if (0 != MH_VECTOR_LENGTH (vector) &&
		    ! MH_EMPTY_P (MH_VECTOR_REF (vector, 0)))
#endif
		if (0 != MH_VECTOR_LENGTH (vector))
		  {
		    bprintf (b, " ");
		    mh_object_to_buffer_internal
		      (MH_ALIST_VALUE (alist), 
		       quote_depth + 1, b,
		       " ", alist_spacer,
		       false);
		  }
	      }
	    else
	      {
		bprintf (b, " . ");

		mh_object_to_buffer_internal
		  (MH_ALIST_VALUE (alist),
		   quote_depth + 1, b,
		   " ", "",
		   false);
	      }
	    bprintf (b, ")");

	    alist = MH_ALIST_TAIL (alist);

	    /* CONS get printed with cons_spacing */
	    if (MH_ALIST_P (alist))
	      bprintf (b, "%s", alist_spacer);
	  }

	bprintf (b, ")%s", quotes);
      }
      break;
      
    default:
      bprintf (b, "%p (%d)", object, MH_OBJECT_TAG (object));
      assert (0);
      break;
    }

  xfree (quotes);

  return;
}

extern void
mh_object_to_buffer (mh_object_t object,
		     boolean_t   quoted, /* QUOTED means READABLE */
		     BPRINTF_BUFFER *b)
{
  /* VECTORs have newlines spacing, CONSes have single spacing.
     ... unless they are printed in a list */
  mh_object_to_buffer_internal
    (object, (quoted ? 1 : 0), b, "\n", "", false);
}
      
extern string_t
mh_object_to_string (mh_object_t object,
		     boolean_t   quoted)

{
  char *result;

  BPRINTF_BUFFER *b = 
    bprintf_create_buffer ();

#if defined (CONVERSION_TRACING)
  printf ("mh_object_to_string()\n");
#endif
  mh_object_to_buffer_internal
    (object, (quoted ? 1 : 0), b, "\n", "", false);

  result = strdup (b->buffer ? b->buffer : "");
  
  bprintf_free_buffer (b);

  return result;
}

extern mh_string_t
mh_object_to_string_object (mh_object_t object,
			    boolean_t   quoted)
{
  mh_string_t string;

  BPRINTF_BUFFER *b = 
    bprintf_create_buffer ();

  mh_object_to_buffer_internal
    (object, (quoted ? 1 : 0), b, " ", " ", false);
  
  string = mh_string_new (b->buffer ? b->buffer : "");
  
  bprintf_free_buffer (b);

  return string;
}

extern void
mh_object_to_file (mh_object_t object,
		   boolean_t   quoted,
		   FILE       *out)
{
  string_t result = 
    mh_object_to_string (object, quoted);

  fprintf (out, "%s", result);
  xfree (result);
  return;
}


/* Size in OBJECTS */
extern unsigned int
mh_object_size (mh_object_t object)
{
  switch (MH_OBJECT_TAG (object))
    {
    case MH_STRING_TAG:
      {
	unsigned int length =
	  MH_STRING_LENGTH (MH_AS_STRING (object));

	return OBJECT_SIZE
	  (sizeof (struct mh_string) + length * sizeof (char));
      }

    case MH_NUMBER_TAG:
      return OBJECT_SIZE (MH_NUMBER_ALLOC_SIZE);
      
    case MH_FUNCTION_TAG:
      return OBJECT_SIZE (MH_TAG_ALLOC_SIZE ());

    case MH_VECTOR_TAG:
      {
	unsigned int length =
	  MH_VECTOR_LENGTH (MH_AS_VECTOR (object));

	return OBJECT_SIZE (MH_VECTOR_ALLOC_SIZE (length));
      }

    case MH_NIL_TAG:
      return OBJECT_SIZE (sizeof (mh_nil));

    case MH_ALIST_TAG:
      return OBJECT_SIZE (MH_ALIST_ALLOC_SIZE);

    default:
      return 0;
    }
}

/****************************************************************
 *
 * OBJ_TEST main()
 *
 *
 *
 */
#if defined (OBJ_TEST)

static struct mh_argument arg1 =
{ "X", MH_ARGUMENT_REQUIRED, MH_ARGUMENT_EVALLED, MH_ARGUMENT_VALUE };

static struct mh_argument arg2 =
{ "Y", MH_ARGUMENT_REQUIRED, MH_ARGUMENT_EVALLED, MH_ARGUMENT_VALUE };

static mh_argument_t args_record[2] = { & arg1, & arg2 };

extern int
main (int   argc,
      char *argv[])
{
#if 0
  string_t source = strdup ("(0 1)((0 1) 2)(\"abc\" . \"def\")");
  string_t source = strdup ("abc\"def\\\"ghi\\\"jkl\"mno");
  strdup ("abc \n(0 1 2)hij=10 xyz[  i];;; cmnt
zzz<if <foo bar>><defun foo>\"bar\\n\\\"baz\"</defun>");

  string_t source_save = source;
  string_t source_dup  = strdup (source);

  mh_object_t object;

  printf ("String : {%s}\n", source);

  do
    {
      object = mh_string_to_object (&source);
      printf ("\nObject : {%s}\n", 
	      object ? mh_object_to_string (object, true) : "");
      printf ("Remains: {%s}\n\n", source);
    } while (*source && object);
#endif

  mh_object_init ();

  {
    mh_alist_t  alist  = 
      mh_alist_new   (MH_AS_ALIST (mh_nil), mh_nil, mh_nil);
    mh_string_t string = mh_string_new ("This is a test!");
    mh_number_t number = mh_number_new (1.2);
    mh_vector_t vector = mh_vector_new (3);
    mh_tag_t tag = mh_tag_new
      ("TESTER",
       TAG_TYPE_DEFUN,
       false,
       false,
       false,
       "No Body",
       NULL,
       "No Documentation",
       args_record,
       2);

    mh_byte_code_t code[] =
    { MH_CALL_OP, 2, MH_SHIFT_OP, 2, MH_RETURN_OP };

    FILE *file;
    string_t data;

    BPRINTF_BUFFER *b = bprintf_create_buffer ();
      
    MH_VECTOR_REF (vector, 0) = MH_AS_OBJECT (alist);
    MH_VECTOR_REF (vector, 1) = MH_AS_OBJECT (string);
    MH_VECTOR_REF (vector, 2) = MH_AS_OBJECT (number);

    mh_tag_install_machine 
      (tag,
       code,
       sizeof (code),
       MH_VECTOR_VALUES (vector),
       MH_VECTOR_LENGTH (vector),
       10);

    mh_object_serialize (MH_AS_OBJECT (alist),  b);
    mh_object_serialize (MH_AS_OBJECT (string), b);
    mh_object_serialize (MH_AS_OBJECT (number), b);
    mh_object_serialize (MH_AS_OBJECT (vector), b);
    mh_object_serialize (MH_AS_OBJECT (tag),    b);

    file = fopen ("/tmp/serial", "w");
    fwrite (b->buffer, 1, b->bindex, file);
    fclose (file);

    data = b->buffer;
    alist  = MH_AS_ALIST  (mh_object_restore (&data));
    string = MH_AS_STRING (mh_object_restore (&data));
    number = MH_AS_NUMBER (mh_object_restore (&data));
    vector = MH_AS_VECTOR (mh_object_restore (&data));
    tag    = MH_AS_TAG    (mh_object_restore (&data));

    printf ("\nALIST  : ");
    mh_object_to_file (MH_AS_OBJECT (alist),  true, stdout);
    printf ("\nSTRING: ");
    mh_object_to_file (MH_AS_OBJECT (string), true, stdout);
    printf ("\nNUMBER: ");
    mh_object_to_file (MH_AS_OBJECT (number), true, stdout);
    printf ("\nVECTOR: ");
    mh_object_to_file (MH_AS_OBJECT (vector), true, stdout);
    printf ("\nTAG: ");
    mh_tag_disassemble (tag, stdout);

  }
  return 0;
}
#endif /* defined (OBJ_TEST) */
