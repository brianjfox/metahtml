/* streamfuncs.c: -*- C -*-  Functions for stream manuipulation. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Tue Sep 26 21:55:45 1995.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1995, 2001, Brian J. Fox (bfox@ai.mit.edu).

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

#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_with_open_stream (PFunArgs);
static void pf_stream_put (PFunArgs);
static void pf_stream_put_contents (PFunArgs);
static void pf_stream_get (PFunArgs);
static void pf_stream_get_contents (PFunArgs);
static void pf_stream_readable (PFunArgs);
static void pf_stream_writable (PFunArgs);
static void pf_stream_shutdown (PFunArgs);
static void pf_stream_info (PFunArgs);

static PFunDesc func_table[] =
{
  { "WITH-OPEN-STREAM",		1, 0, pf_with_open_stream },
  { "STREAM-PUT",		0, 0, pf_stream_put },
  { "STREAM-GET",		0, 0, pf_stream_get },
  { "STREAM-PUT-CONTENTS",	0, 0, pf_stream_put_contents },
  { "STREAM-GET-CONTENTS",	0, 0, pf_stream_get_contents },
  { "STREAM-READABLE",		0, 0, pf_stream_readable },
  { "STREAM-WRITABLE",		0, 0, pf_stream_writable },
  { "STREAM-SHUTDOWN",		0, 0, pf_stream_shutdown },
  { "STREAM-INFO",		0, 0, pf_stream_info },
  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_stream_functions)

DEFINE_SECTION (STREAM-OPERATORS, files; streams; reading; writing;
creating, "A <i>stream</i> is a data object upon which various input
and output operations may be performed.

<meta-html> provides commands for creating, opening, reading from, and
writing to, stream objects.  The referenced underlying object may
either be a file or a network connection.", "There are three special
names which can be used with <funref stream-operators
with-open-stream>, these are <code>*standard-input*</code>,
<code>*standard-output*</code>, and <code>*standard-error*</code>.

<code>*standard-input*</code> always names the stream that from which
input was being read at the time the engine, server, or
<code>mhc</code> invoked.  <code>*standard-output*</code> always names
the stream to which output is being written by the engine, server or
<code>mhc</code>.

<code>*standard-error*</code> always names the stream where error
output will appear -- this stream could be the same as the one named
by <code>*standard-output*</code>, but this does not have to be the
case.")

#if defined (mhtml_stream_MUST_SEEK)
#  define stream_MUST_SEEK mhtml_stream_MUST_SEEK
#else
#  define stream_MUST_SEEK 0x01
#endif /* mhtml_stream_MUST_SEEK */

#if defined (mhtml_stream_NOTIMEOUT)
#  define stream_NOTIMEOUT mhtml_stream_NOTIMEOUT
#else
#  define stream_NOTIMEOUT 0x02
#endif /* mhtml_stream_NOTIMEOUT */

#if defined (mhtml_stream_op_NONE)
#  define op_NONE mhtml_stream_op_NONE
#else
#  define op_NONE  0
#endif /* mhtml_stream_op_NONE */

#if defined (mhtml_stream_op_READ)
#  define op_READ mhtml_stream_op_READ
#else
#  define op_READ  1
#endif /* mhtml_stream_op_READ */

#if defined (mhtml_stream_op_WRITE)
#  define op_WRITE mhtml_stream_op_WRITE
#else
#  define op_WRITE 2
#endif /* mhtml_stream_op_WRITE */

#if defined (HAVE_MHTML_STREAM_TYPE)
#  define Stream	MHTMLStream
#  define StreamType	MHTMLStreamType
#  define stream_FILE	mhtml_stream_FILE
#  define stream_TCP	mhtml_stream_TCP
#  define stream_FD	mhtml_stream_FD
#  define stream_PROG	mhtml_stream_PROG
#  define stream_CLOSED	mhtml_stream_CLOSED
#else
typedef struct
{
  char *name;		/* The name used to open this stream. */
  StreamType type;	/* The type of stream: stream_FILE, stream_TCP, etc. */
  int fd;		/* The file descriptor that describes this stream. */
  int ifd;		/* For PROG pipes, the input channel to the stream. */
  pid_t pid;		/* For PROG pipes, the pid of the child. */
  int mode;		/* Access mode. */
  int flags;		/* Special flags indicate changes in behaviour. */
  int timeout;		/* Default value for IO timer. */
  int last_op;		/* The last operation done on this stream. */
} Stream;

typedef enum { stream_FILE, stream_TCP, stream_PROG, stream_CLOSED }StreamType;

#endif /* !HAVE_MHTML_STREAM_TYPE */

typedef void mhtml_sigfun (int);
#undef SIG_DFL
#define SIG_DFL (mhtml_sigfun *)NULL

static int stream_environment_level = 0;
static Stream **open_streams = (Stream **)NULL;
int open_streams_slots = 0;
int open_streams_index = 0;

