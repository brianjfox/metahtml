/* server.c: This main file for the Meta-HTML linked in WWW server. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Nov 10 18:48:53 1995.  */

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

#include "language.h"
#include "globals.h"
#include "logging.h"

#define I_UNDERSTAND_IDENT
#if !defined (MHTML_SYSTEM_TYPE)
#  define MHTML_SYSTEM_TYPE "Incorrectly Compiled"
#endif

#if defined (HAVE_GETPWNAM)
#  include <pwd.h>
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

#if !defined (errno)
extern int errno;
#endif

#if defined (Solaris)
extern int killpg (pid_t, int);
#endif

#if !defined (SEEK_END)
#  define SEEK_END 2
#endif

/* The name of this program, as taken from argv[0]. */
static char *progname = (char *)NULL;

/* When you set the port number using a command line argument, it overrides
   any variable assignments in mhttpd.conf.  This variable tells us to do
   that. */
static int forced_port_number = 0;

/* The port number that we normally connect on. */
static int http_port = 80;

/* When non-null, this is the hostname of the IP address that we wish to
   bind our socket tightly to.  If not set, it defaults to any available
   interface on our machine.  Otherwise, it is hostname that has been
   assigned to a network interface with ifconfig.  This allows hostname
   aliasing to work. */
static char *http_host = (char *)NULL;

/* When non-zero, the caller specified mhtml::server-name in mhttpd.conf. */
static int http_host_set = 0;

/* The full pathname to the configuration file. */
static char *mhttpd_config_path = (char *)NULL;

/* Non-zero means don't fork, etc. */
static int debug_mode = 0;

/* The default number of seconds a child is allowed to run. */
static int default_limit = 120;

/* One less than the number of servers to start running for each bound
   address. */
static int servers_to_fork = 0;

/* Don't fork if this is non-zero (--dont-fork). */
static int dont_fork = 0;

/* Non-zero means run once under inetd, and exit. */
static int once_and_exit = 0;

#define MHTTPD_DNS_CACHE "/tmp/mhttpd-dns.cache"

#if defined (MHTTPD_DNS_CACHE)
/* Non-zero means lookup and save DNS information in a local cache file. */
static int mhttpd_cache_dns = 0;
#endif

/* Forward declarations. */
static void parse_program_args (int argc, char *argv[]);
static void loop_over_connections (void);
static void initialize_server (void);
static void setup_signal_handling (void);
static void fatal (char *format, ...);

#if defined (CHECK_ACTIVATION)
extern int check_activation_key (char *key);
#endif

int
main (int argc, char *argv[])
{
  pid_t pid;

  progname = argv[0];
  parse_program_args (argc, argv);

  if (!once_and_exit && isatty (fileno (stderr)))
    {
      fprintf (stderr, "%s: Initializing server...", argv[0]); 
      fflush (stderr);
    }

  initialize_server ();

  if (!once_and_exit && isatty (fileno (stderr)))
    {
      fprintf (stderr, "done.\n");
      fflush (stderr);
      fflush (stdout);
    }

  if (debug_mode || once_and_exit)
    loop_over_connections ();
  else
    {
      if (dont_fork)
	pid = 0;
      else
	pid = fork ();

      /* If this is the child, then print out startup message and give
	 up the controlling terminal. */
      if (!pid)
	{
	  char server_pid[20];
	  char *prefix = (char *)xmalloc (20 + strlen (mhttpd_config_path));
	  FILE *pid_stream;
	  char *temp;

	  /* Save the pid of this process in the same directory as the
	     mhttpd.conf file. */
	  sprintf (server_pid, "%d", (int)getpid ());
	  sprintf (prefix, "%s", mhttpd_config_path);

	  temp = strrchr (prefix, '/');
	  if (temp != (char *)NULL)
	    temp++;
	  else
	    temp = prefix;

	  {
	    char *pid_filename =
	      pagefunc_get_variable ("mhttpd::pid-filename");

	    if (!empty_string_p (pid_filename))
	      sprintf (temp, "%s", pid_filename);
	    else
	      sprintf (temp, "mhttpd.pid");
	  }

	  pid_stream = fopen (prefix, "w");
	  if (pid_stream != (FILE *)NULL)
	    {
	      fprintf (pid_stream, "%s", server_pid);
	      fclose (pid_stream);
	    }

	  pagefunc_set_variable ("mhttpd::server-pid", server_pid);
	  pagefunc_set_variable ("mhttpd::copyright-string",
				 metahtml_copyright_string);

	  if (isatty (fileno (stderr)))
	    fprintf (stderr, "%s: starting server (%s:%d) (pid %s)\n",
		     argv[0], http_host, http_port, server_pid);

#if defined (HAVE_SETSID)
	  setsid ();
#else
#if defined (TIOCNOTTY)
	  {
	    int tty;
	    tty = open ("/dev/tty", O_RDWR, 0666);
	    if (tty) ioctl (tty, TIOCNOTTY, 0);
	    close (tty);
	  }
#endif
#if defined (SETPGRP_VOID)
	  setpgrp ();
#else
	  setpgrp (0, getpid());
#endif /* !SETPGRP_VOID */
#endif /* HAVE_SETSID */
	  freopen ("/dev/null", "r", stdin);
	  freopen ("/dev/null", "w", stdout);
	  /* close (0); */
	  /* close (1); */

	  loop_over_connections ();
	}
    }

  exit (0);
}

static char *deletable_variables[] =
{
  "mhtml::server-name",
  "mhtml::server-port",
  "mhtml::document-root",
  "mhtml::access-log",
  "mhtml::error-log",
  "mhtml::referer-log",
  "mhtml::agent-log",
  "mhtml::debug-log",
  "mhtml::log-performance",
  "mhtml::default-user",
  "mhttpd::per-request-function",
  "mhttpd::session-database-file",
  "mhttpd::get-remote-ident",
#if defined (MHTTPD_DNS_CACHE)
  "mhttpd::cache-dns",
#endif

  (char *)NULL
};

