/* parse.c: -*- C -*-  */

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

/* "{\it Parsing} organizes a sequence of tokens into syntactic
   elements such as expressions, statements, and blocks.  These are
   generally specified using BNF or some equivalent formalism.  The
   syntactic elements are typically nested, so the output of a parser
   is usually called a {\it parse tree} or {\it abstract syntax tree}, 
   and they provide convenient access to the syntactic components of a 
   program"

   from "Essentials of Programming Languages," page 375 */

#include "compiler/parse.h"

typedef void (*mh_parse_function_t) (mh_parse_t parse);


extern mh_parse_type_t
mh_parse_type (mh_parse_t parse)
{ return MH_PARSE_TYPE (parse); }

extern void
mh_parser_exception (string_t     source,
		     unsigned int lineno,
		     string_t     message)
{
  /* For now, simply page_debug */
  printf ("%s: %d: %s\n", source, lineno, message);
}





/**************************************************************************
 *
 *  MH_PARSE_SHOW ()
 *
 *
 * Forward Declaration */
static void
mh_parse_show_1 (mh_parse_t   parse, 
		 boolean_t    newline_p,
		 unsigned int indent_1,
		 unsigned int indent_2);

/* Print a COUNT spaces */
static void spaces (unsigned int count)
{ while (count--) putchar (' '); }

/* Print a NEWLINE */
static void newline (void)
{ putchar ('\n'); }

/* Show a list (via MH_PARSE_TYPE_NEXT) of PARSES */
static void 
mh_parse_show_list_1 (mh_parse_t   parse,
		      boolean_t    newline_p,
		      unsigned int indent_1, 
		      unsigned int indent_2)
{
  for ( ; parse; parse = MH_PARSE_LIST_TAIL (parse))
    mh_parse_show_1 (MH_PARSE_LIST_HEAD (parse), 
		     true,
		     indent_1, 
		     indent_2);
}

static void
mh_parse_show_token_1 (mh_token_t   token, 
		       boolean_t    newline_p,
		       unsigned int indent_1,
		       unsigned int indent_2)
{
  static char labels[] = { 'N', 'P', 'C', 'D', 'S', 'W' };

  string_t chars;

  if (NULL == token)
    return;

  if (true == newline_p)
    newline();

  spaces (indent_1);

  if (mh_token_match_type_p (token, MH_TOKEN_NEWLINE))
    chars = strdup ("\\n");
  else
    chars = mh_token_to_string (token);
  
  printf ("%c-\"%s\"", labels [mh_token_type (token)], chars);
  free (chars);
}

/* Show a single PARSE */
static void
mh_parse_show_1 (mh_parse_t parse,
		 boolean_t    newline_p,
		 unsigned int indent_1,
		 unsigned int indent_2)
{
  if (NULL == parse)   return;
  if (true == newline_p) newline();
  spaces (indent_1);

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
      mh_parse_show_token_1 (MH_PARSE_TOKEN (parse),
			     false,
			     0, /* indent_1,*/
			     indent_2);
      break;

    case MH_PARSE_TYPE_STRING:
      printf ("{STR");
      mh_parse_show_1 (MH_PARSE_STRING (parse),
		       false,
		       1,
		       5 + indent_2);
      printf ("}");
      break;

    case MH_PARSE_TYPE_KEY:
      printf ("{KEY");
      mh_parse_show_1 (MH_PARSE_KEY_NAME (parse), 
		       false,
		       1,
		       5 + indent_2);
      mh_parse_show_1 (MH_PARSE_KEY_VALUE (parse),
		       true,
		       5 + indent_2,
		       5 + indent_2);
      printf ("}");
      break;

    case MH_PARSE_TYPE_ARRAY:
      printf ("{ARRAY");
      mh_parse_show_1 (MH_PARSE_ARRAY_NAME (parse), 
		       false,
		       1,
		       7 + indent_1);
      mh_parse_show_1 (MH_PARSE_ARRAY_INDEX (parse),
		       true,
		       7 + indent_2,
		       7 + indent_2);
      printf ("}");
      break;

    case MH_PARSE_TYPE_TAG:
      printf ("{TAG");
      mh_parse_show_1 (MH_PARSE_TAG_BODY (parse),
		       false,
		       1, /* true, 2 + indent_2, 2 + indent_2 */
		       5 + indent_2);
      printf ("}");
      break;

    case MH_PARSE_TYPE_BLK:
      printf ("{BLK");
      mh_parse_show_1 (MH_PARSE_BLK_OPEN (parse),
		       false,
		       1,
		       5 + indent_1);

      mh_parse_show_1 (MH_PARSE_BLK_BODY (parse),
		       true,
		       2 + indent_1,
		       2 + indent_1);
      printf ("}");
      break;

    case MH_PARSE_TYPE_LIST:
      printf ("{LIST");
      mh_parse_show_list_1 (parse,
			    true,
			    indent_2 + 2, 
			    indent_2 + 2);
      printf ("}");
      break;


    default:
      break;
    }
}

/* Show a single PARSE */
extern void
mh_parse_show (mh_parse_t parse)
{
  mh_parse_show_1 (parse, false, 0, 0);
  fflush (stdout);
}

/*************************************************************************
 *
 * MH_PARSE_T Constructors
 *
 */
static mh_parse_t
mh_parse_new (mh_parse_type_t type)
{
  mh_parse_t parse = (mh_parse_t) xcalloc (1, sizeof (struct mh_parse));
  MH_PARSE_TYPE   (parse) = type;
  return (parse);
}

extern void
mh_parse_copy (mh_parse_t dest,
	       mh_parse_t source)
{
  memcpy ((void *) dest,
	  (const void *) source,
	  sizeof (struct mh_parse));
}

extern mh_parse_t
mh_parse_dup (mh_parse_t parse)
{
  mh_parse_t result = (mh_parse_t) xcalloc (1, sizeof (struct mh_parse));
  mh_parse_copy (result, parse);
  return result;
}

extern mh_parse_t
mh_parse_token_new (mh_token_t token,
		    boolean_t  copy_token_p)
{
  mh_parse_t parse = mh_parse_new (MH_PARSE_TYPE_TOKEN);
  MH_PARSE_TOKEN (parse) = copy_token_p
    ? mh_token_dup (token)
    : token;
  return parse;
}

extern mh_parse_t
mh_parse_token_make (mh_token_type_t type,
		     string_t        string)
{
  return mh_parse_token_new
    (mh_token_new (type, string),
     false);
}

static mh_parse_t
mh_parse_string_new (mh_parse_t string)
{
  mh_parse_t parse = mh_parse_new (MH_PARSE_TYPE_STRING);
  MH_PARSE_STRING (parse) = string;
  return parse;
}

static mh_parse_t
mh_parse_key_new (mh_parse_t name,
		  mh_token_t delimiter,
		  mh_parse_t value)
{
  mh_parse_t parse = mh_parse_new (MH_PARSE_TYPE_KEY);
  MH_PARSE_KEY_NAME      (parse) = name;
  MH_PARSE_KEY_DELIMITER (parse) = delimiter;
  MH_PARSE_KEY_VALUE     (parse) = value;
  return parse;
}

static mh_parse_t
mh_parse_array_new (mh_parse_t name,
		    mh_token_t open_br,
		    mh_parse_t offset,
		    mh_token_t close_br)
{
  mh_parse_t parse = mh_parse_new (MH_PARSE_TYPE_ARRAY);
  MH_PARSE_ARRAY_NAME  (parse) = name;
  MH_PARSE_ARRAY_OPEN  (parse) = open_br;
  MH_PARSE_ARRAY_INDEX (parse) = offset;
  MH_PARSE_ARRAY_CLOSE (parse) = close_br;
  return parse;
}

static mh_parse_t
mh_parse_tag_new (mh_token_t open_tkn, 
		  mh_parse_t body,
		  mh_token_t close_tkn)
{
  mh_parse_t parse = mh_parse_new (MH_PARSE_TYPE_TAG);
  MH_PARSE_TAG_OPEN  (parse) = open_tkn;
  MH_PARSE_TAG_BODY  (parse) = body;
  MH_PARSE_TAG_CLOSE (parse) = close_tkn;
  return parse;
}

static mh_parse_t
mh_parse_blk_new (mh_parse_t open_tag, 
		  mh_parse_t body,
		  mh_parse_t close_tag)
{
  mh_parse_t parse = mh_parse_new (MH_PARSE_TYPE_BLK);
  MH_PARSE_BLK_OPEN  (parse) = open_tag;
  MH_PARSE_BLK_BODY  (parse) = body;
  MH_PARSE_BLK_CLOSE (parse) = close_tag;
  return parse;
}

extern mh_parse_t
mh_parse_list_new (mh_parse_t head, 
		   mh_parse_t tail)
{
  mh_parse_t parse = mh_parse_new (MH_PARSE_TYPE_LIST);
  MH_PARSE_LIST_HEAD (parse) = head;
  MH_PARSE_LIST_TAIL (parse) = tail;
  return parse;
}

extern mh_parse_t
mh_parse_list_copy (mh_parse_t list)
{
  return MH_PARSE_EMPTY == list
    ? MH_PARSE_EMPTY
    : mh_parse_list_new (MH_PARSE_LIST_HEAD (list),
			 mh_parse_list_copy (MH_PARSE_LIST_TAIL (list)));
}

extern void
mh_parse_list_free (mh_parse_t list)
{
  if (MH_PARSE_EMPTY == list) return;

  mh_parse_list_free (MH_PARSE_LIST_TAIL (list));
  xfree (list);
}

/* Make LIST2 be the tail of LIST1 provided LIST1 is not
   MH_PARSE_EMPTY.  Assumes LIST1 is MH_PARSE_TYPE_LIST. */
static void
mh_parse_list_link (mh_parse_t list1,
		    mh_parse_t list2)
{
  if (MH_PARSE_EMPTY != list1)
    MH_PARSE_LIST_TAIL (list1) = list2;
}

