/* compile.c: -*- C -*- */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 2000, E. B. Gamble (ebg@metahtml.com).

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

#include <sys/time.h>		/* gettimeofday() */
#include "compiler/compile.h"
#include "compiler/corex.h"
#include "compiler/parse.h"


/*************************************************************************
 *
 *  COMPILE
 *
 *
 */
extern mh_tag_t
mh_compile_with_intermediates (string_t    source,
			       mh_string_t string,
			       mh_parse_t *parse_result,
			       mh_core_t  *core_exp_result,
			       mh_core_t  *core_opt_result)
{
  string_t   chars = MH_STRING_CHARS (string);
  mh_tag_t   func  = MH_TAG_NULL;
  mh_parse_t parse = NULL;
  mh_core_t  core  = NULL;
  mh_core_t  core_opt = NULL;

  /* Decide what is destructive and where to 'free' */
  mh_memory_gc_disable ();
  
  parse = mh_parse (source, chars);
  if (parse)
    {
      if (parse_result) *parse_result = parse;

      core = mh_expand (source, parse);
      if (core)
	{
	  if (core_exp_result) *core_exp_result = core;

	  mh_optimize ((core_opt = mh_core_dup (core)));
	  if (core_opt)
	    {
	      if (core_opt_result) *core_opt_result = core_opt;

	      func = mh_generate (core_opt);
	    }
	}
    }

  if (parse    && !parse_result)    mh_parse_free (parse);
  if (core     && !core_exp_result) mh_core_free  (core);
  if (core_opt && !core_opt_result) mh_core_free  (core_opt);

  mh_memory_gc_enable ();
  
  return (func);
}

extern mh_tag_t
mh_compile_with_time (string_t        source,
		      mh_string_t     string,
		      struct timeval *tm)
{
  mh_tag_t tag;
  struct timeval start_time;

  gettimeofday (&start_time, NULL);
  
  tag = mh_compile (source, string);

  gettimeofday (tm, NULL);
  timersub (tm, &start_time, tm);

  return (tag);
}

extern mh_tag_t
mh_compile_with_intermediates_and_time (string_t    source,
					mh_string_t string,
					mh_parse_t *parse_result,
					mh_core_t  *core_exp_result,
					mh_core_t  *core_opt_result,
					struct timeval *tm)
{
  mh_tag_t tag;
  struct timeval start_time;

  gettimeofday (&start_time, NULL);
  
  tag = mh_compile_with_intermediates
    (source, string, parse_result, core_exp_result, core_opt_result);

  gettimeofday (tm, NULL);
  timersub (tm, &start_time, tm);

  return (tag);
}