extern int tcp_to_host (char *host, char *service);
extern jmp_buf page_jmp_buffer;


/* Install this stream, and return the index which is used to identify it. */
static int
add_open_stream (char *name, StreamType type, int fd, int mode, int flags,
		 int timeout)
{
  Stream *stream = (Stream *)xmalloc (sizeof (Stream));
  int result = open_streams_index;

  stream->name = strdup (name);
  stream->type = type;
  stream->fd = fd;
  stream->ifd = -1;
  stream->mode = mode;
  stream->flags = flags;
  stream->timeout = timeout;
  stream->last_op = op_NONE;

  if (open_streams_index + 2 > open_streams_slots)
    open_streams = (Stream **)xrealloc
      (open_streams, ((open_streams_slots += 5) * sizeof (Stream *)));

  open_streams[open_streams_index++] = stream;
  open_streams[open_streams_index] = (Stream *)NULL;

  return (result);
}

/* De-install this stream. */
static void
discard_stream (int stream_index)
{
  /* This really can only be called as the result of ending the
     function with-open-stream.  That means that stream_index should
     point to the stream on the TOS. */
  if (open_streams_index)
    {
      Stream *stream = open_streams[--open_streams_index];

      open_streams[open_streams_index] = (Stream *)NULL;
      if (stream->name) free (stream->name);

      if ((stream->fd != fileno (stdin)) &&
	  (stream->fd != fileno (stdout)) &&
	  (stream->fd != fileno (stderr)) &&
	  (stream->type != stream_FD))
	close (stream->fd);
      else
	{
	  if (stream->fd == fileno (stderr))
	    fflush (stderr);
	  else if (stream->fd == fileno (stdout))
	    fflush (stdout);
	}

      if (stream->ifd != -1)
	{
	  close (stream->ifd);
	  kill (stream->pid, SIGTERM);
	}

      free (stream);
    }
}

static int
stream_mode (char *modename)
{
  int mode = O_RDONLY;

  if (modename != (char *)NULL)
    {
      if (strcasecmp (modename, "write") == 0)
	mode = O_WRONLY;
      else if (strcasecmp (modename, "read-write") == 0)
	mode = O_RDWR;
      else if (strcasecmp (modename, "append") == 0)
	mode = O_APPEND | O_WRONLY | O_CREAT;
      else if (strcasecmp (modename, "write-create") == 0)
	mode = O_TRUNC | O_RDWR | O_CREAT;
    }

  return (mode);
}

StreamType
mhtml_stream_type (char *type_name)
{
  StreamType the_type = stream_FILE;

  if (type_name != (char *)NULL)
    {
      if (strcasecmp (type_name, "tcp") == 0)
	the_type = stream_TCP;
      else if (strcasecmp (type_name, "prog") == 0)
	the_type = stream_PROG;
      else if (strcasecmp (type_name, "fd") == 0)
	the_type = stream_FD;
    }

  return (the_type);
}

static char **
make_environ (void)
{
  register int i, j;
  Symbol **envsyms = symbols_of_package (symbol_lookup_package ("ENV"));
  Symbol *sym;
  char **result = (char **)NULL;

  if (envsyms != (Symbol **)NULL)
    {
      /* Find out how many symbols to make. */
      for (i = 0, j = 0; (sym = envsyms[i]) != (Symbol *)NULL; i++)
	if (sym->type == symtype_STRING)
	  j++;

      /* Make the resultant array the same size. */
      result = (char **)xmalloc ((j + 1) * sizeof (char *));

      /* For each symbol, make an environment variable. */
      for (i = 0, j = 0; (sym = envsyms[i]) != (Symbol *)NULL; i++)
	{
	  if (sym->type == symtype_STRING)
	    {
	      char *value;

	      if (sym->values && sym->values_index && sym->values[0])
		value = sym->values[0];
	      else
		value = "";

	      result[j] = (char *)xmalloc (3 + strlen (sym->name) +
					   strlen (value));
	      sprintf (result[j], "%s=%s", sym->name, value);
	      j++;
	    }
	}
      result[j] = (char *)NULL;
    }

  return (result);
}

/* Release a child that has died in the normal way. */
static int child_status = 0;

static void
release_child (void)
{
  wait (&child_status);
}

/* <with-open-stream variable filename [type=[file|tcp|prog]
	   mode=[write/reade/write-create]>
   [code using the open stream]
   </with-open-stream>
   Opens the file specified by FILENAME, and stores a referent to it
   in VARIABLE.  The file is opened in the mode specified by MODE.  If
   MODE is not specified, the file is opened read-only.  If the
   operation fails, the value of VARIABLE is the empty string.
   TYPE can be specified as "TCP", in which case, a TCP connection is
   opened.  In that case, FILENAME looks like HOST:PORT. */
int mhtml_stdin_fileno = 0;
int mhtml_stdout_fileno = 1;
int mhtml_stderr_fileno = 2;

