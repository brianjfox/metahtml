/* process.c: -*- C -*-  Functions for starting processes in Meta-HTML. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Sep 29 20:53:49 1997.

    This file is part of <Meta-HTML>(tm), a system for the rapid
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

	 http://www.metahtml.com/COPYING */

#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif
static void pf_with_open_process (PFunArgs);
static void pf_process_write (PFunArgs);
static void pf_process_read (PFunArgs);
static void pf_process_kill (PFunArgs);

static PFunDesc ftab[] =
{
  { "WITH-OPEN-PROCESS",	1, 0, pf_with_open_process },
  { "PROCESS-WRITE",		0, 0, pf_process_write },
  { "PROCESS-READ",		0, 0, pf_process_read },
  { "PROCESS-KILL",		0, 0, pf_process_kill },
  { (CHAR *)NULL,		0, 0, (PFunHandler *)NULL }
};

void
module_initialize (void)
{
  static int called = 0;

  if (!called)
    {
      register int i;
      Symbol *sym, *funcnames;

      called++;
      funcnames = symbol_intern ("modules::syms-of-process");

      /* Install the names and pointers. */
      for (i = 0; ftab[i].tag != (char *)NULL; i++)
	{
	  sym = symbol_intern_in_package (mhtml_function_package, ftab[i].tag);
	  symbol_add_value (funcnames, ftab[i].tag);
	  sym->type = symtype_FUNCTION;
	  sym->values = (char **)(&ftab[i]);
	}
    }
}

void _init (void) { module_initialize (); }

DOC_SECTION (PROCESS-OPERATORS)
     /* PACKAGE_INITIALIZER (initialize_process_functions) */

#define op_NONE  0
#define op_READ  1
#define op_WRITE 2

typedef struct
{
  char *invoker;	/* The string used to invoke this process. */
  pid_t pid;		/* The process ID of this process. */
  int input;		/* File descriptor used to write to the proccess. */
  int output;		/* File descriptor used to read from the process. */
  int error;		/* File descriptor used to read from the process. */
  int flags;		/* Special flags indicate changes in behaviour. */
  int last_op;		/* The last operation done on this process. */
} Process;

typedef void mhtml_sigfun (int);
#undef SIG_DFL
#define SIG_DFL (mhtml_sigfun *)NULL

static int process_environment_level = 0;
static Process **open_processes = (Process **)NULL;
int open_processes_slots = 0;
int open_processes_index = 0;

extern jmp_buf page_jmp_buffer;

/* Install this process, and return the index which is used to identify it. */
static int
add_process (char *invocation_string, int flags)
{
  Process *proc = (Process *)xmalloc (sizeof (Proccess));
  int result = open_processes_index;
  int stdout_pipe[2];
  int stdin_pipe[2];
  int stderr_pipe[2];
  int child;

  if (open_processes_index + 2 > open_processes_slots)
    open_processes = (Process **)xrealloc
      (open_processes, ((open_processes_slots += 5) * sizeof (Process *)));

  open_processes[open_processes_index++] = proc;
  open_processes[open_processes_index] = (Process *)NULL;

  pipe (stdout_pipe);
  pipe (stdin_pipe);
  pipe (stderr_pipe);

  proc->invoker = strdup (invocation_string);

  child = vfork ();

  proc->flags = flags;
  proc->pid = child;
  proc->last_op = op_NONE;

  /* In the parent. */
  if (child != (pid_t) 0)
    {
      close (stdout_pipe[1]);
      close (stderr_pipe[1]);
      close (stdin_pipe[0]);
      proc->input = stdin_pipe[1];
      proc->output = stdout_pipe[0];
      proc->error = stderr_pipe[0];

      /* Say what to do when a child dies. */
      signal (SIGCHLD, (sig_t)release_child);
    }
  else /* In the child. */
    {
      close (stdout_pipe[0]);
      close (stderr_pipe[0]);
      close (stdin_pipe[1]);
      dup2 (stdin_pipe[0], 0);
      dup2 (stdout_pipe[1], 1);
      dup2 (stderr_pipe[1], 2);
      proc->input = 0;
      proc->output = 1;
      proc->error = 2;
      close (stdout_pipe[1]);
      close (stderr_pipe[1]);
      close (stdin_pipe[0]);
    }

  return (child);
}

/* De-install this process. */
static void
discard_process (int process_index)
{
  /* This really can only be called as the result of ending the
     function with-open-process.  That means that process_index should
     point to the process on the TOS. */
  if (open_processes_index)
    {
      Process *proc = open_processes[--open_processes_index];

      open_processes[open_processes_index] = (Process *)NULL;
      xfree (proc->invoker);

      if (proc->pid)
	{
	  close (proc->input);
	  close (proc->output);
	  close (proc->error);
	}
      else
	_exit ();

      free (proc);
    }
}

