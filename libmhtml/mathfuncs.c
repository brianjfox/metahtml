/* mathfuncs.c: -*- C -*-  Arithmetic functions for Meta-HTML. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Tue Jul 18 17:50:42 1995.  */

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
extern double floor (double x);

#if defined (__cplusplus)
extern "C"
{
#endif

#if defined (MHTML_ARITHMETIC)

static void pf_gt (PFunArgs);
static void pf_ge (PFunArgs);
static void pf_lt (PFunArgs);
static void pf_le (PFunArgs);
static void pf_eq (PFunArgs);
static void pf_neq (PFunArgs);
static void pf_add (PFunArgs);
static void pf_sub (PFunArgs);
static void pf_max (PFunArgs);
static void pf_min (PFunArgs);
static void pf_mul (PFunArgs);
static void pf_div (PFunArgs);
static void pf_mod (PFunArgs);
static void pf_integerp (PFunArgs);
static void pf_integer (PFunArgs);
static void pf_realp (PFunArgs);
static void pf_set_output_radix (PFunArgs);

static PFunDesc func_table[] =
{
  { "GT",		0, 0, pf_gt },
  { "GE",		0, 0, pf_ge },
  { "LT",		0, 0, pf_lt },
  { "LE",		0, 0, pf_le },
  { "EQ",		0, 0, pf_eq },
  { "NEQ",		0, 0, pf_neq },
  { "ADD",		0, 0, pf_add },
  { "SUB",		0, 0, pf_sub },
  { "MAX",		0, 0, pf_max },
  { "MIN",		0, 0, pf_min },
  { "MUL",		0, 0, pf_mul },
  { "DIV",		0, 0, pf_div },
  { "MOD",		0, 0, pf_mod },
  { "INTEGER?",		0, 0, pf_integerp },
  { "INTEGER",		0, 0, pf_integer },
  { "REAL?",		0, 0, pf_realp },
  { "SET-OUTPUT-RADIX", 0, 0, pf_set_output_radix },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_arithmetic_functions)
DEFINE_SECTION (ARITHMETIC-OPERATORS,
		math; addition; subtraction; artithmetic, 
"<Meta-HTML> contains a small set of commands for operating on
numerical quantities.

All of the arithmetic operators described in this section can accept a
number, or simply the name of a variable, which is then looked up as
if it had been written with <example code><get-var-once ...></example>.

That is to say:

<example>
   <set-var total = <add <get-var-once subtotals[0]>
                         <get-var-once subtotals[current]>>>
</example>

can be written as:

<example>
   <set-var total = <add subtotals[0] subtotals[current]>>
</example>

",
"The binary arithmetic operators always expect two arguments, and will
produce a warning message in <funref DEBUGGING-COMMANDS DEBUGGING-OUTPUT>
when given too few arguments.

You can perform floating point arithmetic if one or both of the
arguments is already a floating point number:

<example>
  <div 10 3>    --> 3
  <div 10.0 3>  --> 3.33
</example>
")

DEFVAR (mhtml::decimal-places,
"Controls the number of decimal places which arithmetic functions
produce.  The default value of this variable is <code>2</code>, but
for times when you need greater resolution in your calculations, you
may wish to set it higher.
<complete-example>
<set-var mhtml::decimal-places = 10>
To 10 places: <div 1 .34567>
<unset-var mhtml::decimal-places>
To  2 places: <div 1 .34567>
</complete-example>")



/* Arithmetic operations. */
/* <gt  12 10> --> "true"
   <lt  10 12> --> "true"
   <eq  10 10> --> "true"
   <neq 10 10> --> "false"
   <add 10 10> --> "20"
   <sub 10 10> --> "0"
   <max 10 1>  --> "10"
   <min 10 1>  --> "1"
   <mul 10 10> --> "100"
   <div 12 10> --> "1"
   <mod 12 10> --> "2" */
#define pf_GT	1
#define pf_GE	2
#define pf_LT	3
#define pf_LE	4
#define pf_EQ	5
#define pf_NEQ	6
#define pf_ADD	7
#define pf_SUB	8
#define pf_MAX	9
#define pf_MIN	10
#define pf_MUL	11
#define pf_DIV	12
#define pf_MOD	13

typedef struct { int op; char *name; char *infix_name; } OP_ALIST;
static OP_ALIST op_alist[] = {
  { pf_GT, "GT", ">" },
  { pf_GE, "GE", ">=" },
  { pf_LT, "LT", "<" },
  { pf_LE, "LE", "<=" },
  { pf_EQ, "EQ", "==" },
  { pf_NEQ, "NEQ", "!=" },
  { pf_ADD, "ADD", "+" },
  { pf_SUB, "SUB", "-" },
  { pf_MAX, "MAX", "MAX" },
  { pf_MIN, "MIN", "MIN" },
  { pf_MUL, "MUL", "*" },
  { pf_DIV, "DIV", "/" },
  { pf_MOD, "MOD", "%" },

  { 0, (char *)NULL }
};

static char *
operator_name (int op)
{
  register int i;

  for (i = 0; op_alist[i].name != (char *)NULL; i++)
    if (op == op_alist[i].op)
      return (op_alist[i].name);

  return ("*invalid-op*");
}

static char *
operator_infix_name (int op)
{
  register int i;

  for (i = 0; op_alist[i].name != (char *)NULL; i++)
    if (op == op_alist[i].op)
      return (op_alist[i].infix_name);

  return ("*invalid-op*");
}

#if !defined (macintosh)
/* Fucking SunOS doesn't declare this, result is assumed to be INT. */
extern double strtod (const char *, char **);
#endif

static int mhtml_output_radix = 10;
static char *mhtml_numeric_output_function = (char *)NULL;

int mhtml_get_output_radix (void) { return (mhtml_output_radix); }
void mhtml_set_output_radix (int radix) { mhtml_output_radix = radix; }
char *mhtml_output_to_radix (long num, int radix);

DEFUN (pf_set_output_radix, new-radix,
"Set the output radix for numbers produced by arithmetic operations to
<var new-radix>, which is specified in decimal.  Returns the value of
the previous output radix.  Note that there is no corresponding function
for setting the input radix.  If the value of <var new-radix> is a number
between  2 and 32 decimal, inclusive, then the output radix is set to that
value.  <var new-radix> can also be the name of a function of one argument,
which should be called to produce the appropriate output.

If the argument to <tag set-output-radix> does not fall between 2 and 32
inclusive, and is not the name of a defined function, then the output radix
remains unchanged, and <tag set-output-radix> produces no output.

Examples:
<example>
<set-output-radix 16>                --> 10
<add 0x3e 1>                         --> 0x3f
<set-output-radix 10>                --> 16
<set-output-radix number-to-english> --> 10
<add 23.4 32.32>                     --> Fifty-Five point Seven Two
</example>")
{
  char *new_radix = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int old_radix = mhtml_output_radix;
  int success = 0;

  if (!empty_string_p (new_radix))
    {
      if (integer_p (new_radix, 10) ||
	  ((new_radix[0] == '0') &&
	   ((new_radix[1] == 'x') || (new_radix[1] == 'X'))))
	{
	  long 	longval = strtol (new_radix, (char **)NULL, 0);

	  mhtml_output_radix = (int) longval;

	  if ((mhtml_output_radix < 2) || (mhtml_output_radix > 32))
	    mhtml_output_radix = old_radix;
	  else
	    success = 1;
	}
      else
	{
	  BPRINTF_BUFFER *buf = bprintf_create_buffer ();
	  char *temp;

	  bprintf (buf, "<defined? %s>", new_radix);
	  temp = mhtml_evaluate_string (buf->buffer);
	  if (!empty_string_p (temp))
	    {
	      xfree (mhtml_numeric_output_function);
	      mhtml_numeric_output_function = strdup (new_radix);
	      success = 1;
	      mhtml_output_radix = 0;
	    }

	  xfree (temp);
	  bprintf_free_buffer (buf);
	}
    }

  if (success)
    {
      if ((old_radix == 0) && (mhtml_numeric_output_function != (char *)NULL))
	{
	  bprintf_insert (page, start, "%s", mhtml_numeric_output_function);
	  *newstart += strlen (mhtml_numeric_output_function);
	}
      else
	bprintf_insert (page, start, "%d", old_radix);
    }

  xfree (new_radix);
}

/* Return a pointer to a static buffer containing the string
   representation of NUMBER in RADIX. */
static unsigned long
mod_div (double *x, int base)
{
  unsigned long r = (unsigned long)(*x) % base;
  *x = *x / base;
  return (r);
}

/* This is interesting.  We use doubles to emulate large fixnums.  Why?
   Because it works in most cases, and it's a lot easier. */
char *
mhtml_output_to_radix (long num, int radix)
{
  register int i;
  static char buffer[128];
  static char result[132];
  static char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  char extra[4] = { '\0', '\0', '\0', '\0' };

  memset (buffer, 0, sizeof (buffer));

  if (num < 0) { num = -num; strcat (extra, "-"); }
  if (radix == 16) strcat (extra, "0x");
  if (radix == 8) strcat (extra, "0");

  if (!num)
    buffer[0] = '0';
  else
    {
      register int j;
      double val = (double)num;
      i = strlen (buffer);

      while (val > 1)
	buffer[i++] = digits[mod_div (&val, radix)];

      sprintf (result, "%s", extra);
      j = strlen (result);
      while (--i != -1)
	result[j++] = buffer[i];

      result[j] = '\0';
    }

  return (result);
}

/* Return a string which is the printed representation of a number.
   DOT_PRESENT, if non-zero, says that the number should be printed with
   mhtml_decimal_places decimal places. */
char *
mhtml_double_to_string (double arith_result, int dot_present)
{
  static int orig_mhtml_decimal_notify = 0;
  char result_buff[1024];
  char *result = result_buff;

  result_buff[0] = '\0';

  if (mhtml_decimal_notify != orig_mhtml_decimal_notify)
    {
      orig_mhtml_decimal_notify = mhtml_decimal_notify;

      if (mhtml_decimal_notify)
	{
	  char *temp = pagefunc_get_variable ("mhtml::decimal-places");

	  if (temp)
	    mhtml_decimal_places = atoi (temp);
	  else
	    mhtml_decimal_places = 0;
	}
      else
	mhtml_decimal_places = 2;
    }

  if (mhtml_get_output_radix() == 10)
    {
      if (mhtml_decimal_notify)
	sprintf (result, "%.*f", mhtml_decimal_places, arith_result);
      else if (!dot_present /* || (arith_result == (int)arith_result) */)
	sprintf (result, "%ld", (long int)arith_result);
      else
	sprintf (result, "%.*f", mhtml_decimal_places, arith_result);
    }
  else
    {
      /* The radix is not decimal.  Hope it is something that we can
	 print easily. */

      if ((mhtml_get_output_radix() == 0) &&
	  (mhtml_numeric_output_function != (char *)NULL))
	{
	  BPRINTF_BUFFER *b = bprintf_create_buffer ();
	  char *temp;

	  bprintf (b, "<%s ", mhtml_numeric_output_function);

	  if (mhtml_decimal_notify || dot_present)
	    {
	      char format[128];
	      sprintf (format, "%%.%df", mhtml_decimal_places);
	      bprintf (b, format, arith_result);
	    }
	  else
	    bprintf (b, "%ld", (long int) arith_result);

	  bprintf (b, ">");

	  temp = mhtml_evaluate_string (b->buffer);
	  if (temp != (char *)NULL) make_gcable (temp);
	  bprintf_free_buffer (b);
	  result = temp;
	}
      else
	{
	  sprintf (result, "%s", mhtml_output_to_radix
		   ((long)arith_result, mhtml_output_radix));
	}
    }
  return (result);
}

static char *
arithmetic_operate (int op, char *arg1, char *arg2, int debug_level)
{
  double val1 = 0.0;
  double val2 = 0.0;
  char *result = "";

  {
    long longval;

    if (float_p (arg1))
      val1 = strtod (arg1, (char **)NULL);
    else
      {
	longval = strtol (arg1, (char **)NULL, 0);
	val1 = (double)longval;
      }

    if (float_p (arg2))
      val2 = strtod (arg2, (char **)NULL);
    else
      {
	longval = strtol (arg2, (char **)NULL, 0);
	val2 = (double)longval;
      }
  }

  if (debug_level > 5)
    page_debug ("(%s %s %s)", arg1, operator_infix_name (op), arg2);

  switch (op)
    {
    case pf_GT:
      if (val1 > val2) result = "true";
      break;

    case pf_GE:
      if (val1 >= val2) result = "true";
      break;

    case pf_LT:
      if (val1 < val2) result = "true";
      break;

    case pf_LE:
      if (val1 <= val2) result = "true";
      break;

    case pf_EQ:
      if (val1 == val2) result = "true";
      break;

    case pf_NEQ:
      if (val1 != val2) result = "true";
      break;

    default:
      {
	double arith_result = 0.0;
	int dot_present = ((arg1 ? (strchr (arg1, '.') != (char *)NULL) : 0) ||
			   (arg2 ? (strchr (arg2, '.') != (char *)NULL) : 0));
    
	switch (op)
	  {
	  case pf_ADD:
	    arith_result = val1 + val2;
	    break;

	  case pf_SUB:
	    arith_result = val1 - val2;
	    break;

	  case pf_MAX:
	    if (val1 > val2)
	      arith_result = val1;
	    else
	      arith_result = val2;
	    break;

	  case pf_MIN:
	    if (val1 < val2)
	      arith_result = val1;
	    else
	      arith_result = val2;
	    break;

	  case pf_MUL:
	    arith_result = val1 * val2;
	    break;

	  case pf_DIV:
	    arith_result = val2 ? val1 / val2 : 0.0;
	    break;

	  case pf_MOD:
	    arith_result = val2 ? (double)((int)val1 % (int)val2) : 0.0;
	    break;
	  }

	result = mhtml_double_to_string (arith_result, dot_present);
      }
    }

  return (result);
}

#define relational_op(op) \
	((op == pf_GT) || (op == pf_GE) || (op == pf_LT) || (op == pf_LE) || \
	 (op == pf_EQ) || (op == pf_NEQ))

static void
arithmetic_process (int op, PFunArgs)
{
  register int i, tsize = 0;
  int arg_index = 0;
  char *arg;
  char *value_1 = (char *)NULL;
  char *value_2 = (char *)NULL;
  int value_size = 0;
  char *result = (char *)NULL;

  if (!value_1) value_1 = (char *)xmalloc (value_size = 50);
  if (!value_2) value_2 = (char *)xmalloc (value_size = 50);

  value_1[0] = '\0';
  value_2[0] = '\0';

  /* Our arithmetic processing commands allow any number of arguments.
     The first two are handled in the standard binary fashion, and
     then sucessive arguments are operated on with the running result.
     Each argument may be a numeric value, an expression, or a symbol
     which should resolve to a numeric value. */
  while ((arg = get_positional_arg (vars, arg_index)) != (char *)NULL)
    {
      char *value_pointer = arg;
      char *temp = (char *)NULL;

      arg_index++;

      /* Is this argument already a number? */
      if (!number_p (arg))
	{
	  /* No, so make it one. */
	  temp = mhtml_evaluate_string (arg);
	  value_pointer = temp;

	  /* Did it change as a result of evaluation? */
	  if (strcmp (arg, temp) == 0)
	    {
	      /* No, so try looking it up as a variable name.
		 Only do this when the operation is not pf_EQ
		 or pf_NEQ. */
	      /* if ((op != pf_EQ) && (op != pf_NEQ)) */
		{
		  for (i = 0; whitespace (arg[i]); i++);
		  value_pointer = pagefunc_get_variable (arg + i);
		}
	    }
	  else
	    {
	      /* Yes, it changed.  Is the result an array reference, and
		 did the original not start with a `<' character? */
	      if ((temp != (char *)NULL) &&
		  (strchr (value_pointer, '[') != (char *)NULL) &&
		  (*arg != '<'))
		{
		  /* Yes, look up the value as a variable. Again. */
		  value_pointer = (pagefunc_get_variable (temp));
		}
	    }
	}

      /* Now, value_pointer contains the numeric argument.
	 Copy that value to either value_1 or _value_2 depending on
	 whether we already have a value_1. */
      if (value_pointer == (char *)NULL) value_pointer = "0";

      /* Make sure there is enough room. */
      tsize = strlen (value_pointer);
      if (tsize + 2 > value_size)
	{
	  value_1 = (char *)xrealloc (value_1, 1 + (value_size += tsize));
	  value_2 = (char *)xrealloc (value_2, value_size);
	}

      if (arg_index < 2)
	strcpy (value_1, value_pointer);
      else
	{
	  strcpy (value_2, value_pointer);

	  /* We have two values.  Call the function and get the result. */
	  result = arithmetic_operate (op, value_1, value_2, debug_level);

	  /* Now, if the operator was relational, value_1 gets value_2,
	     iff result has a value. Otherwise, value_1 gets result. */
	  if (relational_op (op))
	    {
	      if (empty_string_p (result))
		{
		  xfree (temp);
		  break;
		}
	      else
		strcpy (value_1, value_2);
	    }
	  else
	    {
	      tsize = strlen (result);
	      if (tsize + 2 > value_size)
		{
		  value_1 = (char *)xrealloc
		    (value_1, 1 + (value_size += tsize));
		  value_2 = (char *)xrealloc (value_2, value_size + 1);
		}
	      strcpy (value_1, result);
	    }
	}

      /* Free possibly allocated variables. */
      xfree (temp);
    }

  /* If there weren't two arguments, then it might be right to complain. */
  if (arg_index < 2)
    {
      if (arg_index < 1)
	page_debug ("<%s ?> seen with no args", operator_name (op));
      else
	page_debug ("<%s %s ?> seen with one arg", operator_name(op), value_1);
    }

  xfree (value_1);
  xfree (value_2);

  if (!empty_string_p (result))
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

DEFUN (pf_gt, arg1 arg2 &rest more-args...,
"Returns \"true\" if the numeric value of <var arg1> is greater than
the numeric value of <var arg2> (which is greater than <var
more-args>.  Just as with the arithmetic functions (<secref
Arithmetic-Operators>), the arguments may be the names of
variables containing numeric values, and not just the values
themselves.  In that case, <code>gt</code> performs an implicit call
to <example code><get-var-once ...></example> in order to get the value to
operate on.

Examples:
<example>
<gt 4 3>            --> true
<set-var x=3.4 y=3.5>
<gt x y>            -->
<gt y x>            --> true
<gt <get-var-once y> x>  --> true

<defun between? item high low>
  <gt high item low>
</defun>

<between? 7 8 6>    --> true
</example>")
{
  arithmetic_process (pf_GT, PassPFunArgs);
}

DEFUN (pf_ge, arg1 arg2 &rest more-args...,
"Returns \"true\" if the numeric value of <var arg1> is greater than
or equal to numeric value of <var arg2> (which is greater than
or equal to <var more-args>.

Just as with the arithmetic functions (<secref Arithmetic-Operators>),
the arguments may be the names of variables containing numeric values,
and not just the values themselves.  In that case, <code>gt</code>
performs an implicit call to <example code><get-var-once ...></example> in
order to get the value to operate on.

Examples:
<example>
<ge 4 3>            --> true
<set-var x=3.4 y=3.5>
<ge x y>            -->
<ge y x>            --> true
<ge <get-var-once y> x>  --> true
<ge 3 2 3.0>        --> true
<ge 3 2 3>          -->
<ge 3 3 2>          --> true
</example>")
{
  arithmetic_process (pf_GE, PassPFunArgs);
}

DEFUN (pf_lt, arg1 arg2 &rest more-args,
"Returns \"true\" if the numeric value of <var arg1> is less than
the numeric value of <var arg2> (which is less than <var more-args>.
Just as with the arithmetic functions (<secref Arithmetic-Operators>),
<var arg1> and <var arg2> may be the names of variables containing
numeric values, and not just the values themselves.  In that case,
<code>lt</code> performs an implicit call to <example code><get-var-once ...></example> in order to get the value to operate on.

Examples:
<example>
<lt 3 4>            --> true
<lt 4 3>            --> 
<set-var x=3.4 y=3.5>
<lt x y>            --> true
<lt y x>            --> 
<lt x <get-var-once y>>  --> true
<lt 4 5 6>          --> true
</example>")
{
  arithmetic_process (pf_LT, PassPFunArgs);
}

DEFUN (pf_le, arg1 arg2 &rest more-args,
"Returns \"true\" if the numeric value of <var arg1> is less than or
equal to the numeric value of <var arg2> (which is less than or equal
to <var more-args>.

Just as with the arithmetic functions (<secref Arithmetic-Operators>),
<var arg1> and <var arg2> may be the names of variables containing
numeric values, and not just the values themselves.  In that case,
<code>lt</code> performs an implicit call to <example code><get-var-once ...></example> in order to get the value to operate on.

Examples:
<example>
<le 3 4>            --> true
<le 4 3>            --> 
<set-var x=3.4 y=3.5>
<le x y>            --> true
<le x 3.4 y>        --> true
<le x y 3.4>        -->
<le 4 5 6>          --> true
</example>")
{
  arithmetic_process (pf_LE, PassPFunArgs);
}

DEFUN (pf_eq, arg1 arg2,
"Returns \"true\" if the numeric value of <var arg1> is exactly equal to
the numeric value of <var arg2>.  Just as with the arithmetic
functions (<secref Arithmetic-Operators>), the arguments
may be the names of variables containing numeric values, and not just
the values themselves.  In that case, <code>eq</code> performs an
implicit call to <example code><get-var-once ...></example> in order to get
the value to operate on.

Examples:
<example>
<eq 3 4>              --> 
<eq 3 3>              --> true
<eq 3.0 3>            --> true
<set-var x=3.01>
<eq <get-var-once x> 3.01> --> true
</example>")
{
  arithmetic_process (pf_EQ, PassPFunArgs);
}

DEFUN (pf_neq, arg1 arg2,
"Returns \"true\" if the numeric value of <var arg1> is NOT equal to
the numeric value of <var arg2>.  Just as with the arithmetic
functions (<secref Arithmetic-Operators>), the arguments
may be the names of variables containing numeric values, and not just
the values themselves.  In that case, <code>eq</code> performs an
implicit call to <example code><get-var-once ...></example> in order to get
the value to operate on.

Examples:
<example>
<neq 3 4>              --> true
<neq 3 3>              --> 
<neq 3.0 3>            --> 
<set-var x=3.01>
<neq <get-var-once x> 3.00> --> true
</example>")
{
  arithmetic_process (pf_NEQ, PassPFunArgs);
}

DEFUN (pf_add, arg1 arg2 &rest more-args,
"Returns the summation of all of the arguments passed.

Examples:
<example>
<add 1 2>        --> 3
<add -1 2 2 1>   --> 4
</example>")
{
  arithmetic_process (pf_ADD, PassPFunArgs);
}

DEFUN (pf_sub, arg1 arg2 &rest more-args,
"Returns the difference of all of the arguments passed.

Examples:
<example>
<sub 2 1>        --> 1
<sub 6 2 1>      --> 3
<sub -1 -2>      --> 1
</example>")
{
  if ((body == (PAGE *)NULL) || (empty_string_p (body->buffer)))
    {
      bprintf_insert (page, start, "<SUB>");
      *newstart += 4;
    }
  else
    {
      arithmetic_process (pf_SUB, PassPFunArgs);
    }
}

DEFUN (pf_max, arg1 arg2 &rest more-args,
"Returns the largets of all of the arguments passed.

Examples:
<example>
<max 2 2>        --> 2
<max 2.3 8 7>    --> 8.00
<max 4 10 -4>    --> 10
</example>")
{
  arithmetic_process (pf_MAX, PassPFunArgs);
}

DEFUN (pf_min, arg1 arg2 &rest more-args,
"Returns the smallest of all of the arguments passed.

Examples:
<example>
<min 2 2>        --> 2
<min 2.3 8 3>    --> 2.30
<min 3 -4 10>    --> -4
</example>")
{
  arithmetic_process (pf_MIN, PassPFunArgs);
}

DEFUN (pf_mul, arg1 arg2 &rest more-args,
"Returns the product of all of the arguments passed.

Examples:
<example>
<mul 2 2>        --> 4
<mul 2.3 8 3>    --> 55.20
<mul -4 10>      --> -40
</example>")
{
  arithmetic_process (pf_MUL, PassPFunArgs);
}

DEFUN (pf_div, arg1 arg2 &rest more-args,
"Returns the quotient of all of the arguments passed.

Examples:
<example>
<div 100 5>      --> 20
<div 5 100>      --> 0
<div 5.0 100.0>  --> 0.05
<div 100 5 2>    --> 10
</example>")
{
  /* If it only contains assigned attributes, it is meant for the browser,
     not us. */
  if (get_positional_arg (vars, 0) == (char *)NULL)
    {
      char *funargs = mhtml_funargs (vars);
#if !defined (FNAME_ARG)
      char *fname = "Div";
#endif

      bprintf_insert (page, start, "<%s", fname);
      start += strlen (fname);
      if (funargs != (char *)NULL)
	{
	  bprintf_insert (page, " %s", funargs);
	  start += strlen (funargs);
	}
      bprintf_insert (page, ">");
      *newstart = ++start;
    }
  else
    {
      arithmetic_process (pf_DIV, PassPFunArgs);
    }
}

DEFUN (pf_mod, arg1 arg2 &rest more-args,
"Returns the remainder of all of the arguments passed.

Examples:
<example>
<mod 100 10>    --> 0
<mod 101 10>    --> 1
<mod 89 9 3>    --> 2
</example>")
{
  arithmetic_process (pf_MOD, PassPFunArgs);
}

DEFUNX (pf_integer?, string &key base[=10],
"Returns \"true\" if <var string> is the string representation of an integer
value in base <var base> (default <code>10</code>).  This function is useful
for checking the validity of user input to a form.

You call this function with the actual value -- you may not pass the name
of a variable instead.

Some examples:
<example>
<set-var x=123>
<integer? -90>          --> true
<integer? <get-var-once x>>  --> true
<integer? 2.3>          -->
<integer? FEFE base=16> --> true
<integer? 874 base=8>   -->
</example>")

static void
pf_integerp (PFunArgs)
{
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *base_string= mhtml_evaluate_string (get_value (vars, "base"));
  int base = 10;
  char *result = (char *)NULL;

  if (!empty_string_p (base_string))
    base = atoi (base_string);

  xfree (base_string);

  if (integer_p (arg, base))
    result = "true";

  xfree (arg);

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

DEFUN (pf_integer, num,
"Returns the integer representation of <var num>.
<complete-example>
<integer 3.45> is less than <integer 3.54>
</complete-example>")
{
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (arg))
    {
      double dval = strtod (arg, (char **)NULL);
      dval = floor (dval + .5);
      bprintf_insert (page, start, "%d", (int)dval);
    }
}

DEFUNX (pf_real?, string,
"Returns \"true\" if <var string> is the string representation of
a real number.Useful for checking the validity of user input to a form.

You call this function with the actual value -- you may not pass the name
of a variable instead.

Some examples:
<example>
<set-var pi=3.141569>
<real? <get-var-once pi>>    --> true
<real? 45>              --> 
<real? 2.3>             --> true
<real? \"This\">          -->
<real? -.087e4>         --> true
</example>")

static void
pf_realp (PFunArgs)
{
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;

  if (!integer_p (arg, 10) && float_p (arg))
    result = "true";

  xfree (arg);

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

#else /* ! MHTML_ARITHMETIC */

void initialize_arithmetic_functions (Package *package) {}

#endif /* MHTML_ARITHMETIC */

#if defined (__cplusplus)
}
#endif

