/* result_codes.c: Produce result strings for server from code. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sun Nov 12 12:01:18 1995.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
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

	 http://www.metahtml.com/COPYING  */

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "result_codes.h"

#if defined (__cplusplus)
extern "C"
{
#endif

typedef struct
{
  int code;
  char *text;
} CODES_ALIST;

static CODES_ALIST codes[] = {
  { res_SUCCESS,		"OK" },
  { res_CREATED,		"Created" },
  { res_ACCEPTED,		"Accepted" },
  { res_NON_AUTHORATIVE_INFO,	"Non-Authorative_Information" },
  { res_NO_CONTENT,		"No Content" },

  { res_MULTIPLE_CHOICES,	"Multiple Choices" },
  { res_MOVED_PERMANENTLY,	"Moved Permanently" },
  { res_MOVED_TEMPORARILY,	"Found" },
  { res_SEE_OTHER,		"See Other" },
  { res_NOT_MODIFIED,		"Not Modified" },

  { res_BAD_REQUEST,		"Bad Request" },
  { res_UNAUTHORIZED,		"Unauthorized" },
  { res_PAYMENT_REQUIRED,	"Payment Required" },
  { res_FORBIDDEN,		"Forbidden" },
  { res_NOT_FOUND,		"Not Found" },
  { res_METHOD_NOT_ALLOWED,	"Method Not Allowed" },
  { res_NONE_ACCEPTABLE,	"None Acceptable" },
  { res_PROXY_AUTH_REQ,		"Proxy Auth Req" },
  { res_REQUEST_TIMEOUT,	"Request Timeout" },
  { res_CONFLICT,		"Conflict" },
  { res_GONE,			"Gone" },
  { res_AUTHORIZATION_REFUSED,	"Authorization Refused" },

  { res_INTERNAL_SERVER_ERROR,	"Internal Server Error" },
  { res_NOT_IMPLEMENTED,	"Not Implemented" },
  { res_BAD_GATEWAY,		"Bad Gateway" },
  { res_SERVICE_UNAVAILABLE,	"Service Unavailable" },
  { res_GATEWAY_TIMEOUT,	"Gateway Timeout" },

  { 0,		(char *)NULL }
};

char *
mhttpd_result_string (int code)
{
  register int i;
  static char buffer[256];
  int found = 0;

  for (i = 0; codes[i].text != (char *)NULL; i++)
    {
      if (code == codes[i].code)
	{
	  sprintf (buffer, "HTTP/1.0 %d %s", codes[i].code, codes[i].text);
	  found = 1;
	  break;
	}
    }

  if (!found)
    sprintf (buffer, "HTTP/1.0 %d MHTTPD Screwed Up Somehow", code);

  return (buffer);
}
char *
mhttpd_result_reason (int code)
{
  register int i;
  char *result = "No Reason Given";

  for (i = 0; codes[i].text != (char *)NULL; i++)
    {
      if (code == codes[i].code)
	{
	  result = codes[i].text;
	  break;
	}
    }

  return (result);
}

#if defined (__cplusplus)
}
#endif