DEFMACRO (pf_with_open_stream, var name
	  &key type=(FILE|TCP|PROG|FD) mode=OPEN-MODE timeout=(value|NEVER),
"Create an environment in which <var var> is bound to the
<i>indicator</i> of an open stream, named by <var name>.

The stream is either of type <code>FILE</code>, in which case it is a simple
file in the file system, of type <code>TCP</code>,
in which case it is an open network connection, or of type <code>PROG</code>,
in which case it is an open connection to a running process.

When <var type> is <code>TCP</code>, <var name> specifies the host and
port to connect to, in the form <code>Hostname:Port-Number</code>.
<var port-number> can either be a symbolic name which is widely
recognized (e.g., <code>SMTP</code>), or the digits which make up the
port number, (e.g., <code>25</code>). <var port-number> can either be
a symbolic name which is widely recognized (e.g., <code>SMTP</code>),
or the digits which make up the port number, (e.g., <code>25</code>).

When <var type> is <code>FD</code>, <var name> specifies an already open
file descriptor to connect to.  This should be used with caution, as the
standard file descriptors for stdin, stdout, and stderr might not be what
you expect.  To talk to those streams, you should use <var *standard-input*>,
<var *standard-output*>, and <var *standard-error*> as the stream names,
without passing a <var type> argument.

Finally, the keyword argument <var notimeout=true> may be given, which
indicates that the amount of time that <Meta-HTML> should wait during
IO operations on this stream is infinite -- all processing will block
until the stream is successfully read from or written to.  The default
timeout is dependent on which specific operation is being performed,
and the amount of data which is being read or written, but is
generally suitable for writing data at about 14.4kbps.

The possible values for <var mode> are:

<ul>
  <li> <code>READ</code>:<br>
  The stream is opened for reading only.  The underlying object must
already exist.

  <li> <code>WRITE</code>:<br>
  The stream is opened for writing only.  The underlying object must
already exist.

  <li> <code>READ-WRITE</code>:<br>
  The stream is opened for both reading and writing.  The underlying
object must already exist.

  <li> <code>APPEND</code>:<br>
  The stream is opened for writing only.  If the underlying object did
not already exist, it is created.  Information written to this stream
appears after any information that was already present in the
underlying object.

  <li> <code>WRITE-CREATE</code>:<br>
  The stream is opened for writing only.  The underlying object is
created fresh, even if it had already existed.
</ul>

When one is opening, reading to, or writing from a network stream, the 
amount of time it can take to finish the operation is indeterminate.  So,
the keyword argument <var timeout=value> can be used to specify the maximum
amount of time (in seconds) that these operations may take.  If the value
is specified as \"never\", then these operations can take \"forever\".
If the value isn't specified, then the operations calculate the maximum
wait time dynamically, based upon the amount of information to be read
or written.  Any other value specifies an absolute maximum number of
seconds.")
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *filename = mhtml_canonicalize_file_name_argument
    (get_positional_arg (vars, 1));
  int jump_again = 0;

  jump_again = 0;

  if ((!empty_string_p (varname)) && (!empty_string_p (filename)))
    {
      char *mode_name = mhtml_evaluate_string (get_value (vars, "MODE"));
      char *type_name = mhtml_evaluate_string (get_value (vars, "TYPE"));
      char *notimeout = mhtml_evaluate_string (get_value (vars, "NOTIMEOUT"));
      char *timeout_arg = mhtml_evaluate_string (get_value (vars, "TIMEOUT"));
      int mode = stream_mode (mode_name);
      int type = mhtml_stream_type (type_name);
      int flags = notimeout ? stream_NOTIMEOUT : 0;
      int timeout = 0;
      int stream_fd = -1;

      /* For handling PROG streams. */
      int stdin_pipe[2];
      int stdout_pipe[2];
      pid_t child = -1;

      xfree (mode_name);
      xfree (type_name);
      xfree (notimeout);

      if (!empty_string_p (timeout_arg))
	{
	  if (strcasecmp (timeout_arg, "never") == 0)
	    {
	      timeout = 99999;
	      flags |= stream_NOTIMEOUT;
	    }
	  else
	    timeout = atoi (timeout_arg);
	}
      else
	timeout = 0;

      xfree (timeout_arg);

      switch (type)
	{
	case stream_FILE:
	  if (strcasecmp (filename, "*standard-input*") == 0)
	    stream_fd = mhtml_stdin_fileno;
	  else if (strcasecmp (filename, "*standard-output*") == 0)
	    stream_fd = mhtml_stdout_fileno;
	  else if (strcasecmp (filename, "*standard-error*") == 0)
	    stream_fd = mhtml_stderr_fileno;
	  else
	    stream_fd = os_open (filename, mode, 0666);
	  break;

	case stream_TCP:
	  {
	    char *host_part = strdup (filename);
	    char *port_part = strchr (host_part, ':');

	    if (port_part)
	      *port_part++ = '\0';
	    else
	      port_part = "80"; /* Default the port for HTTP connections. */

	    stream_fd = tcp_to_host (host_part, port_part);
	    free (host_part);
	    flags |= stream_MUST_SEEK;
	  }
	  break;

	case stream_FD:
	  stream_fd = atoi (filename);
	  break;

	case stream_PROG:
	  {
	    pipe (stdin_pipe);
	    pipe (stdout_pipe);
	    child = vfork ();

	    if (child != (pid_t) 0)
	      {
		/* In the parent. */
		close (stdout_pipe[1]);
		close (stdin_pipe[0]);

		/* Say what to do when a child dies. */
		signal (SIGCHLD, (sig_t)release_child);

		/* Say what to do if the pipe is broken. */
		signal (SIGPIPE, SIG_IGN);
		stream_fd = stdin_pipe[1];
	      }
	    else
	      {
		/* In the child. */
		char *argv[4];
#if defined (__CYGWIN32__)
		char *physical_path = "/usr/bin/bash.exe";
#else
		char *physical_path = "/bin/sh";
#endif

		close (stdout_pipe[0]);
		close (stdin_pipe[1]);
		dup2 (stdout_pipe[1], 1);
		dup2 (stdin_pipe[0], 0);
		dup2 (1, 2);
		close (stdout_pipe[1]);
		close (stdin_pipe[0]);

		pagefunc_set_variable
		  ("env::path", pagefunc_get_variable ("mhtml::exec-path"));
		symbol_remove ("env::query_string");
		symbol_remove ("env::path_info");
		symbol_remove ("env::content_length");

		argv[0] = physical_path;
		argv[1] = "-c";
		argv[2] = filename;
		argv[3] = (char *)NULL;
		execve (physical_path, argv, make_environ ());
		_exit (127);
	      }
	  }
	break;
	}

      if (stream_fd > -1)
	{
	  char stream_value[40];
	  PAGE *body_code = page_copy_page (body);
	  int page_value =
	    add_open_stream (filename, type, stream_fd, mode, flags, timeout);

	  sprintf (stream_value, "%d", page_value);
	  pagefunc_set_variable (varname, stream_value);

	  if (child > (pid_t) 0)
	    {
	      Stream *stream = open_streams[page_value];
	      stream->ifd = stdout_pipe[0];
	      stream->pid = child;
	    }

	  {
	    PageEnv *environment = pagefunc_save_environment ();

	    stream_environment_level++;
	    if ((jump_again = setjmp (page_jmp_buffer)) == 0)
	      page_process_page_internal (body_code);

	    stream_environment_level--;

	    pagefunc_restore_environment (environment);
	  }

	  if (body_code != (PAGE *)NULL)
	    {
	      if (!jump_again && body_code->buffer)
		{
		  bprintf_insert (page, start, "%s", body_code->buffer);
		  *newstart = start + (body_code->bindex);
		}

	      page_free_page (body_code);
	    }

	  discard_stream (page_value);
	}
      else
	{
	  char *temp = mhtml_evaluate_string (mhtml_funargs (vars));

	  page_syserr ("<with-open-stream%s%s>: %s",
		       temp ? " " : "", temp ? temp : "",
		       (char *)strerror (errno));
	  xfree (temp);
	}
    }

  xfree (filename);
  xfree (varname);
  if (jump_again) longjmp (page_jmp_buffer, 1);
}

