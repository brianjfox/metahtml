/* http.h: -*- C -*-  DESCRIPTIVE TEXT. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Nov 10 14:17:13 1995. */

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

#if !defined (_HTTP_H_)
#define _HTTP_H_

#include "result_codes.h"

#if defined (__cplusplus)
extern "C"
{
#endif
typedef struct
{
  char *tag;
  char *value;
} MIME_HEADER;

#define flag_NONE       0
#define flag_KEEP_ALIVE 1

/* Stucture used for storing requests. */
typedef struct
{
  char *method;
  char *location;
  char *protocol;
  char *protocol_version;
  MIME_HEADER **headers;
  char *requester;
  char *requester_addr;
  int flags;
} HTTP_REQUEST;

/* Values used in DOC_SPEC->doc_type. */
#define doc_NONE		0
#define doc_PARSEABLE_MHTML	1 /* File of Meta-HTML. */
#define doc_EXTERNAL_CGI	2 /* Program to execute. */
#define doc_BASIC		3 /* Return according to mime-types. */
#define doc_SERVER_REDIRECT	4 /* The client must try again. */

typedef struct
{
  char *requested_path;		/* What the user asked for. */
  char *logical_path;		/* What the user should see. */
  char *physical_path;		/* What the server should see. */
  char *path_info;		/* Text after the physical filename. */
  char *query_string;		/* String following ? in URL. */
  char **argv;			/* Array of values after "?" in URL. */
  int argc;			/* Elements in argv. */
  char *content;		/* Information passed via POST. */
  int content_length;		/* Non-zero if content contains anything. */
  char *mime_type;		/* The MIME type gleaned from the extension. */
  int doc_type;			/* How to handle the document. */
} DOC_SPEC;

/* Structure used for returning a result. */
typedef struct
{
  HTTP_REQUEST *request;
  DOC_SPEC *spec;
  PAGE *page;
  int result_code;
} HTTP_RESULT;

#define ReqFunArgs HTTP_REQUEST *request, int fd

typedef HTTP_RESULT *ReqFun (ReqFunArgs);

typedef struct
{
  char *name;
  ReqFun *handler;
} METHOD_HANDLER;

extern char *http_readline (int fd);
extern HTTP_REQUEST *http_read_request (int fd);
extern HTTP_RESULT *http_handle_request (HTTP_REQUEST *request, int fd);
extern HTTP_REQUEST *http_parse_request (char *line);
extern MIME_HEADER **mime_headers_from_stream (FILE *stream);
extern MIME_HEADER **mime_headers_from_fd (int fd);
extern char *mhttpd_get_mime_header (MIME_HEADER **headers, char *which);
extern void mhttpd_free_request (HTTP_REQUEST *request);
extern void mhttpd_debug_request (HTTP_REQUEST *req);
extern void mhttpd_reset_server_variables (void);
extern void mhttpd_handle_empty_page (HTTP_RESULT *result);

/* Yechh!  This is solely for making the Open Market Fast CGI implementation
   work without having to recompile this library separately for the engine.
   This makes me feel a little sick. */
extern char *fast_cgi_content;
extern int fast_cgi_content_length;

#if defined (__cplusplus)
}
#endif

#endif /* !_HTTP_H_ */
