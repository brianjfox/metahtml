/* globals.h: -*- C -*-  Global functions and variables for server library. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Nov 10 18:27:56 1995. */

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

#if !defined (_SERVER_GLOBALS_H_)
#define _SERVER_GLOBALS_H_ 1

#include "http.h"
#include "path_resolve.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* The current version of the Meta-HTML engine. */
extern char *sv_MHTML_VERSION;
extern char *sv_VersionString;

/* The document root directory. */
extern char *sv_DocumentRoot;

/* The session ID as gobbled from the URL, or from the HTTP Cookie.
   This is the value of the variable "SID". */
extern char *gbl_passed_sid;

/* When non-zero, this server and client can handle http cookies.
   MHTML::COOKIE-COMPATIBLE. */
extern int mhtml_cookie_compatible;

/* When non-zero, debugging messages are written to debug log. */
extern int mhttpd_debugging;

/* When non-zero, performance messages are written to debug log. */
extern int mhttpd_log_performance;

/* When non-zero, requests that have a referer are written to referer log. */
extern int mhttpd_log_referer;

/* When non-zero, write the name of the connecting browser to the agent log. */
extern int mhttpd_log_agent;

/* When non-null, this is the name of a Meta-HTML defsubst to run for
   each request the server receives. */
extern char *mhttpd_per_request_function;

/* When non-zero, this server communicates with the client using SSL
   security. */
extern int mhttpd_ssl_server;

/* Get the filename extension of FILENAME. */
extern char *mhtml_filename_extension (char *filename);

extern char *mhtml_concat (int count, ...);
extern char *mhtml_path_translate (char *path);
extern char *mhtml_user_translate (char *path);
extern void mhtml_get_sid (HTTP_REQUEST *request, char *path_info);
extern int mhtml_check_cookie_compatible (HTTP_REQUEST *request);
extern int mhttpd_page_redirect_p (PAGE *page);
extern void mhtml_gobble_argv (DOC_SPEC *spec, char *string);
extern Package *mhttpd_mime_headers_to_package (MIME_HEADER **, char *);
extern int mhttpd_read (int, void *, size_t);
extern int mhttpd_write (int, const void *, size_t);
extern char *mhttpd_gets (char *, size_t, int);
extern void mhttpd_initialize_ssl (void);
extern void mhttpd_negotiate_ssl (int connection);

#if defined (__cplusplus)
}
#endif

#endif /* !_SERVER_GLOBALS_H_ */

