/* cgi-exec.c: Implementation of the <cgi-exec> command in Meta-HTML. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jul 31 09:07:24 1996.  */

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

#define NEW_CGI_EXEC 1
#include "language.h"

#if defined (__cplusplus)
extern "C"
{
#endif

#if defined (NEW_CGI_EXEC)
#  if defined (__CYGWIN32__)
     /* Needed for select(). */
#    include <sys/socket.h>
#  endif /* __CYGWIN32__ */
#  if defined (HAVE_VFORK_H)
#    include <vfork.h>
#  endif
#endif /* NEW_CGI_EXEC */


static void pf_cgi_exec (PFunArgs);

static PFunDesc func_table[] =
{
  { "CGI-EXEC",		0, 0, pf_cgi_exec },
  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_cgi_exec_functions)
DOC_SECTION (FILE-OPERATORS)

DEFVAR (mhtml::exec-path,
"A colon-seperated list of directories in which <Meta-HTML> searches
for executables when executing the <funref FILE-OPERATORS cgi-exec> tag.
Each component is a path to a directory, without the final trailing
slash, separated from its neighbors by a colon character
(<code>:</code>).

For example:

<example>
   <set-var mhtml::exec-path=\"/www/bin:/www/docs/cgi-bin\">
</example>")

#if defined (NEW_CGI_EXEC)
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

static char *
gather_input (int fd, int timeout_seconds)
{
  char *result = (char *)NULL;
  int select_result = 1;
#if defined (FD_SET)
  struct timeval timeout;
  fd_set read_fds;
  int intr = 0;

  timeout.tv_sec = timeout_seconds;
  timeout.tv_usec = 0;

  FD_ZERO (&read_fds);
  FD_SET (fd, &read_fds);

  while (intr < 2)
    {
      select_result = select (fd + 1, (fd_set *)(&read_fds), 0, 0, &timeout);
      if ((select_result == -1) && (errno == EINTR))
	intr++;
      else
	break;
    }
#else /* !FD_SET */
  select_result = 1;
#endif /* !FD_SET */

  switch (select_result)
    {
    case 0:
    case -1:
      break;

    default:
      {
	char *buffer = (char *)NULL;
	int bindex = 0, bsize = 0;
        int amount_read;
        int done = 0;

        while (!done)
          {
            while ((bindex + 1024) > bsize)
              buffer = (char *)xrealloc (buffer, (bsize += 1025));
            buffer[bindex] = '\0';

            amount_read = read (fd, buffer + bindex, 1023);

            if ((amount_read < 0) && errno != EINTR)
              {
                done = 1;
              }
            else
              {
		if (amount_read != -1)
		  {
		    bindex += amount_read;
		    buffer[bindex] = '\0';
		  }

                if (amount_read == 0)
                  done = 1;
              }
          }
	result = buffer;
      }
    }

  return (result);
}

static int
shell_execute (char *command, char **output_textp, char **errors_textp,
	       int timeout_seconds, int nowait_p)
{
  char *output_text = (char *)NULL;
  char *errors_text = (char *)NULL;

  if (command == (char *)NULL)
    errors_text = strdup ("cgi-exec: No command given");

  if (errors_text == (char *)NULL)
    {
#if defined (__CYGWIN32__)
      char *physical_path = "/www/bin/bash.exe";
#else
      char *physical_path = "/bin/sh";
#endif

      if (errors_text == (char *)NULL)
	{
	  pid_t child;
	  int stdout_pipe[2];
	  int stderr_pipe[2];

	  /* Make a receiving pipes for stdout and stderr, iff the
	     user wishes to receive the output. */
	  if (!nowait_p)
	    {
	      pipe (stdout_pipe);
	      pipe (stderr_pipe);
	    }

	  child = vfork ();

	  /* In the parent... */
	  if (child != (pid_t)0)
	    {
	      /* Setup pipes for communication. */
	      if (!nowait_p)
		{
		  close (stdout_pipe[1]);
		  close (stderr_pipe[1]);
		}

	      /* Say what to do when a child dies. */
	      signal (SIGCHLD, (sig_t)release_child);

	      /* Say what to do if the pipe is broken. */
	      signal (SIGPIPE, SIG_IGN);

	      if (!nowait_p)
		{
		  output_text = gather_input (stdout_pipe[0], timeout_seconds);
		  errors_text = gather_input (stderr_pipe[0], timeout_seconds);

		  close (stdout_pipe[0]);
		  close (stderr_pipe[0]);
		}
	    }
	  else			/* In the child... */
	    {
	      char *argv[4];

	      pagefunc_set_variable
		("env::path", pagefunc_get_variable ("mhtml::exec-path"));
	      symbol_remove ("env::query_string");
	      symbol_remove ("env::path_info");
	      symbol_remove ("env::content_length");

	      argv[0] = physical_path;
	      argv[1] = "-c";
	      argv[2] = command;
	      argv[3] = (char *)NULL;

	      /* In the child, make stdout and stderr be our pipes. */
	      if (!nowait_p)
		{
		  close (stdout_pipe[0]);
		  close (stderr_pipe[0]);
		  dup2 (stdout_pipe[1], 1);
		  dup2 (stderr_pipe[1], 2);
		  close (stdout_pipe[1]);
		  close (stderr_pipe[1]);
		}
	      else
		{
#if defined (TIOCNOTTY)
		  int tty;
		  tty = open ("/dev/tty", O_RDWR, 0666);
		  if (tty) ioctl (tty, TIOCNOTTY, 0);
		  close (tty);
#endif
#if defined (SETPGRP_VOID)
		  setpgrp ();
#else
		  setpgrp (0, getpid());
#endif /* !SETPGRP_VOID */
		  close (0);
		  close (1);
		}

	      execve (physical_path, argv, make_environ ());
	      _exit (127);
	    }
	}
    }

  *output_textp = output_text;
  *errors_textp = errors_text;

  return (errors_text != (char *)NULL);
}

#else /* !NEW_CGI_EXEC */

static int
shell_execute (char *command, char **output_textp, char **errors_textp,
	       int timeout_seconds, int nowait_p)
{
  char *output_text = (char *)NULL;
  char *errors_text = (char *)NULL;
  BPRINTF_BUFFER *ebuff;

  if (command == (char *)NULL)
    errors_text = strdup ("No command given");
  else
    {
      char *exec_path = (char *)NULL;
      FILE *stream = (FILE *)NULL;

#if !defined (macintosh)
      exec_path = pagefunc_get_variable ("mhtml::exec-path");

      if (!empty_string_p (exec_path))
	{
	  BPRINTF_BUFFER *cb = bprintf_create_buffer ();
	  bprintf (cb, "export PATH; PATH=%s:$PATH; %s", exec_path, command);
	  command = cb->buffer;
	  free (cb);
	}
      stream = popen (command, "r");
#endif

      if (stream == (FILE *)NULL)
	{
	  ebuff = bprintf_create_buffer ();
	  bprintf (ebuff, "Cannot execute %s", command);
	  errors_text = ebuff->buffer;
	  free (ebuff);
	}
      else
	{
	  BPRINTF_BUFFER *gather = bprintf_create_buffer ();
	  char buffer[1024];

	  while (fgets (buffer, 1023, stream) != (char *)NULL)
	    bprintf (gather, "%s", buffer);

#if !defined (macintosh)
	  pclose (stream);
#endif
	  output_text = gather->buffer;
	  free (gather);
	}

      if (!empty_string_p (exec_path))
	free (command);
    }

  *output_textp = output_text;
  *errors_textp = errors_text;
  
  return (errors_text != (char *)NULL);
}
#endif /* !NEW_CGI_EXEC */

DEFUN (pf_cgi_exec,
       pathname &optional arg... &key output=varname errors=varname
       timeout=seconds nowait=true,
"Execute the system function named by <var pathname>, perhaps passing
it arguments <var arg ... argn>.

If <var output=varname> is supplied, then the variable referenced by
<var varname> receives the output of the command.  Otherwise, the
resultant output is placed in the page.

If <var errors=varname> is supplied, then <var varname> receives the
error output of the command. Otherwise, the resultant output is placed
in <funref language-operators SYSTEM-ERROR-OUTPUT>.")
{
  if (vars != (Package *)NULL)
    {
      register int i;
      BPRINTF_BUFFER *exec_string;
      char *output_varname = (char *)NULL;
      char *errors_varname = (char *)NULL;
      char *timeout_string = (char *)NULL;
      char *nowait_string  = (char *)NULL;
      char *output_text = (char *)NULL;
      char *errors_text = (char *)NULL;
      char **names = get_vars_names (vars);

      exec_string = bprintf_create_buffer ();

      for (i = 0; names[i] != (char *)NULL; i++)
	{
	  Symbol *sym = symbol_lookup_in_package (vars, names[i]);

	  if (sym != (Symbol *)NULL)
	    {
	      if (sym->values == (char **)NULL)
		{
		  /* Note that we use NAMES[i] here because that is the
		     exact text that the user typed in, and not the
		     canonicalized version of the variable name.
		     This is how case is preserved. */
		  bprintf (exec_string, "%s ", names[i]);
		}
	      else if (strcmp (sym->name, "OUTPUT") == 0)
		output_varname = mhtml_evaluate_string (sym->values[0]);
	      else if (strcmp (sym->name, "ERRORS") == 0)
		errors_varname = mhtml_evaluate_string (sym->values[0]);
	      else if (strcmp (sym->name, "TIMEOUT") == 0)
		timeout_string = mhtml_evaluate_string (sym->values[0]);
	      else if (strcmp (sym->name, "NOWAIT") == 0)
		nowait_string = mhtml_evaluate_string (sym->values[0]);
	    }
	}

      if (exec_string->buffer != (char *)NULL)
	{
	  char *temp = mhtml_evaluate_string (exec_string->buffer);

	  if (temp != (char *)NULL)
	    {
	      int timeout_seconds = 0;

	      if (!empty_string_p (timeout_string))
		{
		  if (strcasecmp (timeout_string, "none") == 0)
		    timeout_seconds = 2147483647;
		  else
		    timeout_seconds = atoi (timeout_string);
		}

	      if (timeout_seconds < 1)
		timeout_seconds = 90;

	      if (debug_level)
		page_debug ("cgi-exec:  `%s'", temp);

	      shell_execute (temp, &output_text, &errors_text,
			     timeout_seconds, !empty_string_p (nowait_string));
	      free (temp);
	    }
	}

      if (output_varname != (char *)NULL)
	pagefunc_set_variable (output_varname, output_text ? output_text : "");
      else if (output_text)
	bprintf_insert (page, start, "%s", output_text);

      if (errors_varname != (char *)NULL)
	pagefunc_set_variable (errors_varname, errors_text ? errors_text : "");
      else if (!empty_string_p (errors_text))
	{
#if defined (I_DONT_THINK_SO)
	  bprintf_insert (page, start, "%s", errors_text);
#else
	  page_debug ("%s", errors_text);
#endif /* I_DONT_THINK_SO */
	}

      bprintf_free_buffer (exec_string);

      xfree (output_varname);
      xfree (errors_varname);
      xfree (timeout_string);
      xfree (nowait_string);
      xfree (output_text);
      xfree (errors_text);
    }
}

#if defined (__cplusplus)
}
#endif
