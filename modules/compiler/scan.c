/* scan.c: -*- C -*-  */

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

/* "{\it Scanning} is the process of analyzing a sequence of characters
   into larger units, called tokens.  This process is sometimes called 
   lexical scanning.  Typical tokens are variables, keywords, numbers, 
   punctuation, whitespace and comments.  The output of a scanner is a 
   sequence of tokens. ...  Scanning creates a token sequence that
   abstracts a character sequence by removing unnecessary details."

   from "Essentials of Programming Languages," page 375 */

#include "compiler/scan.h"

struct mh_token
{
  mh_token_type_t type;
  string_t        string;
};

extern mh_token_type_t
mh_token_type (mh_token_t token)
{ return token->type; }

extern string_t
mh_token_string (mh_token_t token)
{ return token->string; }

extern unsigned int
mh_token_string_length (mh_token_t token)
{ return strlen (token->string); }

struct mh_token_list
{
  mh_token_t      token;
  mh_token_list_t next;
  mh_token_list_t prev;
};

extern mh_token_t
mh_token_list_token (mh_token_list_t list)
{ return list->token; }

extern mh_token_list_t
mh_token_list_next (mh_token_list_t list)
{ return list->next; }

extern mh_token_list_t
mh_token_list_prev (mh_token_list_t list)
{ return list->prev; }


static string_t mh_token_type_names_with_error [] =
{
  "MEMORY ERROR",
  "nln",
  "spc",
  "com",
  "dlm",
  "str",
  "wrd"
};
static string_t *mh_token_type_names = 1 + mh_token_type_names_with_error;

extern mh_token_t
mh_token_new_substring (mh_token_type_t type,
			string_t        string,
			unsigned int    length)
{
  mh_token_t token = (mh_token_t) xmalloc (sizeof (struct mh_token));
  mh_token_fill_substring (token, type, string, length);
  return (token);
}

extern void
mh_token_fill_substring (mh_token_t      token,
			 mh_token_type_t type,
			 string_t        string,
			 unsigned int    length)
{
  token->type   = type;
  token->string = (string_t) xmalloc (1 + length);
  token->string [0] = '\0';
  strncat (token->string, string, length);
}

extern void
mh_token_copy (mh_token_t dst,
	       mh_token_t src)
{
  free (dst->string);
  dst->type   = src->type;
  dst->string = strdup (src->string);
}

extern void
mh_token_free (mh_token_t token)
{
  free (token->string);
  token->type   = (mh_token_type_t) -1;
  token->string = (string_t) -1;
  free (token);
}

extern boolean_t
mh_token_whitespace_p (mh_token_t token)
{
  return
    mh_token_match_type_p (token, MH_TOKEN_NEWLINE) ||
    mh_token_match_type_p (token, MH_TOKEN_SPACE) ||
    mh_token_match_type_p (token, MH_TOKEN_COMMENT);
}

/* mh_token_to_string()
 *
 * What about escapes? */
extern string_t
mh_token_to_string (mh_token_t token)
{
  return strdup (token->string);
}

/*
 *
 *
 *
 *
 */
static string_t
mh_token_delimiters = " \n\r\t\f<>[]=\"";

static mh_token_t
mh_string_to_token_word (string_t *string_pointer)
{
  string_t string_start = *string_pointer;
  string_t string       = *string_pointer;

  while (*string)
    {
      if (*string == '\\')
	string++;
      else if (0 != index (mh_token_delimiters, *string))
	break;

      string++;
    }

  *string_pointer = string;

  return mh_token_new_substring
    (MH_TOKEN_WORD, string_start, string - string_start);
}

static mh_token_t
mh_string_to_token_string (string_t *string_pointer)
{
  string_t string_start = *string_pointer;
  string_t string       = *string_pointer;

  string++ /* skip initial doublequote */ ; 

  /* Find final doublequote */
  while (*string && *string != '"')
    {
      if (*string++ == '\\')
	{
	  /* Skip escaped character, no matter what it is! */
	  if (*string)
	    string++;
	}
    }

  if ('\0' == *string)		/* unbalanced string */
    {
#if 1
      /* Make the first doublequote into a delimiter */
      *string_pointer = string_start + 1;

      return mh_token_new_substring
	(MH_TOKEN_DELIMITER, string_start, 1);
#endif
      return NULL;
    }

  /* Skip final doublequote */
  
  *string_pointer = ++string;

  /* Use string_to_string_escaped() here */

  return mh_token_new_substring
    (MH_TOKEN_STRING, string_start, string - string_start);
}