/* Searching from ROOT, find the PARSE whose tail is TAIL.
   Assumes ROOT is MH_PARSE_TYPE_LIST. */
static mh_parse_t
mh_parse_list_prev (mh_parse_t root,
		    mh_parse_t tail)
{
  while (root && tail != MH_PARSE_LIST_TAIL (root))
    root = MH_PARSE_LIST_TAIL (root);
  return root;
}

extern mh_parse_t
mh_parse_list_append (mh_parse_t list1,
		      mh_parse_t list2)
{
  if (MH_PARSE_EMPTY == list1)
    return list2;
  else if (MH_PARSE_EMPTY == list2)
    return list1;
  else
    {
      mh_parse_t tail = list1;

      /* Find the last tail */
      while (MH_PARSE_LIST_TAIL (tail))
	tail = MH_PARSE_LIST_TAIL (tail);

      mh_parse_list_link (tail, list2);

      return list1;
    }
}

static mh_parse_t
mh_parse_list_reverse_internal (mh_parse_t target,
				mh_parse_t source)
{
  return MH_PARSE_EMPTY == source
    ? target
    : (mh_parse_list_reverse_internal
       (mh_parse_list_new (MH_PARSE_LIST_HEAD (source), target),
	MH_PARSE_LIST_TAIL (source)));
}

extern mh_parse_t
mh_parse_list_reverse (mh_parse_t parse)
{ return mh_parse_list_reverse_internal (MH_PARSE_EMPTY, parse); }

extern mh_parse_t
mh_parse_list_rest (mh_parse_t list,
		    unsigned int n)
{
  if (MH_PARSE_EMPTY == list) return MH_PARSE_EMPTY;

  return 0 == n
    ? list
    : mh_parse_list_rest (MH_PARSE_LIST_TAIL (list), n - 1);
}
  
extern mh_parse_t
mh_parse_list_nth (mh_parse_t list,
		   unsigned int n)
{
  mh_parse_t rest =
    mh_parse_list_rest (list, n);

  return MH_PARSE_EMPTY == rest
    ? MH_PARSE_EMPTY
    : MH_PARSE_LIST_HEAD (rest);
}

static void
mh_parse_list_split_keys_1 (mh_parse_t  list,
			    mh_parse_t *list_of_nonkeys,
			    mh_parse_t *list_of_keys)
{
  if (MH_PARSE_EMPTY == list)
    {
      mh_parse_t temp;

      temp             = *list_of_nonkeys;
      *list_of_nonkeys =  mh_parse_list_reverse (*list_of_nonkeys);
      mh_parse_list_free (temp);

      temp             = *list_of_keys;
      *list_of_keys =  mh_parse_list_reverse (*list_of_keys);
      mh_parse_list_free (temp);

      return;
    }

  if (mh_parse_type_p (MH_PARSE_LIST_HEAD (list), MH_PARSE_TYPE_KEY))
    *list_of_keys = mh_parse_list_new
      (MH_PARSE_LIST_HEAD (list), *list_of_keys);
  else
    *list_of_nonkeys = mh_parse_list_new
      (MH_PARSE_LIST_HEAD (list), *list_of_nonkeys);

  mh_parse_list_split_keys_1 (MH_PARSE_LIST_TAIL (list),
			      list_of_nonkeys,
			      list_of_keys);
}

extern void
mh_parse_list_split_keys (mh_parse_t  list,
			  mh_parse_t *list_of_nonkeys,
			  mh_parse_t *list_of_keys)
{ 
  *list_of_nonkeys = MH_PARSE_EMPTY;
  *list_of_keys    = MH_PARSE_EMPTY;
  mh_parse_list_split_keys_1 (list, list_of_nonkeys, list_of_keys);
}



extern mh_parse_t
mh_parse_list_nth_nonkey_recur (mh_parse_t list,
				unsigned int n)
{
  if (MH_PARSE_EMPTY == list) return MH_PARSE_EMPTY;

  return mh_parse_type_p (MH_PARSE_LIST_HEAD (list), MH_PARSE_TYPE_KEY)
    ? mh_parse_list_nth_nonkey_recur (MH_PARSE_LIST_TAIL (list), n)
    : (0 != n
       ? mh_parse_list_nth_nonkey_recur (MH_PARSE_LIST_TAIL (list), n - 1)
       : MH_PARSE_LIST_HEAD (list));
}
    

extern mh_parse_t
mh_parse_list_nth_nonkey (mh_parse_t list,
			  unsigned int n)
{
  register int where = 0;
  mh_parse_t result = MH_PARSE_EMPTY;

  while (MH_PARSE_EMPTY != list)
    {
      register mh_parse_t item = MH_PARSE_LIST_HEAD (list);

      list = MH_PARSE_LIST_TAIL (list);

      if ((MH_PARSE_TYPE (item) != MH_PARSE_TYPE_KEY) && (where == n))
	{
	  result = item;
	  break;
	}
      else if (MH_PARSE_TYPE (item) != MH_PARSE_TYPE_KEY)
	where++;
    }

  return result;
}

/* Return the number of parses in PARSE; PARSE must be a list */
extern unsigned int 
mh_parse_list_count (mh_parse_t parse)
{
  return MH_PARSE_EMPTY == parse
    ? 0
    : 1 + mh_parse_list_count (MH_PARSE_LIST_TAIL (parse));
}

extern boolean_t
mh_parse_list_every (mh_parse_t parse,
		     mh_parse_tester_t tester,
		     mh_parse_maparg_t arg)
{
  return MH_PARSE_EMPTY == parse
    ? true
    : ((*tester) (MH_PARSE_LIST_HEAD (parse), arg)
       ? mh_parse_list_every (MH_PARSE_LIST_TAIL (parse), tester, arg)
       : false);
}

extern boolean_t
mh_parse_list_any (mh_parse_t parse,
		   mh_parse_tester_t tester,
		   mh_parse_maparg_t arg)
{
  return MH_PARSE_EMPTY == parse
    ? false
    : ((*tester) (MH_PARSE_LIST_HEAD (parse), arg)
       ? true
       : mh_parse_list_any (MH_PARSE_LIST_TAIL (parse), tester, arg));
}

extern mh_parse_t
mh_parse_list_find (mh_parse_t parse,
		    mh_parse_tester_t tester,
		    mh_parse_maparg_t arg)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  return (*tester) (MH_PARSE_LIST_HEAD (parse), arg)
    ? MH_PARSE_LIST_HEAD (parse)
    : mh_parse_list_find (MH_PARSE_LIST_TAIL (parse), tester, arg);
}

extern mh_parse_t
mh_parse_list_find_tail (mh_parse_t parse,
			 mh_parse_tester_t tester,
			 mh_parse_maparg_t arg)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  return (*tester) (MH_PARSE_LIST_HEAD (parse), arg)
    ? parse
    : mh_parse_list_find_tail (MH_PARSE_LIST_TAIL (parse), tester, arg);
}

/***********************************************************************
 *
 * MH_PARSE_T Desctructor 
 *
 *
 */
extern void
mh_parse_free_special (mh_parse_t parse,
		       boolean_t  deep_p,
		       boolean_t  token_p,
		       boolean_t  parse_p)
{
#define recurse( parse )   \
  mh_parse_free_special (parse, deep_p, token_p, parse_p)

  if (MH_PARSE_EMPTY == parse) return;

  if (deep_p)
    switch (MH_PARSE_TYPE (parse))
      {
      case MH_PARSE_TYPE_TOKEN:
	if (token_p)
	  mh_token_free (MH_PARSE_TOKEN (parse));
	break;

      case MH_PARSE_TYPE_STRING:
	recurse (MH_PARSE_STRING (parse));
	break;

      case MH_PARSE_TYPE_KEY:
	if (token_p)
	  mh_token_free (MH_PARSE_KEY_DELIMITER (parse));
	recurse (MH_PARSE_KEY_NAME  (parse));
	recurse (MH_PARSE_KEY_VALUE (parse));
	break;

      case MH_PARSE_TYPE_ARRAY:
	if (token_p)
	  {
	    mh_token_free (MH_PARSE_ARRAY_OPEN  (parse));
	    mh_token_free (MH_PARSE_ARRAY_CLOSE (parse));
	  }
	recurse (MH_PARSE_ARRAY_NAME  (parse));
	recurse (MH_PARSE_ARRAY_INDEX (parse));
	break;

      case MH_PARSE_TYPE_TAG:
	if (token_p)
	  {
	    mh_token_free (MH_PARSE_TAG_OPEN (parse));
	    mh_token_free (MH_PARSE_TAG_CLOSE (parse));
	  }
	recurse (MH_PARSE_TAG_BODY (parse));
	break;

      case MH_PARSE_TYPE_BLK:
	recurse (MH_PARSE_BLK_OPEN  (parse));
	recurse (MH_PARSE_BLK_BODY  (parse));
	recurse (MH_PARSE_BLK_CLOSE (parse));
	break;

      case MH_PARSE_TYPE_LIST:
	recurse (MH_PARSE_LIST_HEAD (parse));
	recurse (MH_PARSE_LIST_TAIL (parse));
	break;
      }
  if (parse_p) free (parse);
#undef recurse
}

static void
mh_parse_list_clean (mh_parse_t parse)
{
  if (MH_PARSE_EMPTY != parse &&
      MH_PARSE_EMPTY == MH_PARSE_LIST_TAIL (parse))
    {
      mh_parse_t parse_head = MH_PARSE_LIST_HEAD (parse);
      mh_parse_copy (parse, parse_head);
      mh_parse_free_special (parse_head, false, false, true);
    }
}

/***************************************************************************
 *
 * MH_PARSE_TO_STRING ()
 *
 *
 */

/* DEPTH of  0 means unquoted: "ABC" -> ABC
   DEPTH of  1 means 1-quoted: "ABC" -> "ABC" ... */