/* What to do when we get a SIGHUP.  Done once at startup. */
static void
reinitialize_server (void)
{
  register int i;
  char *temp;
  PAGE *page;
  static int first_time = 1;
  char *include_prefix = strdup (mhttpd_config_path);

  if ((temp = strrchr (include_prefix, '/')) != (char *)NULL)
    {
      *temp = '\0';
      pagefunc_set_variable ("mhtml::include-prefix", include_prefix);
    }

  free (include_prefix);
      
  if (!first_time)
    mhttpd_log (log_ERROR, "Got SIGHUP: Reading %s again...",
		mhttpd_config_path);

  /* Delete all of the variables which we can reasonably expect the
     config file to set. */
  for (i = 0; deletable_variables[i] != (char *)NULL; i++)
    symbol_remove (deletable_variables[i]);

  if (mhttpd_ssl_server)
    {
      mhttpd_initialize_ssl ();
      pagefunc_set_variable ("mhttpd::ssl-server", "true");
    }

  page = page_read_template (mhttpd_config_path);

  if (!page)
    fatal ("Cannot read configuration file `%s'!", mhttpd_config_path);

  if (once_and_exit)
    {
      /* Set up the input and output file descriptors. */
      mhtml_stdout_fileno = fileno (stdout);
      mhtml_stdin_fileno = fileno (stdin);
      mhtml_stderr_fileno = fileno (stderr);

      /* Quickly make a package containing the minimum mime-types. */
      pagefunc_set_variable ("mime-type::.mhtml", "metahtml/interpreted");
      pagefunc_set_variable ("mime-type::.jpeg", "image/jpeg");
      pagefunc_set_variable ("mime-type::.jpg", "image/jpeg");
      pagefunc_set_variable ("mime-type::.gif", "image/gif");
      pagefunc_set_variable ("mime-type::.html", "text/html");
      pagefunc_set_variable ("mime-type::.txt", "text/plain");
      pagefunc_set_variable ("mime-type::.mov", "video/quicktime");
      pagefunc_set_variable ("mime-type::.default", "text/plain");

      /* The minimum startup documents. */
      pagefunc_set_variable ("mhtml::default-filenames[]",
			     "Welcome.mhtml\nwelcome.mhtml\n\
Index.mhtml\nindex.mhtml\n\
Home.mhtml\nhome.mhtml\n\
Directory.mhtml\ndirectory.mhtml\n\
index.html\nhome.html");

      /* Set mhtml::require-directories[], to a reasonable value here. */
      pagefunc_set_variable ("mhtml::require-directories[]",
".\ntagsets\nmacros\nincludes\n\
..\n../tagsets\n../macros\n../includes\n\
../..\n../../tagsets\n../../macros\n../../includes");

      /* The default extensions for running files through the engine. */
      pagefunc_set_variable ("mhtml::metahtml-extensions[]", ".mhtml");
    }

  if (forced_port_number)
    mhtml_set_numeric_variable ("mhtml::forced-server-port",
				forced_port_number);
  if (debug_mode)
    pagefunc_set_variable ("mhtml::debug-mode", "true");

  if (page->buffer)
    page_process_page (page);

  if (page)
    page_free_page (page);

  /* Now set our local variables. */
  temp = pagefunc_get_variable ("mhtml::server-name");

  if (temp != (char *)NULL)
    {
      http_host = strdup (temp);
      http_host_set = 1;
    }
  else
    {
      char buffer[100];

      strcpy (buffer, "www.nohost.net");
      gethostname (buffer, 100);
      http_host = strdup (buffer);
      pagefunc_set_variable ("mhtml::server-name", http_host);
      http_host_set = 0;
    }

  {
    int val = 0;

    temp = (char *)NULL;

    if (mhttpd_ssl_server)
      temp = pagefunc_get_variable ("mhtml::ssl-port");

    if (!temp)
      temp = pagefunc_get_variable ("mhtml::server-port");

    if (temp != (char *)NULL)
      val = atoi (temp);

    if (val < 1)
      {
	if (mhttpd_ssl_server)
	  val = 443;
	else
	  val = 80;
      }

    http_port = val;

    /* Watch out.  If the user forced a port number on us,
       use that instead. */
    if (forced_port_number != 0)
      {
	char num[20];
	http_port = forced_port_number;
	sprintf (num, "%d", http_port);
	pagefunc_set_variable ("mhtml::server-port", num);
      }
  }

  temp = pagefunc_get_variable ("mhtml::document-root");
  if (temp)
    {
      sv_DocumentRoot = strdup (temp);
      pagefunc_set_variable ("mhtml::include-prefix", temp);

      chdir (sv_DocumentRoot);
    }

  /* Create a reasonable default PATH variable if the user didn't do so. */
  if (pagefunc_get_variable ("mhtml::exec-path") == (char *)NULL)
    {
      page = page_create_page ();
      bprintf (page, "<set-var mhtml::exec-path =");
      bprintf (page, "<get-var mhtml::server-root>/bin:/bin:");
      bprintf (page, "<get-var mhtml::document-root>/cgi-bin:/usr/bin:");
      bprintf (page, "/usr/local/bin:/usr/ucb:/www/bin:/opt/metahtml/bin:");
      bprintf (page, "/usr/lib/metahtml");
      bprintf (page, ">");
      page_process_page (page);

      if (page)
	page_free_page (page);
    }

  /* Now to the logfiles. */
  mhttpd_set_logfile (log_ACCESS,  "");
  mhttpd_set_logfile (log_ERROR,   "");
  mhttpd_set_logfile (log_DEBUG,   "");
  mhttpd_set_logfile (log_REFERER, "");
  mhttpd_set_logfile (log_AGENT, "");

  temp = pagefunc_get_variable ("mhtml::access-log");
  if (temp)
    mhttpd_set_logfile (log_ACCESS, temp);

  temp = pagefunc_get_variable ("mhtml::error-log");
  if (temp)
    mhttpd_set_logfile (log_ERROR, temp);

  mhttpd_debugging = 0;
  temp = pagefunc_get_variable ("mhtml::debug-log");
  if (temp)
    {
      mhttpd_set_logfile (log_DEBUG, temp);
      mhttpd_debugging = 1;
    }

  mhttpd_log_performance = 0;
  temp = pagefunc_get_variable ("mhtml::log-performance");
  if (temp)
    mhttpd_log_performance = 1;

  mhttpd_log_referer = 0;
  temp = pagefunc_get_variable ("mhtml::referer-log");
  if (temp)
    {
      mhttpd_set_logfile (log_REFERER, temp);
      mhttpd_log_referer = 1;
    }

  mhttpd_log_agent = 0;
  temp = pagefunc_get_variable ("mhtml::agent-log");
  if (temp)
    {
      mhttpd_set_logfile (log_AGENT, temp);
      mhttpd_log_agent = 1;
    }

  default_limit = 120;
  temp = pagefunc_get_variable ("mhtml::response-timeout");
  if (temp)
    default_limit = atoi (temp);

  mhttpd_per_request_function =
    pagefunc_get_variable ("mhttpd::per-request-function");

  set_session_database_location
    (pagefunc_get_variable ("mhttpd::session-database-file"));

#if defined (MHTTPD_DNS_CACHE)
  temp = pagefunc_get_variable ("mhttpd::cache-dns");
  if (empty_string_p (temp))
    mhttpd_cache_dns = 0;
  else
    mhttpd_cache_dns = 1;
#endif

  if (first_time)
    first_time = 0;
  else
    mhttpd_log (log_ERROR, "Finshed reading %s again...", mhttpd_config_path);

  if (!once_and_exit)
    signal (SIGHUP, (sig_t) reinitialize_server);
}