extern mh_token_t
mh_string_to_token (string_t *string_pointer)
{
  mh_token_t token;
  string_t   string_start = *string_pointer;
  string_t   string       = *string_pointer;

  switch (*string)
    {
    case '\0':
      token = (mh_token_t) NULL;
      break;

    case '\n':			/* MH_TOKEN_NEWLINE */
      token = mh_token_new_substring (MH_TOKEN_NEWLINE, string++, 1);
      break;

    case ' ':			/* MH_TOKEN_SPACE */
    case '\t':
    case '\r':
    case '\f':
      while (*string && 0 != index (" \t\r", *string)) string++;
      token = mh_token_new_substring
	(MH_TOKEN_SPACE, string_start, string - string_start);
      break;

    case ';':			/* MH_TOKEN_COMMENT */
      if (0 == strncmp (string, ";;;", 3))
	{
	  while ('\0' != *string && '\n' != *string) string++;
	  token = mh_token_new_substring
	    (MH_TOKEN_COMMENT, string_start, string - string_start);
	}
      else
	token = mh_string_to_token_word (&string);
      break;

    case '<':			/* MH_TOKEN_DELIMITER */
    case '>':
    case '[':
    case ']':
    case '=':
      token = mh_token_new_substring (MH_TOKEN_DELIMITER, string++, 1);
      break;
      
    case '"':
      token = mh_string_to_token_string (&string);
      break;

    default:
      token = mh_string_to_token_word (&string);
      break;
    }

  *string_pointer = string;
  return token;
}

/*
 *
 *
 *
 *
 */
extern mh_token_list_t
mh_token_list_make (mh_token_t token,
		    mh_token_list_t next)
{
  mh_token_list_t list  = (mh_token_list_t)
    xmalloc (sizeof (struct mh_token_list));
  
  list->token = token;
  list->next  = next;
  list->prev  = next ? next->prev : NULL;

  return list;
}

extern void
mh_token_list_free_special (mh_token_list_t list,
			    boolean_t next_too_p,
			    boolean_t token_too_p)
{
  if (list)
    {
      mh_token_t      token = list->token;
      mh_token_list_t next  = list->next;

      if (token_too_p && token) mh_token_free (token);
  
      list->token = NULL;
      list->prev  = NULL;
      list->next  = NULL;
      free (list);

      if (next_too_p) 
	mh_token_list_free_special (next, next_too_p, token_too_p);
    }
  return;
}

extern boolean_t
mh_token_list_equal_p (mh_token_list_t list1,
		       mh_token_list_t list2)
{
  return list1 == list2 ||
    (list1 && 
     list2 &&
     mh_token_equal_p      (list1->token, list2->token) &&
     mh_token_list_equal_p (list1->next,  list2->next));
}

static mh_token_list_t
mh_token_list_append_1 (mh_token_list_t list1,
			mh_token_list_t list2)
{
  mh_token_list_t result = list1;
  for (; list1->next; list1 = list1->next) /* nothing */ ; 
  list1->next = list2;
  return result;
}

extern mh_token_list_t
mh_token_list_append (mh_token_list_t list1,
		      mh_token_list_t list2)
{
  return list1 == NULL
    ? list2
    : (list2 == NULL
       ? list1
       : mh_token_list_append_1 (list1, list2));
}

static mh_token_list_t
mh_token_list_copy (mh_token_list_t list)
{
  return list == (mh_token_list_t ) NULL
    ? list
    : mh_token_list_make (list->token,
			  mh_token_list_copy (list->next));
}

static void
mh_token_list_link (mh_token_list_t list,
		    mh_token_list_t next)
{
  if (list) list->next = next;
  if (next) next->prev = list;
}

#if defined (NEVER_DEFINED)
static mh_token_list_t
mh_token_list_end (mh_token_list_t list)
{
  for (; list && list->next; list = list->next);
  return list;
}

static void
mh_token_list_splice_in (mh_token_list_t list,
			 mh_token_list_t splice)
{
  mh_token_list_t splice_end =
    mh_token_list_end (splice);
  mh_token_list_t list_next  =
    list->next;

  if (splice != list_next)
    {
      mh_token_list_link (list, splice);
      mh_token_list_link (splice_end, list_next);
    }
}

static mh_token_list_t
mh_token_list_splice_out (mh_token_list_t list,
			  mh_token_list_t next)
{
  mh_token_list_t splice = list->next;

  if (splice != next)
    {
      mh_token_list_link (list, next);
      mh_token_list_link (NULL, splice);
      mh_token_list_link (mh_token_list_end (splice), NULL);
      return splice;
    }
  return NULL;
}
#endif

extern mh_token_list_t
mh_token_list_map (mh_token_list_t   list,
		   mh_token_mapper_t mapper,
		   mh_token_maparg_t argument)
{
  mh_token_list_t result = mh_token_list_copy (list);

  for (list = result; list; list = list->next)
    list->token = (*mapper) (list->token, argument);

  return result;
}

