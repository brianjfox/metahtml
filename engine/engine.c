/* engine.c: -*- C -*-  Mini-server runs as CGI. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jul 20 17:48:27 1996.  */

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

#include "language.h"
#include "http.h"
#include "globals.h"
#include "logging.h"
#if defined (HAVE_GETPWNAM)
#  include <pwd.h>
#endif

#if defined (__WINNT__)
#include <termios.h>
#endif

#if defined (__cplusplus)
extern "C"
{
#endif


#if defined (BUILDING_WITH_FAST_CGI)
#  include "./fcgi/include/fcgi_stdio.h"
#endif

#if !defined (BUILDING_WITH_FAST_CGI)
#  if defined (_FCGI_STDIO)
#    define BUILDING_WITH_FAST_CGI 1
#  endif
#endif

#if !defined (MHTML_SYSTEM_TYPE)
#  define MHTML_SYSTEM_TYPE "Incorrectly Compiled"
#endif

/* The name of this program, as taken from argv[0]. */
static char *rawprogname = (char *)NULL;

/* The last componenent of rawprogname. */
static char *progname = (char *)NULL;

/* The port number, as taken from SERVER_PORT. */
static int http_port = 80;

/* The host name as taken from SERVER_HOST. */
static char *http_host = (char *)NULL;

/* The full pathname to the configuration file. */
static char *engine_config_path = (char *)NULL;

/* Non-zero means this engine is named "nph-" something. */
static int nph = 0;

#if !defined (MHTML_VERSION_STRING)
static char *mhtml_version_string = "";
#else
static char *mhtml_version_string = MHTML_VERSION_STRING;
#endif

/* Forward declarations. */
static void parse_program_args (int argc, char *argv[]);
static void initialize_engine (void);
static void fatal (char *format, ...);

#if defined (ISPENGINE)
#  undef CHECK_ACTIVATION
#endif

#if defined (CHECK_ACTIVATION)
extern int check_activation_key (char *key);
#endif /* CHECK_ACTIVATION */

static void process_request (void);
static void find_config_file (void);

extern MIME_HEADER **mime_headers_from_string (char *string, int *last_char);

extern char **environ;

int
main (int argc, char *argv[])
{
  char *temp;
#if defined (BUILDING_WITH_FAST_CGI)
  int times_called = -1;
#endif

  if ((argc > 0) && (argv[0] != (char *)NULL) && (argv[0][0] == '/'))
    rawprogname = strdup (argv[0]);
  else if (((temp = (char *)getenv ("SCRIPT_FILENAME")) != (char *)NULL) &&
	   (temp[0] == '/'))
    rawprogname = strdup (temp);
  else
#if defined (ISPENGINE)
    rawprogname = strdup ("mhtml.cgi");
#else
    rawprogname = strdup ("nph-engine");
#endif

  temp = strrchr (rawprogname, '/');
  if (temp != (char *)NULL)
    progname = strdup (temp + 1);
  else
    progname = strdup (rawprogname);

  nph = (strncasecmp (progname, "nph-", 4) == 0);

  pagefunc_set_variable ("mhtml::program-name", progname);

  parse_program_args (argc, argv);
  initialize_engine ();

#if defined (BUILDING_WITH_FAST_CGI)
  while (FCGI_Accept () >= 0)
    {
      pid_t child = (pid_t)0;

      /* We got called again. */
      times_called++;

      /* So start a child to handle the request. */
      child = fork ();

      if (child != (pid_t) 0)
	{
	  int status;

	  waitpid (child, &status, 0);
	}
      else
	{
	  char number[20];
	  sprintf (number, "%d", times_called);
	  pagefunc_set_variable ("fcgi::times-called", number);

	  process_request ();
	  _exit (0);
	}
    }
#else
  process_request ();
#endif /* !BUILDING_WITH_FAST_CGI */

  exit (0);
}

/* How to initialize the Engine. */
static void
initialize_engine (void)
{
  char *temp;
  PAGE *page;
  char *include_prefix = (char *)NULL;

  /* Set up the input and output file descriptors. */
  mhtml_stdout_fileno = fileno (stdout);
  mhtml_stdin_fileno = fileno (stdin);
  mhtml_stderr_fileno = fileno (stderr);

#if defined (__WINNT__)
  setmode (mhtml_stdout_fileno, O_BINARY);
  setmode (mhtml_stdin_fileno, O_BINARY);
#endif /* ! __WINNT__ */

  pagefunc_set_variable ("mhtml::version", mhtml_version_string);
  pagefunc_set_variable ("mhttpd::copyright-string",metahtml_copyright_string);
  pagefunc_set_variable ("mhtml::system-type", MHTML_SYSTEM_TYPE);

  bootstrap_metahtml (0);

  /* Quickly make a package containing the minimum mime-types. */
  pagefunc_set_variable ("mime-type::.mhtml", "metahtml/interpreted");
  pagefunc_set_variable ("mime-type::.jpeg", "image/jpeg");
  pagefunc_set_variable ("mime-type::.jpg", "image/jpeg");
  pagefunc_set_variable ("mime-type::.gif", "image/gif");
  pagefunc_set_variable ("mime-type::.txt", "text/plain");
  pagefunc_set_variable ("mime-type::.mov", "video/quicktime");
  pagefunc_set_variable ("mime-type::.default", "text/plain");
#if defined (ISPENGINE)
  pagefunc_set_variable ("mime-type::.html", "metahtml/interpreted");
  pagefunc_set_variable ("mime-type::.htm", "metahtml/interpreted");
  pagefunc_set_variable ("mhtml::isp-engine?", "true");
#else
  pagefunc_set_variable ("mime-type::.html", "text/html");
  pagefunc_set_variable ("mime-type::.htm", "text/html");
#endif

  /* The minimum startup documents. */
  pagefunc_set_variable ("mhtml::default-filenames[]",
"Welcome.mhtml\nwelcome.mhtml\n\
Index.mhtml\nindex.mhtml\n\
Home.mhtml\nhome.mhtml\n\
Directory.mhtml\ndirectory.mhtml\n\
index.html\nhome.html");

  /* The default extensions for running files through the engine. */
#if defined (ISPENGINE)
  pagefunc_set_variable
    ("mhtml::metahtml-extensions[]", ".mhtml\n.html\n.htm");
#else
  pagefunc_set_variable ("mhtml::metahtml-extensions[]", ".mhtml");
#endif

  /* Default the value of mhtml::include-prefix to the directory above
     the location of this program.  This empirically seems to be the
     right thing -- if the program resides in /www/foo/docs/cgi-bin/nph-engine,
     then the right include-prefix is /www/foo/docs.  So we try. */
  include_prefix = strdup (rawprogname);
  temp = strrchr (include_prefix, '/');
  if (temp != (char *)NULL)
    {
      *temp = '\0';
#if !defined (ISPENGINE)
      temp = strrchr (include_prefix, '/');

      if (temp != (char *)NULL)
	*temp = '\0';
#endif
    }

  pagefunc_set_variable ("mhtml::include-prefix", include_prefix);
  pagefunc_set_variable ("mhtml::document-root", include_prefix);
  free (include_prefix);

  /* Try hard to find a configuration file. */
  find_config_file ();

  if (engine_config_path == (char *)NULL)
    {
      engine_config_path = (char *)getenv ("METAHTML_ENGINE_CONFIG_FILE");
      if (engine_config_path == (char *)NULL)
#if defined (__WINNT__)
	engine_config_path = "/METAHTML/bin/engine.conf";
#else
	engine_config_path = "/www/bin/engine.conf";
#endif /* __WINNT__ */
    }

  page = page_read_template (engine_config_path);

#if defined (FORCE_ENGINE_CONFIG)
  if (!page)
    fatal ("Cannot read configuration file `%s'!", engine_config_path);
#endif

  /* Define a default function for handling a missing page.  This will be
     used unless we find a configuration file. */
  {
    char *x = mhtml_evaluate_string
      ("<defun mhttpd::default-document> <html> <body bgcolor=white> <dump-package mhttpd mhtml env> </body> </html> </defun>");
    xfree (x);
  }

  if (page && page->buffer)
    {
      /* Kill the version of the debugging page function that we defined
	 above. */
      char *x = mhtml_evaluate_string ("<undef mhttpd::default-document>");
      xfree (x);

      /* Now run the engine config file. */
      page_process_page (page);
    }
  else
    {
      char *ignore = mhtml_evaluate_string ("<engine::initialize>");
      xfree (ignore);
    }

  if (page)
    page_free_page (page);

#if defined (CHECK_ACTIVATION)
  {
    char *key = pagefunc_get_variable ("mhtml::activation-key");
    if (check_activation_key (key) == 0)
      {
	if (nph)
	  fprintf (stdout, "HTTP/1.0 200 Found\n");
	fprintf (stdout, "Content-type: text/html\n");
	fprintf (stdout, "<html>\n<body bgcolor=white text=red>\n");
	fprintf (stdout, "<h3>You don't have a valid activation key!</h3>\n");
	fprintf (stdout, "<p><b>Please contact ");
	fprintf (stdout, "<a href=\"mailto:info@metahtml.com\">");
	fprintf (stdout, "info@metahtml.com</a> in order to obtain one.</b>");
	fprintf (stdout, "<p>\n</body>\n</html>\n");
	exit (1);
      }
    symbol_free (symbol_remove ("mhtml::activation-key"));
  }
#endif /* CHECK_ACTIVATION */

  /* If the user didn't set mhtml::require-directories[], give a reasonable
     value here. */
  temp = pagefunc_get_variable ("mhtml::require-directories");
  if (temp == (char *)NULL)
    pagefunc_set_variable ("mhtml::require-directories[]",
".\ntagsets\nmacros\nincludes\n\
..\n../tagsets\n../macros\n../includes\n\
../..\n../../tagsets\n../../macros\n../../includes");

  /* Now set our local variables. */
  temp = (char *)getenv ("SERVER_NAME");

  if (temp != (char *)NULL)
    {
      http_host = strdup (temp);
    }
  else
    {
      char buffer[100];

      strcpy (buffer, "www.nohost.net");
      gethostname (buffer, 100);
      http_host = strdup (buffer);
    }
  pagefunc_set_variable ("mhtml::server-name", http_host);

  temp = (char *)getenv ("SERVER_PORT");
  if (temp == (char *)NULL)
    temp = "80";

  http_port = atoi (temp);
  pagefunc_set_variable ("mhtml::server-port", temp);

  temp = pagefunc_get_variable ("mhtml::document-root");
  if (temp == (char *)NULL)
    temp = pagefunc_get_variable ("mhtml::include-prefix");

  if (temp == (char *)NULL)
    temp = (char *)getenv ("WEBBASEDIR");

  if (temp != (char *)NULL)
    {
      sv_DocumentRoot = strdup (temp);
      pagefunc_set_variable ("mhtml::include-prefix", temp);

      chdir (sv_DocumentRoot);
    }

  mhttpd_per_request_function =
    pagefunc_get_variable ("mhttpd::per-request-function");

  set_session_database_location
    (pagefunc_get_variable ("mhttpd::session-database-file"));

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

  /* Finally, the logfiles. */
  mhttpd_set_logfile (log_ACCESS,  "");
  mhttpd_set_logfile (log_ERROR,   "");
  mhttpd_set_logfile (log_DEBUG,   "");
  mhttpd_set_logfile (log_REFERER, "");
  mhttpd_set_logfile (log_AGENT, "");

  mhttpd_debugging = 0;
  temp = pagefunc_get_variable ("mhtml::debug-log");
  if (temp)
    {
      mhttpd_set_logfile (log_DEBUG, temp);
      mhttpd_debugging = 1;
    }

#if defined (HAVE_GETPWNAM)
  temp = pagefunc_get_variable ("mhtml::default-user");
  if (!empty_string_p (temp))
    {
      struct passwd *entry = (struct passwd *)getpwnam (temp);

      if (entry)
	setuid (entry->pw_uid);
    }
#endif /* HAVE_GETPWNAM */
}

int
mhttpd_check_access (HTTP_RESULT *result)
{
  return (1);
}

static int
file_exists_p (char *path)
{
  struct stat finfo;

  return (stat (path, &finfo) != -1);
}


static char *
find_config_in_dir (char *dir, char *filename)
{  
  char *result = (char *)NULL;
  char path[1024 + 512];
  char *temp;

  /* Try ./engine.conf */
  sprintf (path, "%s/%s", dir, filename);

  if (file_exists_p (path))
    result = strdup (path);
  else
    {
      /* Try ./conf/engine.conf */
      sprintf (path, "%s/conf/%s", dir, filename);
      if (file_exists_p (path))
	result = strdup (path);
      else
	{
	  /* Try ../engine.conf */
	  strcpy (path, dir);
	  temp = strrchr (path, '/');
	  if (temp)
	    {
	      *temp = '\0';
	      sprintf (temp, "/%s", filename);
	      if (file_exists_p (path))
		result = strdup (path);
	      else
		{
		  /* Try ../conf/engine.conf */
		  sprintf (temp, "/conf/%s", filename);
		  if (file_exists_p (path))
		    result = strdup (path);
		}
	    }
	}
    }

  return (result);
}

static char *
find_config_locally (char *dir)
{
  char *result = (char *)NULL;
  char filename[512];
  char *hostname = (char *)getenv ("HTTP_HOST");

  /* If there is a specific host config file, use that first. */
  if ((hostname != (char *)NULL) && (hostname[0] != '\0'))
    {
      sprintf (filename, "%s.conf", hostname);
      result = find_config_in_dir (dir, filename);

      if (result == (char *)NULL)
	{
	  sprintf (filename, "%s-engine.conf", hostname);
	  result = find_config_in_dir (dir, filename);
	}
    }

  if (result == (char *)NULL)
    result = find_config_in_dir (dir, "engine.conf");

  return (result);
}

static void
find_config_file (void)
{
  char dir[1024];
  char *temp;

  if (engine_config_path)
    return;

  /* Try starting with the directory that this program is in. */
  strcpy (dir, rawprogname);
  temp = strrchr (dir, '/');
  if (temp != (char *)NULL)
    {
      *temp = 0;
      engine_config_path = find_config_locally (dir);
    }

  if (engine_config_path == (char *)NULL)
    {
      /* Okay, search in the CWD. */
      temp = getcwd (dir, sizeof (dir));

      if (!temp)
	fatal ("Can't get working directory!");

      engine_config_path = find_config_locally (dir);
    }
}

static void
usage (void)
{
  fprintf (stderr, "Usage: %s --config config-path\n", progname);
  exit (1);
}

static void
parse_program_args (int argc, char *argv[])
{
  int arg_index = 1;

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if ((strcasecmp (arg, "--config") == 0) ||
	  (strcasecmp (arg, "-config") == 0) ||
	  (strcasecmp (arg, "-f") == 0))
	{
	  engine_config_path = strdup (argv[arg_index++]);
	}
      else if ((strcasecmp (arg, "--key") == 0) ||
	       (strcasecmp (arg, "-key") == 0))
	{
	  pagefunc_set_variable ("mhtml::activation-key", argv[arg_index++]);
	}
      else
	{
	  if ((char *)NULL == (char *)getenv ("PATH_INFO")
#if defined (BUILDING_WITH_FAST_CGI)
	      && FCGX_IsCGI()
#endif
	      )
	    usage ();
	}
    }
}