static mh_token_list_t
mh_parse_to_token_list (mh_parse_t parse,
			int        depth)
{
#define recurse( parse )        mh_parse_to_token_list (parse, depth)
#define listing( token, list )  mh_token_list_make (token, list)
#define listnew( token )        mh_token_list_make (token, NULL)
#define appends( list1, list2 ) mh_token_list_append (list1, list2)

  if (MH_PARSE_EMPTY == parse) return NULL;

  /* Need all new tokens to memory manage */

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
      {
	mh_token_t token = MH_PARSE_TOKEN (parse);

	mh_token_type_t type   = mh_token_type   (token);
	string_t        string = mh_token_string (token);

	switch (type)
	  {
	  case MH_TOKEN_NEWLINE:
	  case MH_TOKEN_SPACE:
	  case MH_TOKEN_COMMENT:

	  case MH_TOKEN_DELIMITER:

	  case MH_TOKEN_STRING:
	  case MH_TOKEN_WORD:
	    string = depth > 1
	      ? mh_string_to_string_quoted_to_depth (string, depth - 1)
	      : strdup (string);

	    break;
	  }

	token = mh_token_new (type, string);

	return listnew (token);
      }

    case MH_PARSE_TYPE_STRING:
      {
	string_t string = 
	  mh_parse_to_string_at_depth (MH_PARSE_STRING (parse), ++depth);

	string_t quotes = depth < 1
	  ? strdup ("")
	  : mh_string_quotes_at_depth (depth - 1);

	/* Add some quotes */
	string_t quoted_string =
	  (string_t) xmalloc (1 + 
			      2 * strlen (quotes)  +
			      strlen (string));

	mh_token_list_t result;

	sprintf (quoted_string, "%s%s%s", quotes, string, quotes);
	
	result = listnew (mh_token_new (MH_TOKEN_STRING, quoted_string));

	free (string);
	free (quotes);
	free (quoted_string);
	return result;
      }

    case MH_PARSE_TYPE_KEY:
      return appends
	(recurse (MH_PARSE_KEY_NAME (parse)),
	 listing (mh_token_new (MH_TOKEN_DELIMITER, "="),
		  recurse (MH_PARSE_KEY_VALUE (parse))));

    case MH_PARSE_TYPE_ARRAY:
      return appends
	(recurse (MH_PARSE_ARRAY_NAME (parse)),
	 listing (mh_token_new (MH_TOKEN_DELIMITER, "["),
		  appends (recurse (MH_PARSE_ARRAY_INDEX (parse)),
			   listnew (mh_token_new (MH_TOKEN_DELIMITER,
						  "]")))));

    case MH_PARSE_TYPE_TAG:
      {
	mh_parse_t body = MH_PARSE_TAG_BODY (parse);

	if (body &&
	    mh_parse_type_p (body, MH_PARSE_TYPE_LIST) &&
	    MH_PARSE_LIST_HEAD (body) &&
	    mh_parse_token_match_p (MH_PARSE_LIST_HEAD (body),
				    MH_TOKEN_WORD,
				    "%%concat"))
	  return recurse (MH_PARSE_LIST_TAIL 
			  (MH_PARSE_LIST_TAIL (body)));
	else
	  return listing
	    (mh_token_new (MH_TOKEN_DELIMITER, "<"),
	     appends (recurse (MH_PARSE_TAG_BODY (parse)),
		      listnew (mh_token_new (MH_TOKEN_DELIMITER, ">"))));
      }
    case MH_PARSE_TYPE_BLK:
      return appends
	(recurse (MH_PARSE_BLK_OPEN (parse)),
	 appends (recurse (MH_PARSE_BLK_BODY  (parse)),
		  recurse (MH_PARSE_BLK_CLOSE (parse))));

    case MH_PARSE_TYPE_LIST:
      return appends
	(recurse (MH_PARSE_LIST_HEAD (parse)),
	 recurse (MH_PARSE_LIST_TAIL (parse)));

    default:
      return NULL;
    }
#undef appends
#undef listnew
#undef listing
#undef recurse
}

extern string_t 
mh_parse_to_string_at_depth (mh_parse_t parse,
			     int        depth)
{
  mh_token_list_t list   = mh_parse_to_token_list  (parse, depth);
  string_t        result = mh_token_list_to_string (list);
  mh_token_list_free_special (list, true, true);
  return result;
}

extern mh_parse_t
mh_parse_list_with_space (mh_parse_t list)
{
  mh_parse_t head, tail;
  mh_token_t space;

  if (MH_PARSE_EMPTY == list) return MH_PARSE_EMPTY;

  head = MH_PARSE_LIST_HEAD (list);
  tail = MH_PARSE_LIST_TAIL (list);

  space = mh_token_new (MH_TOKEN_WORD, " ");

  return mh_parse_list_new
    (head,
     (MH_PARSE_EMPTY == tail
      ? tail
      : mh_parse_list_new (mh_parse_token_new (space, false),
			   mh_parse_list_with_space (tail))));
}

extern string_t
mh_parse_to_string_with_space (mh_parse_t parse)
{
  if (false == mh_parse_type_p (parse, MH_PARSE_TYPE_LIST))
    return mh_parse_to_string_at_depth (parse, 1);

  {
    mh_parse_t spaced_parse =
      mh_parse_list_with_space (parse);
    
    string_t result = mh_parse_to_string_at_depth (spaced_parse, 1);

    mh_parse_list_free (spaced_parse);

    return result;
  }
}
  

/*************************************************************************
 *
 * MH_PARSE_COPY_DEEP ()
 *
 *
 */
extern mh_parse_t
mh_parse_copy_deep (mh_parse_t parse)
{
#define recurse( parse )     mh_parse_copy_deep (parse)

  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
      return mh_parse_token_new
	(mh_token_dup (MH_PARSE_TOKEN (parse)), 
	 true);

    case MH_PARSE_TYPE_STRING:
      return mh_parse_string_new
	(recurse (MH_PARSE_STRING (parse)));

    case MH_PARSE_TYPE_KEY:
      return mh_parse_key_new
	(recurse (MH_PARSE_KEY_NAME (parse)),
	 mh_token_dup (MH_PARSE_KEY_DELIMITER (parse)),
	 recurse (MH_PARSE_KEY_VALUE (parse)));

    case MH_PARSE_TYPE_ARRAY:
      return mh_parse_array_new
	(recurse (MH_PARSE_ARRAY_NAME (parse)),
	 mh_token_dup (MH_PARSE_ARRAY_OPEN (parse)),
	 recurse (MH_PARSE_ARRAY_INDEX (parse)),
	 mh_token_dup (MH_PARSE_ARRAY_CLOSE (parse)));

    case MH_PARSE_TYPE_TAG:
      return mh_parse_tag_new
	(mh_token_dup (MH_PARSE_TAG_OPEN (parse)),
	 recurse (MH_PARSE_TAG_BODY (parse)),
	 mh_token_dup (MH_PARSE_TAG_CLOSE (parse)));

    case MH_PARSE_TYPE_BLK:
      return mh_parse_blk_new
	(recurse (MH_PARSE_BLK_OPEN  (parse)),
	 recurse (MH_PARSE_BLK_BODY  (parse)),
	 recurse (MH_PARSE_BLK_CLOSE (parse)));

    case MH_PARSE_TYPE_LIST:
      return mh_parse_list_new
	(recurse (MH_PARSE_LIST_HEAD (parse)),
	 recurse (MH_PARSE_LIST_TAIL (parse)));

    default:
      return NULL;
    }
#undef recurse
}

/*************************************************************************
 *
 * MH_PARSE_EQUAL_P ()
 *
 *
 */
extern boolean_t
mh_parse_equal_p (mh_parse_t parse1,
		  mh_parse_t parse2)
{
#define recurse( p1, p2 )     mh_parse_equal_p (p1, p2)

  if (parse1 == parse2) return true;

  if (MH_PARSE_EMPTY == parse1 ||
      MH_PARSE_EMPTY == parse2 ||
      MH_PARSE_TYPE (parse1) != MH_PARSE_TYPE (parse2))
    return false;

  switch (MH_PARSE_TYPE (parse1))
    {
    case MH_PARSE_TYPE_TOKEN:
      return mh_token_equal_p
	(MH_PARSE_TOKEN (parse1),
	 MH_PARSE_TOKEN (parse2));

    case MH_PARSE_TYPE_STRING:
      return recurse
	(MH_PARSE_STRING (parse1),
	 MH_PARSE_STRING (parse2));

    case MH_PARSE_TYPE_KEY:
      return 
	recurse (MH_PARSE_KEY_NAME  (parse1),
		 MH_PARSE_KEY_NAME  (parse2)) &&
	recurse (MH_PARSE_KEY_VALUE (parse1),
		 MH_PARSE_KEY_VALUE (parse2));
	
    case MH_PARSE_TYPE_ARRAY:
      return
	recurse (MH_PARSE_ARRAY_NAME  (parse1),
		 MH_PARSE_ARRAY_NAME  (parse2)) &&
	recurse (MH_PARSE_ARRAY_INDEX (parse1),
		 MH_PARSE_ARRAY_INDEX (parse2));

    case MH_PARSE_TYPE_TAG:
      return
	recurse (MH_PARSE_TAG_BODY (parse1),
		 MH_PARSE_TAG_BODY (parse2));

    case MH_PARSE_TYPE_BLK:
      return
	recurse (MH_PARSE_BLK_OPEN  (parse1),
		 MH_PARSE_BLK_OPEN  (parse2)) &&
	recurse (MH_PARSE_BLK_BODY  (parse1),
		 MH_PARSE_BLK_BODY  (parse2)) &&
	recurse (MH_PARSE_BLK_CLOSE (parse1),
		 MH_PARSE_BLK_CLOSE (parse2));

    case MH_PARSE_TYPE_LIST:
      return
	recurse (MH_PARSE_LIST_HEAD  (parse1),
		 MH_PARSE_LIST_HEAD  (parse2)) &&
	recurse (MH_PARSE_LIST_TAIL  (parse1),
		 MH_PARSE_LIST_TAIL  (parse2));

    default:
      return false;
    }
#undef recurse
}