extern void
mh_token_list_walk (mh_token_list_t   list,
		    mh_token_mapper_t mapper,
		    mh_token_maparg_t argument)
{
  mh_token_t token;
  for (; list; list = list->next)
    {
      token = (*mapper) (list->token, argument);
      if (token != list->token)
	mh_token_free (token);
    }
  return;
}

/* Return the first TOKEN in LIST that has TYPE */
extern mh_token_list_t
mh_token_list_match_type (mh_token_list_t list,
			  mh_token_type_t type)
{
  for (; list; list = list->next)
    if (mh_token_match_type_p (list->token, type))
      break /* for */ ;
  return list;
}

extern mh_token_list_t
mh_token_list_match (mh_token_list_t list,
		     mh_token_type_t type,
		     string_t        string)
{
  for (; list; list = list->next)
    if (mh_token_match_p (list->token, type, string))
      break /* for */ ;
  return list;
}

extern string_t
mh_token_list_to_string_until_token (mh_token_list_t list,
				     mh_token_t      token)
{
  mh_token_list_t l;
  unsigned int count = 0;
  string_t string, result;

  for (l = list; l && token != l->token; l = l->next)
    count += mh_token_string_length (l->token);
  
  result = string = (string_t) xmalloc (1 + count);
  string [0] = '\0';

  for (l = list; l && token != l->token; l = l->next)
    {
      strcpy (string, l->token->string);
      string += mh_token_string_length (l->token);
    }
  
  return result;
}

extern unsigned int
mh_token_list_type_count_until_token (mh_token_list_t list,
				      mh_token_type_t type,
				      mh_token_t      token)
{
  unsigned int count = 0;
  for (; list && token != list->token; list = list->next)
    if (mh_token_match_type_p (list->token, type))
      count++;
  return count;
}

extern unsigned int
mh_token_list_type_count (mh_token_list_t list,
			  mh_token_type_t type)
{
  return mh_token_list_type_count_until_token
    (list, type, NULL);
}

#if defined (NEVER_DEFINED)
extern unsigned int
mh_token_list_type_count (mh_token_list_t list,
			  mh_token_type_t type)
{
  unsigned int count = 0;
  for (; list; list = list->next)
    if (mh_token_match_type_p (list->token, type))
      count++;
  return count;
}
#endif


extern unsigned int
mh_token_list_match_count (mh_token_list_t list,
			   mh_token_type_t type,
			   string_t        string)
{
  unsigned int count = 0;
  for (; list; list = list->next)
    if (mh_token_match_p (list->token, type, string))
      count++;
  return count;
}

static unsigned int
mh_token_string_newline_count (mh_token_t token)
{
  unsigned int count = 0;
  string_t     str   = token->string;

  for (; *str; str++)
    if ('\n' == *str)
      count++;

  return count;
}

extern unsigned int
mh_token_list_line_number (mh_token_list_t list,
			   mh_token_t      token)
{
  unsigned int count = 1;	/* start on the first line */

  for (; list && token != list->token; list = list->next)
    if (mh_token_match_type_p (list->token, MH_TOKEN_NEWLINE))
      count++;
    else if (mh_token_match_type_p (list->token, MH_TOKEN_STRING))
      count += mh_token_string_newline_count (list->token);

  return count;
}
extern mh_token_list_t
mh_token_list_delete_type (mh_token_list_t list,
			   mh_token_type_t type,
			   mh_token_list_t *deleted)
{
  mh_token_list_t result = NULL;

  /* Make DELETED be NULL if needed */
  if (deleted) *deleted = NULL;

  while (list)
    if (mh_token_match_type_p (list->token, type))
      {
	/* Preserve the next for LIST */
	mh_token_list_t next = list->next;
	mh_token_list_t prev = list->prev;

	/* Save LIST if needed; otherwise free */
	if (deleted)
	  {
	    list->next = *deleted;
	    *deleted   = list;
	  }
	else
	  mh_token_list_free_special (list, false, true);

	mh_token_list_link (prev, next);

	/* Step to NEXT */
	list = next;
      }
    else if (NULL == result)
      {
	result = list;
	list   = list->next;
      }
    else
      list = list->next;
      

  return result;
}

/*
 *
 *
 *
 *
 */
extern mh_token_list_t
mh_scan (string_t *string_pointer)
{
  mh_token_t token;
  mh_token_list_t list, result = NULL;

  while ((token = mh_string_to_token (string_pointer)))
    {
      mh_token_list_t next = 
      mh_token_list_make (token, NULL);

      if (NULL == result)
	result = next;
      else
	mh_token_list_link (list, next);

      list = next;
    }

  return result;
}