static int
file_exists_p (char *path)
{
  struct stat finfo;

  return (stat (path, &finfo) != -1);
}

static void
find_config_file (void)
{
  char dir[1024];
  char path[1024 + 256];
  char *temp;

  if (mhttpd_config_path)
    return;

  /* Try finding mhttpd.conf locally. */
  temp = getcwd (dir, sizeof (dir));

  if (!temp)
    fatal ("Can't get working directory!");

  /* Try ./mhttpd.conf */
  sprintf (path, "%s/mhttpd.conf", dir);

  if (file_exists_p (path))
    mhttpd_config_path = strdup (path);
  else
    {
      /* Try ./conf/mhttpd.conf */
      sprintf (path, "%s/conf/mhttpd.conf", dir);
      if (file_exists_p (path))
	mhttpd_config_path = strdup (path);
      else
	{
	  /* Try ../mhttpd.conf */
	  temp = strrchr (dir, '/');
	  if (temp)
	    {
	      *temp = '\0';
	      sprintf (path, "%s/mhttpd.conf", dir);
	      if (file_exists_p (path))
		mhttpd_config_path = strdup (path);
	      else
		{
		  /* Try ../conf/mhttpd.conf */
		  sprintf (path, "%s/conf/mhttpd.conf", dir);
		  if (file_exists_p (path))
		    mhttpd_config_path = strdup (path);
		}
	    }
	}
    }

  if (!mhttpd_config_path)
    {
      sprintf (path, "%s/mhttpd.conf", dir);
      mhttpd_config_path = strdup (path);
    }
}

static void
initialize_server ()
{
  setup_signal_handling ();
  mhtml_system_preload (1);

  find_config_file ();

  if (!mhttpd_config_path)
    fatal ("Cannot find mhttpd.conf!");

  pagefunc_set_variable ("mhtml::system-type", MHTML_SYSTEM_TYPE);
  reinitialize_server ();
  bootstrap_metahtml (0);

#if defined (CHECK_ACTIVATION)
  if (!debug_mode)
    {
      char *key = pagefunc_get_variable ("mhtml::activation-key");
      if (check_activation_key (key) == 0)
	exit (1);
    }
#endif /* CHECK_ACTIVATION */

  if (mhttpd_debugging)
    mhttpd_log (log_ERROR, "%s: Started server on port %s",
		pagefunc_get_variable ("mhtml::server-name"),
		pagefunc_get_variable ("mhtml::server-port"));
}

static void
display_version (void)
{
  fprintf (stderr, "Meta-HTML Web Server Authored by Brian J. Fox\n");
  fprintf (stderr, "(bfox@ai.mit.edu) on Fri Nov 10 18:48:53 1995\n");
  fprintf (stderr, "%s\n", sv_VersionString);
  fprintf (stderr, "%s\n", metahtml_copyright_string);
}

static void
display_version_and_exit (void)
{
  display_version ();
  exit (0);
}

static void
usage (void)
{
  display_version ();
  fprintf (stderr, "Usage: %s --config config-path [--version]\n", progname);
  exit (1);
}

static void
parse_program_args (int argc, char *argv[])
{
  int arg_index = 1;

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if (strcasecmp (arg, "--version") == 0)
	display_version_and_exit ();
      else if (strcasecmp (arg, "--debug") == 0)
	debug_mode = 1;
      else if (strcasecmp (arg, "--port") == 0)
	forced_port_number = atoi (argv[arg_index++]);
      else if (strcasecmp (arg, "--servers") == 0)
	{
	  servers_to_fork = atoi (argv[arg_index++]) - 1;
	  if (servers_to_fork < 0)
	    servers_to_fork = 0;
	}
      else if (strcasecmp (arg,  "--dont-fork") == 0)
	dont_fork = 1;
      else if (strcasecmp (arg,  "--ssl") == 0)
	mhttpd_ssl_server = 1;
      else if (strcasecmp (arg, "--inetd") == 0)
	once_and_exit = 1;
      else if ((strcasecmp (arg, "--config") == 0) ||
	       (strcasecmp (arg, "-config") == 0) ||
	       (strcasecmp (arg, "-f") == 0))
	{
	  mhttpd_config_path = strdup (argv[arg_index++]);
	}
      else
	usage ();
    }
}

