/*  wisp.h: -*- C -*- Data structures for interfacing with the backend. */

/* Author: Brian J. Fox (bfox@ua.com) Fri Mar 31 13:58:06 1995. */

/* This file is part of <Meta-HTML>(tm), a system for the rapid
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

	http://www.metahtml.com/COPYING
*/

#if !defined (_WISP_H_)
#define _WISP_H_

#if defined (__cplusplus)
extern "C"
{
#endif

typedef enum { LT_string, LT_number, LT_cons, LT_symbol, LT_nil } WispType;

typedef struct WISPOBJECT WispObject;
typedef struct
{
  WispObject *car;
  WispObject *cdr;
} Cons;

typedef struct
{
  char *pname;
  WispObject *value;
} WispSymbol;

struct WISPOBJECT
{
  WispType type;
  union
  {
    char *string;
    double number;
    Cons cons;
    WispSymbol *symbol;
  } val;
};

/* Here reside some macros to make life more palatable. */
#define CAR(obj) ((obj)->val.cons.car)
#define CDR(obj) ((obj)->val.cons.cdr)
#define CADR(obj) CAR (CDR (obj))
#define CDAR(obj) CDR (CAR (obj))
#define CDDR(obj) CDR (CDR (obj))
#define CAAR(obj) CAR (CAR (obj))

#define STRING_VALUE(obj)  ((obj)->val.string)
#define NUMBER_VALUE(obj)  ((obj)->val.number)
#define STRING_LENGTH(obj) (strlen (STRING_VALUE (obj)))
#define SYMBOL_PNAME(obj)  ((obj)->val.symbol->pname)
#define SYMBOL_VALUE(obj)  ((obj)->val.symbol->value)
#define SETQ(symbol_object, value) (SYMBOL_VALUE (symbol_object) = value)

/* Macros which check the type. */
#define WISP_TYPE(obj) ((obj)->type)
#define CHECK_TYPE(obj, type) (((obj) != (WispObject *)NULL) && \
			       (WISP_TYPE (obj) == type))

#define CONS_P(obj) CHECK_TYPE(obj, LT_cons)
#define STRING_P(obj) CHECK_TYPE(obj, LT_string)
#define NUMBER_P(obj) CHECK_TYPE(obj, LT_number)
#define SYMBOL_P(obj) CHECK_TYPE(obj, LT_symbol)
#define LIST_P(obj) (CONS_P (obj) && (CONS_P (CDR (obj)) || NIL_P (CDR (obj))))
#define LIST_STRINGS(s1, s2) (make_list (make_string_object (s1), \
					 make_string_object (s2)))

/* The nil value is a static structure containing LT_nil. */
extern WispObject wisp_nil_value;
#define NIL &wisp_nil_value
#define NIL_P(obj) ((obj) == NIL)

/* The test for a character to see if it is self_delimiting. */
#define self_delimiting(x) \
  (((x) == '"') || ((x) == '\'') || \
   ((x) == '(') || ((x) == ')') || ((x) == ';'))

#if !defined (whitespace)
#  define whitespace(c) ((c == ' ') || (c == '\t') || (c == '\r'))
#endif /* !whitespace */

#if !defined (whitespace_or_newline)
#  define whitespace_or_newline(c) (whitespace (c) || (c == '\n'))
#endif /* !whitespace_or_newline */

#define SET_POINTER(p, val)  do { if (p) *p = val; } while (0)

/*******************************************************************/
/*								   */
/*		  Externally Visible Functions in wisp.c	   */
/*								   */
/*******************************************************************/

extern WispObject *wisp_from_string (char *string);
extern char       *string_from_wisp (WispObject *object);
#define string_to_wisp(x) wisp_from_string (x)
#define wisp_to_string(x) string_from_wisp (x)
extern void wisp_push_input_string (char *string);
extern void wisp_pop_input_string (void);

extern WispObject *make_string_object (char *string);
extern WispObject *make_number_object (double number);
extern WispObject *make_cons (WispObject *car, WispObject *cdr);
extern WispObject *wisp_read (void);
extern void	   gc_wisp_free (WispObject *object);
extern void	   gc_wisp_objects (void);
extern void	   gc_steal (WispObject *object);

/* Lisp-Like functions for manipulating our wisp data structures. */
extern WispObject *copy_object (WispObject *object);
extern WispObject *wisp_append (WispObject *list, WispObject *object);
extern WispObject *assoc (char *key, WispObject *list);
extern char       *sassoc (char *key, WispObject *list);
extern WispObject *wisp_nth (int n, WispObject *list);
extern int         wisp_length (WispObject *object);

/* Return the wisp-readable representation of STRING.  That is, create a
   new string from STRING such that:

   strcmp (string_from_wisp (wisp_from_string (STRING)), STRING) == 0 */
extern char *wisp_readable (char *string);

#if defined (__cplusplus)
}
#endif

#endif /* _WISP_H_ */
