/* tcp.c -- Functions for talking to other machines via TCP services. */

/* Brian J. Fox (bfox@ai.mit.edu): Wed Mar  8 12:15:38 1995 */

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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <xmalloc/xmalloc.h>

#if !defined (hpux)
#include <arpa/inet.h>
#endif

#if !defined (HAVE_TYPE_SIG_T) || defined (__WINNT__)
typedef void (*sig_t) (int);
#endif

#include "tcp.h"

/* Because Unix is too stupid to make this a define.  This is
   worse than the ptrace stuff.  Value signifies Internet Protocol. */
#define IP 0

#define byte unsigned char

#if defined (__cplusplus)
extern "C"
{
#endif

static void connect_timed_out (void);
byte *hostname_or_ip_to_address (char *hostname_specifier);

/* Non-Zero means place an alarm around the call to socket(). */
int tcp_fast_connections = 0;

/* **************************************************************** */
/*								    */
/*			TCP Stream Functions			    */
/*								    */
/* **************************************************************** */

/* How come these don't come in a library somewhere?  Everyone could
   use one.  Like to open a SMTP connection, or talk, or anything. */

/* Number of seconds before timing out on connect call. */
static int connection_timeout_counter = TCP_TIME_OUT;

/* Non-zero means only allow TIME_OUT seconds for a connect () to
   succeed, instead of whatever the infernal network code allows. */
static int allow_time_outs = TCP_ALLOW_TIMEOUTS;

typedef void SIGFUN (int sig);

/* Open a filedes to HOST at port SERVICE.  HOST can either be the name of
   an internet host, or the ASCII representation of that host in dot format.
   If SERVICE is the name of a service, then it must exist on the local
   machine.  SERVICE can also be the ASCII representation of a decimal number,
   in which case it is interpreted as the port number to connect to.  Returns
   a valid file descriptor if successful, or -1 if not. */
int
tcp_to_host (char *host, char *service)
{
  struct sockaddr_in name;
  byte *address;
  int connection;

  address = hostname_or_ip_to_address (host);
  if (!address)
    return (-1);

  /* Prepare the socket name for binding. */
  memset (&name, 0, sizeof (name));

  name.sin_family = AF_INET;
  memcpy (&name.sin_addr.s_addr, address, 4);

  /* Find the port to use for the requested service. */
  if (isdigit (*service))
    name.sin_port = htons (atoi (service));
  else
    {
      struct servent *server;

      server = getservbyname (service, "tcp");
      if (!server)
	return (-1);
      name.sin_port = server->s_port;
    }

  connection = socket (PF_INET, SOCK_STREAM, IP);

  if (connection < 0)
    return (-1);

  /* Connect to the desired port.  We have a shorter timeout than
     the connect call uses by default. */
  {
    int error;

    if (allow_time_outs)
      {
	signal (SIGALRM, (sig_t)connect_timed_out);
	alarm (connection_timeout_counter);
	error = connect (connection, (struct sockaddr *)&name, sizeof (name));
	alarm (0);
	signal (SIGALRM, (SIGFUN *)NULL);
      }
    else
      error = connect (connection, (struct sockaddr *)&name, sizeof (name));

    if (error < 0)
      {
	close (connection);
	return (-1);
      }
  }
  
  return (connection);
}

static void
connect_timed_out (void)
{
  alarm (0);
}

static byte address_buffer[8];
byte *
hostname_or_ip_to_address (char *hostname_specifier)
{
  struct hostent *entry;

  /* Check for IP address. */
  {
    register int i, dots = 0, invalid = 0;

    for (i = 0; hostname_specifier[i]; i++)
      {
	if (hostname_specifier[i] == '.')
	  dots++;
	else if (!isdigit (hostname_specifier[i]))
	  {
	    invalid = 1;
	    break;
	  }
      }

    /* Convert to address in network order. */
    if (dots == 3 && !invalid)
      {
	int offset = 0;
	
	i = 0;
	while (hostname_specifier[i])
	  {
	    int number = 0;

	    while ((hostname_specifier[i] != '\0') &&
		   (hostname_specifier[i] != '.'))
	      number = (number * 10) + hostname_specifier[i++] - '0';

	    address_buffer[offset++] = number;

	    if (hostname_specifier[i])
	      i++;
	  }

	return (address_buffer);
      }
  }

  /* Not in dot notation.  Must be hostname. */
  entry = gethostbyname (hostname_specifier);
  if (entry)
    {
      memcpy (address_buffer, entry->h_addr, 4);
      return (address_buffer);
    }
  else
    return ((byte *)NULL);
}

#if defined (__cplusplus)
}
#endif
