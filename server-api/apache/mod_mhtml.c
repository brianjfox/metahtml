/* mod_mhtml.c: -*- C -*-  Meta-HTML Apache (1.3) module.

   This file is part of <Meta-HTML>(tm), a system for the rapid
   deployment of Internet and Intranet applications via the use
   of the Meta-HTML language.

   Meta-HTML is copyright (c) 1995, 2003, Brian J. Fox (bfox@ai.mit.edu).
   This module is copyright (c) 2003 Joanna Merecka & Mariusz Zynel
   (mariusz@math.uwb.edu.pl), except portions written by Brian J. Fox,
   which are copyright (c) 1998, 2003.

   Meta-HTML is free software; you can redistribute it and/or modify
   it under the terms of the General Public License as published
   by the Free Software Foundation; either version 1, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   FSF GPL for more details. */

#define HAVE_CONFIG_H
/* # include "limits.h" */
#define NO_REGEX_H 1
#include "../../libmhtml/language.h"
#undef NO_REGEX_H
#include "../../libserver/http.h"
#include "../../libserver/globals.h"
#include "../../libserver/logging.h"
#include "mod_mhtml.h"

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_script.h"

module MODULE_VAR_EXPORT mhtml_module;

#if !defined (MHTML_VERSION_STRING)
#  define MHTML_VERSION_STRING "6.11"
#endif

static char *mhtml_version_string = MHTML_VERSION_STRING;

static MIME_RESOLVER mime_resolvers[] = {
  { "Location",		mr_location, 1 },
  { "Meta-HTML-Engine",	mr_engine, 1 },
  { "Set-cookie",	mr_set_cookie, -1 },
  { "Expires",		mr_expires, 0 },
  { "Last-modified",	mr_last_modified, 0 },
  { "Status",		mr_status, 1 },
  { (char *)NULL,	(GFunc *)NULL, 0 }
};

static void
mhtml_init (server_rec *s, pool *p)
{
  ap_add_version_component ("MetaHTML/" MHTML_VERSION_STRING);
}

static void
mhtml_destroy_all_packages (request_rec *r)
{
  Package *package;
  int i, j;

  if (AllPackages != (Package **)NULL)
    {
      for (j = AP_index; j > -1; j--)
	{
	  if ((package = AllPackages[j]) != (Package *)NULL)
	    {
	      if (package == mhtml_function_package)
		continue;

	      package_pdl_remove (package);

	      /* Now, simply free the package contents. */
	      if (package->name)
		free (package->name);

	      if (package->table != (SymbolTable *)NULL &&
		  package->table->symbol_list != (SymbolList **)NULL)
		{
		  for (i = 0; i < package->table->rows; i++)
		    {
		      if (package->table->symbol_list[i] != (SymbolList *)NULL)
			{
			  SymbolList *list = package->table->symbol_list[i];

			  while (list)
			    {
			      Symbol *sym = list->symbol;
			      SymbolList *thissym = list;
			      list = list->next;

			      symbol_free (sym);
			      free (thissym);
			    }
			}
		    }

		  free (package->table->symbol_list);
		  free (package->table);
		}
	      free (package);
	    }
	}

      free (AllPackages);
      AP_index = 0;
      AP_slots = 0;

      if (mhtml_function_package != (Package *)NULL)
	{
	  AllPackages = (Package **)xrealloc
	    (AllPackages, (AP_slots += 10) * sizeof (Package *));
	  AllPackages[AP_index++] = mhtml_function_package;
	  AllPackages[AP_index] = (Package *)NULL;
	}
      else
	{
	  AllPackages = (Package **)NULL;
	}
    }

  CurrentPackage = (Package *)NULL;
  PageVars = (Package *)NULL;
  mhtml_user_keywords = (Package *)NULL;
  mhtml_flag_newly_interned_symbols = 0;
}

/* Modified initialize_engine () from metahtml/engine/engine.c */