Stream *
mhtml_get_stream_reference (Package *vars)
{
  Stream *stream = (Stream *)NULL;

  if (stream_environment_level != 0)
    {
      char *stream_ref = mhtml_evaluate_string (get_positional_arg (vars, 0));

      if (stream_ref)
	{
	  register int i;
	  char *rep = pagefunc_get_variable (stream_ref);

	  free (stream_ref);

	  /* The entire reference must be digits. */
	  if (!empty_string_p (rep))
	    {
	      for (i = 0; rep[i] != '\0'; i++);

	      if (i == strlen (rep))
		{
		  int stream_index = atoi (rep);

		  if ((stream_index < open_streams_index) &&
		      (stream_index > -1))
		    stream = open_streams[stream_index];
		}
	    }
	}
    }

  return (stream);
}

char *
stream_type_string (StreamType type)
{
  switch (type)
    {
    case stream_FILE: return ("FILE");
    case stream_TCP:  return ("TCP");
    case stream_FD:  return ("FD");
    case stream_PROG: return ("PROG");
    default:	      return ("CLOSED");
    }

  /* NOT REACHED */
  return ((char *)NULL);
}

static char *
stream_mode_string (int mode)
{
  if (O_RDONLY == mode)
    return ("READ");
  else if (O_WRONLY == mode)
    return ("WRITE");
  else if (O_RDWR == mode)
    return ("READ-WRITE");
  else if ((O_APPEND | O_WRONLY | O_CREAT) == mode)
    return ("APPEND");
  else if ((O_TRUNC | O_RDWR | O_CREAT) == mode)
    return ("WRITE-CREATE");
  else
    return ("READER?");
}
      
