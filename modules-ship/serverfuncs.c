/* serverfuncs.c: Implements TCP/IP server technology directly in Meta-HTML. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Tue Sep 30 09:02:13 1997.  */

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

#include "modules.h"

#if defined (__cplusplus)
extern "C"
{
#endif
static void pf_make_server (PFunArgs);
static void pf_kill_server (PFunArgs);

static PFunDesc ftab[] =
{
  { "SERVER::MAKE-SERVER", 0, 0, pf_make_server },
  { "SERVER::KILL-SERVER", 0, 0, pf_kill_server },
  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

MODULE_INITIALIZE ("serverfuncs", ftab)

DEFINE_SECTION (META-HTML-TCP/IP-SERVERS, tcp/ip;port;telnet,
"<Meta-HTML> provides an extremely convenient path for creating a TCP/IP
server which will listen on a particular port, and handle line-based
commands.

Typically, an <b>mhc</b> script is used to start the server process -- it
loads the <code>serverfuncs</code> module, creates a server process, binds
<code>*standard-input*</code> and <code>*standard-output</code> to the
specified port, and waits for connections.

When a connection is received, the server process forks and executes the
function that you have specified to run.  Upon exit from that function,
the connection is closed.

The libraries that are provided in the <code>modules/serverfunc-examples</code>
tagsets directory do most of the work for implementing a complete server --
you need only to write the commands which implement the specific functionality
that you require.",
"The type of server that is implemented by the <code>tagsets/server.mhtml</code>
code expect to interact in an ASCII based conversation mode; each request
from the client is a single line of ASCII text, and your server is expected
to produce a response (which can be multiple lines, or a single line, or
anything that your protocol implements).

The code in <code>tagsets/server.mhtml</code> implement a few commands for you
already, including <code>quit</code>, <code>help</code>, and <code>login</code>.

You implement new commands by simply writing a function whose name is
<code>COMMAND::<i>function-name</i></code>, where <i>function-name</i>
is the exact string that the client should send to invoke the command.

The convenience functions <tag server::put-line> and <tag server::get-line>
write and read newline terminated lines of text to and from the client
respectively.

If you are interested in writing a TCP/IP based server, we would suggest that
you read the source code to the <code>tagsets/server.mhtml</code> library,
and peruse the examples.")

DEFUNX (pf_server::kill-server, ,
"Kill the server process created with <tag server::make-server>.
This should only be called from within a running server.")

static void
pf_kill_server (PFunArgs)
{
  kill (getppid (), SIGTERM);
  kill (getpid(), SIGTERM);
}

DEFUNX (pf_server::make_server, &key hostname &rest start-fun port,
"Create a server process which will listen on <var port> for incoming
TCP/IP connections, and return a <i>server indentifier</i> which can
be used to crudely control that process.

When a connection is received on that port, the standard streams are
bound to the <Meta-HTML> variables <code>*standard-input*</code> and
<code>*standard-output*</code>, and <var start-fun> is invoked.  Only the
functions which have been defined before the invocation of
<code>server::make-server</code> are available to the server process.

A number of variables are bound at connection time.  These are:

<ul>
<li> <b>SERVER::REMOTE-ADDR</b><br>The IP address of the connecting machine.
<li> <b>SERVER::REMOTE-PORT</b><br>The port number which the remote machine connected to.
</ul>")

/* Release a child that has died in the normal way. */
static void
release_child (void)
{
  int status;
  pid_t pid;

  while ((pid = waitpid (-1, &status, WNOHANG)) > 0);
}

typedef struct
{
  struct sockaddr_in addr;
  int sock;
  int port;
} Server;

static Server **servers = (Server **)NULL;
static int servers_index = 0;
static int servers_slots = 0;

static void
pf_make_server (PFunArgs)
{
  int i = 0, high_sock = 0;
  pid_t child;
  int done = 0;
  Package *server_info = symbol_get_package ((char *)NULL);
  int sock = -1, one = 1;
  unsigned int sock_size = sizeof (struct sockaddr_in);
  struct in_addr host_address;
  fd_set sock_fds;
  char *bind_address = (char *)NULL;
  char *host = mhtml_evaluate_string
    (get_one_of (vars, "hostname", "host", (char *)NULL));

  if (!empty_string_p (host))
    {
      unsigned char *addr = hostname_or_ip_to_address (host);

      if (addr)
	{
	  bind_address = (char *)xmalloc (8);
	  memcpy (bind_address, addr, 8);
	}
    }

  FD_ZERO (&sock_fds);

  /* Make the server process right away.  The child is a single process,
     perhaps listening on several ports. */
  child = fork ();

  /* Say what to do when a child dies. */
  signal (SIGCHLD, (sig_t)release_child);

  /* In both the parent and the child, parse the arguments, and add the
     information to a package which describes what this server does. */
  while (!done)
    {
      char *func = mhtml_evaluate_string (get_positional_arg (vars, i++));
      char *port = mhtml_evaluate_string (get_positional_arg (vars, i++));

      if ((empty_string_p (func)) || (empty_string_p (port)))
	done = 1;
      else
	{
	  forms_set_tag_value_in_package (server_info, port, func);

	  if (child == (pid_t) 0)
	    {
	      int portnum = atoi (port);
	      struct sockaddr_in s;

	      close (mhtml_stdin_fileno);
	      close (mhtml_stdout_fileno);
	      host_address.s_addr = htonl (INADDR_ANY);

	      if (bind_address != (char *)NULL)
		memcpy (&host_address, bind_address, 8);

	      /* Set up the socket address. */
	      memset (&s, 0, sizeof (s));
	      s.sin_addr = host_address;
	      s.sin_family = AF_INET;
	      s.sin_port = htons ((short) portnum);

	      /* Set up the socket. */
	      sock = socket (AF_INET, SOCK_STREAM, 0);
	      if (sock < 0)
		{
		  fprintf (stderr, "Cannot create socket!");
		}
	      else if (sock > high_sock)
		high_sock = sock;

	      setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
			  (char *)&one, sizeof (one));

	      /* Bind the socket to our address. */
	      if (bind (sock, (struct sockaddr *)&s, sizeof (s)) == -1)
		{
		 fprintf (stderr, "Cannot bind socket to socket address");
		}

	      {
		Server *server = (Server *)xmalloc (sizeof (Server));

		memcpy (&server->addr, &s, sizeof (s));
		server->sock = sock;
		server->port = portnum;

		if (servers_index + 2 > servers_slots)
		  servers = (Server **)xrealloc
		    (servers, (servers_slots += 4) * sizeof (Server *));
		servers[servers_index++] = server;
		servers[servers_index] = (Server *)NULL;
	      }

	      /* Start listening on this socket. */
	      listen (sock, 5);
	      FD_SET (sock, &sock_fds);
	    }
	}
    }

  if (child == (pid_t) 0)
    {
      while (1)
	{
	  register int j;
	  fd_set read_fds;
	  int ready;

	  memcpy (&read_fds, &sock_fds, sizeof (fd_set));

	  ready = select (high_sock + 1, &read_fds, NULL, NULL, NULL);

	  if (ready > -1)
	    {
	      int connection = -1;
	      Server *s = (Server *)NULL;

	      for (j = 0; j < servers_index; j++)
		if (FD_ISSET (servers[j]->sock, &read_fds))
		  {
		    s = servers[j];

		    connection = accept
		      (s->sock, (struct sockaddr *)&s->addr, &sock_size);

		    break;
		  }

	      if (connection > -1)
		{
		  /* Fork a server process to handle this request. */
		  pid_t instance = fork ();

		  if (instance)
		    {
		      signal (SIGCHLD, (sig_t)release_child);
		      close (connection);
		    }
		  else
		    {
		      struct sockaddr_in client;

		      if (getpeername
			  (connection,
			   (struct sockaddr *)&client, &sock_size) != -1)
			{
			  long addr =  client.sin_addr.s_addr;
			  const char *x = (const char *)&addr;
			  char addr_rep[24];
			  char port_rep[12];
			  BPRINTF_BUFFER *exec_buffer;

			  exec_buffer = bprintf_create_buffer ();

			  sprintf (addr_rep, "%d.%d.%d.%d",
				   (unsigned char)x[0],
				   (unsigned char)x[1],
				   (unsigned char)x[2],
				   (unsigned char)x[3]);
			  sprintf (port_rep, "%d", s->port);

			  signal (SIGCHLD, SIG_DFL);
			  signal (SIGHUP,  SIG_IGN);

			  /* Bind variables in this instance. */
			  mhtml_stdout_fileno = connection;
			  mhtml_stdin_fileno = connection;

			  pagefunc_set_variable
			    ("server::remote-addr", addr_rep);

			  pagefunc_set_variable
			    ("server::remote-port", port_rep);

			  bprintf (exec_buffer, "<%s>",
				   forms_get_tag_value_in_package
				   (server_info, port_rep));
			  mhtml_evaluate_string (exec_buffer->buffer);
			  shutdown (connection, 2);
			  close (connection);
			  _exit (0);
			}
		    }
		}
	    }
	}
      _exit (0);
    }

  {
    char *info = package_to_alist (server_info, 0);
    Package *pack = symbol_get_package ((char *)NULL);
    char *result;
    char num[20];

    sprintf (num, "%d", (int)child);
    forms_set_tag_value_in_package (pack, num, info);
    if (!empty_string_p (host))
      {
	Symbol *sym = symbol_lookup_in_package (pack, num);
	symbol_add_value (sym, host);
      }

    result = package_to_alist (pack, 0);
    symbol_destroy_package (pack);
    symbol_destroy_package (server_info);
    xfree (info);

    if (result != (char *)NULL)
      {
	bprintf_insert (page, start, "%s", result);
	*newstart += strlen (result);
	xfree (result);
      }
  }

  xfree (bind_address);
  xfree (host);
}

#if defined (__cplusplus)
}
#endif