static void
initialize_engine (request_rec *r)
{
  char *temp;
  int i;    
  char buf[10];
  char *include_prefix = (char *)NULL;

  pagefunc_set_variable ("mhtml::program-name", "mod_mhtml");
  pagefunc_set_variable ("mhtml::version", mhtml_version_string);
  pagefunc_set_variable ("mhttpd::copyright-string",metahtml_copyright_string);
  pagefunc_set_variable ("mhtml::system-type", "linux-i386");

  /* MAKE THAT CONFIGURABLE!!! */
  pagefunc_set_variable
    ("mhtml::module-directories[]", "/usr/lib/metahtml");

  bootstrap_metahtml (0);

  /* Quickly make a package containing the minimum mime-types. */
  pagefunc_set_variable ("mime-type::.mhtml", "metahtml/interpreted");
  pagefunc_set_variable ("mime-type::.jpeg", "image/jpeg");
  pagefunc_set_variable ("mime-type::.jpg", "image/jpeg");
  pagefunc_set_variable ("mime-type::.gif", "image/gif");
  pagefunc_set_variable ("mime-type::.txt", "text/plain");
  pagefunc_set_variable ("mime-type::.mov", "video/quicktime");
  pagefunc_set_variable ("mime-type::.default", "text/plain");
  pagefunc_set_variable ("mime-type::.html", "text/html");
  pagefunc_set_variable ("mime-type::.htm", "text/html");

  /* The minimum startup documents. */
  pagefunc_set_variable ("mhtml::default-filenames[]",
			 "Welcome.mhtml\nwelcome.mhtml\n\
Index.mhtml\nindex.mhtml\n\
Home.mhtml\nhome.mhtml\n\
Directory.mhtml\ndirectory.mhtml\n\
index.html\nhome.html");

  /* The default extensions for running files through the engine. */
  pagefunc_set_variable ("mhtml::metahtml-extensions[]", ".mhtml");

  /* Default the value of mhtml::include-prefix to the directory above
     the location of this program.  This empirically seems to be the
     right thing -- if the program resides in /www/foo/docs/cgi-bin/nph-engine,
     then the right include-prefix is /www/foo/docs.  So we try. */
  include_prefix = (char *)ap_document_root (r);
  pagefunc_set_variable ("mhtml::include-prefix", include_prefix);
  pagefunc_set_variable ("mhtml::document-root", include_prefix);

  /* MAKE THAT CONFIGURABLE!!! */
  pagefunc_set_variable ("mhtml::~directory", "public_html");
    
  /* Even if there is something to setup in a configuration file, this
     should rather be done once in initialization of the module - here
     it's a pure waste of time here. So, the code to read it is dropped. */
    
  /* Define a default function for handling a missing page.  This will be
     used unless we find a configuration file. */
  {
    char *x = mhtml_evaluate_string
      ("<defun mhttpd::default-document> <html> <body bgcolor=white> <dump-package mhttpd mhtml env> </body> </html> </defun>");
    xfree (x);
  }

  /* If the user didn't set mhtml::require-directories[], give a reasonable
     value here. */
  temp = pagefunc_get_variable ("mhtml::require-directories");
  if (temp == (char *)NULL)
    pagefunc_set_variable ("mhtml::require-directories[]",
			   ".\ntagsets\nmacros\ninclude\n\
..\n../tagsets\n../macros\n../include\n\
../..\n../../tagsets\n../../macros\n../../include");

  /* Now set our local variables. */
  pagefunc_set_variable ("mhtml::server-name", (char *)ap_get_server_name (r));

  i = ap_get_server_port (r);
  snprintf (buf, 10, "%d", i);
  pagefunc_set_variable ("mhtml::server-port", buf);

  sv_DocumentRoot = strdup (ap_document_root (r));
  chdir (sv_DocumentRoot);

  mhttpd_per_request_function =
    pagefunc_get_variable ("mhttpd::per-request-function");

  set_session_database_location
    (pagefunc_get_variable ("mhttpd::session-database-file"));

  /* Create a reasonable default PATH variable if the user didn't do so. */
  if (pagefunc_get_variable ("mhtml::exec-path") == (char *)NULL)
    {
      pagefunc_set_variable ("mhtml::exec-path", "/bin:/usr/bin:/usr/local/bin:/usr/lib/metahtml/bin:/opt/metahtml/bin");
    }
}

/* sapi_apache_read_post() from php/sapi/apache/mod-php4.c.
   Call ap_get-client () as many times as required to read
   count_bytes characters. */