/* Return an alist representing the stream referenced by the first
   positional argument in VARS. */
char *
mhtml_stream_reference_to_alist (Package *vars)
{
  Stream *stream = mhtml_get_stream_reference (vars);
  char *result = (char *)NULL;

  if (stream != (Stream *)NULL)
    {
      Package *p = symbol_get_package ((char *)NULL);

      forms_set_tag_value_in_package
	(p, "name", stream->name ? stream->name : "Anonymous Stream");

      forms_set_tag_value_in_package
	(p, "type", stream_type_string (stream->type));
      mhtml_set_numeric_variable_in_package
	(p, "descriptor",
	 (stream->type == stream_PROG) ? stream->ifd : stream->fd);
      forms_set_tag_value_in_package
	(p, "mode", stream_mode_string (stream->mode));
      mhtml_set_numeric_variable_in_package
	(p, "timeout", stream->timeout);

      /* Handle the flags. */
      {
	Symbol *sym = symbol_intern_in_package (p, "flags");

	if (stream->flags & stream_MUST_SEEK)
	  symbol_add_value (sym, "MUST-SEEK");
	if (stream->flags & stream_NOTIMEOUT)
	  symbol_add_value (sym, "NOTIMEOUT");
      }

      if (stream->type == stream_PROG)
	mhtml_set_numeric_variable_in_package (p, "pid", (int)stream->pid);

      result = package_to_alist (p, 1);
    }

  return (result);
}

DEFUN (pf_stream_info, stream,
       "Return an association list providing information about <var stream>.")
{
  char *alist = mhtml_stream_reference_to_alist (vars);

  if (alist != (char *)NULL)
    {
      int len = strlen (alist);
      bprintf_insert (page, start, "%s", alist);
      free (alist);
      *newstart += len;
    }
}

/* Gets called if/when we run out of time waiting for a known number
   of bytes that are available. */
static int operation_timed_out_p = 0;
static jmp_buf operation_jmp_buf;

#if !defined (HAVE_SYS_SELECT_H)
static void
operation_timed_out (int sig)
{
  alarm (0);
  operation_timed_out_p = 1;
  longjmp (operation_jmp_buf, 1);
}
#endif /* !HAVE_SYS_SELECT_H */

/* A 28.8 modem can deliver a maximum of 3.6k per second without
   taking compression into consideration.  A 14.4 can only deliver
   1.8k per second.  So, we will give the network 2 seconds to
   deliver each 1k of data, and then cushion that with a 10 second
   grace period.  Gee, isn't the Internet Fantastic? */
static int
set_io_timer (Stream *stream, int length)
{
  char *minimum_data_transfer_rate = 
    pagefunc_get_variable ("mhtml::minimum-data-transfer-rate");
  int timeout, divisor = 0;

  if (minimum_data_transfer_rate != (char *)NULL)
    divisor = atoi (minimum_data_transfer_rate);

  if (divisor < 1)
    divisor = 512;

  if (stream->timeout)
    timeout = stream->timeout;
  else
    {
      timeout = length / divisor;
      timeout += 10;
    }

#if !defined (HAVE_SYS_SELECT_H)
  operation_timed_out_p = 0;
  signal (SIGALRM, operation_timed_out);
  alarm (timeout);
#endif
  return (timeout);
}

static void
stream_write (Stream *stream, int length, char *bytes)
{
  char *write_from_loc = bytes;
  int bytes_written, seconds;

  if ((stream->flags & stream_MUST_SEEK) && (stream->last_op != op_WRITE))
    lseek (stream->fd, 0l, 1);

  seconds = set_io_timer (stream, length);
  
#if !defined (HAVE_SYS_SELECT_H)
  if (setjmp (operation_jmp_buf))
    {
      length = 0;
      stream->type = stream_CLOSED;
    }
#endif

  while (length != 0)
    {
#if defined (HAVE_SYS_SELECT_H)
      struct timeval timer;
      fd_set writefds;
      int ready = 0;

      FD_ZERO (&writefds);
      FD_SET (stream->fd, &writefds);
      timer.tv_sec = seconds;
      timer.tv_usec = 0;
      ready = select (stream->fd + 1,
		      (fd_set *)NULL, &writefds, (fd_set *)NULL, &timer);

      if (ready < 1)
	{
	  operation_timed_out_p = 1;
	  bytes_written = 0;
	  if (ready < 0)
	    stream->type = stream_CLOSED;
	}
      else
	bytes_written = write (stream->fd, write_from_loc, length);
#else
      bytes_written = write (stream->fd, write_from_loc, length);
#endif

      if ((bytes_written == -1) || (operation_timed_out_p == 1))
	{
	  stream->type = stream_CLOSED;
	  return;
	}

      length -= bytes_written;
      write_from_loc += bytes_written;
    }

#if !defined (HAVE_SYS_SELECT_H)
  alarm (0);
  signal (SIGALRM, SIG_DFL);
#endif
}

