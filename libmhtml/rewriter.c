/* rewriter.c: -*- C -*-  Rewrite HTML accessors, including <a> and <form>. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sun Jan  5 10:07:50 1997.  */

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

static void pf_anchor (PFunArgs);
static void pf_form (PFunArgs);
static void pf_img (PFunArgs);
static void pf_frame (PFunArgs);
static void pf_base (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc func_table[] =
{
  /*   tag	     complex? debug_level	   code    */
  { "REWRITER::A",	-1,	 0,		pf_anchor },
  { "REWRITER::FORM",	1,	 0,		pf_form },
  { "REWRITER::IMG",	0,	 0,		pf_img },
  { "REWRITER::FRAME",	0,	 0,		pf_frame },
  { "REWRITER::BASE",	0,	 0,		pf_base },
  { (char *)NULL,	0,	 0,		(PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_rewriter_funcs)

static void
rewrite_attribute (Package *vars, char *label)
{
  register int i;
  char **names = get_vars_names (vars);
  char **values= get_vars_vals (vars);

  if (names != (char **)NULL)
    {
      for (i = 0; names[i] != (char *)NULL; i++)
	{
	  if (strcasecmp (names[i], label) == 0)
	    {
	      char *value = mhtml_evaluate_string (values[i]);

	      if (value != (char *)NULL)
		{
		  free (values[i]);
		  values[i] = value;

		  if (*value == '/')
		    {
		      char *rewrite_text;

		      rewrite_text =
			pagefunc_get_variable ("rewriter::rewrite-prefix");
		      if (rewrite_text == (char *)NULL)
			rewrite_text = 
			  pagefunc_get_variable ("mhtml::http-prefix");

		      if (rewrite_text != (char *)NULL)
			{
			  BPRINTF_BUFFER *newval = bprintf_create_buffer ();
			  bprintf (newval, "%s%s", rewrite_text, value);
			  free (values[i]);
			  values[i] = newval->buffer;
			  free (newval);
			}
		    }
		}
	      /* Do all of them, not just the first one seen. */
#if defined (NOTDEF)
	      break;
#endif
	    }
	}
    }
}

/* <a href=... ...> ... </a> */
static void
pf_anchor (PFunArgs)
{
  rewrite_attribute (vars, "HREF");

  bprintf_insert (page, start,
		  "<A %s>%s</A>", mhtml_funargs (vars),
		  body->buffer ? body->buffer : "");
  *newstart = start + 1;
}

/* <form action=... ...> ... </form> */
static void
pf_form (PFunArgs)
{
  rewrite_attribute (vars, "ACTION");

  bprintf_insert (page, start,
		  "<FORM %s>%s</FORM>", mhtml_funargs (vars),
		  body->buffer ? body->buffer : "");
  *newstart = start + 1;
}

/* <img src=...> */
static void
pf_img (PFunArgs)
{
  rewrite_attribute (vars, "SRC");

  bprintf_insert (page, start, "<IMG %s>", mhtml_funargs (vars));
  *newstart = start + 1;
}

/* <frame src=...> */
static void
pf_frame (PFunArgs)
{
  rewrite_attribute (vars, "SRC");

  bprintf_insert (page, start, "<frame %s>", mhtml_funargs (vars));
  *newstart = start + 1;
}

/* <base href=...> */
static void
pf_base (PFunArgs)
{
  rewrite_attribute (vars, "HREF");

  bprintf_insert (page, start, "<base %s>", mhtml_funargs (vars));
  *newstart = start + 1;
}


#if defined (__cplusplus)
}
#endif
