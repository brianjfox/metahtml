/* scan.h: -*- C -*-  */

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

#if !defined (_MH_SCAN_H_)
#define _MH_SCAN_H_ 1

#include "compiler/compile.h"

/****************************************************************************
 *
 * SCAN (TOKENIZATION)
 *
 *
 */
typedef enum
{
  MH_TOKEN_NEWLINE,		/* <newline> */
  MH_TOKEN_SPACE,		/* <whitespace except newline> */
  MH_TOKEN_COMMENT,		/* ;;; <character except newline>* */
  MH_TOKEN_DELIMITER,		/* '<', '>', '[', ']', '=' */
  MH_TOKEN_STRING,		/* '"' <character except \" and \\>* '"' */
  MH_TOKEN_WORD,		/* <character except space and newline>* */
} mh_token_type_t;

typedef struct mh_token      *mh_token_t;
typedef struct mh_token_list *mh_token_list_t;

extern mh_token_list_t
mh_scan (string_t *string);

typedef void *mh_token_maparg_t;
typedef mh_token_t (*mh_token_mapper_t) (mh_token_t        token,
					 mh_token_maparg_t argument);

/*
 *
 * MH_TOKEN_T
 *
 */
extern mh_token_type_t
mh_token_type (mh_token_t token);

extern string_t
mh_token_string (mh_token_t token);

extern unsigned int
mh_token_string_length (mh_token_t token);

extern mh_token_t
mh_token_new_substring (mh_token_type_t type,
			string_t        string,
			unsigned int    length);

extern void
mh_token_fill_substring (mh_token_t      token,
			 mh_token_type_t type,
			 string_t        string,
			 unsigned int    length);

static inline mh_token_t
mh_token_new (mh_token_type_t type,
	      string_t        string)
{
  return mh_token_new_substring
    (type, string, strlen (string));
}

static inline mh_token_t
mh_token_dup (mh_token_t token)
{
  return mh_token_new
    (mh_token_type (token),
     strdup (mh_token_string (token)));
}

static inline void
mh_token_fill (mh_token_t      token,
	       mh_token_type_t type,
	       string_t        string)
{ return mh_token_fill_substring (token, type, string, strlen (string)); }

extern void
mh_token_copy (mh_token_t dst,
	       mh_token_t src);

extern void
mh_token_free (mh_token_t token);

static inline boolean_t
mh_token_match_type_p (mh_token_t      token,
		       mh_token_type_t type)
{ return type == mh_token_type (token); }


static inline boolean_t
mh_token_match_p (mh_token_t      token,
		  mh_token_type_t type,
		  string_t        string)
{ 
  return
    mh_token_match_type_p (token, type) &&
    0 == strcmp (mh_token_string (token), string);
}

static inline boolean_t
mh_token_equal_p (mh_token_t token1,
		  mh_token_t token2)
{ 
  return mh_token_match_p
    (token2, mh_token_type (token1), mh_token_string (token1));
}

extern boolean_t
mh_token_whitespace_p (mh_token_t token);

extern string_t
mh_token_to_string (mh_token_t token);

extern mh_token_t
mh_string_to_token (string_t *string);

/*
 *
 * MH_TOKEN_LIST_T
 *
 */
extern mh_token_t
mh_token_list_token (mh_token_list_t list);

extern mh_token_list_t
mh_token_list_next (mh_token_list_t list);

extern mh_token_list_t
mh_token_list_prev (mh_token_list_t list);

extern mh_token_list_t
mh_token_list_make (mh_token_t      token,
		    mh_token_list_t list);

extern void
mh_token_list_free_special (mh_token_list_t list,
			    boolean_t next_too_p,
			    boolean_t token_too_p);

static inline void
mh_token_list_free (mh_token_list_t list)
{ mh_token_list_free_special (list, true, true); }  

extern boolean_t
mh_token_list_equal_p (mh_token_list_t list1,
		       mh_token_list_t list2);

extern mh_token_list_t
mh_token_list_append (mh_token_list_t list1,
		      mh_token_list_t list2);

extern mh_token_list_t
mh_token_list_map (mh_token_list_t   list,
		   mh_token_mapper_t mapper,
		   mh_token_maparg_t argument);

extern void
mh_token_list_walk (mh_token_list_t   list,
		    mh_token_mapper_t mapper,
		    mh_token_maparg_t argument);

extern mh_token_list_t
mh_token_list_match_type (mh_token_list_t list,
			  mh_token_type_t type);

extern mh_token_list_t
mh_token_list_match (mh_token_list_t list,
		     mh_token_type_t type,
		     string_t        string);

extern string_t
mh_token_list_to_string_until_token (mh_token_list_t list,
				     mh_token_t      token);

static inline string_t
mh_token_list_to_string (mh_token_list_t list)
{ return mh_token_list_to_string_until_token (list, NULL); }

extern unsigned int
mh_token_list_type_count (mh_token_list_t list,
			  mh_token_type_t type);

extern unsigned int
mh_token_list_type_count_until_token (mh_token_list_t list,
				      mh_token_type_t type,
				      mh_token_t      token);

static inline unsigned int
mh_token_list_newline_count (mh_token_list_t list)
{ return mh_token_list_type_count (list, MH_TOKEN_NEWLINE); }

static inline unsigned int
mh_token_list_newline_count_until_token (mh_token_list_t list,
					 mh_token_t      token)   
{ return mh_token_list_type_count_until_token
    (list, MH_TOKEN_NEWLINE, token); }

extern unsigned int
mh_token_list_line_number (mh_token_list_t list,
			   mh_token_t      token);

extern unsigned int
mh_token_list_match_count (mh_token_list_t list,
			   mh_token_type_t type,
			   string_t        string);

extern mh_token_list_t
mh_token_list_delete_type (mh_token_list_t list,
			   mh_token_type_t type,
			   mh_token_list_t *deleted);


#endif /* ! _MH_SCAN_H_ */