static int stream_last_read_object_size = 0;

static char *
stream_read (Stream *stream, char char_to_stop_at)
{
  char *buffer = (char *)NULL;
  int buffer_index = 0;
  int buffer_size = 0;
  int done = 0;
  char minbuf[512];
  int mindex = 0;
  int read_result = 0;
  int input_fd = stream->fd;
  int seconds = 0;

  if (stream->type == stream_PROG)
    input_fd = stream->ifd;

  if ((stream->flags & stream_MUST_SEEK) && (stream->last_op != op_READ))
    lseek (input_fd, 0l, 1);

  if (stream->flags & stream_NOTIMEOUT)
    seconds = set_io_timer (stream, 999999);
  else
    seconds = set_io_timer (stream, 80);

#if !defined (HAVE_SYS_SELECT_H)
  if (setjmp (operation_jmp_buf))
    {
      done = 1;
      stream->type = stream_CLOSED;
    }
#endif

  while (!done)
    {
      if (char_to_stop_at != '\0')
	{
	  if (mindex > 511)
	    {
	      if (buffer_index + mindex + 1 >= buffer_size)
		buffer = (char *)xrealloc (buffer, (buffer_size += mindex +2));

	      memmove (buffer + buffer_index, minbuf, mindex);
	      buffer_index += mindex;
	      buffer[buffer_index] = '\0';
	      mindex = 0;
	    }
#if defined (HAVE_SYS_SELECT_H)
	  {
	    struct timeval timer;
	    fd_set readfds;
	    int ready = 0;

	    FD_ZERO (&readfds);
	    FD_SET (stream->fd, &readfds);
	    timer.tv_sec = seconds;
	    timer.tv_usec = 0;
	    ready = select (stream->fd + 1,
			    &readfds, (fd_set *)NULL, (fd_set *)NULL, &timer);

	    if (ready < 1)
	      {
		operation_timed_out_p = 1;
		done = 1;
		read_result = 0;
		if (ready < 0)
		  stream->type = stream_CLOSED;
	      }
	    else
	      read_result = read (input_fd, minbuf + mindex, 1);
	  }
#else
	  read_result = read (input_fd, minbuf + mindex, 1);
#endif
	}
      else
	{
	  /* Read as much as we can, as fast as we can.
	     For the case of file streams, this means get the length
	     of the file, and try to read that much. */
	  if (stream->type == stream_FILE)
	    {
	      struct stat finfo;

	      if (fstat (stream->fd, &finfo) == -1)
		read_result = -1;
	      else
		{
		  buffer = (char *)
		    xrealloc (buffer, (buffer_size = 1 + (int)finfo.st_size));
		  buffer[0] = '0';
		  read_result = read (stream->fd, buffer, buffer_size);
		  done = 1;

		  if (read_result > -1)
		    {
		      buffer_index = read_result;
		      buffer[buffer_index] = '\0';
		    }
		}
	    }
	  else if ((stream->type == stream_TCP) ||
		   (stream->type == stream_PROG))
	    {
	      /* For TCP streams, we try hard to read in 20k chunks. */
	      int bits_size = 20 * 1024;

	      while (!done)
		{
		  if ((buffer_index + bits_size) >= buffer_size)
		    {
		      buffer = (char *)
			xrealloc (buffer, (buffer_size += bits_size));
		    }

		  read_result =
		    read (input_fd, buffer + buffer_index, bits_size);

		  if (read_result > 0)
		    {
		      buffer_index += read_result;
		    }
		  else
		    done = 1;
		}
	    }
	}

      if ((read_result < 1) || (operation_timed_out_p == 1))
	{
	  done = 1;
	  stream->type = stream_CLOSED;
	}
      else
	{
	  if ((char_to_stop_at != 0) && (minbuf[mindex] == char_to_stop_at))
	    done = 1;
	  mindex++;
	}
    }

#if !defined (HAVE_SYS_SELECT_H)
  alarm (0);
  signal (SIGALRM, SIG_DFL);
#endif

  if ((char_to_stop_at != '\0') && (mindex != 0))
    {
      if (buffer_index + mindex + 1 >= buffer_size)
	buffer = (char *)xrealloc (buffer, (buffer_index + mindex + 20));

      memmove (buffer + buffer_index, minbuf, mindex);
      buffer_index += mindex;
    }

  stream_last_read_object_size = buffer_index;

  if (buffer != (char *)NULL)
    buffer[buffer_index] = '\0';

  return (buffer);
}