static void
fatal (char *format, ...)
{
  int actual_error = errno;

  va_list args;
  va_start (args, format);

  fprintf (stderr, "%s: ", rawprogname);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  if (actual_error)
    fprintf (stderr, "Error returned in errno: %d: %s\n", actual_error,
	     strerror (actual_error));
  exit (2);
}

static HTTP_REQUEST *
engine_make_request (void)
{
  HTTP_REQUEST *req = (HTTP_REQUEST *)xmalloc (sizeof (HTTP_REQUEST));
  char *temp;

  memset (req, 0, sizeof (HTTP_REQUEST));

  /* Try to get the client's request method. */
  temp = (char *)getenv ("REQUEST_METHOD");
  if (temp == (char *)NULL)
    {
      if (((char *)getenv ("CONTENT_LENGTH")) != (char *)NULL)
	temp = "POST";
      else
	temp = "GET";
    }
  req->method = strdup (temp);

  /* Get the relative URL. */
  {
    char *query_string, *path_info, *url, *prefix;

    path_info = (char *)getenv ("PATH_INFO");
    query_string = (char *)getenv ("QUERY_STRING");

    if (path_info == (char *)NULL)
      path_info = "/";

    prefix = pagefunc_get_variable ("mhtml::include-prefix");

    if ((prefix != (char *)NULL) &&
	(strncmp (path_info, prefix, strlen (prefix)) == 0))
      path_info += strlen (prefix);

    if ((query_string != (char *)NULL) && (*query_string))
      {
	url = (char *)xmalloc (3 + strlen (path_info) + strlen (query_string));
	sprintf (url, "%s?%s", path_info, query_string);
      }
    else
      url = strdup (path_info);

    req->location = url;
  }

  /* Get the protocol and version used by this client. */
  temp = (char *)getenv ("SERVER_PROTOCOL");
  if (temp == (char *)NULL)
    temp = "HTTP/1.0";

  req->protocol = strdup (temp);
  temp = strchr (req->protocol, '/');
  if (temp != (char *)NULL)
    {
      *temp = '\0';
      temp++;
    }
  else
    temp = "1.0";

  req->protocol_version = strdup (temp);

  /* Get the client's address. */
  temp = (char *)getenv ("REMOTE_ADDR");
  if (temp != (char *)NULL)
    req->requester_addr = strdup (temp);
  else
    req->requester_addr = strdup ("127.0.0.1");

  /* Get the client's hostname. */
  temp = (char *)getenv ("REMOTE_HOST");
  if (temp != (char *)NULL)
    req->requester = strdup (temp);
  else
    req->requester = strdup (req->requester_addr);

  temp = (char *)getenv ("REMOTE_USER");
  pagefunc_set_variable ("env::remote_user", temp);
  pagefunc_set_variable ("mhtml::remote-user", temp);

  temp = (char *)getenv ("REMOTE_IDENT");
  pagefunc_set_variable ("env::remote_ident", temp);
  pagefunc_set_variable ("mhtml::remote-ident", temp);

  temp = (char *)getenv ("HTTP_HOST");
  pagefunc_set_variable ("env::http_host", temp);

#if defined (BUILDING_WITH_FAST_CGI)
  /* Now set our local variables. */
  temp = (char *)getenv ("SERVER_NAME");

  if (temp != (char *)NULL)
    {
      http_host = strdup (temp);
    }
  else
    {
      char buffer[100];

      strcpy (buffer, "www.nohost.net");
      gethostname (buffer, 100);
      http_host = strdup (buffer);
    }
  pagefunc_set_variable ("mhtml::server-name", http_host);

  temp = (char *)getenv ("SERVER_PORT");
  if (temp == (char *)NULL)
    temp = "80";

  http_port = atoi (temp);
  pagefunc_set_variable ("mhtml::server-port", temp);

#endif /* BUILDING_WITH_FAST_CGI */

  /* Build a reasonable copy of the Mime headers that we might expect to be
     present. */
  {
    BPRINTF_BUFFER *headers = bprintf_create_buffer ();
    int ignore = 0;

    /* Get all names from the environment starting with HTTP_. */
    if (environ != (char **)NULL)
      {
	register int i;
	char *entry;

	for (i = 0; (entry = environ[i]) != (char *)NULL; i++)
	  {
	    if (strncmp (entry, "HTTP_", 5) == 0)
	      {
		register int j;
		char *header = strdup (entry + 5);

		for (j = 0; header[j] != '\0'; j++)
		  {
		    if (header[j] == '_')
		      header[j] = '-';
		    else if (header[j] == '=')
		      {
			header[j] = ':';
			break;
		      }
		  }
		bprintf (headers, "%s\n", header);
		free (header);
	      }
	  }
      }

    /* Watch out!  The Apache server may have given us a cookie stolen from
       the URL.  In order to make this work the right way, we put that darn
       cookie right back in the URL!  This is because we don't want the
       engine to randomly assume that the browser is cookie compatible,
       especially when it isn't! */
    temp = (char *)getenv ("METAHTML_URL_COOKIE");
    if (temp == (char *)NULL)
      temp = (char *)getenv ("HTTP_METAHTML_URL_COOKIE");
    if (temp == (char *)NULL)
      temp = (char *)getenv ("REDIRECT_METAHTML_URL_COOKIE");
    if (temp != (char *)NULL)
      bprintf (headers, "URL-Cookie: SID=%s\n", temp);

    /* Content-Length */
    temp = (char *)getenv ("CONTENT_LENGTH");
    if (temp != (char *)NULL)
      {
	bprintf (headers, "Content-length: %s\n", temp);
#if (BUILDING_WITH_FAST_CGI)
	/* When we are using the losing OpenMarket FCGI library, there
	   isn't a single clean way to simply pass a file descriptor off
	   to our previously built libraries.  This just totally sucks.

	   If we pass off the socket file descriptor, it is possible that
	   the over-zealous buffering of the Open Market code will have
	   already read data into a buffer, and we won't have access to it.

	   This means that for the case of Fast CGI, we cannot use the
	   existing file descriptor.  Great.  I'll just create yet one
	   more level of indirection here.  Fucking lossage big time. */
  	fast_cgi_content_length = atoi (temp);
	if (fast_cgi_content_length)
	  {
	    fast_cgi_content = (unsigned char *)
	      xmalloc (1 + fast_cgi_content_length);

	    fread (fast_cgi_content, 1, fast_cgi_content_length, stdin);
	  }
#endif /* BUILDING_WITH_FAST_CGI */
      }
	
    /* Content-Type */
    temp = (char *)getenv ("CONTENT_TYPE");
    if (temp != (char *)NULL)
      bprintf (headers, "Content-type: %s\n", temp);

    bprintf (headers, "\n");

    req->headers = mime_headers_from_string (headers->buffer, &ignore);
    bprintf_free_buffer (headers);
  }

  return (req);
}

