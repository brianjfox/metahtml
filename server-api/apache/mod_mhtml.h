/* mod_mhtml.h: -*- C -*-  DESCRIPTIVE TEXT. */

/*  Copyright (c) 2003 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Thu Apr  3 06:57:41 2003.  */

extern char *mr_location (HTTP_RESULT *result, char *existing_location);
extern char *mr_engine (HTTP_RESULT *result, char *existing_location);
extern char *mr_set_cookie (HTTP_RESULT *result, char *existing_location);
extern char *mr_expires (HTTP_RESULT *result, char *existing_location);
extern char *mr_last_modified (HTTP_RESULT *result, char *existing_location);
extern char *mr_status (HTTP_RESULT *result, char *existing_location);
extern MIME_HEADER **mime_headers_from_string (char *string, int *last_char);
/* In ../../libmhtml/symbols.c, of course. */
extern void package_pdl_remove (Package *p);
extern int AP_index;
extern int AP_slots;

/* In ../../libserver/http.c. */
extern void mhttpd_metahtml_engine (HTTP_RESULT *result);
extern HTTP_RESULT *mhtml_make_result ();