/*************************************************************************
 *
 * MH_PARSE_SUBST ()
 *
 *
 */
extern void
mh_parse_subst (mh_parse_t parse,
		mh_parse_t this,
		mh_parse_t that)
{
#define recurse( parse )        mh_parse_subst (parse, this, that)

  if (MH_PARSE_EMPTY == parse) return;

  if (mh_parse_equal_p (parse, this))
    {
      mh_parse_free_special (parse, true, true, false);
      mh_parse_copy (parse, that);
      return;
    }

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
      break;

    case MH_PARSE_TYPE_STRING:
      recurse (MH_PARSE_STRING (parse));
      break;

    case MH_PARSE_TYPE_KEY:
      recurse (MH_PARSE_KEY_NAME  (parse));
      recurse (MH_PARSE_KEY_VALUE (parse));
      break;

    case MH_PARSE_TYPE_ARRAY:
      recurse (MH_PARSE_ARRAY_NAME  (parse));
      recurse (MH_PARSE_ARRAY_INDEX (parse));
      break;

    case MH_PARSE_TYPE_TAG:
      recurse (MH_PARSE_TAG_BODY  (parse));
      break;

    case MH_PARSE_TYPE_BLK:
      recurse (MH_PARSE_BLK_OPEN  (parse));
      recurse (MH_PARSE_BLK_BODY  (parse));
      recurse (MH_PARSE_BLK_CLOSE (parse));
      break;

    case MH_PARSE_TYPE_LIST:
      recurse (MH_PARSE_LIST_HEAD (parse));
      recurse (MH_PARSE_LIST_TAIL (parse));
      break;

    default:
      break;
    }
#undef recurse
}


extern boolean_t
mh_parse_has_positional_reference_p (mh_parse_t parse)
{
#define recurse( parse )        mh_parse_has_positional_reference_p (parse)

  return (false);

  if (MH_PARSE_EMPTY == parse) return false;

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
      {
	mh_token_t token  = MH_PARSE_TOKEN  (parse);
	string_t   string = mh_token_string (token);
#if 0
	int   position = 0;
	char  extra    = 0;

	/* It is known that there is no whitespace at the start or end of
	   STRING and thus sscanf() won't skip over it.  The %u directive
	   seems not to exclude negative integers contrary to the
	   documentation; thus we use %d and explicitly check the value of
	   POSITION.  The %c directive ensures that extraneous characters
	   following %n lead to a false result. */
	return string && 
	  sscanf (string, "%%%1d%c", &position, &extra) == 1 &&
	  position >= 0;
#endif
	/* Well, it is overly restrictive */
	return NULL != strchr (string, '%');
      }

    case MH_PARSE_TYPE_STRING:
      return recurse (MH_PARSE_STRING (parse));

    case MH_PARSE_TYPE_KEY:
      return
	recurse (MH_PARSE_KEY_NAME  (parse)) ||
	recurse (MH_PARSE_KEY_VALUE (parse));

    case MH_PARSE_TYPE_ARRAY:
      return
	recurse (MH_PARSE_ARRAY_NAME  (parse)) ||
	recurse (MH_PARSE_ARRAY_INDEX (parse));

    case MH_PARSE_TYPE_TAG:
      return recurse (MH_PARSE_TAG_BODY  (parse));

    case MH_PARSE_TYPE_BLK:
      return
	recurse (MH_PARSE_BLK_OPEN  (parse)) ||
	recurse (MH_PARSE_BLK_BODY  (parse)) ||
	recurse (MH_PARSE_BLK_CLOSE (parse));

    case MH_PARSE_TYPE_LIST:
      return
	recurse (MH_PARSE_LIST_HEAD (parse)) ||
	recurse (MH_PARSE_LIST_TAIL (parse));

    default:
      return false;
    }
#undef recurse
}


/*************************************************************************
 *
 * 
 *
 *
 */

/* Return the number of parses in PARSE.  If PARSE is EMPTY, result is
   0; if PARSE is a list, result is mh_parse_list_count(); otherwise
   result is 1. */ 
extern unsigned int
mh_parse_count (mh_parse_t parse)
{
  return MH_PARSE_EMPTY == parse
    ? 0
    : (! mh_parse_type_p (parse, MH_PARSE_TYPE_LIST)
       ? 1
       : mh_parse_list_count (parse));
}

/* Return true if PARSE is a token and if that token matches TYPE and
   STRING; otherwise returns false. */
extern boolean_t
mh_parse_token_match_p (mh_parse_t      parse,
			mh_token_type_t type,
			string_t        string)
{
  return mh_parse_type_p (parse, MH_PARSE_TYPE_TOKEN) &&
    mh_token_match_p (MH_PARSE_TOKEN (parse), type, string);
}

/* Return true if PARSE is a token and if that token matches TYPE;
   otherwise returns false */
extern boolean_t
mh_parse_token_match_type_p (mh_parse_t      parse,
			     mh_token_type_t type)
{
  return mh_parse_type_p (parse, MH_PARSE_TYPE_TOKEN) &&
    mh_token_match_type_p (MH_PARSE_TOKEN (parse), type);
}

/* Returns true is PARSE is a token and if that token is whitespace;
   otherwise returns false */
static boolean_t
mh_parse_token_whitespace_p (mh_parse_t parse)
{
  return mh_parse_type_p (parse, MH_PARSE_TYPE_TOKEN) &&
    mh_token_whitespace_p (MH_PARSE_TOKEN (parse));
}

/* Returns true if PARSE is a KEY and if the KEY name is a token that
   matches TYPE and STRING; otherwise return false. */
static boolean_t
mh_parse_key_match_p (mh_parse_t      parse,
		      mh_token_type_t type,
		      string_t        string)
{
  return mh_parse_type_p (parse, MH_PARSE_TYPE_KEY) &&
    mh_parse_token_match_p (MH_PARSE_KEY_NAME (parse), type, string);
}

#if defined (NEVER_DEFINED)
/* Returns true if PARSE is a KEY and if the KEY name is a token that
   matches TYPE; otherwise return false. */
static boolean_t
mh_parse_key_match_type_p (mh_parse_t      parse,
			   mh_token_type_t type,
			   string_t        string)
{
  return mh_parse_type_p (parse, MH_PARSE_TYPE_KEY) &&
    mh_parse_token_match_type_p (MH_PARSE_KEY_NAME (parse), type);
}
#endif


/* Returns the parse from PARSE with a HEAD that is not whitespace.
   That is, iterate along the list PARSE until the HEAD is not
   whitespace; return that PARSE */
extern mh_parse_t
mh_parse_list_trim_whitespace (mh_parse_t parse)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  assert (mh_parse_type_p (parse, MH_PARSE_TYPE_LIST));

  return mh_parse_token_whitespace_p (MH_PARSE_LIST_HEAD (parse))
    ? mh_parse_list_trim_whitespace  (MH_PARSE_LIST_TAIL (parse))
    : parse /* that is not whitespace */ ;
}

/* Returns the parse from PARSE with a HEAD that is not a token nor a
   token matching TYPE; otherwise returns MH_PARSE_EMPTY.  If PARSE is
   not a LIST, then MH_PARSE_EMPTY is returned (which might not be
   best; but is consistent with the function name). */
static mh_parse_t
mh_parse_list_avoid_type (mh_parse_t      parse,
			  mh_token_type_t type)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;
  if (! mh_parse_type_p (parse, MH_PARSE_TYPE_LIST))
    return MH_PARSE_EMPTY;

  return mh_parse_token_match_type_p (MH_PARSE_LIST_HEAD (parse), type)
    ? mh_parse_list_avoid_type (MH_PARSE_LIST_TAIL (parse), type)
    : parse;
}

/* Returns the parse from PARSE with a HEAD that is a token and
   matches TYPE and STRING; otherwise returns MH_PARSE_EMPTY.  If
   PARSE is not a LIST, then MH_PARSE_EMPTY is returned (which might not
   be best; but is consistent with the function name). */
static mh_parse_t
mh_parse_list_match (mh_parse_t      parse,
		     mh_token_type_t type,
		     string_t        string)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;
  if (! mh_parse_type_p (parse, MH_PARSE_TYPE_LIST))
    return MH_PARSE_EMPTY;

  return mh_parse_token_match_p (MH_PARSE_LIST_HEAD (parse), type, string)
    ? parse
    : mh_parse_list_match (MH_PARSE_LIST_TAIL (parse), type, string);
}

/* Returns the parse from PARSE with a HEAD that is a token and
   matches TYPE and STRING; otherwise returns MH_PARSE_EMPTY.  If
   PARSE is not a LIST, then MH_PARSE_EMPTY is returned (which might not
   be best; but is consistent with the function name). */
static mh_parse_t
mh_parse_list_match_type (mh_parse_t      parse,
			  mh_token_type_t type)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;
  if (! mh_parse_type_p (parse, MH_PARSE_TYPE_LIST))
    return MH_PARSE_EMPTY;

  return mh_parse_token_match_type_p (MH_PARSE_LIST_HEAD (parse), type)
    ? parse
    : mh_parse_list_match_type (MH_PARSE_LIST_TAIL (parse), type);
}

extern mh_parse_t
mh_parse_list_delete_key (mh_parse_t parse,
			  string_t   name)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  assert (mh_parse_type_p (parse, MH_PARSE_TYPE_LIST));

  if (mh_parse_key_match_p (MH_PARSE_LIST_HEAD (parse),
			    MH_TOKEN_WORD,
			    name))
    {
      mh_parse_t head = MH_PARSE_LIST_HEAD (parse);
      mh_parse_t tail = MH_PARSE_LIST_TAIL (parse);
      
      /* No nothing will point to HEAD */
      /* mh_parse_free (head); */
      mh_parse_free_special (parse, false, false, true);

      return mh_parse_list_delete_key (tail, name);
    }
  else
    {
      MH_PARSE_LIST_TAIL (parse) =
	mh_parse_list_delete_key (MH_PARSE_LIST_TAIL (parse), name);
      return parse;
    }
}

