/* parse.h: -*- C -*- Header file for compiler's parser. */

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
/*
 * METAHTML is a 'string language' in which the primary expressed values are
 * strings and thus one might think that strings alone ought to be sufficient
 * in the parser output.  The job of the parse is to do the lexical analysis
 * required to produce numbers, strings and symbols.  In the process the
 * string is retained (see below) and the compiler has the option of using the
 * string or the number / symbol as appropriate for the function/special-form
 * to be compiled.
 */

#if !defined (_MH_PARSE_H_)
#define _MH_PARSE_H_ 1

#include "compiler/compile.h"
#include "compiler/scan.h"

typedef enum
{
  /* Primitive Parses */
  MH_PARSE_TYPE_TOKEN,		/* token */

  /* Compound Parses */
  MH_PARSE_TYPE_STRING,		/* " <parse> " */
  MH_PARSE_TYPE_KEY,		/*  <parse> '=' <parse> */ 
  MH_PARSE_TYPE_ARRAY,		/*  <parse> '[' <parse> ']' */
  MH_PARSE_TYPE_TAG,		/* '<' <parse> '>' */
  MH_PARSE_TYPE_BLK,		/* <tag> <parse> </tag> */
  MH_PARSE_TYPE_LIST		/*  <parse>+ */
} mh_parse_type_t;

#define MH_NUMBER_OF_PARSE_TYPES (1 + MH_PARSE_TYPE_LIST)

/*
 * Seems {LIST {TOKENS tok-1} {TOKENS tok-2} ...} is identical to
 * {TOKENS tok-1 tok-2 ...}; however, the difference occurs when the
 * parse is {LIST {TOKENS tok-1} {KEY tok-2 {TAG ...}} ...}
 *
 */
struct mh_parse
{
  /* The type for this MH_PARSE object */ 
  mh_parse_type_t type;
# define MH_PARSE_TYPE( parse )     ((parse)->type)

  union {

    /* MH_PARSE_TYPE_TOKENS */
    mh_token_t  token;
#define MH_PARSE_TOKEN( parse )   ((parse)->u.token)

    /* MH_PARSE_TYPE_STRING */
    mh_parse_t  string;
#define MH_PARSE_STRING( parse )     ((parse)->u.string)

    /* MH_PARSE_TYPE_KEY

       MH_PARSE_TYPE_KEY is the parse for keywords in METAHTML tags.  A tag
       of "<... foo=<get-var bar> ...>" with one KEY containing parses
       with name={NAME foo}, delimiter="=", and VALUE={TAG ...}.  It
       is a parse error if name is not a NAME parse.  The delimiter
       can include arbitrary whitespace about it. */
    struct 
    {
      mh_parse_t  name;		/* NAME */
      mh_token_t  delimiter;
      mh_parse_t  value;	/* ANY */
    } key;
#define MH_PARSE_KEY_NAME( parse )      ((parse)->u.key.name)
#define MH_PARSE_KEY_DELIMITER( parse ) ((parse)->u.key.delimiter)
#define MH_PARSE_KEY_VALUE( parse )     ((parse)->u.key.value)

    /* MH_PARSE_TYPE_ARRAY

       Derived from 'name[index]' */
    struct 
    {
      mh_parse_t  name;
      mh_token_t  open;
      mh_parse_t  index;
      mh_token_t  close;
    } array;
#define MH_PARSE_ARRAY_NAME( parse )     ((parse)->u.array.name)
#define MH_PARSE_ARRAY_OPEN( parse )     ((parse)->u.array.open)
#define MH_PARSE_ARRAY_INDEX( parse )    ((parse)->u.array.index)
#define MH_PARSE_ARRAY_CLOSE( parse )    ((parse)->u.array.close)

    /* MH_PARSE_TYPE_TAG */
    struct
    {
      mh_token_t open;
      mh_parse_t body;
      mh_token_t close;
    } tag;
#define MH_PARSE_TAG_OPEN( parse )  ((parse)->u.tag.open)
#define MH_PARSE_TAG_BODY( parse )  ((parse)->u.tag.body)
#define MH_PARSE_TAG_CLOSE( parse ) ((parse)->u.tag.close)
		     
    /* MH_PARSE_TYPE_BLK */
    struct
    {
      mh_parse_t open;          /* TAG:  <defun ....> */
      mh_parse_t body;          /* TEXT: ... text ... */
      mh_parse_t close;		/* TAG:  </defun> */
    } blk;
#define MH_PARSE_BLK_OPEN( parse )  ((parse)->u.blk.open)
#define MH_PARSE_BLK_BODY( parse )  ((parse)->u.blk.body)
#define MH_PARSE_BLK_CLOSE( parse ) ((parse)->u.blk.close)