static char *
stream_read_chunk (Stream *stream, int length)
{
  char *buffer = (char *)NULL;
  int buffer_index = 0;
  int bytes_left = length;
  int done = 0;
  int input_fd = stream->fd;

  if (stream->type == stream_PROG)
    input_fd = stream->ifd;

  if ((stream->flags & stream_MUST_SEEK) && (stream->last_op != op_READ))
    lseek (input_fd, 0l, 1);

  buffer = (char *)xmalloc (length + 1);

  set_io_timer (stream, length);

  if (setjmp (operation_jmp_buf))
    {
      done = 1;
      stream->type = stream_CLOSED;
    }

  while (!done)
    {
      int bytes_read = read (input_fd, buffer + buffer_index, bytes_left);

      if (bytes_read > 0)
	{
	  buffer_index += bytes_read;
	  bytes_left -= bytes_left;
	}
      else if ((bytes_read < 0) && (errno == EINTR))
	{
	  continue;
	}
      else
	{
	  done = 1;
	  stream->type = stream_CLOSED;
	}
    }

  alarm (0);
  signal (SIGALRM, SIG_DFL);
  stream_last_read_object_size = buffer_index;

  return (buffer);
}

DEFUN (pf_stream_put, stream string,
"Writes <var string> to the open <var stream>.")
{
  Stream *stream = mhtml_get_stream_reference (vars);

  if (stream && stream->type != stream_CLOSED)
    {
      char *put_text = mhtml_evaluate_string (get_positional_arg (vars, 1));

      if (put_text)
	{
	  stream_write (stream, strlen (put_text), put_text);
	  free (put_text);
	}
      stream->last_op = op_WRITE;
    }
}