/* Returns the parse from PARSE with a HEAD that satisfies
   mh_parse_key_match_p (head, MH_TOKEN_WORD, name); otherwise returns 
   MH_PARSE_EMPTY.  */
static mh_parse_t
mh_parse_list_match_key (mh_parse_t parse,
			 string_t   name)
{
  /* Ground the recursion */
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  return mh_parse_key_match_p (MH_PARSE_LIST_HEAD (parse),
			       MH_TOKEN_WORD,
			       name)
    ? MH_PARSE_LIST_HEAD (parse)
    : mh_parse_list_match_key (MH_PARSE_LIST_TAIL (parse), name);
}

/* Returns the parse from PARSE that is a key which has a name that
   matches type MH_TOKEN_WORD and NAME; otherwise returns
   MH_PARSE_EMPTY. */
extern mh_parse_t
mh_parse_match_key (mh_parse_t parse,
		    string_t   name)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  return mh_parse_key_match_p (parse, MH_TOKEN_WORD, name)
    ? parse
    : (mh_parse_type_p (parse, MH_PARSE_TYPE_LIST)
       ? mh_parse_list_match_key (parse, name)
       : MH_PARSE_EMPTY);
}

/* Returns the HEAD from PARSE that is not whitespace; otherwise
   return MH_PARSE_EMPTY.  Assumes PARSE is MH_PARSE_TYPE_LIST. */
static mh_parse_t
mh_parse_list_operator (mh_parse_t parse)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

#if defined (SKIP_OVER_WHITESPACE_IN_TAG_OPERATOR)
  /* Could be implemented with mh_parse_list_trim_whitespace () */
  return mh_parse_token_whitespace_p (MH_PARSE_LIST_HEAD (parse))
    ? mh_parse_list_operator         (MH_PARSE_LIST_TAIL (parse))
    : MH_PARSE_LIST_HEAD (parse);
#endif
  return mh_parse_type_p (MH_PARSE_LIST_HEAD (parse), MH_PARSE_TYPE_TOKEN)
    ? MH_PARSE_LIST_HEAD (parse)
    : MH_PARSE_EMPTY;
}

/* Returns the operator from the body of TAG; otherwise
   MH_PARSE_EMPTY.  An operator is the first non-whitespace parse.
   Examples:
     <foo bar ...>      ==> operator is foo
     <   foo bar ...>   ==> operator is foo
     <"abc" bar ...>    ==> operator is "abc"
     <<foo> bar ...>    ==> operator is <foo>
   Conclusion: operator is syntactic; not semantic(ly correct). */
extern mh_parse_t
mh_parse_tag_operator (mh_parse_t parse)
{
  mh_parse_t body;

  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  assert (mh_parse_type_p (parse, MH_PARSE_TYPE_TAG));

  body = MH_PARSE_TAG_BODY (parse);
  return body && mh_parse_type_p (body, MH_PARSE_TYPE_LIST)
    ? mh_parse_list_operator (body)
    : body;
}

#if defined (NEVER_DEFINED)
static boolean_t
mh_parse_tag_match_opener (mh_parse_t parse,
			   string_t   operator)
{
  return false;
}
#endif

static boolean_t
mh_parse_tag_match_closer (mh_parse_t parse,
			   string_t   opener)
{
  mh_parse_t operator;
  string_t   closer;

  if (MH_PARSE_EMPTY == parse) return false;

  assert (mh_parse_type_p (parse, MH_PARSE_TYPE_TAG));

  operator = mh_parse_tag_operator (parse);

  if (MH_PARSE_EMPTY == operator) return false;

  closer = mh_token_string (MH_PARSE_TOKEN (operator));

  /* TAG_OPERATOR is a word */

  return mh_parse_type_p (operator, MH_PARSE_TYPE_TOKEN) &&
    closer && '/' == closer[0] &&
    0  == strcmp (opener, &closer[1]);
}

/**********************************************************************
 *
 * MH_PARSE_DELETE_SPACE ()
 *
 *
 */
extern mh_parse_t
mh_parse_list_delete_space_internal (mh_parse_t parse,
				     boolean_t  newline_p,
				     boolean_t  always_p)
{
  mh_parse_t head, tail;

  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  assert (mh_parse_type_p (parse, MH_PARSE_TYPE_LIST));

  head = MH_PARSE_LIST_HEAD (parse);
  tail = MH_PARSE_LIST_TAIL (parse);

  /* Delete whitespace if:
       1) All whitespace should be deleted or
       2) a newline has been seen or
       3) head is itself a newline */
  if (mh_parse_type_p (head, MH_PARSE_TYPE_TOKEN) &&
      mh_parse_token_whitespace_p (head) &&
      (always_p || newline_p ||
       (newline_p = mh_parse_token_match_type_p (head, MH_TOKEN_NEWLINE))))
    {
      /* Memory manage HEAD and PARSE */
      
      return mh_parse_list_delete_space_internal
	(tail, newline_p, always_p);
    }
  else
    {
      /* Move on over */
      MH_PARSE_LIST_TAIL (parse) =
	mh_parse_list_delete_space_internal (tail, false, always_p);

      return parse;
    }
}

extern mh_parse_t
mh_parse_list_delete_space (mh_parse_t parse)
{
  return mh_parse_list_delete_space_internal (parse, false, true);
}

extern mh_parse_t
mh_parse_delete_space (mh_parse_t parse)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  return mh_parse_type_p (parse, MH_PARSE_TYPE_LIST)
    ? mh_parse_list_delete_space (parse)
    : (mh_parse_token_whitespace_p  (parse)
       ? MH_PARSE_EMPTY		/* FREE parse? */
       : parse);
}

extern mh_parse_t
mh_parse_list_delete_interline_space (mh_parse_t parse)
{
  return mh_parse_list_delete_space_internal (parse, false, false);
}

extern mh_parse_t
mh_parse_delete_interline_space (mh_parse_t parse)
{
  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  return mh_parse_type_p (parse, MH_PARSE_TYPE_LIST)
    ? mh_parse_list_delete_interline_space (parse)
    : (mh_parse_token_whitespace_p  (parse)
       ? MH_PARSE_EMPTY		/* FREE parse? */
       : parse);
}

/*
 * Delete all space up to and including the first NEWLINE and beyond
 * and including the last NEWLINE
 */ 
extern mh_parse_t
mh_parse_list_delete_bounding_space_internal (mh_parse_t parse,
					      boolean_t  reverse_p)
{
  mh_parse_t tail;

  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  tail = parse;
  
  /* Skip all leading SPACE */
  while (tail && mh_parse_token_match_type_p
	 (MH_PARSE_LIST_HEAD (tail), MH_TOKEN_SPACE))
    tail = MH_PARSE_LIST_TAIL (tail);

  /* If TAIL is NEWLINE then delete the NEWLINE and SPACE */
  if (tail && mh_parse_token_match_type_p
      (MH_PARSE_LIST_HEAD (tail), MH_TOKEN_NEWLINE))
    tail = MH_PARSE_LIST_TAIL (tail);

  /* TAIL now points to something that we will return */
  /* Reverse the list and do it all again */
  if (reverse_p)
    {
      mh_parse_t reverse  = mh_parse_list_reverse (tail);

      tail = mh_parse_list_delete_bounding_space_internal
	(reverse, false);

      tail = mh_parse_list_reverse (tail);

      mh_parse_list_free (reverse);
    }
  return tail;
}

extern mh_parse_t
mh_parse_list_delete_bounding_space (mh_parse_t parse)
{ return mh_parse_list_delete_bounding_space_internal (parse, true); }

extern mh_parse_t
mh_parse_list_as_body (mh_parse_t body)
{
  string_t string = mh_parse_to_string
    (mh_parse_list_trim_whitespace (body));
  mh_parse_t parse = mh_parse ("mh_exp_args_as_body()", string);
  xfree (string);

  return parse;
}

static mh_parse_t
mh_parse_block_flatten (mh_parse_t block)
{
  mh_parse_t bpen = mh_parse_list_new
    (MH_PARSE_BLK_OPEN  (block),
     MH_PARSE_EMPTY);

  mh_parse_t body = ! MH_PARSE_BLK_BODY (block)
    ? MH_PARSE_EMPTY
    : (mh_parse_type_p (MH_PARSE_BLK_BODY (block), MH_PARSE_TYPE_LIST)
       ? mh_parse_list_copy (MH_PARSE_BLK_BODY (block))
       : mh_parse_list_new  (MH_PARSE_BLK_BODY (block),
			     MH_PARSE_EMPTY));

  mh_parse_t clos = mh_parse_list_new
    (MH_PARSE_BLK_CLOSE (block),
     MH_PARSE_EMPTY);

  return mh_parse_list_append
    (bpen, mh_parse_list_append (body, clos));
}

extern mh_parse_t
mh_parse_list_flatten_blocks (mh_parse_t parse)
{
  mh_parse_t head, tail;

  if (MH_PARSE_EMPTY == parse) return MH_PARSE_EMPTY;

  head = MH_PARSE_LIST_HEAD (parse);
  tail = MH_PARSE_LIST_TAIL (parse);

  return (! mh_parse_type_p (head, MH_PARSE_TYPE_BLK))
    ? mh_parse_list_new (head, mh_parse_list_flatten_blocks (tail))
    : mh_parse_list_append (mh_parse_block_flatten       (head),
			    mh_parse_list_flatten_blocks (tail));
}

static void
mh_parse_blocks (mh_parse_t parse);

extern mh_parse_t
mh_parse_list_as_concat (mh_parse_t parse)
{
  mh_parse_t concat = mh_parse_list_delete_space
    (mh_parse_list_flatten_blocks (parse));

  mh_parse_blocks (concat);

  return concat;
}


/*************************************************************************
 *
 * MH_PARSE_RECURSE ()
 *
 */