static void
fatal (char *format, ...)
{
  int actual_error = errno;

  va_list args;
  va_start (args, format);

  fprintf (stderr, "%s: ", progname);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  if (actual_error)
    fprintf (stderr, "Error returned in errno: %d: %s\n", actual_error,
	     strerror (actual_error));
  exit (2);
}

/* Handling child processes. */

static int signals_are_blocked_p = 0;
#define SIGNALS_CAN_BE_BLOCKED sigset_t new_set, old_set
#define BLOCK_SIGNALS() \
   do {							\
        if (!signals_are_blocked_p)			\
	  {						\
	    sigemptyset (&new_set);			\
	    sigaddset (&new_set, SIGCHLD);		\
	    sigprocmask (SIG_BLOCK, &new_set, &old_set);\
	  }						\
	signals_are_blocked_p++;			\
      } while (0)

#define UNBLOCK_SIGNALS() \
   do {							\
	signals_are_blocked_p--;			\
	if (!signals_are_blocked_p)			\
	  sigprocmask (SIG_SETMASK, &old_set, &new_set);\
	if (signals_are_blocked_p < 0) abort();		\
      } while (0)

typedef struct
{
  pid_t pid;			/* PID of the running child. */
  int killed;			/* Signal child received from parent. */
  time_t start;			/* Time when the child started. */
  time_t end;			/* Time when the child finished. */
  time_t timeout;		/* When the child should be finished by. */
  int status;			/* How the child finished. */
} Child;

/* The list of existing children. */
static Child **children = (Child **)NULL;

/* Number of slots allocated to CHILDREN. */
static int children_slots = 0;

/* Number of children currently running. */
static int children_index = 0;

/* Add a child to the list of our children. */
static void
add_child (pid_t pid)
{
  Child *child;
  SIGNALS_CAN_BE_BLOCKED;

  BLOCK_SIGNALS ();
  child = (Child *)xmalloc (sizeof (Child));

  child->pid = pid;
  child->killed = 0;
  child->start = (time_t)time ((time_t *)0);
  child->timeout = child->start + default_limit;
  child->end = 0;
  child->status = 0;

  if (children_index + 2 >= children_slots)
    children = (Child **)xrealloc
      (children, (children_slots += 10) * sizeof (Child *));

  children[children_index++] = child;
  children[children_index] = (Child *)NULL;
  UNBLOCK_SIGNALS ();
}

/* Delete a child from our list of children.
   The deleted child is returned. */
static Child *
delete_child (pid_t pid)
{
  register int i;
  Child *child = (Child *)NULL;
  SIGNALS_CAN_BE_BLOCKED;

  BLOCK_SIGNALS ();

  for (i = 0; i < children_index; i++)
    {
      if (children[i]->pid == pid)
	{
	  child = children[i];

	  for (; i < children_index; i++)
	    children[i] = children[i + 1];

	  children_index--;
	  break;
	}
    }

  UNBLOCK_SIGNALS ();
  return (child);
}

/* Find the child structure for PID. */
static Child *
find_child (pid_t pid)
{
  register int i;
  Child *child = (Child *)NULL;
  SIGNALS_CAN_BE_BLOCKED;

  BLOCK_SIGNALS ();

  for (i = 0; i < children_index; i++)
    if (children[i]->pid == pid)
      {
	child = children[i];
	break;
      }

  UNBLOCK_SIGNALS ();

  return (child);
}

/* What to do when a child dies.  Signals do not have to be blocked
   in this function since it is only called from the signal handler. */
static void
release_child (void)
{
  int status;
  pid_t pid;
  Child *child;

  while ((pid = waitpid (-1, &status, WNOHANG)) > 0)
    {
      child = find_child (pid);

      if (child)
	{
	  child->end = (time_t)time ((time_t *)0);
	  child->status = status;

	  if (mhttpd_log_performance)
	    mhttpd_log (log_DEBUG,
			"Child process (%d) took (%d) seconds to complete",
			child->pid, child->end - child->start);
	}
    }
}

/* Reap dead children, perhaps reporting information about them. */
static void
reap_children (void)
{
  register int i, j;
  Child **dead_kids;
  SIGNALS_CAN_BE_BLOCKED;

  BLOCK_SIGNALS ();

  /* Might as well gather up the zombies left over from the child's child. */
  release_child ();

  dead_kids = (Child **)xmalloc ((1 + children_index) * sizeof (Child *));
  for (j = 0, i = 0; i < children_index; i++)
    if (children[i]->end)
      dead_kids[j++] = delete_child (children[i]->pid);

  dead_kids[j] = (Child *)NULL;

  /* Free the children, reporting on the ones that died abnormally. */
  for (i = 0; i < j; i++)
    {
      if (WIFSIGNALED (dead_kids[i]->status))
	{
	  int killing_signal = WTERMSIG (dead_kids[i]->status);

	  mhttpd_log (log_ERROR, "Child process (%d) was killed by signal %d",
		      (int)dead_kids[i]->pid, killing_signal);
	}

      free (dead_kids[i]);
    }

  free (dead_kids);
  signal (SIGCHLD, (sig_t)reap_children);
  UNBLOCK_SIGNALS ();
}

/* Check to see if any of our children have been alive too long.
   If so, kill them, mercilessly, and log that error. */
