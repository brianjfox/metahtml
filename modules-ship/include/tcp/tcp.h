/* tcp.h -- A Quick & Dirty TCP protocol. */

/* Brian J. Fox (bfox@ai.mit.edu): Wed Mar  8 12:15:38 1995 */

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

#if !defined (_TCP_H_)
#define _TCP_H_

#if defined (__cplusplus)
extern "C"
{
#endif

/* Set to non-zero to override the standard connect() timeout with a
   one of length TIME_OUT. This is useful for large networks where
   otherwise wwwproxy could wait for long periods while trying to
   connect to non-responding hosts. */
#define TCP_ALLOW_TIMEOUTS 1

/* Default number of seconds before timing out on connect call. Only
 used if ALLOW_TIMEOUTS is non-zero.*/
#define TCP_TIME_OUT 30

extern int tcp_to_host (char *host, char *service);
extern unsigned char *hostname_or_ip_to_address (char *hostname_spec);

#if defined (__cplusplus)
}
#endif

#endif /* _TCP_H_ */
