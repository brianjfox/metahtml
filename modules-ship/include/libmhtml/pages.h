/*  pages.h: Declarations of page manipulation functions in pages.c. */

/* Author: Brian J. Fox (bfox@ua.com) Wed May 31 12:13:59 1995. */

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

#if !defined (_PAGES_H_)
#define _PAGES_H_

#if defined (__cplusplus)
extern "C"
{
#endif

/* Defines which make the caller's code more readable. */
#define PAGE BPRINTF_BUFFER
#define page_create_page bprintf_create_buffer
#define page_delete_page(p) do { page_free_page (p); p = NULL; } while (0)
#define page_copy_page(page) (PAGE *)bprintf_copy_buffer (page)
#define pprintf bprintf
#define vpprintf vbprintf

/* A structure for holding onto a list of pages and offsets. */
typedef struct { PAGE *page; int start; int *search_start_modified; } PagePDL;
extern PagePDL *page_pdl_page (int offset);
extern void page_pop_page (void);
extern void page_push_page (PAGE *page, int start, int *search_start_modified);
extern PagePDL *page_pdl_tos (void);

/* Make PAGE contain CONTENTS, and nothing else. */
extern void page_set_contents (PAGE *page, char *contents);

/* Free the entire page, including any attachements. */
extern void page_free_page (PAGE *page);

/* Return a new buffer containing the page template stored in FILENAME.
   If the file couldn't be found or read, then a NULL pointer is returned
   instead. */
extern PAGE *page_read_template (char *filename);

/* Do some substitutions in PAGE; that is, substitute THIS WITH_THAT in PAGE.
   The substitutions happen in place, and the return value is the number of
   characters added or deleted.  THIS is a regular expression. */
extern int page_subst_in_page (PAGE *page, char *ths, char *with_that);

/* Do some substitutions in PAGE; that is, substitute THIS WITH_THAT in PAGE.
   The substitutions happen in place, and the return value is the number of
   characters added or deleted before PIVOT.  THIS is a regular expression.
   PIVOT is a pointer to an integer which is adjusted depending on the number
   of characters added or deleted before it.  An NULL pointer means skip this
   calculation. */
extern int page_subst_in_page_pivot (PAGE *page, char *ths, char *with_that, int *pivot);

/* Return the offset of STRING in PAGE.  Start the search at START.
   The absolute index is returned, not the offset from START, so
   the value is suitable for resubmission to this function as START value.
   A value of -1 indicates that the STRING couldn't be found. */
extern int page_search (PAGE *page, char *string, int start);

/* Get both the start and end points of STRING in PAGE.  The search is
   started at START, and the return value is the absolute offset of
   STRING in page.  END (if non-null) gets the absolute offset of
   the end of the match. The search is caseless by default. */
extern int page_search_boundaries (PAGE *page, char *string, int start,
				   int *end);

/* Grovel through PAGE deleting <INPUT ... name="TAG" ...> constructs, where
   TAG matches the value of the NAME field.
   Returns the number of fields that were deleted. */
extern int page_delete_input (PAGE *page, char *tag);

/* Grovel through PAGE changing the RHS of "value=" statements found within
   `<input name="TAG" value="xxx"...>' structures.
   Returns the number of tags that were changed. */
extern int page_set_form_input_value (PAGE *page, char *tag, char *value);

/* Given TICKS (the number of seconds since the epoch), return a string
   representing that date in the format the HTTP header mandate.  The
   string returned comes from a static buffer, the caller must manually
   save it away if it is not to be used immediately. */
extern char *http_date_format (long ticks);

/* Still in use by utlitiies/imagemap/imagemap.c */
#define PAGE_INSERT_HTTP_HEADER 1
#if defined (PAGE_INSERT_HTTP_HEADER)
/* Insert a standard HTTP header at the start of page if it isn't already
   a redirection specification.  EXPIRATION is one of:

      page_NOT_EXPIRED:	To produce no special expiration date.
      page_IS_EXPIRED:	To inhibit server/browser caching.
      page_EXPIRES_NOW:	To give the page an expiration date of right now.
      integer > 0:	To cause the page to expire that many minutes in the
			future. */
#define page_NOT_EXPIRED  0
#define page_EXPIRES_NOW -1
#define page_IS_EXPIRED  -2
extern void page_insert_http_header (PAGE *page, int expiration);
#endif /* PAGE_INSERT_HTTP_HEADER */

#if defined (PAGE_SET_COOKIE)
/* Set a mime header in PAGE giving it NAME and VALUE as a HTTP-COOKIE.
   PAGE should already have a full Mime header in it. */
extern void page_set_cookie (PAGE *page, char *name, char *value, char *path);
#endif /* PAGE_SET_COOKIE */

#if defined (HTTP_RETURN_IMAGE)
/* Given the pathname of a image file, return a page containing the image
   prefixed with the correct HTTP magic. */
extern PAGE *http_return_image (char *name, char *type);
#endif /* HTTP_RETURN_IMAGE */

/* Like printf, but stores the output in debugger_output. */
extern void page_debug (char *format, ...);
extern char *page_debug_buffer (void);
extern void page_debug_clear (void);

/* Like printf, but stores the output in syserr_output. */
extern void page_syserr (char *format, ...);
extern char *page_syserr_buffer (void);
extern void page_syserr_clear (void);

/* Create a generic error page with placeholders for <HEADER>, <ERROR-MESSAGE>,
   <DEBUGGING-OUTPUT>, <RETURN-TO-URL> and <FOOTER>. */
extern BPRINTF_BUFFER *page_error_page (void);

/* Clean up an error page if it needs it. */
extern void page_clean_up (PAGE *page);

/* Extract the value side of a randomly named variable in PAGE.
   For example, if PAGE contains `<DOCTITLE="this">', then the
   call: page_assigned_label (page, "DOCTITLE") returns "this". */
extern char *page_assigned_label (PAGE *page, char *tag);

/* Find the inclusive boundaries in PAGE of a simple tag named TAG.
   Returns the boundaries in STARTP and ENDP, and a non-zero value
   to the caller if the simple tag TAG was found, else -1 is
   returned.  STARTP is both input and output; it controls the starting
   location in PAGE of the search. If TAG was "FOO", then the bounds ofo
   "<FOO ....>" are returned.  Case is insignificant in the search. */
extern int page_simple_tag_bounds (PAGE *page, char *tag,
				   int *startp, int *endp);

/* Given that PAGE->buffer + POINT is pointing at the start of a simple
   tag, find the location of the matching close bracket.  This counts
   double quotes, and brackets outside of double quotes.  Returns the
   offset in PAGE->buffer just after the matching close bracket, or -1
   if the matching end could not be found. */
extern int page_find_tag_end (PAGE *page, int start);

/* Find and extract the inclusive boundaries in PAGE of a simple tag
   named TAG.  Returns the boundaries in STARTP and ENDP, and a string
   which is the extract simple tag in its entirety, or a NULL pointer
   if TAG wasn't found.  STARTP is both input and output; it controls
   the starting location in PAGE of the search. If TAG was "FOO", then
   the bounds of "<FOO ....>" are returned.  Case is insignificant in
   the search. */
extern char *page_simple_tag_extract (PAGE *page, char *tag,
				      int *startp, int *endp);

/* Find the inclusive boundaries in PAGE of a complex tag named TAG.
   Returns the boundaries in STARTP and ENDP, and a non-zero value
   to the caller if the simple tag TAG was found, else -1 is
   returned.  STARTP is both input and output; it controls the starting
   location in PAGE of the search.  If TAG was "FOO", then the bounds
   of "<FOO ...> .... </FOO>" are returned.  Case is insignificant in
   the search. */
extern int page_complex_tag_bounds (PAGE *page, char *tag,
				    int *startp, int *endp);

/* Find and extract the inclusive boundaries in PAGE of a complex tag
   named TAG.  Returns the boundaries in STARTP and ENDP, and a string
   which is the extracted complex tag in its entirety, or a NULL
   pointer if TAG wasn't found.  STARTP is both input and output; it
   controls the starting location in PAGE of the search.  If TAG was
   "FOO", then the bounds of "<FOO ...> .... </FOO>" are returned.
   Case is insignificant in the search. */
extern char *page_complex_tag_extract (PAGE *page, char *tag,
				       int *startp, int *endp);

/* Find out if <indicator> belongs to the complex <tag>, and return 3 values:
      1) The output of the function is non-zero if INDICATOR was found,
         It is 1 if the indicator was found and it was ours, -1 if the
	 indicator was found and it wasn't ours, and 0 if the indicator
	 wasn't found at all.
      2) The value of STARTP gets the offset of INDICATOR,
      3) the value of ENDP gets the offset of the end of INDICATOR. */
extern int page_indicator_owned_by (PAGE *page, char *indicator, char *tag,
				    int *startp, int *endp, int search_start);

/* The name of the file most recently loaded with page_read_template (). */
extern char *page_last_read_filename;

/* The time of the most recent modification of all pages loaded through
   page_read_template (). */
extern unsigned long page_most_recent_modification_time;

#if defined (__cplusplus)
}
#endif

#endif /* !_PAGES_H_ */