static void
mh_parse_recurse (mh_parse_t parse,
		  mh_parse_function_t parser,
		  mh_parse_function_t tag_parser,
		  mh_parse_function_t blk_parser,
		  mh_parse_function_t list_parser)
{
#define recurse( parse )			\
  mh_parse_recurse (parse, parser, 		\
		    tag_parser, 		\
		    blk_parser, 		\
		    list_parser)

  /* Ground the recursion */
  if (MH_PARSE_EMPTY == parse) return;

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
      break;

    case MH_PARSE_TYPE_STRING:
      recurse (MH_PARSE_STRING (parse));
      break;

    case MH_PARSE_TYPE_KEY:
      /* <parse> '=' <parse> */
      recurse (MH_PARSE_KEY_NAME  (parse));
      recurse (MH_PARSE_KEY_VALUE (parse));
      break;

    case MH_PARSE_TYPE_ARRAY:
      /* <parse> '[' <parse> ']' */
      recurse (MH_PARSE_ARRAY_NAME  (parse));
      recurse (MH_PARSE_ARRAY_INDEX (parse));
      break;

    case MH_PARSE_TYPE_TAG:
      /* '<' <parse> '>' */
      recurse (MH_PARSE_TAG_BODY (parse));
      if (tag_parser)
	(*tag_parser) (parse);
      break;

    case MH_PARSE_TYPE_BLK:
      /* <tag> <parse> </tag> */
      recurse (MH_PARSE_BLK_OPEN  (parse));
      recurse (MH_PARSE_BLK_BODY  (parse));
      recurse (MH_PARSE_BLK_CLOSE (parse));
      if (blk_parser)
	(*blk_parser) (parse);
      break;

    case MH_PARSE_TYPE_LIST:
      recurse (MH_PARSE_LIST_HEAD (parse));
      recurse (MH_PARSE_LIST_TAIL (parse));
      if (list_parser)
	(*list_parser) (parse);
      break;
    }
#undef recurse
}


/*************************************************************************
 *
 * MH_PARSE_BLOCKS ()
 *
 */

/* Return the CLOSER from PARSE that completes the complex tag named
   OPENER; otherwise returns MH_PARSE_EMPTY.  The CLOSER must look
   like '</opener>' - except that whitespace is _probably_ allowed
   between and adjacent to the bounding angle brackets.  PARSE itself
   practically must be of type MH_PARSE_TYPE_LIST. */
static mh_parse_t
mh_parse_list_blocks_match_closer (mh_parse_t parse,
				   string_t   opener)
{
  assert (MH_PARSE_EMPTY == parse ||
	  mh_parse_type_p (parse, MH_PARSE_TYPE_LIST));

  /* HEAD is a TAG with operator of "/operator" */
  return (MH_PARSE_EMPTY == parse ||
	  (mh_parse_type_p (MH_PARSE_LIST_HEAD (parse), MH_PARSE_TYPE_TAG) &&
	   mh_parse_tag_match_closer (MH_PARSE_LIST_HEAD (parse), opener)))
    ? parse
    : mh_parse_list_blocks_match_closer (MH_PARSE_LIST_TAIL (parse), opener);
}

/* Destructively modify PARSE by forming block parses for all
   OPENER/CLOSER pairs. */
static void
mh_parse_list_blocks (mh_parse_t parse)
{
  mh_parse_t head = MH_PARSE_LIST_HEAD (parse);
  mh_parse_t tail = MH_PARSE_LIST_TAIL (parse);

  /* It only makes sense to look for a BLOCK when HEAD is a TAG and
     has a well-formed OPERATOR */
  mh_parse_t tag_operator = 
    (mh_parse_type_p (head, MH_PARSE_TYPE_TAG)
     ? mh_parse_tag_operator (head)
     : MH_PARSE_EMPTY);

  /* The TAG_OPERATOR needs to be a token; anything else won't do.  That
     is, don't look for BLOCKS to <<foo> bar baz> matching <foo>.  Do,
     however, look to match foo in <foo bar baz>. */
  if (tag_operator && mh_parse_type_p (tag_operator, MH_PARSE_TYPE_TOKEN))
    {
      /* TAG opener as the string for TAG_OPERATOR */ 
      string_t opener = 
	mh_token_string (MH_PARSE_TOKEN (tag_operator));

      /* The critical test */
      mh_parse_t blk_close_list =
	mh_parse_list_blocks_match_closer (tail, opener);

      if (MH_PARSE_EMPTY != blk_close_list)
	{
	  mh_parse_t blk_open, blk_body,  blk_close;

	  blk_open = head;
	  blk_body = (tail == blk_close_list)
	    ? MH_PARSE_EMPTY
	    : tail;
	  blk_close = MH_PARSE_LIST_HEAD (blk_close_list);

	  /* Terminate BLK_BODY */
	  mh_parse_list_link
	    (mh_parse_list_prev (parse, blk_close_list),
	     MH_PARSE_EMPTY);

	  /* If BODY has no TAIL use BODY's HEAD */
	  mh_parse_list_clean (blk_open);
	  mh_parse_list_clean (blk_body);
	  mh_parse_list_clean (blk_close);

	  MH_PARSE_LIST_HEAD (parse) =
	    mh_parse_blk_new (blk_open,
			      blk_body,
			      blk_close);

	  /* Replace TAIL with subsequent */
	  mh_parse_list_link
	    (parse, MH_PARSE_LIST_TAIL (blk_close_list));

#if defined (FAIL_IF_FINAL_PARSE)
	  /* If PARSE has no TAIL; replace PARSE with HEAD. */
	  mh_parse_list_clean (parse);
#endif
	  /* Memory Manage */
	  mh_parse_free_special (blk_close_list, false, false, true);
	}
    }
}

static void
mh_parse_blocks (mh_parse_t parse)
{
  mh_parse_recurse (parse, 
		    mh_parse_blocks, 
		    NULL,
		    NULL,
		    mh_parse_list_blocks);
}

/*************************************************************************
 *
 * MH_PARSE_KEYS ()
 *
 */

/* Returns true if PARSE is an ATOM; otherwise false.  Actually ATOM is
   defined by this function. */
static boolean_t
mh_parse_is_atom_p (mh_parse_t parse)
{
  if (MH_PARSE_EMPTY == parse) return false;

  switch (MH_PARSE_TYPE (parse))
    {
    case MH_PARSE_TYPE_TOKEN:
      return ! mh_token_whitespace_p (MH_PARSE_TOKEN (parse));

    case MH_PARSE_TYPE_STRING:
    case MH_PARSE_TYPE_KEY:
    case MH_PARSE_TYPE_ARRAY:
    case MH_PARSE_TYPE_TAG:
    case MH_PARSE_TYPE_BLK:
      return true;
      
    case MH_PARSE_TYPE_LIST:
      return false;

    default:
      return false;
    }
}

/* TAG OPERANDS are delimited by any parse that is not typed as
   MH_PARSE_TYPE_TAG or a parse typed as MH_PARSE_TYPE_TOKEN with a
   token type that is not MH_TOKEN_WORD. */
static boolean_t
mh_parse_tag_list_key_delimiter (mh_parse_t parse)
{
  /* Already have found TAG OPERANDS and TAG ARRAYS */
  return 
    mh_parse_type_p (parse, MH_PARSE_TYPE_TOKEN) &&
    (mh_parse_token_match_type_p (parse, MH_TOKEN_NEWLINE) ||
     mh_parse_token_match_type_p (parse, MH_TOKEN_SPACE) ||
     mh_parse_token_match_type_p (parse, MH_TOKEN_COMMENT));
}


static void
mh_parse_tag_list_keys (mh_parse_t parse)
{
  mh_parse_t head = MH_PARSE_LIST_HEAD (parse);
  mh_parse_t tail = MH_PARSE_LIST_TAIL (parse);

  /* <name> '=' <value> */
  
  if (mh_parse_is_atom_p (head))
    {
      /* Skip 'tailing' whitespace */
      mh_parse_t equals_list =
	mh_parse_list_trim_whitespace (tail);

      /* Find a match of {DELIMITER '='} */
      if (MH_PARSE_EMPTY != equals_list && 
	  mh_parse_token_match_p (MH_PARSE_LIST_HEAD (equals_list),
				  MH_TOKEN_DELIMITER,
				  "="))
	{
	  /* Find VALUE */
	  mh_parse_t value_list = 
	    mh_parse_list_trim_whitespace (MH_PARSE_LIST_TAIL (equals_list));

	  /* VALUE_LIST of MH_PARSE_EMPTY must be parsable as
	   *    <tag key=> to {KEY "key" ""} */
	  mh_parse_t
	    name    = parse,
	    equals  = MH_PARSE_LIST_HEAD (equals_list),
	    value   = MH_PARSE_LIST_TAIL (equals_list),
	    next    = MH_PARSE_EMPTY;

	  /* NEXT is the first whitespace after VALUE_LIST.  That
	     is consistent with whitespace delimiting tag operands 
	     and allows 'foo=abc?=<random>' to parse into
	     {KEY FOO {LIST abc?= <random>}} */
	  if (MH_PARSE_EMPTY != value_list)
	    next = mh_parse_list_find_tail
	      (MH_PARSE_LIST_TAIL (value_list),
	       (mh_parse_tester_t) mh_parse_tag_list_key_delimiter,
	       (mh_parse_maparg_t) NULL);

	  /* Terminate NAME */
	  mh_parse_list_link
	    (mh_parse_list_prev (parse, equals_list),
	     MH_PARSE_EMPTY);

	  /* Terminate VALUE - was value_list */
	  if (MH_PARSE_EMPTY != value_list &&
	      MH_PARSE_EMPTY != next)
	    mh_parse_list_link
	      (mh_parse_list_prev (value, next),
	       MH_PARSE_EMPTY);

	  /* EQUALS_LIST is no longer used; NAME and PARSE are identical */
	  mh_parse_copy (equals_list, name);
	  name = equals_list;

	  /* OK to install KEY in PARSE */

	  /* Terminate PARSE */
	  mh_parse_list_link (parse, next);

	  mh_parse_list_clean (name);
	  mh_parse_list_clean (value);

	  /* Replace HEAD with MH_PARSE_TYPE_TAG */
	  MH_PARSE_LIST_HEAD (parse) =
	    mh_parse_key_new (name,
			      MH_PARSE_TOKEN (equals),
			      value);

#if defined (FAIL_IF_FINAL_PARSE)
	  /* If PARSE has no TAIL; replace PARSE with HEAD. */
	  mh_parse_list_clean (parse);
#endif

	  /* Memory Manage - NOTHING (!?) */
	}
    }
}

