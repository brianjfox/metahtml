/*  forms.h: The data structures that we use to parse incoming forms. */

/* Author: Brian J. Fox (bfox@ua.com) Sat May 20 11:34:04 1995. */

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

#if !defined (_FORMS_H_)
#define _FORMS_H_
#include "symbols.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* Return the package containing only those items which were found in
   the posted material.  This excludes environment variables.  You
   must have first called forms_input_data () before you can get
   anything useful. */
extern Package *forms_posted_data (void);

/* Read the input data from all of the available sources.  This means
   the environment variables PATH_INFO and QUERY_STRING, the contents
   of standard input, if there is any, and the arguments passed into
   the CGI program.  Nothing is returned, the symbols and values are
   simply interned. The program arguments are returned in the item
   PROGRAM-ARGUMENTS. */
extern void forms_input_data (int argc, char *argv[]);

/* Read name/value pairs from BUFFER, and intern the symbols in PACKAGE.
   The pairs are delimited with ampersand (`&') or end of data.  The name
   is separated from the value by an equals sign (`=').  Space characters
   are encoded as plus signs (`+').  A percent sign (`%') is used to
   introduce two hex digits, which when coerced to an ASCII character is
   the result.  This mechanism is used to get plus signs into the name or
   value string, for example. */
extern void forms_parse_data_string (const char *input, Package *package);

/* Turn SYMBOLS into a string suitable for appending onto a URL.
   This means that we encode special characters, and write name
   value pairs into a new string.
   A newly allocated string is returned. */
extern char *forms_unparse_items (Symbol **symbols);

/* Get the value of the variable named by TAG.  Tag may contain an
   array index referent, in which case, that value is returned.
   The magic referent "tag[]" refers to each element of the array,
   separated by newlines. The current package is used unless TAG
   contains a package part, as in "foo::bar". */
extern char *forms_get_tag_value (char *tag);
#define sym_get_var(tag) forms_get_tag_value (tag)

/* Get the value in PACKAGE of the variable named by TAG.
   Passing a PACKAGE of NULL is the same as calling forms_get_tag_value. */
extern char *forms_get_tag_value_in_package (Package *package, char *tag);
#define pkg_get_var(pkg,tag) forms_get_tag_value_in_package (pkg, tag)

/* Give TAG VALUE as one of the values.
   Special syntax allows you to create an array from newline separated
   strings in VALUE, or to set a specific array element of TAG.
   The syntax "foo::tag[3]" sets the third array element of TAG in the
   package FOO, while the syntax "tag[]" arrayifies VALUE. */
extern void forms_set_tag_value (char *tag, char *value);
extern void forms_set_tag_value_in_package (Package *p, char *tag, char *val);

/* This is a bit of cruft.  You can put pointers in this list which, for
   one reason or the other, just *had* to be malloc'ed, but which normally
   would not have to be (e.g., forms_get_tag_value of an array reference).
   Then, later, when you think it is safe, you can gc them all.  Ugh. */
extern void forms_gc_remember (char *pointer);
extern void forms_gc_pointers (void);

#if defined (__cplusplus)
}
#endif

#endif /* !_FORMS_H_ */