static int
apache_read_post (request_rec *r, char *buffer, int count_bytes)
{
  uint total_read_bytes = 0, read_bytes;
  void (*handler) (int);

  handler = ap_signal (SIGPIPE, SIG_IGN);

  while (total_read_bytes < count_bytes)
    {
      /* Start timeout timer. */
      ap_hard_timeout ("Read POST data", r);
      read_bytes = ap_get_client_block (r, buffer + total_read_bytes,
					count_bytes - total_read_bytes);
      ap_reset_timeout (r);

      if (read_bytes <= 0)
	{
	  break;
	}

      total_read_bytes += read_bytes;
    }

  ap_signal (SIGPIPE, handler);

  return (total_read_bytes);
}

/* Modified mhtml_read_content_from_fd() from metahtml/libserver/http.c 
   Read POST data from the client. Content-Length is taken from headers
   sent by the client. */

static void
mhtml_read_content_from_ap (HTTP_RESULT *result, request_rec *r)
{
  char *content_length =
    (char *)ap_table_get (r->headers_in, "Content-Length");

  if (content_length && ap_should_client_block (r))
    {
      result->spec->content_length = atoi (content_length);

      /* I just hate the whole world.  Why O Why is there an extra
	 CR/LF at the end of this fucking post that doesn't show up
	 in the Content-Length? */
      result->spec->content = (char *)xmalloc
	(3 + result->spec->content_length);

      apache_read_post (r, result->spec->content,
			result->spec->content_length);

      result->spec->content[result->spec->content_length] = '\0';
    }
}

/* Decode the username from Authorization header sent by the client. */
static void
mhtml_set_remote_user (HTTP_REQUEST *req, request_rec *r)
{
  int i;
  char *decoded, *username, *password;
  char *auth = (char *)ap_table_get (r->headers_in, "Authorization");
    
  if (auth != (char *)NULL)
    {
      /* Skip past "Basic", and any following whitespace. */
      for (i = 5; whitespace (auth[i]); i++);

      /* Get the decoded string. */
      decoded = mhtml_base64decode (auth + i, (int *)NULL);

      /* Check the authorization string here. */
      username = decoded;
      password = strchr (username, ':');

      if (password != (char *)NULL)
	{
	  *password = '\0';
	  password++;
	}

      pagefunc_set_variable_readonly ("env::remote_user", username);
      pagefunc_set_variable_readonly ("mhtml::remote-user", username);
    }
}

/* Strips off headers from resulting page if any and puts them into Apache's 
   headers_out table to send back to client. */
static void
mhtml_handle_headers (HTTP_RESULT *result, request_rec *r)
{
  register int i;
  MIME_HEADER **present = (MIME_HEADER **)NULL;
  int end_of_headers;

  /* If this page already has an HTTP result line, snarf the result code. */
  if (result->page && result->page->buffer &&      
      strncasecmp (result->page->buffer, "HTTP", 4) == 0)
    {
      char *buffer = result->page->buffer;
      for (i = 0; !whitespace (buffer[i]); i++);
      result->result_code = atoi (buffer + i);
      for (; buffer[i] != '\0' && buffer[i] != '\n'; i++);
      if (buffer[i] == '\n') i++;
      bprintf_delete_range (result->page, 0, i);
    }

  if (!result->page)
    result->page = page_create_page ();

  present = mime_headers_from_string (result->page->buffer, &end_of_headers);

  if (end_of_headers)
    bprintf_delete_range (result->page, 0, end_of_headers);

  /* Next, make sure that the required headers are there. */
  for (i = 0; mime_resolvers[i].mime_name != (char *)NULL; i++)
    {
      int val_initially_present = 1;
      char *orig_value =
	mhttpd_get_mime_header (present, mime_resolvers[i].mime_name);
      char *value = orig_value;

      if (value == (char *)NULL)
	{
	  val_initially_present = 0;
	  value = (*mime_resolvers[i].value_generator) (result, (char *)NULL);
	}
      else if (mime_resolvers[i].call_anyway)
	{
	  value = (*mime_resolvers[i].value_generator) (result, value);
	}

      if (value)
	ap_table_set(r->headers_out, mime_resolvers[i].mime_name, value);
        
      if (val_initially_present && (mime_resolvers[i].call_anyway > -1))
	*orig_value = '\0';
    }

  /* Now, insert all of the headers that were present, but that were not
     required. */
  for (i = 0; present && present[i]; i++)
    if ((present[i]->value != (char *)NULL) && (present[i]->value[0] != '\0'))
      ap_table_set (r->headers_out, present[i]->tag, present[i]->value);

  for (i = 0; present && present[i]; i++)
    {
      if (present[i]->tag) free(present[i]->tag);
      if (present[i]->value) free(present[i]->value);
      if (present[i]) free(present[i]);
    }

  if (present) free (present);
}