static void
mh_parse_tag_keys (mh_parse_t parse)
{
  mh_parse_t body;

  if (MH_PARSE_EMPTY == parse) return;

  /* If BODY is a list, look for KEYS */
  body = MH_PARSE_TAG_BODY (parse);

  if (body && mh_parse_type_p (body, MH_PARSE_TYPE_LIST))
    for (; body; body = MH_PARSE_LIST_TAIL (body))
      mh_parse_tag_list_keys (body);
}

static void
mh_parse_keys (mh_parse_t parse)
{
  mh_parse_recurse (parse, 
		    mh_parse_keys, 
		    mh_parse_tag_keys,
		    NULL,
		    NULL);
}


/*************************************************************************
 *
 * MH_PARSE_ARRAYS ()
 *
 */
static void
mh_parse_tag_list_arrays (mh_parse_t parse)
{
  /* {LIST head tail} ==> {LIST p0 p1 p2 ... NULL} */
  mh_parse_t head = MH_PARSE_LIST_HEAD (parse);
  mh_parse_t tail = MH_PARSE_LIST_TAIL (parse);

  /* <name> '[' <index> ']' */
  
  if (mh_parse_is_atom_p (head))
    {
      /* Skip 'tailing' whitespace */
      mh_parse_t opener_list =
	mh_parse_list_trim_whitespace (tail);

      mh_parse_t indx_list, closer_list;

      /* Find a match of {DELIMITER '['} */
      if (MH_PARSE_EMPTY != opener_list && 
	  mh_parse_token_match_p (MH_PARSE_LIST_HEAD (opener_list),
				  MH_TOKEN_DELIMITER,
				  "["))
	{
	  /* Find INDX */
	  indx_list = mh_parse_list_trim_whitespace
	    (MH_PARSE_LIST_TAIL (opener_list));

	  if (MH_PARSE_EMPTY != indx_list)
	    {
	      boolean_t found_array_p = false;

	      /* Find INDEX and CLOSER */
	      if (mh_parse_token_match_p (MH_PARSE_LIST_HEAD (indx_list),
					  MH_TOKEN_DELIMITER,
					  "]"))
		{
		  /* INDX is empty */
		  closer_list = indx_list;
		  indx_list   = MH_PARSE_EMPTY;
		  found_array_p = true;
		}
	      else
		{
		  /* Skip whitespce */
		  closer_list = mh_parse_list_trim_whitespace
		    (MH_PARSE_LIST_TAIL (indx_list));
		    
		  /* Find CLOSER */
		  if (MH_PARSE_EMPTY != closer_list &&
		      mh_parse_token_match_p (MH_PARSE_LIST_HEAD (closer_list),
					      MH_TOKEN_DELIMITER,
					      "]"))
		    found_array_p = true;
		}

	      /* Munge it all into an ARRAY */
	      if (found_array_p)
		{
		  mh_parse_t
		    name    = parse,
		    opener  = MH_PARSE_LIST_HEAD (opener_list),
		    indx    = (MH_PARSE_EMPTY != indx_list
			       ? MH_PARSE_LIST_TAIL (opener_list)
			       : MH_PARSE_EMPTY),
		    closer  = MH_PARSE_LIST_HEAD (closer_list),
		    next    = MH_PARSE_LIST_TAIL (closer_list);

		  /* Terminate NAME */
		  mh_parse_list_link
		    (mh_parse_list_prev (parse, opener_list),
		     MH_PARSE_EMPTY);

		  if (MH_PARSE_EMPTY != indx_list)
		    /* Terminate INDX */
		    mh_parse_list_link
		      (mh_parse_list_prev (indx_list, closer_list), 
		       MH_PARSE_EMPTY);

		  /* OPENER_LIST and CLOSER_LIST are no longer used
		     NAME and PARSE are identical */
		  mh_parse_copy (opener_list, name);
		  name = opener_list;

		  /* OK to install ARRAY in PARSE */

		  /* Terminate PARSE */
		  mh_parse_list_link (parse, next);

		  mh_parse_list_clean (name);
		  mh_parse_list_clean (indx);
		  
		  /* Replace HEAD with MH_PARSE_TYPE_TAG */
		  MH_PARSE_LIST_HEAD (parse) =
		    mh_parse_array_new (name,
					MH_PARSE_TOKEN (opener),
					indx,
					MH_PARSE_TOKEN (closer));
		  
#if defined (FAIL_IF_FINAL_PARSE)
		  /* If PARSE has no TAIL; replace PARSE with HEAD. */
		  mh_parse_list_clean (parse);
#endif

		  /* MH_PARSE_LIST with HEAD of CLOSE */
		  mh_parse_free_special (closer_list, false, false, true);

		}
	    }
	}
    }

}

static void
mh_parse_tag_arrays (mh_parse_t parse)
{
  mh_parse_t body;

  if (MH_PARSE_EMPTY == parse) return;

  /* If BODY is a list, look for ARRAYS */
  body = MH_PARSE_TAG_BODY (parse);
  if (body && mh_parse_type_p (body, MH_PARSE_TYPE_LIST))
    for (; body; body = MH_PARSE_LIST_TAIL (body))
      mh_parse_tag_list_arrays (body);
}


static void
mh_parse_arrays (mh_parse_t parse)
{
  mh_parse_recurse (parse, 
		    mh_parse_arrays, 
		    mh_parse_tag_arrays,
		    NULL,
		    NULL);
}

/*************************************************************************
 *
 * MH_PARSE_TAGS ()
 *
 */
static void
mh_parse_list_tags (mh_parse_t parse)
{
  /* {LIST head tail} ==> {LIST p0 p1 p2 ... NULL} */
  mh_parse_t head = MH_PARSE_LIST_HEAD (parse);
  mh_parse_t tail = MH_PARSE_LIST_TAIL (parse);

  /* Find a match of {DELIMITER '<'} */
  if (mh_parse_token_match_p (head, MH_TOKEN_DELIMITER, "<"))
    {
      /* Find a match of {DELIMITER '>'} */
      mh_parse_t close_list = mh_parse_list_match 
	(tail, MH_TOKEN_DELIMITER, ">");

      if (MH_PARSE_EMPTY != close_list)
	{
	  mh_parse_t opener, body, closer;

	  /* OPEN is MH_PARSE_TYPE_TOKEN */
	  opener = head;
	  assert (true);

	  /* BODY is MH_PARSE_LIST; possibly points to CLOSE_LIST */
	  body = (tail == close_list ? MH_PARSE_EMPTY : tail);
	  assert (true);

	  /* CLOSE is MH_PARSE_TYPE_TOKEN */
	  closer = MH_PARSE_LIST_HEAD (close_list);
	  assert (true);

	  /* Terminate BODY */
	  assert (true);
	  mh_parse_list_link
	    (mh_parse_list_prev (parse, close_list),
	     MH_PARSE_EMPTY);

	  /* If BODY has no TAIL use HEAD */
	  mh_parse_list_clean (body);

	  /* Replace HEAD with MH_PARSE_TYPE_TAG */
	  MH_PARSE_LIST_HEAD (parse) =
	    mh_parse_tag_new (MH_PARSE_TOKEN (opener),
			      body, 
			      MH_PARSE_TOKEN (closer));

	  /* Replace TAIL with subsequent */
	  mh_parse_list_link (parse, MH_PARSE_LIST_TAIL (close_list));

	  /* Memory Manage */

	  /* MH_PARSE_LIST with HEAD of OPEN */
	  mh_parse_free_special (opener, false, false, true);
	  /* MH_PARSE_LIST with HEAD of CLOSE */
	  mh_parse_free_special (closer,     false, false, true);
	  mh_parse_free_special (close_list, false, false, true);
	}
    }
}

static void
mh_parse_tags (mh_parse_t parse)
{
  mh_parse_recurse (parse,
		    mh_parse_tags,
		    NULL,
		    NULL,
		    mh_parse_list_tags);
}

/*************************************************************************
 *
 * MH_PARSE_TAG_OPERANDS ()
 *
 * Any operands in a tag parse that are not seperated by whitespace
 * are in fact one operand. Thus 
 *    <foo <get-var dir>/<get-var file>>
 * must parse to
 *    <foo <concat <get-var dir> "/" <get-var file>>>
 *
 * Questions about
 *    <foo <get-var x><get-var y> = 10>
 * or
 *    <foo <get-var x><get-var y>[2]>
 */
/* TAG OPERANDS are delimited by any parse that is not typed as
   MH_PARSE_TYPE_TAG or a parse typed as MH_PARSE_TYPE_TOKEN with a
   token type that is not MH_TOKEN_WORD. */
static boolean_t
mh_parse_tag_list_operand_delimiter (mh_parse_t parse)
{
  return 
    ! mh_parse_type_p (parse, MH_PARSE_TYPE_TAG) &&
    ( mh_parse_type_p (parse, MH_PARSE_TYPE_TOKEN) &&
      ! mh_parse_token_match_type_p (parse, MH_TOKEN_WORD));
}

