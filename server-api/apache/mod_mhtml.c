/* mod_mhtml.c: -*- C -*-  Apache mini-module code for Meta-HTML. */

/* ====================================================================
 * Copyright (c) 1996 Brian J. Fox
 * Author: Brian J. Fox (bfox@ai.mit.edu) Tue Sep 24 02:51:16 1996.
 *
 * Copyright (c) 1996 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * IT'S CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
    Date: Sat, 28 Sep 1996 09:42:41 -0700
    Message-Id: <199609281642.JAA29427@nirvana.datawave.net>
    From: "Brian J. Fox" <bfox@datawave.net>
    To: metahtml-wizards@metahtml.com
    Subject: Apache mini-module installation.

    Put this in "src/mod_mhtml.c", and then edit "src/Configuration",
    adding

        Module mhtml_module            mod_mhtml.o

    as the last line in the file.  Then do:

        cd src; ./Configure; make

    Then the only thing needed in the "srm.conf" file is:

	AddType metahtml/interpreted .mhtml
	Action metahtml/interpreted /cgi-bin/nph-engine

    Brian */

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_log.h"
#include "http_main.h"
#include "util_script.h"

/* If the URI starts with a string of digits, leave them there, but
   make the filename be everything after it. */
static int
translate_filename (struct request_rec *req)
{
  register int i;
  int result = DECLINED;

  if ((req->uri != (char *)NULL) &&
      ((req->uri[0] == '/') && (isdigit (req->uri[1]))))
    {
      for (i = 1; isdigit (req->uri[i]); i++);


      /* If the URI contains a cookie, then gobble it, stuff it into
	 req->subprocess_env, and move along. */
      if (req->uri[i] == '\0' || req->uri[i] == '/')
	{
	  register int j;
	  char *value = (char *)ap_palloc (req->pool, 1 + i);

	  strncpy (value, req->uri + 1, i - 1);
	  value[i - 1] = '\0';

	  for (j = 0; req->uri[i] != '\0'; j++, i++)
	    req->uri[j] = req->uri[i];

	  req->uri[j] = '\0';

	  /* Now stuff the value into a meaningful environment variable. */
	  ap_table_set (req->subprocess_env, "METAHTML_URL_COOKIE", value);
	}
    }

  return (result);
}

module mhtml_module =
{
   STANDARD_MODULE_STUFF,
   NULL,			/* initializer */
   NULL,			/* dir config creater */
   NULL,			/* dir merger --- default is to override */
   NULL,			/* server config */
   NULL,			/* merge server config */
   NULL,			/* command table */
   NULL,			/* handlers */
   translate_filename,		/* filename translation */
   NULL,			/* check_user_id */
   NULL,			/* check auth */
   NULL,			/* check access */
   NULL,			/* type_checker */
   NULL,			/* fixups */
   NULL				/* logger */
};