DEFMACRO (pf_with_open_process, var cmd &rest args,
"Create an environment in which <var var> is bound to the
<i>indicator</i> of an open process, the execution of <var CMD> with
<var args>.")
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *filename = mhtml_canonicalize_file_name_argument
    (get_positional_arg (vars, 1));
  int jump_again = 0;

  jump_again = 0;

  if ((!empty_string_p (varname)) && (!empty_string_p (filename)))
    {
      char *modename = mhtml_evaluate_string (get_value (vars, "MODE"));
      char *typename = mhtml_evaluate_string (get_value (vars, "TYPE"));
      char *notimeout = mhtml_evaluate_string (get_value (vars, "NOTIMEOUT"));
      int mode = stream_mode (modename);
      int type = stream_type (typename);
      int flags = notimeout ? stream_NOTIMEOUT : 0;
      int stream_fd = -1;

      xfree (modename);
      xfree (typename);
      xfree (notimeout);

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
	}

      if (stream_fd > -1)
	{
	  char stream_value[40];
	  PAGE *body_code = page_copy_page (body);
	  int page_value;

	  page_value =
	    add_open_stream (filename, type, stream_fd, mode, flags);

	  sprintf (stream_value, "%d", page_value);
	  pagefunc_set_variable (varname, stream_value);

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
	  char *temp = mhtml_funargs (vars);

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

static Stream *
get_stream_reference (Package *vars)
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

/* Gets called if/when we run out of time waiting for a known number
   of bytes that are available. */
static int operation_timed_out_p = 0;
static jmp_buf operation_jmp_buf;

static void
operation_timed_out (int sig)
{
  alarm (0);
  operation_timed_out_p = 1;
  longjmp (operation_jmp_buf, 1);
}

/* A 28.8 modem can deliver a maximum of 3.6k per second without
   taking compression into consideration.  A 14.4 can only deliver
   1.8k per second.  So, we will give the network 2 seconds to
   deliver each 1k of data, and then cushion that with a 10 second
   grace period.  Gee, isn't the Internet Fantastic? */
static void
set_io_timer (int length)
{
  char *minimum_data_transfer_rate = 
    pagefunc_get_variable ("mhtml::minimum-data-transfer-rate");
  int timeout, divisor = 0;

  if (minimum_data_transfer_rate != (char *)NULL)
    divisor = atoi (minimum_data_transfer_rate);

  if (divisor < 1)
    divisor = 512;

  timeout = length / 512;
  timeout += 10;

  operation_timed_out_p = 0;
  signal (SIGALRM, operation_timed_out);
  alarm (timeout);
}

static void
stream_write (Stream *stream, int length, char *bytes)
{
  char *write_from_loc = bytes;
  int bytes_written;

  if ((stream->flags & stream_MUST_SEEK) && (stream->last_op != op_WRITE))
    lseek (stream->fd, 0l, 1);

  set_io_timer (length);

  if (setjmp (operation_jmp_buf))
    {
      length = 0;
      stream->type = stream_CLOSED;
    }

  while (length != 0)
    {
      bytes_written = write (stream->fd, write_from_loc, length);

      if ((bytes_written == -1) || (operation_timed_out_p == 1))
	{
	  stream->type = stream_CLOSED;
	  return;
	}

      length -= bytes_written;
      write_from_loc += bytes_written;
    }

  alarm (0);
  signal (SIGALRM, SIG_DFL);
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

  if ((stream->flags & stream_MUST_SEEK) && (stream->last_op != op_READ))
    lseek (stream->fd, 0l, 1);

  if (stream->flags & stream_NOTIMEOUT)
    set_io_timer (999999);
  else
    set_io_timer (80);

  if (setjmp (operation_jmp_buf))
    {
      done = 1;
      stream->type = stream_CLOSED;
    }

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

	  read_result = read (stream->fd, minbuf + mindex, 1);
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
	  else if (stream->type == stream_TCP)
	    {
	      /* For TCP streams, we try hard to read in 10k chunks. */
	      int bits_size = 20 * 1024;

	      while (!done)
		{
		  if ((buffer_index + bits_size) >= buffer_size)
		    {
		      buffer = (char *)
			xrealloc (buffer, (buffer_size += bits_size));
		    }

		  read_result =
		    read (stream->fd, buffer + buffer_index, bits_size);

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

  alarm (0);
  signal (SIGALRM, SIG_DFL);

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

  if ((stream->flags & stream_MUST_SEEK) && (stream->last_op != op_READ))
    lseek (stream->fd, 0l, 1);

  buffer = (char *)xmalloc (length + 1);

  if (stream->flags & stream_NOTIMEOUT)
    set_io_timer (999999);
  else
    set_io_timer (length);

  if (setjmp (operation_jmp_buf))
    {
      done = 1;
      stream->type = stream_CLOSED;
    }

  while (!done)
    {
      int bytes_read = read (stream->fd, buffer + buffer_index, bytes_left);

      if (bytes_read > 0)
	{
	  buffer_index += bytes_read;
	  bytes_left -= bytes_left;
	}
      else
	{
	  done = 1;
	  stream->type = stream_CLOSED;
	}

      if (bytes_left == 0)
	done = 1;
    }

  alarm (0);
  signal (SIGALRM, SIG_DFL);
  stream_last_read_object_size = buffer_index;

  return (buffer);
}

DEFUN (pf_stream_put, stream string,
"Writes <var string> to the open <var stream>.")
{
  Stream *stream = get_stream_reference (vars);

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
"Writes the contents of the binary variable <var var> to the open <var
stream>.  Binary variables are generally created with <funref
stream-operators stream-get-contents>, or when uploading a file from
a Web browser.")
{
  Stream *stream = get_stream_reference (vars);

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
  Stream *stream = get_stream_reference (vars);
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
  Stream *stream = get_stream_reference (vars);
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
  Stream *stream = get_stream_reference (vars);
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

	FD_ZERO (&read_fds);
	FD_SET (stream->fd, &read_fds);
	poll.tv_sec = delay_in_seconds;
	poll.tv_usec = 0;
	ready = select (stream->fd + 1, &read_fds, (fd_set *)0, (fd_set *)0,
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
  Stream *stream = get_stream_reference (vars);
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
  Stream *stream = get_stream_reference (vars);

  if ((stream != (Stream *)NULL) && (stream->type != stream_CLOSED))
    shutdown (stream->fd, 2);
}

#if defined (__cplusplus)
}
#endif
