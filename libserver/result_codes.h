/* result_codes.h: -*- C -*-  List of HTTP result codes. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Nov 12 11:57:45 1995. */

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

#if !defined (_RESULT_CODES_H_)
#define _RESULT_CODES_H_ 1

/* Possible values for result->result_code. */
#define res_SUCCESS	          200
#define res_CREATED	          201
#define res_ACCEPTED	          202
#define res_NON_AUTHORATIVE_INFO  203
#define res_NO_CONTENT	          204
#define res_RESET_CONTENT         205

#define res_MULTIPLE_CHOICES      300
#define res_MOVED_PERMANENTLY     301
#define res_MOVED_TEMPORARILY     302
#define res_SEE_OTHER	          303
#define res_NOT_MODIFIED          304

#define res_BAD_REQUEST	          400
#define res_UNAUTHORIZED          401
#define res_PAYMENT_REQUIRED      402
#define res_FORBIDDEN	          403
#define res_NOT_FOUND	          404
#define res_METHOD_NOT_ALLOWED    405
#define res_NONE_ACCEPTABLE       406
#define res_PROXY_AUTH_REQ        407
#define res_REQUEST_TIMEOUT       408
#define res_CONFLICT	          409
#define res_GONE	          410
#define res_AUTHORIZATION_REFUSED 411

#define res_INTERNAL_SERVER_ERROR 500
#define res_NOT_IMPLEMENTED	  501
#define res_BAD_GATEWAY		  502
#define res_SERVICE_UNAVAILABLE   503
#define res_GATEWAY_TIMEOUT	  504

#if defined (__cplusplus)
extern "C"
{
#endif

extern char *mhttpd_result_string (int code);
extern char *mhttpd_result_reason (int code);

#if defined (__cplusplus)
}
#endif

#endif /* !_RESULT_CODES_H_ */