    /* MH_PARSE_TYPE_LIST

       MH_PARSE_TYPE_LIST is the parse for arbitrary list. NULL terminated*/
    struct
    {
      mh_parse_t head;
      mh_parse_t tail;
    } list;
#define MH_PARSE_LIST_HEAD( parse )   ((parse)->u.list.head)
#define MH_PARSE_LIST_TAIL( parse )   ((parse)->u.list.tail)

  } u;
};



/****************************************************************************
 *
 * PARSE 
 *
 *
 * Forward Declarations */
/*
 *
 */

extern mh_parse_t mh_parse (string_t str_source, string_t str);
extern mh_parse_type_t mh_parse_type (mh_parse_t parse);

static inline boolean_t
mh_parse_type_p (mh_parse_t parse, mh_parse_type_t type)
{
  return (type == mh_parse_type (parse));
}

extern void mh_parse_copy (mh_parse_t dest, mh_parse_t source);
extern mh_parse_t mh_parse_dup (mh_parse_t parse);
extern mh_parse_t mh_parse_copy_deep (mh_parse_t parse);
extern unsigned int mh_parse_count (mh_parse_t parse);
extern mh_parse_t mh_parse_token_new (mh_token_t token, boolean_t copy_p);
extern mh_parse_t mh_parse_token_make (mh_token_type_t type, string_t string);
extern mh_parse_t mh_parse_list_new (mh_parse_t head, mh_parse_t tail);
extern mh_parse_t mh_parse_list_append (mh_parse_t list1, mh_parse_t list2);
extern mh_parse_t mh_parse_list_copy (mh_parse_t list);
extern void mh_parse_list_free (mh_parse_t list);
extern unsigned int mh_parse_list_count (mh_parse_t parse);
extern mh_parse_t mh_parse_list_reverse (mh_parse_t parse);
extern mh_parse_t mh_parse_list_rest (mh_parse_t list, unsigned int n);
extern mh_parse_t mh_parse_list_nth (mh_parse_t list, unsigned int n);
extern mh_parse_t mh_parse_list_nth_nonkey (mh_parse_t list, unsigned int n);

extern void mh_parse_list_split_keys (mh_parse_t list,
				      mh_parse_t *list_of_nonkeys,
				      mh_parse_t *list_of_keys);

extern mh_parse_t mh_parse_list_delete_space (mh_parse_t parse);
extern mh_parse_t mh_parse_delete_space (mh_parse_t parse);
extern mh_parse_t mh_parse_list_delete_bounding_space (mh_parse_t parse);
extern mh_parse_t mh_parse_list_delete_interline_space (mh_parse_t parse);
extern mh_parse_t mh_parse_delete_interline_space (mh_parse_t parse);
extern mh_parse_t mh_parse_list_delete_key (mh_parse_t parse, string_t name);
extern mh_parse_t mh_parse_list_trim_whitespace (mh_parse_t parse);
extern mh_parse_t mh_parse_tag_operator (mh_parse_t tag);

extern string_t mh_parse_to_string_at_depth (mh_parse_t parse, int depth);

static inline string_t
mh_parse_to_string (mh_parse_t parse)
{
  return (mh_parse_to_string_at_depth (parse, 1));
}

extern string_t mh_parse_to_string_with_space (mh_parse_t parse);
extern mh_parse_t mh_parse_match_key (mh_parse_t parse, string_t   name);

extern boolean_t mh_parse_token_match_p (mh_parse_t parse,
					 mh_token_type_t type,
					 string_t string);

extern boolean_t mh_parse_token_match_type_p (mh_parse_t parse,
					      mh_token_type_t type);

extern boolean_t mh_parse_has_positional_reference_p (mh_parse_t parse);

typedef void *mh_parse_maparg_t;
typedef boolean_t (*mh_parse_tester_t) (mh_parse_t parse, mh_parse_maparg_t argument);

extern boolean_t mh_parse_list_every (mh_parse_t parse, 
				      mh_parse_tester_t tester,
				      mh_parse_maparg_t arg);

extern boolean_t mh_parse_list_any (mh_parse_t parse, mh_parse_tester_t tester,
				    mh_parse_maparg_t arg);

extern mh_parse_t mh_parse_list_find (mh_parse_t parse,
				      mh_parse_tester_t tester,
				      mh_parse_maparg_t arg);

extern mh_parse_t mh_parse_list_as_body (mh_parse_t body);
extern mh_parse_t mh_parse_list_as_concat (mh_parse_t parse);

#endif /* ! _MH_PARSE_H_ */