static void
check_child_timeouts (void)
{
  SIGNALS_CAN_BE_BLOCKED;
  BLOCK_SIGNALS ();

  if (children_index)
    {
      register int i;
      time_t now = (time_t)time ((time_t *)0);

      for (i = 0; i < children_index; i++)
	if (now >= children[i]->timeout)
	  {
	    /* Give up on recalcitrant children. */
	    if (children[i]->killed != 0)
	      {
		children[i]->end = now;
	      }
	    else
	      {
		mhttpd_log
		  (log_ERROR, "Killing child %d because it was too slow",
		   (int)children[i]->pid);

		kill (children[i]->pid, SIGKILL);
		children[i]->killed = SIGKILL;
	      }
	  }
    
      reap_children ();
    }

  UNBLOCK_SIGNALS ();
}

/* Close databases, clean up and exit. */
static void
shutdown_cleanly (int sig)
{
#if defined (hpux) || defined (__WINNT__)
  kill (-getpgrp(), sig);
#else
  killpg (0, sig);
#endif
  reap_children ();
  exit (0);
}

/* Make this program do the right thing with various signals. */
static void
setup_signal_handling (void)
{
  signal (SIGINT,  (sig_t) shutdown_cleanly);
  signal (SIGQUIT, (sig_t) shutdown_cleanly);
  signal (SIGTERM, (sig_t) shutdown_cleanly);

  /* On some systems (such as Linux) you must set the signal handler
     up each time it has been used.  Alternatively, I could use
     sigaction (), but it is a little bit painful just to change a
     signal flag.  Who thought up this shit, anyway? */
  signal (SIGHUP,  (sig_t) reinitialize_server);
  signal (SIGCHLD, (sig_t) reap_children);
}

#if defined (I_UNDERSTAND_IDENT)
static void
connect_timed_out (void)
{
  alarm (0);
}
#endif

typedef struct
{
  char *hostname;
  char *portname;
  struct in_addr host_address;
  struct sockaddr_in socket_address;
  int port;
  int sock;
} BINDING;

static void
add_binding (char *hostname, char *portname, BINDING ***bindingsp, int *indexp, int *slotsp)
{
  register int i;
  BINDING *binding, **bindings = *bindingsp;
  int slots = *slotsp, idx = *indexp;
  struct in_addr host_address;
  struct sockaddr_in socket_address;
  int port = 0, sock = -1;
  unsigned char *addr;
  int one = 1;

  if (!portname) portname = "80";

  /* Default address is any address on the main interface. */
  host_address.s_addr = htonl (INADDR_ANY);

  /* If this host and port are not already bound, then bind them now. */

  /* Check for special string of "_default_".  In that case, the server
     is loosely bound. */
  if (strcasecmp (hostname, "_default_") == 0)
    addr = (unsigned char *)htonl (INADDR_ANY);
  else
    {
      addr = hostname_or_ip_to_address (hostname);

      if (!addr)
	fatal ("Cannot find host address: %s:%s", hostname, portname);
    }
  port = atoi (portname);
  if (port < 1) fatal ("Unusable port: %s:%s", hostname, portname);

  /* Set up the socket address. */
  if (addr)
    memcpy (&host_address, addr, sizeof (host_address));
  memset (&socket_address, 0, sizeof (socket_address));
  socket_address.sin_addr = host_address;
  socket_address.sin_family = AF_INET;
  socket_address.sin_port = htons ((short) port);

  /* See if we already have an entry for this host and port.  If so, then
     it is a virtual host (multiple names per IP) and so we don't have to
     bind it. */
  for (i = 0; i < idx; i++)
    {
      BINDING *b = bindings[i];
      if ((memcmp (&(b->host_address),&host_address,sizeof(host_address)) == 0)
	  && (port == b->port))
	return;
    }

  /* Set up the socket. */
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0) fatal ("Cannot create socket!: %s:%s", hostname, portname);
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof (one));

  /* Bind the socket to our address. */
  if (bind (sock, (struct sockaddr *)&socket_address,
	    sizeof (socket_address)) == -1)
    fatal ("Cannot bind socket for %s:%s", hostname, portname);

  binding = (BINDING *)xmalloc (sizeof (BINDING));
  binding->hostname = strdup (hostname);
  binding->portname = strdup (portname);
  binding->host_address = host_address;
  binding->socket_address = socket_address;
  binding->port = port;
  binding->sock = sock;

  if ((idx + 2) > slots)
    bindings = (BINDING **)
      xrealloc (bindings, (slots += 10) * sizeof (BINDING *));
  bindings[idx++] = binding;
  bindings[idx] = (BINDING *)NULL;
  *bindingsp = bindings;
  *indexp = idx;
  *slotsp = slots;
}