static const char *
set_mhtml_module_directories (cmd_parms *cmd, void* empty, char* text)
{
  /* If we wanted to return an error code, we would return a string.
     This would prevent Apache from running. */
  return ((const char *)NULL);
}

static int
mhtml_handle_req (request_rec *r)
{
  HTTP_REQUEST *req = (HTTP_REQUEST *)NULL;
  HTTP_RESULT *result = (HTTP_RESULT *)NULL;
  MIME_HEADER *header;
  array_header *hdrs_arr = ap_table_elts (r->headers_in);
  table_entry *hdrs = (table_entry *) hdrs_arr->elts;
  int hdrs_index = 0;
  int hdrs_slots = 0;
  int retval, i;
  char buf[256];

  if (r->method_number == M_OPTIONS)
    {
      /* We only support GET and POST methods. */
      r->allowed |= (1 << M_GET);
      r->allowed |= (1 << M_POST);
      return (DECLINED);
    }
    
  if (r->method_number != M_GET && r->method_number != M_POST)
    {
      fprintf (stderr, "Bad Method!\n");
      return (METHOD_NOT_ALLOWED);
    }

  /* Before we initialize the engine, check to see if there is POST data to 
     read. If it is chunked transfer-coding reject it, so the policy 
     is REQUEST_CHUNKED_ERROR. */
  if ((retval = ap_setup_client_block (r, REQUEST_CHUNKED_ERROR)))
    return (retval);

  /* We need to initialize req data structure and set method, protocol, 
     protocol_version and location adequately.
     Content-Length header will be necessary later to read POST data. */
  req = http_parse_request (r->the_request);

  if (req != (HTTP_REQUEST *)NULL)
    {
      char *t = (char *)r->connection->remote_host;

      if (t != (char *)NULL) t = strdup (t);
      req->requester = t;

      t = (char *)r->connection->remote_ip;
      if (t != (char *)NULL) t = strdup (t);
      req->requester_addr = t;

      /* Additional CGI environment variables can be put 
	 into r->subprocess_env table by calling:
	 ap_add_common_vars(r);
	 ap_add_cgi_vars(r);
      */

      /* Copy all the headers from Apache to MHTML. */
      for (i = 0; i < hdrs_arr->nelts; ++i)
	{
	  if (!hdrs[i].key)
	    {
	      continue;
	    }

	  header = (MIME_HEADER *)xmalloc (sizeof (MIME_HEADER));

	  fprintf (stderr, "\n\t%s: %s", hdrs[i].key, hdrs[i].val? hdrs[i].val : "(null)");
	  header->tag = strdup (hdrs[i].key);
	  header->value = strdup (hdrs[i].val);

	  if (hdrs_index + 2 > hdrs_slots)
	    req->headers = (MIME_HEADER **)xrealloc
	      (req->headers, (hdrs_slots += 10) * sizeof(MIME_HEADER *));

	  req->headers[hdrs_index++] = header;
	  req->headers[hdrs_index] = (MIME_HEADER *)NULL;

#if 0
	  /* Headers that come in the request are stored in a
	     special package. */

	  /* This is already done with mhttpd_mime_headers_to_package... */
	  snprintf (buf, 255, "req::%s", header->tag);
	  pagefunc_set_variable (buf, header->value);
#endif
        }
        
      /* Now it's time to initialize MetaHTML engine. */
      initialize_engine (r);

      /* Set required variables, mostly for compatibility reasons */
      mhttpd_mime_headers_to_package (req->headers, "mhttpd-received-headers");
      pagefunc_set_variable ("mhttpd::method", req->method);
      pagefunc_set_variable ("mhttpd::protocol", req->protocol);
      pagefunc_set_variable ("mhttpd::protocol-version", req->protocol_version);
      pagefunc_set_variable ("mhttpd::location", req->location);
      pagefunc_set_variable ("mhttpd::requester", req->requester);
      pagefunc_set_variable ("mhttpd::requester-addr", req->requester_addr);

      /* Where the two are set in MetaHTML engine? Better set it here. */
      pagefunc_set_variable_readonly
	("env::server_protocol", req->protocol);
      pagefunc_set_variable_readonly
	("env::protocol_version", req->protocol_version);

      /* Set mhtml::remote-user variable */
      mhtml_set_remote_user (req, r);

      /* Handle the request using MetaHTML engine. */
      result = mhtml_make_result ();

      if (result != (HTTP_RESULT *)NULL)
	{
	  result->request = req;
	  result->spec = mhttpd_resolve_location (req);

	  /* Read in POST data */
	  mhtml_read_content_from_ap (result, r);

	  /* Process the request and return resulting page */
	  mhttpd_metahtml_engine (result);

	  if (pagefunc_get_variable
	      ("mhtml::server-pushed") == (char *)NULL)
	    mhtml_handle_headers (result, r); 

	  mhttpd_free_request (req);

	  /* If everything went well send the result back to the client. */
	  if (result->result_code == res_SUCCESS &&
	      result && result->page && result->page->buffer)
	    {
	      r->content_type = "text/html";
	      ap_set_content_length (r, result->page->bindex);

	      /* First send headers to the client */
	      ap_send_http_header (r);
                
	      /* Now send the resulting page */
	      if (!r->header_only)
		{
		  ap_rwrite (result->page->buffer, result->page->bindex, r);
		  ap_rflush (r);
                }
            }
        }
    }
    
  /* Report the result code to Apache core. */
  fprintf (stderr, "r->status = result->result_code; --> ");
  r->status = result->result_code;
  fprintf (stderr, "done.\n");

  /* Destroy result structure, */
  if (result)
    {
      if (result->spec)
	{
	  fprintf (stderr, "mhttpd_free_doc_spec (result->spec) --> ");
	  mhttpd_free_doc_spec (result->spec);
	  fprintf (stderr, "done.\n");
	}

      if (result->page)
	page_free_page (result->page);

      free (result);
    }

  /* Cleanup variables and user macros. */
  fprintf (stderr, "mhtml_destroy_all_packages (r); --> ");
  mhtml_destroy_all_packages (r);
  fprintf (stderr, "done.\n");

  /* Cleanup debugging and error messages */
  page_debug_clear ();
  page_syserr_clear ();
   
  ap_kill_timeout (r);

  if (r->status == res_MOVED_TEMPORARILY)
    return (REDIRECT);
  else
    return (OK);
}

