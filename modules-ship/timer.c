/* timer.c: -*- C -*-  DESCRIPTIVE TEXT. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Thu Mar 27 13:33:06 1997.

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

	 http://www.metahtml.com/COPYING  */

#include "modules.h"

#if defined (__cplusplus)
extern "C"
{
#endif

/* 0) #include any files that are specific to your module. */

/* 1) Declare the functions which implement the Meta-HTML functionality. */
static void pf_measuring_elapsed_time (PFunArgs);
static void pf_mtime (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag		     complex? debug_level	   code    */
  { "MEASURING-ELAPSED-TIME",	1,	 0,	pf_measuring_elapsed_time },
  { "MTIME",			0,	 0,	pf_mtime },
  { (char *)NULL,		0,	 0,	(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("timer", ftab)

/* 4) Write the actual code which implements your functionality. */

DOC_SECTION (CODE-PROFILING)
DEFMACRO (pf_measuring_elapsed_time,
	  varname &key units=milliseconds|microseconds|seconds,
"Place the amount of time it takes to execute <var code> into <var varname>.
The value appears as a floating point number which represents the
elapsed time in milliseconds (by default -- use the <var units> keyword
argument to change this).

For example:
<complete-example>
<measuring-elapsed-time timer>
  <set-var sample-code=<get-var x>>
</measuring-elapsed-time>
It took <get-var timer> milliseconds to execute the sample code.
</complete-example>")
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *units = mhtml_evaluate_string (get_value (vars, "units"));
  struct timeval start_time, end_time;
  PAGE *code = page_copy_page (body);

  gettimeofday (&start_time, (struct timezone *)NULL);
  page_process_page_internal (code);
  gettimeofday (&end_time, (struct timezone *)NULL);

  if ((code != (PAGE *)NULL) && (code->bindex != 0))
    {
      bprintf_insert (page, start, "%s", code->buffer);
      start += (code->bindex);

      *newstart = start;
    }

  if (code != (PAGE *)NULL)
    page_free_page (code);

  if (!empty_string_p (varname))
    {
      char rep[100];
      double usecs_start, usecs_end, usecs_diff;
      double divisor = 1000.0;

      if (start_time.tv_sec == end_time.tv_sec)
	start_time.tv_sec = end_time.tv_sec = 0;

      usecs_start = (double)((start_time.tv_sec * 1.0e6) + start_time.tv_usec);
      usecs_end = (double)((end_time.tv_sec * 1.0e6) + end_time.tv_usec);
      usecs_diff = usecs_end - usecs_start;

      if (!empty_string_p (units))
	{
	  if (strcasecmp (units, "milliseconds") == 0)
	    divisor = 1000.0;
	  else if (strcasecmp (units, "microseconds") == 0)
	    divisor = 1.0;
	  else if (strcasecmp (units, "seconds") == 0)
	    divisor = 1.0e6;
	}

      sprintf (rep, "%03.04f", usecs_diff / divisor);
       pagefunc_set_variable (varname, rep);
    }
  xfree (varname);
  xfree (units);
}

DEFUN (pf_mtime, ,
"Returns the number of milliseconds that have elapsed since Jan 1st, 1970,
as a large floating point value.

This can be more convenient to use than <tag measuring-elapsed-time> since
the use of it can be broken up across many include files.  It isn't as
accurate to use this over <tag measuring-elapsed-time> since the overhead
of setting a variable and getting its value is included in your timing:
<complete-example>
<set-var start=<mtime>>
<set-var sample-code=<get-var x>>
<set-var end=<mtime>>
It took <sub end start> milliseconds to execute the sample code.
</complete-example>")
{
  struct timeval now;
  char rep[100];
  double the_time;
  double divisor = 1000.0;

  gettimeofday (&now, (struct timezone *)NULL);
  the_time = (double)((now.tv_sec * 1.0e6) + now.tv_usec);
  sprintf (rep, "%03.04f", the_time / divisor);
  bprintf_insert (page, start, "%s", rep);
  *newstart += strlen (rep);
}


#if defined (__cplusplus)
}
#endif