static void
loop_over_connections (void)
{
  register int i;
  static BINDING **bindings = (BINDING **)NULL;
  static int b_index = 0;
  static int b_slots = 0;
  int high_fd = -1;
  int sock_size = sizeof (struct sockaddr_in);

  if (pagefunc_get_variable ("mhttpd::debug-running-server") != (char *)NULL)
    {
      int hangon = 1;

      while (hangon);
    }

  if (!debug_mode && !once_and_exit)
    {
      char *hostname, *portname;
      Package *vh = symbol_lookup_package ("mhttpd-virtual-host");
      Symbol **packsyms = symbols_of_package (vh);

      /* Create a list of host/ports to listen on.
	 Start with top-level default hosts. */
      hostname = pagefunc_get_variable ("mhtml::server-name");

      if (!empty_string_p (hostname))
	{
	  portname = pagefunc_get_variable ("mhtml::server-port");
	  if (empty_string_p (portname)) portname = strdup ("80");
	  add_binding (hostname, portname, &bindings, &b_index, &b_slots);
	}

      /* Now do virtual hosts, both multi-hosted IP, and real host to IP. */
      if (packsyms != (Symbol **)NULL)
	{
	  Symbol *sym;

	  for (i = 0; (sym = packsyms[i]) != (Symbol *)NULL; i++)
	    if ((sym->values_index != 0) && (sym->type == symtype_STRING))
	      {
		Package *p = alist_to_package (sym->values[0]);

		/* We do the binding if the user hasn't inhibited us by
		   specifiying NoBind in the VirtualHost specification. */
		if (symbol_lookup_in_package (p, "nobind") == (Symbol *)NULL)
		  {
		    hostname = pkg_get_var (p, "server-name");
		    portname = pkg_get_var (p, "server-port");
		    add_binding (hostname, portname,
				 &bindings, &b_index, &b_slots);
		    symbol_destroy_package (p);
		  }
	      }
	  free (packsyms);
	}

      if (servers_to_fork)
	for (i = 0; i < servers_to_fork; i++)
	  if (fork ())
	    break;

      /* Listen on all of the sockets that we have defined.  Remember the
	 highest socket number. */
      for (i = 0; i < b_index; i++)
	{
	  if (bindings[i]->sock > high_fd) high_fd = bindings[i]->sock;
	  listen (bindings[i]->sock, 512);
	}
    }

  while (1)
    {
      int connection = -1;
      pid_t child = (pid_t)0;
      int the_binding = -1;

      if (!debug_mode && !once_and_exit)
	{
	  struct timeval nohang = { 0, 0 };
	  fd_set read_fds;

	  while (1)
	    {
	      int ready;

	      the_binding = -1;
	      FD_ZERO (&read_fds);

	      for (i = 0; i < b_index; i++)
		FD_SET (bindings[i]->sock, &read_fds);

	      ready = select (high_fd + 1, &read_fds,
			      (fd_set *)0, (fd_set *)0, &nohang);
	      check_child_timeouts ();

	      if (ready < 1)
		{
		  struct timeval period = { 60, 0 };

		  FD_ZERO (&read_fds);

		  for (i = 0; i < b_index; i++)
		    FD_SET (bindings[i]->sock, &read_fds);

		  ready = select (high_fd + 1, &read_fds,
				  (fd_set *)0, (fd_set *)0, &period);
		}

	      if (ready > 0)
		{
		  the_binding = -1;

		  for (i = 0; i < b_index; i++)
		    if (FD_ISSET (bindings[i]->sock, &read_fds))
		      {
			the_binding = i;
			break;
		      }

		  if (the_binding != -1)
		    break;
		}
	    }

	  if (FD_ISSET (bindings[the_binding]->sock, &read_fds))
	    {
	      connection =
		accept (bindings[the_binding]->sock,
			&(bindings[the_binding]->socket_address),
			&sock_size);
	    }

	  if (connection < 0)
	    continue;

	  mhttpd_log (log_DEBUG, "Connected: fd = %d", connection);

	  /* We now have a file descriptor on which we may do transactions.
	     Fork a child to do the transaction, and wait for it to die.
	     If the child doesn't die in the time allotted, kill it, and
	     return an error code. */
	  child = fork ();
	}
      else
	{
	  child = 0;
	}

      /* What to do when we are in the parent. */
      if (child != (pid_t)0)
	{
	  /* Add this child to our list. */
	  add_child (child);

	  /* Close the open file descriptor in the parent. */
	  if (!debug_mode)
	    close (connection);
	}
      else
	{
	  /* In the child, handle a single request. */
	  HTTP_REQUEST *req = (HTTP_REQUEST *)NULL;
	  HTTP_RESULT *result = (HTTP_RESULT *)NULL;
	  int kept_alive = 0;
	  int know_my_client = 0;

	  signal (SIGCHLD, SIG_DFL);
	  signal (SIGHUP,  SIG_IGN);

	  if (!debug_mode && !once_and_exit)
	    {
	      /* If this connection is SSL, negotiate the security now. */
	      if (mhttpd_ssl_server)
		mhttpd_negotiate_ssl (connection);

		/* Set the user permissions of this server if specified. */
		{
#if defined (HAVE_GETPWNAM)
		  char *user = pagefunc_get_variable ("mhtml::default-user");
		  int group_set = 0;

#if defined (HAVE_GETGRNAM)
		  char *group = pagefunc_get_variable ("mhtml::default-group");

		  if (!empty_string_p (group))
		    {
		      struct group *entry = getgrnam (group);

		      if (entry != (group *)NULL)
			{
			  group_set++;
			  setgid (group->gr_gid);
			}
		    }
#endif /* HAVE_GETGRNAM */

		  if (!empty_string_p (user))
		    {
		      struct passwd *entry = (struct passwd *)getpwnam (user);

		      if (entry)
			{
			  if (!group_set)
			    setgid (entry->pw_gid);

			  setuid (entry->pw_uid);

			  /* On some machines, calling setuid twice is just
			     like calling seteuid.  Well, we'll see. */
#if defined (HAVE_SETEUID)
			  seteuid (entry->pw_uid);
#else
			  setuid (entry->pw_uid);
#endif
			}
		    }
#endif /* HAVE_GETPWNAM */
		}
	    }
	  else
	    connection = fileno (stdin);

#if defined (USE_KEEP_ALIVE)
	get_request:

	  /* Something better show up in the next 60 seconds if we
	     are keeping the connection alive.  I sure do hate Netscape. */
	  if (kept_alive)
	    {
	      fd_set alive;
	      struct timeval hurry_up = { 60, 0 };
	      int hurried_up_p;

	      FD_ZERO (&alive);
	      FD_SET (connection, &alive);
	      hurried_up_p = select
		(1 + connection, &alive, (fd_set *)0, (fd_set *)0, &hurry_up);

	      if (hurried_up_p < 1)
		{
		  mhttpd_log (log_DEBUG, "(keep-alive: Select got %d (%d))",
			      hurried_up_p, errno);
		  goto child_exit;
		}

	      if (!FD_ISSET (connection, &alive))
		{
		  mhttpd_log (log_DEBUG, "(keep-alive: CONNECTION not set)");
		  goto child_exit;
		}

	      mhttpd_log (log_DEBUG, "Keeping connection alive...");
	    }
#endif /* USE_KEEP_ALIVE */

	  /* Okay, read the HTTP request and handle it. */
	  req = http_read_request (connection);

	  mhtml_stdout_fileno = connection;
	  mhtml_stdin_fileno = connection;
	  mhtml_stderr_fileno = connection;

	  if (req != (HTTP_REQUEST *)NULL)
	    {
#if defined (USE_KEEP_ALIVE)
	      if (kept_alive)
		req->flags |= flag_KEEP_ALIVE;
#endif /* USE_KEEP_ALIVE */

	      if (!debug_mode && !know_my_client)
		{
#if defined (MHTTPD_DNS_CACHE)
		  char dns_cached_result[256];
		  int cache_found = 0;
#endif
		  struct sockaddr_in client;

		  if (getpeername
		      (connection,
		       (struct sockaddr *)&client,
		       &sock_size) != -1)
		    {
		      struct hostent *client_info = (struct hostent *)NULL;
		      char *client_info_name = (char *)NULL;
		      long addr =  client.sin_addr.s_addr;
		      const char *x = (const char *)&addr;
		      char *addr_rep = (char *)xmalloc (16);
		      char *get_remote_host =
			pagefunc_get_variable ("mhttpd::get-remote-host");

		      sprintf (addr_rep, "%d.%d.%d.%d",
			       (unsigned char)x[0],
			       (unsigned char)x[1],
			       (unsigned char)x[2],
			       (unsigned char)x[3]);

		      req->requester_addr = addr_rep;

		      if ((get_remote_host == (char *)NULL) ||
			  (strcasecmp (get_remote_host, "true") == 0))
			{
#if defined (MHTTPD_DNS_CACHE)
			  if (mhttpd_cache_dns)
			    {
			      DBFILE db;

			      db = database_open (MHTTPD_DNS_CACHE, DB_READER);

			      if (db != (DBFILE)NULL)
				{
				  DBOBJ key, *val;

				  key.data =
				    ((unsigned char *)req->requester_addr);
				  key.length = 1 + strlen ((char *)key.data);
				  val = database_fetch (db, &key);

				  if (val != (DBOBJ *)NULL)
				    {
				      if (val->data)
					{
					  cache_found++;
					  strncpy
					    (dns_cached_result,
					     (char *)val->data, val->length);
					  dns_cached_result[val->length] ='\0';
					  client_info_name = dns_cached_result;
					  free (val->data);
					}
				      free (val);
				    }
				  database_close (db);
				}
			    }

			  if (client_info_name == (char *)NULL)
#endif /* MHTTPD_DNS_CACHE */
			    {
			      client_info = gethostbyaddr (x, 4, AF_INET);
			      if (client_info)
				client_info_name = (char *)client_info->h_name;
			    }
			}

		      if (client_info_name)
			{
			  req->requester = strdup (client_info_name);

			  /* Some sneaky bastards don't return a real hostname.
			     When that happens, use the address instead of the
			     name. */
			  {
			    register int dots = 0;

			    for (i = 0; req->requester[i] != '\0'; i++)
			      if (req->requester[i] == '.')
				dots++;

			    if (dots < 2)
			      {
				free (req->requester);
				req->requester = strdup (addr_rep);
			      }
			  }
			}
		      else
			req->requester = strdup (addr_rep);

#if defined (MHTTPD_DNS_CACHE)
		      if (mhttpd_cache_dns && !cache_found)
			{
			  /* I don't really want to save the data in this
			     process, but I can't see a relatively inexpensive
			     way to do it otherwise. */
			  DBFILE db;

			  db = database_open (MHTTPD_DNS_CACHE, DB_WRCREAT);
			  if (db != (DBFILE)NULL)
			    {
			      DBOBJ key, val;
			      key.data = (unsigned char *)req->requester_addr;
			      val.data = (unsigned char *)client_info_name;
			      key.length = 1 + strlen ((char *)key.data);
			      val.length = 1 + strlen ((char *)val.data);
			      database_store (db, &key, &val);
			      database_close (db);
			    }
			}
#endif /* MHTTPD_DNS_CACHE */

#if defined (I_UNDERSTAND_IDENT)
		      /* Do the IDENT lookup if that is requested. */
		      if (pagefunc_get_variable ("mhttpd::get-remote-ident"))
			{
			  int fd = -1;

			  mhttpd_log (log_DEBUG, "Trying to get ident from %s",
				      req->requester);

#if defined (NOTDEF)
			  fd = tcp_to_host (req->requester_addr, "113");
#else
			  /* Get a socket on our local machine which is
			     using the same IP address as the server. */
			  {
			    int ident_fd = socket (AF_INET, SOCK_STREAM, 0);
			    struct sockaddr_in local, remote;
			    int one = 1;

			    memset (&local, 0, sizeof (local));
			    memset (&remote, 0, sizeof (remote));
			    memcpy (&local.sin_addr,
				    &(bindings[the_binding]->host_address),
				    sizeof (struct sockaddr_in));
			    remote.sin_addr = client.sin_addr;
			    local.sin_family = AF_INET;
			    remote.sin_family = AF_INET;
			    remote.sin_port = htons (113);

			    if (ident_fd > 0)
			      {
				int error = 0;
				setsockopt (ident_fd, SOL_SOCKET, SO_REUSEADDR,
					    (char *)&one, sizeof (one));

				/* Bind the socket to our address. */
				error = bind
				  (ident_fd,
				   (struct sockaddr *)&local,
				   sizeof (local));
				if (error != -1)
				  {
				    signal (SIGALRM, (sig_t)connect_timed_out);
				    alarm (10);
				    error = connect
				      (ident_fd,
				       (struct sockaddr *)&remote,
				       sizeof (remote));
				    alarm (0);
				    signal (SIGALRM, SIG_DFL);

				    if (error != -1)
				      fd = ident_fd;
				  }
			      }
			  }
#endif
			  if (fd > -1)
			    {
			      char comm[100];
			      char *ident;

#if defined (__WINNT__)
#  define ntohs(x) x
#endif
			      sprintf (comm, "%u , %u\r\n",
				       ntohs (client.sin_port),
				       bindings[the_binding]->port);
			      write (fd, comm, strlen (comm));
			      lseek (fd, 0, SEEK_END);
			      shutdown (fd, 1);

			      if ((read (fd, comm, 99) > 0) &&
				  ((ident = strstr (comm, "USERID :"))
				   != (char *)NULL))
				{
				  for (i = ident - comm; i < 100; i++)
				    if ((comm[i] == '\n') || (comm[i] == '\r'))
				      {
					comm[i] = '\0';
					break;
				      }

				  while ((i--) > 0)
				    if (comm[i] == ':')
				      {
					i++;
					while (comm[i] == ' ' && i < 100) i++;
					break;
				      }
				  pagefunc_set_variable_readonly
				    ("mhtml::remote-ident", comm + i);
				}
			      mhttpd_log (log_DEBUG, "Got %s", comm);
			      shutdown (fd, 2);
			      close (fd);
			    }
			}
#endif /* I_UNDERSTAND_IDENT */
		    }
		  know_my_client = 1;
		}

	      mhttpd_log (log_DEBUG, "Getting Headers");
	      mhttpd_mime_headers_to_package
		(req->headers, "mhttpd-received-headers");
	      pagefunc_set_variable ("mhttpd::method", req->method);
	      pagefunc_set_variable ("mhttpd::protocol", req->protocol);
	      pagefunc_set_variable ("mhttpd::protocol-version",
				     req->protocol_version);
	      pagefunc_set_variable ("mhttpd::location", req->location);
	      pagefunc_set_variable ("mhttpd::requester", req->requester);
	      pagefunc_set_variable ("mhttpd::requester-addr",
				     req->requester_addr);
	      mhttpd_log (log_DEBUG, "Headers Gotten");
	      if (mhttpd_per_request_function)
		{
		  PAGE *perfun = page_create_page ();
		  bprintf (perfun, "<%s>\n", mhttpd_per_request_function);
		  page_process_page (perfun);

		  if (mhttpd_page_redirect_p (perfun))
		    {
		      mhttpd_log (log_DEBUG, "Per Request Func Redirected!");
		      if (!strncasecmp (perfun->buffer, "HTTP/", 5) == 0)
			bprintf_insert (perfun, 0, "HTTP/1.0 302 Found\n");

		      mhttpd_write (connection, perfun->buffer,perfun->bindex);
		      goto exit_now;
		    }

		  if (perfun)
		    page_free_page (perfun);
		}

	      if (mhttpd_debugging)
		{
		  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();

		  bprintf (buffer, "\nGot Request: `%s %s %s/%s' from %s:\n",
			   req->method, req->location,
			   req->protocol, req->protocol_version,
			   req->requester);

		  if (kept_alive)
		    bprintf (buffer, "(Connection was Kept Alive)\n");

		  bprintf (buffer, "Headers Received:\n");

		  for (i = 0; req->headers && req->headers[i]; i++)
		    bprintf (buffer, "   %s: %s\n",
			     req->headers[i]->tag, req->headers[i]->value);
		  mhttpd_log (log_DEBUG, "%s", buffer->buffer);
		  bprintf_free_buffer (buffer);
		}
	    }

	  kept_alive = 0;

#if defined (USE_KEEP_ALIVE)
	  /* Before we handle the request, check for Keep-Alive connections. */
	  if ((req != (HTTP_REQUEST *)NULL) &&
	      (req->headers != (MIME_HEADER **)NULL))
	    {
	      char *conn = mhttpd_get_mime_header (req->headers, "Connection");

	      if ((conn != (char *)NULL) &&
		  (strcasecmp (conn, "Keep-Alive") == 0))
		req->flags |= flag_KEEP_ALIVE;
	    }
#endif /* USE_KEEP_ALIVE */

	  if (req != (HTTP_REQUEST *)NULL)
	    result = http_handle_request (req, connection);

	  if (result && result->page && result->page->buffer)
	    mhttpd_write (connection, result->page->buffer,
			  result->page->bindex);

#if defined (USE_KEEP_ALIVE)
	  if ((result != (HTTP_RESULT *)NULL) &&
	      (result->result_code == res_SUCCESS) &&
	      (result->request != (HTTP_REQUEST *)NULL) &&
	      (result->request->flags & flag_KEEP_ALIVE))
	    {
	      kept_alive = 1;
	      mhttpd_free_request (result->request);
	      mhttpd_free_doc_spec (result->spec);
	      page_free_page (result->page);
	      free (result);
	      goto get_request;
	    }
	child_exit:
#endif /* USE_KEEP_ALIVE */

	exit_now:
	  if (!debug_mode)
	    {
#if defined (SHUTDOWN_2_WORKS)
	      shutdown (connection, 2);
#else
	      struct timeval fin_wait = { 5, 0 };
	      fd_set holdit;
	      char bitbuf[64];

	      FD_ZERO (&holdit);
	      FD_SET (connection, &holdit);

	      shutdown (connection, 1);

	      if (select (connection + 1, &holdit,
			  (fd_set *)NULL, (fd_set *)NULL, &fin_wait) != 0)
		read (connection, bitbuf, sizeof (bitbuf) - 1);
#endif
	      page_process_page_internal (get_after_page_return_buffer ());
	      _exit (0);
	    }
	  else
	    page_process_page_internal (get_after_page_return_buffer ());
	}
    }
}

#if defined (__cplusplus)
}
#endif