extern void
mh_token_write (mh_token_t token,
		FILE      *file)
{
  switch (mh_token_type (token))
    {
    case MH_TOKEN_NEWLINE:
      fprintf (file, "<%s>",
	       mh_token_type_names [mh_token_type (token)]);
      break;

    case MH_TOKEN_SPACE:
      fprintf (file, "<%s \"%s\">",
	       mh_token_type_names [mh_token_type (token)],
	       token->string);
      break;

    case MH_TOKEN_DELIMITER:
      fprintf (file, "<%s '%s'>",
	       mh_token_type_names [mh_token_type (token)],
	       token->string);
      break;

    default:
      fprintf (file, "<%s %s>",
	       mh_token_type_names [mh_token_type (token)],
	       token->string);
      break;
    }
}


extern void
mh_token_list_write (mh_token_list_t list,
		     FILE      *file)
{
  fprintf (file, "<list");
  for (; list; list = list->next)
    {
      fprintf (file, "\n  ");
      mh_token_write (list->token, file);
    }
  fprintf (file, ">");
}

#if defined (SCAN_TEST)

static void
scan_test_repeat (string_t string, unsigned long repeat)
{
  string_t save = string;
  printf ("Scan Test Starting %ld ...\n", repeat);
  while (repeat-- > 0)
    {
      string = save;
      mh_token_list_free (mh_scan (&string));
    }
  printf ("  Done!\n");
}


extern int
main (int   argc,
      char *argv[])
{
  /* <subst-in-var val "[ \t\r][ \t\r]+" " " "\\\\\"" "\""> */
  char buffer [1024];
  string_t source;
  string_t source_save;
  string_t source_dup;
  mh_token_list_t list, deleted, cleaned;

#if 0
  sprintf (buffer,
	   "<subst-in-var val \"%s\" \" \" \"%s\" \"%s\">",
	   "[ \t][ \t]+",
	   "\\\\\"",
	   "\"");
#endif

#if 0
  sprintf (buffer, "abc\"\\\"123\"def");
#endif

#if 0
  sprintf (buffer, "abc\"def\\\"123\\\"ghi\"jkl");
#endif

#if 0
  sprintf (buffer, "abc\"def\\\"123ghi\"jkl");
#endif

#if 1
  sprintf (buffer, "abc\n\n\n   jkl");
#endif

  source      = buffer;
  source_save = source;
  source_dup  = strdup (source);

  printf ("Source: '%s'\n", source);
  list = mh_scan (&source);
  printf ("Result: '%s'\n", source);
  mh_token_list_write (list, stdout);
  printf ("\n");
  printf ("Newlines:  %d\n", mh_token_list_newline_count (list));
  printf ("Delimiter (<): %d\n", 
	  mh_token_list_match_count (list, MH_TOKEN_DELIMITER, "<"));
  printf ("Delimiter (=): %d\n", 
	  mh_token_list_match_count (list, MH_TOKEN_DELIMITER, "="));
  printf ("List: %s\n", mh_token_list_to_string (list));
  if (0 != strcmp (source_save, mh_token_list_to_string (list)))
    printf ("\n ** MISMATCH ** \n");

  /* Delete some stuff */
  cleaned = mh_token_list_delete_type (list, MH_TOKEN_SPACE, &deleted);
  printf ("CLEANED:\n");
  mh_token_list_write (cleaned, stdout);
  printf ("\n");
  printf ("DELETED SPACE:\n");
  mh_token_list_write (deleted, stdout);
  printf ("\n");
  mh_token_list_free (deleted);
  

  /* Delete some stuff */
  cleaned = mh_token_list_delete_type (list, MH_TOKEN_COMMENT, &deleted);
  printf ("CLEANED:\n");
  mh_token_list_write (cleaned, stdout);
  printf ("\n");
  printf ("DELETED COMMENTS:\n");
  mh_token_list_write (deleted, stdout);
  printf ("\n");
  mh_token_list_free (deleted);
  

  source_save = source = "abc\"def";
  list = mh_scan (&source);
  if (*source)
    printf ("Incomplete SCAN: %s\n", source);
  if (list) 
    {
      mh_token_list_write (list, stdout);
      if (0 != strcmp (source_save, mh_token_list_to_string (list)))
	printf ("\n ** MISMATCH ** \n");
    }

  source_save = source = "";
  list = mh_scan (&source);
  if (*source)
    printf ("Incomplete SCAN: %s\n", source);
  if (list) 
    {
      mh_token_list_write (list, stdout);
      if (0 != strcmp (source_save, mh_token_list_to_string (list)))
	printf ("\n ** MISMATCH ** \n");
    }

  scan_test_repeat (source_dup, 10000L);
  return 0;
}
#endif /* defined (SCAN_TEST) */
