/* compile.h: -*- C -*- */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1996, 2000, E. B. Gamble (ebg@metahtml.com).

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

#if !defined (_MH_COMPILE_H_)
#define _MH_COMPILE_H_ 1

#include <sys/time.h>		/* gettimeofday(), struct timeval */

#include "machine/machine.h"	/* For struct mh_parse_t, mh_core_t... */

#define MH_PARSE_EMPTY         ((mh_parse_t) NULL)

extern void mh_parse_show (mh_parse_t parse);

extern void mh_parse_free_special (mh_parse_t parse, boolean_t  deep_p,
				   boolean_t  token_p, boolean_t  parse_p);

static inline void
mh_parse_free (mh_parse_t parse)
{
  mh_parse_free_special (parse, true, true, true);
}

#define MH_CORE_NULL ((mh_core_t) NULL)

extern void mh_core_show (mh_core_t core);

extern void mh_core_free_special (mh_core_t core, boolean_t recurse);

static inline void
mh_core_free (mh_core_t core)
{
  mh_core_free_special (core, true);
}


extern mh_tag_t mh_compile_with_intermediates (string_t source,
					       mh_string_t string,
					       mh_parse_t *parse_result,
					       mh_core_t  *core_exp_result,
					       mh_core_t  *core_opt_result);

extern mh_tag_t mh_compile_with_time (string_t        source,
				      mh_string_t     string,
				      struct timeval *tm);

extern mh_tag_t mh_compile_with_intermediates_and_time
	(string_t source, mh_string_t string, mh_parse_t *parse_result,
	 mh_core_t *core_exp_result, mh_core_t *core_opt_result,
	 struct timeval *tm);

static inline mh_tag_t
mh_compile  (string_t source, mh_string_t string)
{
  return mh_compile_with_intermediates (source, string, NULL, NULL, NULL);
}


/* Compiler Internal Externals */
extern mh_core_t mh_expand   (string_t  parse_source, mh_parse_t parse);
extern void      mh_optimize (mh_core_t core);
extern mh_tag_t  mh_generate (mh_core_t core);

typedef enum
{
  MH_COMP_EXCEPTION_WARN,
  MH_COMP_EXCEPTION_ERROR
} mh_comp_exception_type_t;

extern boolean_t mh_comp_verbose_debugging;

#endif /* ! _MH_COMPILE_H_ */

/* Thu Nov  7 15:19:12 1996.  */