static void
mh_parse_tag_list_operands (mh_parse_t parse)
{
  /* Group delimited parses */
  mh_parse_t head = MH_PARSE_LIST_HEAD (parse);
  mh_parse_t tail = MH_PARSE_LIST_TAIL (parse);

  /* Find a match of MH_TOKEN_WORD */
  if (mh_parse_type_p (head, MH_PARSE_TYPE_TAG) ||
      mh_parse_token_match_type_p (head, MH_TOKEN_WORD))
    {
      /* Find a match of anything but MH_TOKEN_WORD */
      mh_parse_t close_list = mh_parse_list_find_tail
	(tail, 
	 (mh_parse_tester_t) mh_parse_tag_list_operand_delimiter,
	 (mh_parse_maparg_t) NULL);

      if (tail != close_list)
	{
	  /* First, bundle everything between PARSE and one before
	     CLOSE_LIST into a new parse of TAG.  Then replace PARSE
	     with the bundle. */
	  mh_parse_t bundle = mh_parse_dup (parse);

	  /* Close off the bundle */
	  mh_parse_list_link
	    (mh_parse_list_prev (bundle, close_list),
	     MH_PARSE_EMPTY);

	  /* Add CONCAT to bundle */
	  bundle = mh_parse_list_new
	    (mh_parse_token_make (MH_TOKEN_WORD, "%%concat"),
	     mh_parse_list_new   (mh_parse_token_make (MH_TOKEN_SPACE, " "),
				  bundle));

	  /* Replace PARSE with the bundle */
	  MH_PARSE_LIST_HEAD (parse) = mh_parse_tag_new
	    (mh_token_new (MH_TOKEN_DELIMITER,"<"),
	     bundle, 
	     mh_token_new (MH_TOKEN_DELIMITER, ">"));

	  /* Replace TAIL with subsequent */
	  mh_parse_list_link (parse, close_list);

	}
    }
}


static void
mh_parse_tag_operands (mh_parse_t parse)
{
  mh_parse_t body;

  if (MH_PARSE_EMPTY == parse) return;

  /* If BODY is a list, look for OPERANDS */
  body = MH_PARSE_TAG_BODY (parse);
  if (body && mh_parse_type_p (body, MH_PARSE_TYPE_LIST))
    for (; body; body = MH_PARSE_LIST_TAIL (body))
      mh_parse_tag_list_operands (body);
}

static void
mh_parse_operands (mh_parse_t parse)
{
  mh_parse_recurse (parse,
		    mh_parse_operands,
		    mh_parse_tag_operands,
		    NULL,
		    NULL);
}


/*************************************************************************
 *
 * MH_PARSE()
 *
 */
static char *
strsub (char *str, size_t length)
{
  char *result = (char *) xmalloc (1 + length);
  result[0] = '\0';
  strncat (result, str, length);
  return result;
}

static mh_parse_t
mh_parse_string (mh_token_t token,
		 boolean_t  copy_p)
{
  mh_parse_t parse;

  /* TOKEN is MH_TOKEN_STRING */
  string_t   string = mh_token_string (token);

  /* STRING has quotes; we don't want those quotes */
  string_t unquoted = strsub (string + 1, strlen (string) - 2);

  /* Remove escapes to prepare for recursion */
  mh_string_to_string_escaped (unquoted);

  /* Hmmmmm */
  parse = mh_parse ("stdin", unquoted);

  free (unquoted);

  return mh_parse_string_new (parse);
}

static mh_parse_t
mh_parse_list_head (mh_token_t token,
		    boolean_t  copy_p)
{
  return mh_token_match_type_p (token, MH_TOKEN_STRING)
    ? mh_parse_string    (token, copy_p)
    : mh_parse_token_new (token, copy_p);
}
    
/* Share TOKEN */
static mh_parse_t
mh_parse_list_from_tokens (mh_token_list_t tokens,
			   boolean_t       copy_p)
{
  return tokens == NULL
    ? MH_PARSE_EMPTY
    : (mh_parse_list_new
       (mh_parse_list_head        (mh_token_list_token (tokens), copy_p),
	mh_parse_list_from_tokens (mh_token_list_next  (tokens), copy_p)));
}

extern void
mh_token_list_write (mh_token_list_t list,
		     FILE      *file);

extern mh_parse_t 
mh_parse (string_t str_source,
	  string_t str)
{
  mh_token_list_t tokens;
  mh_parse_t      parse;
  string_t        str_save;

  /* Scan */
  str_save = str = strdup (str);
  tokens = mh_scan (&str);

  if (*str)
    {
      char message [128];
      /* The only scanner error!? */
      sprintf (message,
	       "scanner: unterminated string starting at '%.20s' in '%s'",
	       str, str_save);

      /* Make it VARARGS soon; avoid the above */
      mh_parser_exception
	(str_source,
	 /* This misses newlines in strings! */
	 mh_token_list_line_number (tokens, NULL),
	 message);

      free (str_save);
      return MH_PARSE_EMPTY;
    }

  free (str_save);

  /* TOKENS will be NULL for STR of "" */

  /* Immediately AXE comments */
  tokens = mh_token_list_delete_type (tokens, MH_TOKEN_COMMENT, NULL);

  parse = mh_parse_list_from_tokens (tokens, false);

  /* Memory Manage TOKENS; but not an individual TOKEN */
  mh_token_list_free_special (tokens, true, false);

  if (MH_PARSE_EMPTY != parse)
    {
      /* TAGS */
      mh_parse_tags (parse);

      /* TAG OPERANDS */
      mh_parse_operands (parse);

      /* ARRAYS */
      mh_parse_arrays (parse);

      /* KEYS */
      mh_parse_keys (parse);

      /* BLOCKS */
      mh_parse_blocks (parse);

      /* Replace {LIST <head>} with <head> */
      mh_parse_list_clean (parse);
    }

  /* Done! */
  return parse;
}

/* Wed Nov  6 16:27:20 1996.  */

#if defined (PARSE_TEST)

static void
parse_test_repeat (string_t string, unsigned long repeat)
{
  mh_parse_t parse;
  mh_token_list_t tokens;

  tokens = mh_scan (&string);
  tokens = mh_token_list_delete_type (tokens, MH_TOKEN_COMMENT, NULL);
  parse = mh_parse_list_from_tokens (tokens, false);
  /* Memory Manage TOKENS; but not an individual TOKEN */
  mh_token_list_free_special (tokens, true, false);

  while (repeat-- > 0)
    {
      mh_parse_t copy = mh_parse_copy_deep (parse);
      mh_parse_free (copy);
    }
}

extern int
main (int   argc,
      char *argv[])
{
  string_t recon;
#if 0
  string_t source =
    "
<defun foo &rest x><a>ghi< /a ></defun>\"
<jkl < mno > pqr> stu<>><\"<get-var foo>
\"<xxx array[index]=foo[<get-var ~::def[>]> foo=bar stuff::<get-var foo>";

  string_t source = "abc\"def\\\"123ghi\"jkl";
  string_t source = "abc<def <ghi>jkl/foo<bar>[]=10 <x>mno>pqr";
  string_t source = "abc\n\n\n   jkl  def";
  string_t source = "
<func link=foo=bar name=foo?=<random>
      part=foo[]=bar time=\"a time x\"=<time>
      date[]=12:1:00>";
  string_t source = "<set-var foo=10 foo[1]=20 <get-var-once foo>=30 <get-var-once foo>[2]=40 <get-var-once foo[1]>[2]=50>";

string_t source =
  "<defsubst parser::canonicalize-var :pcv-var :pcv-pack whitespace=delete>
  <defvar :pcv-pack \"^\">
  <if <not <match <get-var-once <get-var-once :pcv-var>> \"::\">>
      <set-var <get-var-once :pcv-var> =
	<get-var-once :pcv-pack>::<get-var-once <get-var-once :pcv-var>>>>
</defsubst>";
#endif

string_t source =
  "<defun foo>
     <ext \"\\\\123 and \\\"456 and \\\n and \\\t and \\\\789\">
  </defun>";

  mh_parse_t parse = mh_parse ("main", source);

  printf ("\nSTRING: %s\n\nPARSE:\n", source);
  mh_parse_show (parse);
  printf ("\n");

  recon = mh_parse_to_string (parse);
  printf ("\nPARSE_AS_STRING: %s\n\nIDENTICAL: %s\n",
	  recon,
	  0 == strcmp (recon, source) ? "YES" : "NO");

  parse = mh_parse_delete_space (parse);
  printf ("\nDELETED:\n");
  mh_parse_show (parse);
  
  mh_parse_free (parse);

  printf ("\n\nPARSE_LIST_AS_CONCAT\n");
  source =
    "<concat
<href a b>
  \"This \\\"is a test.\"
</href>>";

  parse = mh_parse ("main", source);
  mh_parse_show (parse);
  parse = mh_parse_list_as_concat
    (MH_PARSE_LIST_TAIL (MH_PARSE_TAG_BODY (parse)));
  mh_parse_show (parse);
  printf ("\n");
  /*  printf ("STRING-X: %s\n", mh_parse_to_string_1 (parse, -1)); */
  printf ("STRING-0: %s\n", mh_parse_to_string_at_depth (parse, 0));
  printf ("STRING-1: %s\n", mh_parse_to_string_at_depth (parse, 1));
  printf ("STRING-2: %s\n", mh_parse_to_string_at_depth (parse, 2));
  printf ("STRING-3: %s\n", mh_parse_to_string_at_depth (parse, 3));

  printf ("\n\nPARSE_LIST_SPLIT\n");
  source = "<foo a b c=1 d e=2 f>";
  parse = mh_parse ("main", source);
  mh_parse_show (parse);
  {
    mh_parse_t key, nonkey;
    mh_parse_list_split_keys
      (mh_parse_list_delete_space 
       (MH_PARSE_LIST_TAIL (MH_PARSE_TAG_BODY (parse))),
       &nonkey, &key);
    mh_parse_show (nonkey);
    printf ("\n");
    mh_parse_show (key);
    printf ("\n");
  }

#if 0
  source = "<abc>";
  parse_test_repeat (source, 200000L);
#endif

#if 0
  printf ("\nQuotes[5](%d) : %s\nQuotes[10](%d): %s",
	  mh_string_backslashes_at_depth (5),
	  mh_string_quotes_at_depth (5),
	  mh_string_backslashes_at_depth (10),
	  mh_string_quotes_at_depth (10));
#endif
  return 0;
}
#endif /* defined (PARSE_TEST) */