command_rec mhtml_cmds[] =
  {
    { "MHTMLModuleDirectories", set_mhtml_module_directories, NULL, RSRC_CONF, RAW_ARGS, "Set MHTML module directories" },
    { "MetaHTMLModuleDirectories", set_mhtml_module_directories, NULL, RSRC_CONF, RAW_ARGS, "Set MHTML module directories" },
    { NULL }
  };

static const handler_rec mhtml_handlers[] =
  {
    { "metahtml/interpreted", mhtml_handle_req },
    { NULL }
  };


module MODULE_VAR_EXPORT mhtml_module =
  {
    STANDARD_MODULE_STUFF,
    mhtml_init,                     /* module initializer */
    NULL,                           /* per-directory config creator */
    NULL,                           /* dir config merger */
    NULL,                           /* server config creator */
    NULL,                           /* server config merger */
    mhtml_cmds,                     /* command table */
    mhtml_handlers,                 /* [9] list of handlers */
    NULL,                           /* [2] filename-to-URI translation */
    NULL,                           /* [5] check/validate user_id */
    NULL,                           /* [6] check user_id is valid *here* */
    NULL,                           /* [4] check access by host address */
    NULL,                           /* [7] MIME type checker/setter */
    NULL,                           /* [8] fixups */
    NULL,                           /* [10] logger */
#if MODULE_MAGIC_NUMBER >= 19970103
    NULL,                           /* [3] header parser */
#endif
#if MODULE_MAGIC_NUMBER >= 19970719
    NULL,                           /* process initializer */
#endif
#if MODULE_MAGIC_NUMBER >= 19970728
    NULL,                           /* process exit/cleanup */
#endif
#if MODULE_MAGIC_NUMBER >= 19970902
    NULL                            /* [1] post read_request handling */
#endif
};