DEFUN (pf_stream_put_contents, stream var,
"Writes the contents of the variable <var var> to the open <var
stream>.  This is the only way to get the contents of a binary variable
written to a stream.  Binary variables are generally created with <funref
stream-operators stream-get-contents>, or when uploading a file from
a Web browser.")
{
  Stream *stream = mhtml_get_stream_reference (vars);

  if (stream && stream->type != stream_CLOSED)
    {
      char *name = mhtml_evaluate_string (get_positional_arg (vars, 1));
      Symbol *sym = (name ? symbol_lookup (name) : (Symbol *)NULL);

      if (sym != (Symbol *)NULL)
	{
	  char *data = (char *)NULL;
	  int length = 0;

	  switch (sym->type)
	    {
	    case symtype_STRING:
	      if (sym->values_index)
		{
		  data = sym->values[0];
		  length = data ? (1 + strlen (data)) : 0;
		}
	      break;

	    case symtype_FUNCTION:
	      break;

	    case symtype_BINARY:
	      data = ((Datablock *)sym->values)->data;
	      length = ((Datablock *)sym->values)->length;
	      break;
	    }

	  if (length != 0)
	    {
	      stream->last_op = op_WRITE;
	      stream_write (stream, length, data);
	    }
	}

      xfree (name);
    }
}

DEFUN (pf_stream_get, stream &key stop-at=character,
"Reads a chunk of data from <var stream> and returns it. <var stream>
is read until the character value specified by <var character>
(defaulting to a newline) is reached, and that data upto and including
the final <var character> is returned. One way to read all of the
available data is to supply the empty string as the value of <var stop-at>: 

<example>
<with-open-stream stream www.ua.com:80 type=tcp mode=read-write>
  <stream-put stream \"GET /welcome.mhtml HTTP/1.0\\n\\n\">
  <set-var the-page = <stream-get stream stop-at=\"\">>
</with-open-stream>
</example>")
{
  Stream *stream = mhtml_get_stream_reference (vars);
  char *stop_at = mhtml_evaluate_string (get_value (vars, "stop-at"));
  char *result = (char *)NULL;

  if (stream)
    {
      if (stream->type != stream_CLOSED)
	{
	  result = stream_read (stream, stop_at ? *stop_at : '\n');
	  stream->last_op = op_READ;
	}
    }

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }

  xfree (stop_at);
}

DEFUN (pf_stream_get_contents,
       stream var &key chunk-size=size stop-at=character,
"Reads the contents of the stream <var stream> into the binary
variable <var var>.  You can easily use this in conjunction with
<funref stream-operators stream-put-contents> and <funref variables
coerce-var>.

If <var stop-at> is supplied, it is a character at which the function
should stop reading data from the stream.  Unlike
<code>stream-get</code>, <var stop-at> defaults to the empty
character, thus causing the entire stream to be read.

If <var chunk-size> is supplied, it is the amount to read at one
time.  This is useful for when you have an unbounded amount of data to
read, and you would like to process it in manageable chunks.

For example, the following code copies data from an open network
stream to a file on the local disk, without using more than 32k of
memory:

<example>
<with-open-stream src data-server.ua.com:2345 type=tcp mode=read>
  <with-open-stream dst /tmp/datafile type=file mode=write-create>
    <while <stream-readable s>>
      <stream-get-contents src chunk chunk-size=32768>
      <stream-put-contents dst chunk>
    </while>
  </with-open-stream>
</with-open-stream>
</example>")
{
  Stream *stream = mhtml_get_stream_reference (vars);
  char *stop_at = mhtml_evaluate_string (get_value (vars, "stop-at"));
  char *chunk_size_arg = mhtml_evaluate_string (get_value (vars, "chunk-size"));
  char *name = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *result = (char *)NULL;
  Symbol *sym;

  /* We are replacing the value that this symbol already had, no
     matter what.  Get rid of the existing symbol. */
  if (name != (char *)NULL)
    {
      sym = symbol_remove (name);
      symbol_free (sym);
    }

  /* Now get the data from the stream. */
  if (stream)
    {
      if (stream->type != stream_CLOSED)
	{
	  if (empty_string_p (chunk_size_arg))
	    result = stream_read (stream, stop_at ? *stop_at : '\0');
	  else
	    {
	      int chunk_size = atoi (chunk_size_arg);

	      result = stream_read_chunk (stream, chunk_size);
	    }

	  stream->last_op = op_READ;
	}
    }

  /* If there was any data, create a new binary symbol to contain it. */
  if (result)
    {
      if (name != (char *)NULL)
	{
	  Datablock *block;

	  sym = symbol_intern (name);
	  block = datablock_create (result, stream_last_read_object_size);
	  sym->type = symtype_BINARY;
	  sym->values = (char **)block;
	}
      free (result);
    }

  xfree (name);
  xfree (chunk_size_arg);
  xfree (stop_at);
}

DEFUN (pf_stream_readable, stream &key delay=seconds,
"Returns \"true\" if <var stream> is an open stream which was opened
with a mode of read, or read-write, and which still has data pending.

The optional keyword <var delay> is used to specify the amount of time
in seconds that <code>stream-readable</code> should wait for the
stream to become ready for reading.  The default value is zero seconds;
no waiting period is used.")
{
  Stream *stream = mhtml_get_stream_reference (vars);
  char *delay = get_value (vars, "delay");
  int delay_in_seconds = delay ? atoi (delay) : 0;
  char *result = (char *)NULL;

  if ((stream != (Stream *)NULL) &&
      (stream->type != stream_CLOSED) &&
      ((stream->type == stream_TCP) ||
       (stream->mode == O_RDONLY) ||
       (stream->mode & O_RDWR)))
    {
      result = "true";

      /* The result may in fact be true.  On systems which support
	 select (), check explicity that the fd is readable. */
#if defined (FD_SET)
      {
	fd_set read_fds;
	struct timeval poll;
	int ready;
	int input_fd = stream->fd;

	if (stream->type == stream_PROG)
	  input_fd = stream->ifd;

	FD_ZERO (&read_fds);
	FD_SET (input_fd, &read_fds);
	poll.tv_sec = delay_in_seconds;
	poll.tv_usec = 0;
	ready = select (input_fd + 1, &read_fds, (fd_set *)0, (fd_set *)0,
			&poll);

	if (ready < 1)
	  result = (char *)NULL;
      }
#endif /* FD_SET */
    }
  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}


DEFUN (pf_stream_writable, stream &key delay=seconds,
"Returns \"true\" if <var stream> is an open stream which was opened
with a mode of write, append, read-write, or write-create, and which
is available to have data written to it.

The optional keyword <var delay> is used to specify the amount of time
in seconds that <code>stream-writable</code> should wait for the
stream to become ready for writing.  The default value is zero seconds;
no waiting period is used.")
{
  Stream *stream = mhtml_get_stream_reference (vars);
  char *delay = get_value (vars, "delay");
  int delay_in_seconds = delay ? atoi (delay) : 0;
  char *result = (char *)NULL;

  if ((stream != (Stream *)NULL) &&
      (stream->type != stream_CLOSED) &&
      ((stream->type == stream_TCP) ||
       (stream->mode & (O_APPEND | O_RDWR | O_WRONLY))))
    {
      result = "true";

      /* The result may in fact be true.  On systems which support
	 select (), check explicity that the fd is writeable. */
#if defined (FD_SET)
      {
	fd_set write_fds;
	struct timeval poll;
	int ready;

	FD_ZERO (&write_fds);
	FD_SET (stream->fd, &write_fds);
	poll.tv_sec = delay_in_seconds;
	poll.tv_usec = 0;
	ready = select (stream->fd + 1, (fd_set *)0, &write_fds, (fd_set *)0,
			&poll);

	if (ready < 1)
	  result = (char *)NULL;
      }
#endif /* FD_SET */
    }

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

DEFUN (pf_stream_shutdown, stream,
"Only for use with network streams, this tells the underlying
operating system (and the other end of the network connection) that no
additional input or output will be done using this object. In effect,
it immediately closes the stream.

When used in conjunction with a network stream opened on
<code>*standard-output*</code>, this can be used to close the
<code>HTTP</code> connection to the client, and yet continue
processing data.")
{
  Stream *stream = mhtml_get_stream_reference (vars);

  if ((stream != (Stream *)NULL) && (stream->type != stream_CLOSED))
    shutdown (stream->fd, 2);
}

#if defined (__cplusplus)
}
#endif