static void
process_request (void)
{
  /* Handle a single request. */
  int connection = fileno (stdin);
  HTTP_REQUEST *req = (HTTP_REQUEST *)NULL;
  HTTP_RESULT *result = (HTTP_RESULT *)NULL;

  /* Okay, read the HTTP request and handle it. */
  req = engine_make_request ();

  if (mhttpd_debugging != 0)
    mhttpd_debug_request (req);

#if defined (BUILDING_WITH_FAST_CGI)
  mhtml_stdout_fileno = 1;
#endif

  mhttpd_mime_headers_to_package (req->headers, "mhttpd-received-headers");
  pagefunc_set_variable ("mhttpd::method", req->method);
  pagefunc_set_variable ("mhttpd::protocol", req->protocol);
  pagefunc_set_variable ("mhttpd::protocol-version", req->protocol_version);
  pagefunc_set_variable ("mhttpd::location", req->location);
  pagefunc_set_variable ("mhttpd::requester", req->requester);
  pagefunc_set_variable ("mhttpd::requester-addr", req->requester_addr);
  pagefunc_set_variable ("mhttpd::virtual-host",
			 (char *)getenv ("HTTP_HOST"));
  mhtml_evaluate_string ("<subst-in-var mhttpd::virtual-host \":.*$\" \"\">");

  if (req && mhttpd_per_request_function)
    {
      PAGE *perfun = page_create_page ();

      if (mhttpd_debugging != 0)
	mhttpd_log (log_DEBUG, "mhttpd::per-request-function (%s)",
		    mhttpd_per_request_function);

      bprintf (perfun, "<%s>\n", mhttpd_per_request_function);
      page_process_page (perfun);

      if (mhttpd_page_redirect_p (perfun))
	{
	  if (mhttpd_debugging != 0)
	    mhttpd_log (log_DEBUG, "Per request function redirected!");

	  if (nph)
	    {
	      if ((strncasecmp (perfun->buffer, "HTTP/", 5) != 0) &&
		  (strncasecmp (perfun->buffer, "HTTPS/", 6) != 0))
		bprintf_insert (perfun, 0, "%s/%s 302 Found\n",
				req->protocol ? req->protocol : "HTTP",
				req->protocol_version ? req->protocol : "1.0");
	    }

	  fwrite (perfun->buffer, perfun->bindex, 1, stdout);
	  fflush (stdout);
	  return;
	}

      if (perfun)
	page_free_page (perfun);
    }

  if (req != (HTTP_REQUEST *)NULL)
    {
      result = http_handle_request (req, connection);
      mhttpd_free_request (req);
    }

  if (result && result->page && result->page->buffer)
    {
      register int i = 0;

      /* If we are running as parsed header CGI, then remove the
	 HTTP result code line. */
      if (!nph)
	{
	  for (i = 0; i < result->page->bindex; i++)
	    if (result->page->buffer[i] == '\n')
	      {
		i++;
		break;
	      }
	}

      fwrite (result->page->buffer + i, 1, result->page->bindex - i, stdout);
      fflush (stdout);
    }

  page_process_page_internal (get_after_page_return_buffer ());

  if (result)
    {
      if (result->spec)
	mhttpd_free_doc_spec (result->spec);

      if (result->page)
	page_free_page (result->page);

      free (result);
    }
}

#if defined (__cplusplus)
}
#endif
