/* modmath.c: -*- C -*-  Functions for a little bit of Mathematics. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jun 27 11:22:52 1998.

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
#include <math.h>

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_sqrt (PFunArgs);
static void pf_floor (PFunArgs);
static void pf_ceiling (PFunArgs);
static void pf_round (PFunArgs);
static void pf_acos (PFunArgs);
static void pf_asin (PFunArgs);
static void pf_atan (PFunArgs);
static void pf_cos (PFunArgs);
static void pf_sin (PFunArgs);
static void pf_tan (PFunArgs);
static void pf_cosh (PFunArgs);
static void pf_sinh (PFunArgs);
static void pf_tanh (PFunArgs);
static void pf_acosh (PFunArgs);
static void pf_asinh (PFunArgs);
static void pf_atanh (PFunArgs);
static void pf_exp (PFunArgs);
static void pf_log (PFunArgs);
static void pf_log10 (PFunArgs);
static void pf_logb (PFunArgs);
static void pf_raise (PFunArgs);

#if defined (HAVE_CBRT)
static void pf_cbrt (PFunArgs);
#endif

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag	     complex? debug_level	   code    */
  { "SQRT",	0, 0, pf_sqrt },
  { "FLOOR",	0, 0, pf_floor },
  { "CEILING",	0, 0, pf_ceiling },
  { "ROUND",	0, 0, pf_round },
  { "ACOS",	0, 0, pf_acos },
  { "ASIN",	0, 0, pf_asin },
  { "ATAN",	0, 0, pf_atan },
  { "COS",	0, 0, pf_cos },
  { "SIN",	0, 0, pf_sin },
  { "TAN",	0, 0, pf_tan },
  { "COSH",	0, 0, pf_cosh },
  { "SINH",	0, 0, pf_sinh },
  { "TANH",	0, 0, pf_tanh },
  { "ACOSH",	0, 0, pf_acosh },
  { "ASINH",	0, 0, pf_asinh },
  { "ATANH",	0, 0, pf_atanh },
  { "EXP",	0, 0, pf_exp },
  { "LOG",	0, 0, pf_log },
  { "LOG10",	0, 0, pf_log10 },
  { "LOGB",	0, 0, pf_logb },
  { "RAISE",	0, 0, pf_raise },
#if defined (HAVE_CBRT)
  { "CBRT",	0, 0, pf_cbrt },
#endif

  { (char *)NULL, 0, 0,	(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("modmath", ftab)

DOC_SECTION (ARITHMETIC-OPERATORS)

static int
get_double_arg (Package *vars, int which, double *num)
{
  int result = 0;
  char *arg_string = get_positional_arg (vars, which);
  int gc_arg = 0;

  *num = 0.0;

  if (arg_string != (char *)NULL)
    {
      if (!number_p (arg_string))
	{
	  char *temp = mhtml_evaluate_string (arg_string);

	  if (temp != (char *)NULL)
	    {
	      /* Did it change as a result of evaluation? */
	      if (strcmp (arg_string, temp) == 0)
		{
		  register int i;
		  xfree (temp);
		  for (i = 0; whitespace (arg_string[i]); i++);
		  temp = pagefunc_get_variable (arg_string + i);
		  arg_string = temp;
		}
	      else
		{
		  gc_arg++;
		  arg_string = temp;
		}
	    }
	}
    }

  if (arg_string != (char *)NULL)
    {
      double val = 0.0;
      long longval;

      if (float_p (arg_string))
	val = strtod (arg_string, (char **)NULL);
      else
	{
	  longval = strtol (arg_string, (char **)NULL, 0);
	  val = (double)longval;
	}

      *num = val;
      result = 1;

      if (gc_arg) free (arg_string);
    }

  return (result);
}

static void
put_double (PFunArgs, double x)
{
  char rep[256];
  int len, places;
  char *temp = pagefunc_get_variable ("mhtml::decimal-places");

  if (temp)
    places = atoi (temp);
  else
    places = 6;

  sprintf (rep, "%.*f", places, x);
  len = strlen (rep);
  bprintf_insert (page, start, "%s", rep);
  *newstart += len;
}
  
DEFUN (pf_sqrt, x, "Return the sqaure root of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = sqrt (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_floor, x, "Round <var x> downwards to the nearest integer.")
{
  double x;
  if (get_double_arg (vars, 0, &x))
    {
      double ans = floor (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_ceiling, x, "Round <var x> upwards to the nearest integer.")
{
  double x;
  if (get_double_arg (vars, 0, &x))
    {
      double ans = ceil (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_round, x, "Round <var x> to the nearest integer.")
{
  double x;
  if (get_double_arg (vars, 0, &x))
    {
      double ans = rint (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_acos, x, "Return the arc cosine of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = acos (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_asin, x, "Return the arc sin of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = asin (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_atan, x &optional y,
"Return the arc tangent of <var x>.\n\
If <var y> is supplied, then this returns the arg tangent of <var y/x>,\n\
using the signs of both arguments to determine the quadrant of the result.")
{
  double x, y;

  if (get_double_arg (vars, 0, &x))
    {
      if (get_double_arg (vars, 1, &y))
	{
	  double ans = atan2 (x, y);
	  put_double (PassPFunArgs, ans);
	}
      else
	{
	  double ans = atan (x);
	  put_double (PassPFunArgs, ans);
	}
    }
}

DEFUN (pf_cos, x, "Return the cosine of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = cos (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_sin, x, "Return the sin of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = sin (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_tan, x, "Return the tangent of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = tan (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_cosh, x,
"Return the hyperbolic cosine of <var x>,\n\
i.e., (exp (<var x>) + exp (<var -x>)) / 2.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = cosh (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_sinh, x, "Return the hyperbolic sine of <var x>,\n\
i.e. (exp(<var x>) - exp(<var -x>) / 2.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = sinh (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_tanh, x, "Return the hyperbolic tangent of <var x>,\n\
i.e. sinh(<var x>) / cosh(<var x>).")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = tanh (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_acosh, x, "Return the inverse hyperbolic cosine of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = acosh (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_asinh, x, "Return the inverse hyperbolic sine of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = asinh (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_atanh, x, "Return the inverse hyperbolic tangent of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = atanh (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_exp, x,
"Return the value of <i>e</i> (the base of natural logarithms), raised \n\
to the power of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = exp (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_log, x, "Return the natural logarithm of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = log (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_log10, x, "Return the base-10 logarithm of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = log10 (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_logb, x, "Return the base-2 logarithm of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = logb (x);
      put_double (PassPFunArgs, ans);
    }
}

DEFUN (pf_raise, x y, "Returns <var x> raised to the <var y> power.")
{
  double x, y;

  if ((get_double_arg (vars, 0, &x)) &&
      (get_double_arg (vars, 1, &y)))
    {
      double ans = pow (x, y);
      put_double (PassPFunArgs, ans);
    }
}

#if defined (HAVE_CBRT)
DEFUN (pf_cbrt, x, "Return the cube root of <var x>.")
{
  double x;

  if (get_double_arg (vars, 0, &x))
    {
      double ans = cbrt (x);
      put_double (PassPFunArgs, ans);
    }
}
#endif

#if defined (__cplusplus)
}
#endif
