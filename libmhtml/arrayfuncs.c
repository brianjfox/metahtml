/* arrayfuncs.c: -*- C -*-  Functions specifically for operating on arrays. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jul 20 17:22:47 1996.  */

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

static void pf_array_size (PFunArgs);
static void pf_array_member (PFunArgs);
static void pf_array_append (PFunArgs);
static void pf_array_add_unique (PFunArgs);
static void pf_array_shift (PFunArgs);
static void pf_array_concat (PFunArgs);
static void pf_array_reverse (PFunArgs);
static void pf_foreach (PFunArgs);
static void pf_sort (PFunArgs);

static PFunDesc func_table[] =
{
  /*   tag	     complex? debug_level	   code    */
  { "ARRAY-SIZE",	0,	0,		pf_array_size },
  { "ARRAY-MEMBER",	0,	0,		pf_array_member },
  { "ARRAY-APPEND",	0,	0,		pf_array_append },
  { "ARRAY-ADD-UNIQUE",	0,	0,		pf_array_add_unique },
  { "ARRAY-SHIFT",	0,	0,		pf_array_shift },
  { "ARRAY-CONCAT",	0,	0,		pf_array_concat },
  { "ARRAY-REVERSE",	0,	0,		pf_array_reverse },
  { "FOREACH",		1,	0,		pf_foreach },
  { "SORT",		0,	0,		pf_sort },
  { (char *)NULL,	0,	0,		(PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_array_functions)
DEFINE_SECTION (ARRAYS, variables; arrays; multiple values, 
"<meta-html> allows the use of <i>array</i> variables as well as single\n\
element variables.  In fact, all string variables in <meta-html> can be\n\
treated as array variables -- there is no special command for creating\n\
such variables.",

"Array variable values are referenced by placing the array index directly\n\
after the variable name, enclosed in square brackets (<b>[</b> and <b>]</b>).\n\
Array references use a zero-base index, so that the first accessible element\n\
in the array is at index <b>0</b> and a reference to the 4th accessible\n\
element of the array <var foo> looks like:\n\
\n\
<example>\n\
   foo[3]\n\
</example>\n\
\n\
When an array reference is made without any containing index, the\n\
reference refers to the entire array.  So, to get the value of the\n\
entire array stored in <var foo>, you would write:\n\
\n\
<example>\n\
   <get-var-once foo[]>\n\
</example>\n\
\n\
In order to ease the writing of array references which rely on a\n\
variable index, a variable name seen as an array reference index is\n\
automatically looked up as if you had written <example code><get-var\n\
VAR></example>.  Finally, multiple values may be given in a <funref\n\
variables set-var> command by separating those values with newline\n\
characters.  The following sequence of commands illustrates the\n\
typical use of array variables.\n\
\n\
<complete-example>\n\
<set-var array[] =\n\
       \"value-zero\n\
        value-one\n\
        value-two\n\
        value-three\">\n\
    <set-var i=0>\n\
    <while <get-var-once array[i]>>\n\
      The value of array[<get-var-once i>] is `<get-var-once array[i]>'.<br>\n\
      <increment i>\n\
    </while>\n\
</complete-example>\n\
")

DEFUN (pf_array_size, arrayvar,
"Returns the number of elements in the array referenced by the\n\
variable <var arrayvar>.\n\
\n\
Examples:\n\
\n\
<complete-example>\n\
<set-var array[]=\"this\">\n\
<array-size array>\n\
</complete-example>\n\
\n\
and,\n\
\n\
<complete-example>\n\
<array-shift 4 array>\n\
<array-size array>\n\
</complete-example>")
{
  char *array_name = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int result = 0;

  if (!empty_string_p (array_name))
    {
      Symbol *sym = symbol_lookup (array_name);

      if ((sym != (Symbol *)NULL) && (sym->type == symtype_STRING))
	result = sym->values_index;
    }

  bprintf_insert (page, start, "%d", result);
  xfree (array_name);
}

DEFUN (pf_array_member, item arrayvar &key caseless=true compare=func,
"Look up (and return) the index of <var item> in the contents of the\n\
array referenced by <var arrayvar>.\n\
\n\
If <var item> is not found, then <code>array-member</code> returns the\n\
empty string.\n\
\n\
If <var caseless> is non-empty, then the comparison is done without\n\
regard to character case.  Otherwise, character case is significant in\n\
the location of the item.\n\
\n\
If a function name is passed, as in <var compare=func>, it should be\n\
the name of a function which receives two required arguments -- the\n\
item that is to be looked for, and an element of the array that this\n\
item is to be compared against, and an optional keyword argument of\n\
\"caseless\".  If the function returns a non-empty string, then this\n\
item is considered a match.\n\
\n\
By default, string comparison is done on the elements.\n\
\n\
<complete-example>\n\
<set-var array[] =\n\
  <prog\n\
     this\n\
     another\n\
     multi word\n\
     thing>>\n\
<array-member \"multi word\" array>\n\
</complete-example>")
{
  char *item = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *array = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *func = mhtml_evaluate_string (get_value (vars, "compare"));
  int result = -1;

  if (!empty_string_p (array))
    {
      Symbol *sym = symbol_lookup (array);
      char **values = (char **)NULL;

      if ((sym != (Symbol *)NULL) && sym->type == symtype_STRING)	
	values = symbol_get_values (array);

      if (values != (char **)NULL)
	{
	  register int i;
	  char *caseless;
	  int caseless_p = 0;

	  caseless = mhtml_evaluate_string (get_value (vars, "caseless"));
	  if (!empty_string_p (caseless)) caseless_p++;
	  xfree (caseless);

	  if (item ==(char *)NULL) item = strdup ("");

	  if (empty_string_p (func))
	    {
	      for (i = 0; values[i] != (char *)NULL; i++)
		if ((caseless && (strcasecmp (item, values[i]) == 0)) ||
		    (!caseless && (strcmp (item, values[i]) == 0)))
		  {
		    result = i;
		    break;
		  }
	    }
	  else
	    {
	      char *arg1 = strdup (quote_for_setvar (item));

	      for (i = 0; values[i] != (char *)NULL; i++)
		{
		  char *arg2 = strdup (quote_for_setvar (values[i]));
		  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
		  char *funres;
		  int found = 0;

		  bprintf (buffer, "<%s %s %s", func, arg1, arg2);
		  if (caseless)
		    bprintf (buffer, " caseless=true>");
		  else
		    bprintf (buffer, ">");
		  funres = mhtml_evaluate_string (buffer->buffer);
		  bprintf_free_buffer (buffer);
		  free (arg2);

		  found = (!empty_string_p (funres));
		  xfree (funres);

		  if (found)
		    {
		      result = i;
		      break;
		    }
		}

	      xfree (arg1);
	    }
	}
    }

  if (result > -1)
    bprintf_insert (page, start, "%d", result);

  xfree (item);
  xfree (array);
  xfree (func);
}

DEFUN (pf_array_append, item arrayvar,
"Add <var item> as the last array element of the contents\n\
of <var arrayvar>.  This is especially useful in conjunction with\n\
<funref arrays foreach> as a <i>collector</i>:\n\
\n\
<example>\n\
<foreach name allnames>\n\
  <if <satifies-criteria <get-var-once name>>\n\
     <array-append <get-var-once name> useful-names>>\n\
</foreach>\n\
</example>\n\
\n\
See also <funref arrays array-add-unique>.")
{
  char *item = mhtml_evaluate_string (get_positional_arg (vars, 0)); 
  char *array = mhtml_evaluate_string (get_positional_arg (vars, 1));

  if (!empty_string_p (array))
    {
      Symbol *sym = symbol_intern (array);

      if (item == (char *)NULL) item = strdup ("");
      symbol_add_value (sym, item);
    }

  xfree (array);
  xfree (item);
}

DEFUN (pf_array_add_unique, item arrayvar &key caseless=true tellme=true,
"Add <var item> as the last array element of the contents\n\
of <var arrayvar> if, and only if, <var item> is not already\n\
a member of that array.\n\
\n\
The comparison is a direct string-wise compare.  If <var CASELESS> is\n\
non-empty, then a caseless string compare is done.\n\
\n\
If the keyword argument <var tellme> is supplied with a non-null value,\n\
then this function returns the word \"true\" if the item was added, or\n\
the empty string if not.\n\
\n\
See also <funref arrays array-append>.")
{
  char *item = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *array = mhtml_evaluate_string (get_positional_arg (vars, 1));
  int result = -1;
  int added = 0;
  int report = 0;

  if (!empty_string_p (array))
    {
      char **values = symbol_get_values (array);
      char *tellme = mhtml_evaluate_string (get_value (vars, "tellme"));

      if (!empty_string_p (tellme))
	report = 1;

      xfree (tellme);

      if (item == (char *)NULL) item = strdup ("");

      if (values != (char **)NULL)
	{
	  register int i;
	  char *caseless;
	  int caseless_p = 0;

	  caseless = mhtml_evaluate_string (get_value (vars, "caseless"));
	  if (!empty_string_p (caseless)) caseless_p++;
	  xfree (caseless);

	  for (i = 0; values[i] != (char *)NULL; i++)
	    if ((caseless && (strcasecmp (item, values[i]) == 0)) ||
		(!caseless && (strcmp (item, values[i]) == 0)))
	      {
		result = i;
		break;
	      }
	}

      if (result == -1)
	{
	  Symbol *sym = symbol_intern (array);
	  symbol_add_value (sym, item);
	  added = 1;
	}
    }

  xfree (array);
  xfree (item);

  if (report && added)
    {
      bprintf_insert (page, start, "true");
      *newstart += 4;
    }
}

DEFUN (pf_array_shift, amount arrayvar &key start,
"Shift the elements of <var arrayvar> the indicated amount.\n\
\n\
If <var amount> is negative, the elements are shifted down\n\
(i.e. towards zero), with the lowest number elements being lost.\n\
\n\
If <var amount> is positive, the elements are shifted up, with\n\
no loss at all -- instead empty elements are used to fill the\n\
created space.\n\
\n\
If the keyword argument <var start> is present, it indicates the\n\
zero-based offset from which to start shifting.\n\
\n\
Given the array:\n\
<example>\n\
<set-var array[] =\n\
   <prog\n\
       0\n\
       1\n\
       2>>\n\
</example>\n\
\n\
<set-var array[] =\n\
   <prog\n\
       0\n\
       1\n\
       2>>\n\
\n\
then after executing <example code><array-shift 2 array></example>, \n\
the array looks like:\n\
<example>\n\
   \"\"\n\
   \"\"\n\
   \"0\"\n\
   \"1\"\n\
   \"2\"\n\
</example>\n\
and, a subsequent execution of\n\
<example code><array-shift -3 array></example> leaves <var array>:\n\
<example>\n\
   \"1\"\n\
   \"2\"\n\
</example>")
{
  char *amount_txt =  mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *array_var = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *start_var = mhtml_evaluate_string (get_value (vars, "start"));
  int startpos = 0;

  if (!empty_string_p (start_var))
    startpos = atoi (start_var);

  if (startpos < 0) startpos = 0;

  if (!empty_string_p (array_var))
    {
      Symbol *sym = symbol_lookup (array_var);

      if ((sym != (Symbol *)NULL) &&
	  (sym->type == symtype_STRING) &&
	  (sym->values_index > startpos))
	{
	  register int i;
	  int amount = atoi (amount_txt);
	  int newsize = sym->values_index + amount;

	  symbol_set_modified (sym);

	  if (amount < 0)
	    {
	      if (newsize <= 0)
		{
		  symbol_free_array (sym->values);
		  sym->values = (char **)NULL;
		  sym->values_index = 0;
		}
	      else
		{
		  amount = -amount;

		  for (i = startpos; i < startpos + amount; i++)
		    free (sym->values[i]);

		  for (; i < sym->values_index; i++)
		    sym->values[i - amount] = sym->values[i];

		  sym->values[i - amount] = (char *)NULL;
		  sym->values_index = newsize;
		}
	    }
	  else if (amount > 0)
	    {
	      if (newsize >= sym->values_slots)
		sym->values = (char **)xrealloc
		  (sym->values, (newsize + 1) * sizeof (char *));

	      for (i = sym->values_index; i > startpos - 1; i--)
		sym->values[i + amount] = sym->values[i];

	      for (i = startpos; i < startpos + amount; i++)
		sym->values[i] = strdup ("");

	      sym->values_index = newsize;
	    }
	}
    }
  else if (debug_level)
    {
      page_debug ("--> array-shift: Needs an array to operate on");
    }
      
  xfree (amount_txt);
  xfree (array_var);
  xfree (start_var);
}

DEFUN (pf_array_reverse, arrayvar,
"Directly modify the values of <var arrayvar> making the first element be\n\
the last, and the last be the first.\n\
<complete-example>\n\
<set-var array[]=\"0\n1\n2\n3\">\n\
<array-reverse array>\n\
<get-var-once array[]>\n\
</complete-example>")
{
  char *array_var = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (array_var))
    {
      Symbol *sym = symbol_lookup (array_var);

      if ((sym != (Symbol *)NULL) &&
	  (sym->type == symtype_STRING) &&
	  (sym->values_index > 1))
	{
	  register int head, tail;
	  char **values = sym->values;

	  symbol_set_modified (sym);

	  for (head=0, tail=sym->values_index - 1; head < tail; head++, tail--)
	    {
	      char *temp = values[head];
	      values[head] = values[tail];
	      values[tail] = temp;
	    }
	}
    }

  xfree (array_var);
}

DEFMACRO (pf_foreach, elementvar arrayvar
	  &key start=x end=x step=x iter=var no-copy=true,
"Perform <var body> with <var elementvar> bound to successive memebers\n\
of <var arrayvar>, starting with the element at <var start> (default\n\
0), and ending at <var end> (default <example code><array-size\n\
ARRAYVAR></example>), advancing by <var step> (default 1).\n\
\n\
The <code>foreach</code> command is the basic array looping device in\n\
<Meta-HTML>.  It is guaranteed to iterate over each element that you\n\
specify, whether that element is the empty string or not.\n\
\n\
If <var no-copy=true> is specified, the array is not copied before\n\
iteration, so that changes that you make to the array take place\n\
immediately, during the execution of the surrounding <tag foreach>.\n\
\n\
Starting with the simple array:\n\
<set-var example::array[]=\"0\\n1\\n2\\n3\\n4\\n5\\n6\\n7\\n8\\n9\">\n\
<example>\n\
<set-var example::array[]=\"0\\n1\\n2\\n3\\n4\\n5\\n6\\n7\\n8\\n9\">\n\
</example>\n\
\n\
we can print out the odd numbers of this array by using values for\n\
both <var start> and <var step>:\n\
\n\
<complete-example>\n\
<foreach x example::array start=1 step=2> <get-var-once x>, </foreach>\n\
</complete-example>\n\
\n\
or, we can produce a \"countdown\" with a negative value for <var step>:\n\
\n\
<complete-example>\n\
<foreach x example::array step=-1> <get-var-once x>, </foreach> BOOM!\n\
</complete-example>")
{
  char *element_var = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *array_var = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *start_arg = mhtml_evaluate_string (get_value (vars, "start"));
  char *end_arg = mhtml_evaluate_string (get_value (vars, "end"));
  char *step_arg = mhtml_evaluate_string (get_value (vars, "step"));
  char *iter_var = mhtml_evaluate_string (get_value (vars, "iter"));
  char *copy = mhtml_evaluate_string (get_value (vars, "copy"));
  char *no_copy = mhtml_evaluate_string (get_value (vars, "no-copy"));
  int start_index, end_index, step;

  if ((!empty_string_p (element_var)) && (!empty_string_p (array_var)))
    {
      Symbol *array = symbol_lookup (array_var);

      if ((array != (Symbol *)NULL) && (array->type == symtype_STRING) &&
	  (array->values_index > 0) && (array->values != (char **)NULL))
	{
	  register int i;

	  end_index = array->values_index;
	  start_index = 0;
	  step = 1;

	  if (!empty_string_p (step_arg))
	    {
	      step = atoi (step_arg);
	      if (step == 0) step = 1;

	      if (step < 0)
		{
		  end_index = 0;
		  start_index = array->values_index - 1;
		}
	    }

	  if (!empty_string_p (start_arg))
	    start_index = atoi (start_arg);

	  if (start_index < 0)
	    start_index = 0;

	  if (!empty_string_p (end_arg))
	    end_index = atoi (end_arg);

	  if (end_index > array->values_index)
	    end_index = array->values_index;

	  if (end_index < start_index)
	    {
	      if (step == 1)
		step = -1;
	    }

	  /* Final sanity check.  Make sure that START and END are within
	     the bounds of the final array. */
	  if ((step < 0) && start_index >= array->values_index)
	    start_index = array->values_index - 1;

	  if (((step < 0) && (end_index <= start_index)) ||
	      ((step > 0) && (end_index >= start_index)))
	    {
	      /* Maybe copy the array, so that functions which delete or
		 manipulate the contents of it do not disturb our pointers. */
	      int copyit = 1;
	      char **array_values;

	      if ((empty_string_p (copy)) && (empty_string_p (no_copy)))
		{
		  /* Set the default behavior. */
		  if (var_present_p (vars, "copy"))
		    copyit = 0;
		  else if (var_present_p (vars, "no-copy"))
		    copyit = 1;
		}
	      else if (!empty_string_p (no_copy))
		copyit = 0;


	      if (copyit)
		array_values = symbol_copy_array (array->values);
	      else
		array_values = array->values;

	      for (i = start_index;
		   (((start_index < end_index) && (i < end_index)) ||
		    ((start_index > end_index) && (i >= end_index)) ||
		    ((start_index == end_index) &&
		     (((step < 0) && (i >= end_index)) ||
		      ((step > 0) && (i <= end_index)))));
		   i += step)
		{
		  Symbol *element = symbol_remove (element_var);
		  PAGE *code;
		  int line = parser_current_lineno;

		  if (element) symbol_free (element);
		  element = symbol_intern (element_var);
		  symbol_add_value (element, array_values[i]);
		  code = page_copy_page (body);

		  if (!empty_string_p (iter_var))
		    mhtml_set_numeric_variable (iter_var, i);

		  page_process_page_internal (code);
		  parser_current_lineno = line;

		  if (code != (PAGE *)NULL)
		    {
		      int broken = (code->attachment != (void *)NULL);

		      if (code->bindex != 0)
			{
			  bprintf_insert (page, start, "%s", code->buffer);
			  start += (code->bindex);
			}

		      page_free_page (code);
		      if (broken) break;
		    }
		}

	      if (copyit)
		symbol_free_array (array_values);
	    }

	  *newstart = start;
	}
    }

  xfree (element_var);
  xfree (array_var);
  xfree (start_arg);
  xfree (end_arg);
  xfree (step_arg);
  xfree (iter_var);
  xfree (no_copy);
  xfree (copy);
}

DEFUN (pf_array_concat, receiver &rest contributors...,
"Appends the contents of each <var contributor> array to the \n\
end of <var receiver>.\n\
\n\
Both <var receiver> and each <var contributor> are variable names whose\n\
values are treated as arrays.\n\
\n\
For a single <var contributor>, <code>array-concat</code>\n\
could have been defined as:\n\
\n\
<example>\n\
<defsubst array-concat dest-name source-name>\n\
  <foreach item <get-var-once source-name>>\n\
     <array-append <get-var-once item> <get-var-once dest-name>>\n\
  </foreach>\n\
</defsubst>\n\
</example>")
{
  char *arrayname = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (arrayname))
    {
      Symbol *arraysym = symbol_intern (arrayname);

      if (arraysym->type == symtype_STRING)
	{
	  register int i, arg_index = 0;
	  int offset;
	  char *arg;

	  for (arg_index = 1;
	       (arg = mhtml_evaluate_string
		(get_positional_arg (vars, arg_index))) != (char *)NULL;
	       arg_index++)
	    {
	      if (!empty_string_p (arg))
		{
		  Symbol *sym = symbol_lookup (arg);

		  if ((sym != (Symbol *)NULL) &&
		      (sym->type == symtype_STRING) &&
		      (sym->values_index > 0))
		    {
		      if ((sym->values_index + arraysym->values_index + 1)
			  > arraysym->values_slots)
			arraysym->values =
			  (char **)xrealloc
			  (arraysym->values,
			   (arraysym->values_slots += (sym->values_index + 10))
			   * sizeof (char *));

		      symbol_set_modified (arraysym);

		      offset = arraysym->values_index;
		      for (i = 0; i < sym->values_index; i++)
			arraysym->values[offset++] = strdup (sym->values[i]);
		      arraysym->values_index = offset;
		      arraysym->values[offset] = (char *)NULL;
		    }
		}
	      xfree (arg);
	    }
	}
    }
  xfree (arrayname);
}

static char *mhtml_sort_function_name = (char *)NULL;
static int mhtml_sort_is_caseless = 0;
static int mhtml_sort_is_descending = 0;
static int mhtml_sort_is_numeric = 0;

static int
sort_with_function (const void *item1, const void *item2)
{
  char *string1 = *(char **)item1;
  char *string2 = *(char **)item2;
  int should_free = 0;
  int result = 0;

  if (mhtml_sort_function_name)
    {
      PAGE *page = page_create_page ();

      if (string1)
	{
	  char *temp = quote_for_setvar (string1);
	  bprintf (page, "<%s %s>", mhtml_sort_function_name, temp);
	  string1 = mhtml_evaluate_string (page->buffer);
	}
      page->bindex = 0;

      if (string2)
	{
	  char *temp = quote_for_setvar (string2);
	  bprintf (page, "<%s %s>", mhtml_sort_function_name, temp);
	  page->buffer[page->bindex] = '\0';
	  string2 = mhtml_evaluate_string (page->buffer);
	}

      page_free_page (page);
      should_free++;
    }

  if (string1 && !string2)
    result = 1;
  else if (string2 && !string1)
    result = -1;
  else if (!string1 && !string2)
    result = 0;
  else if (mhtml_sort_is_numeric)
    {
      double x = strtod (string1, (char **)NULL);
      double y = strtod (string2, (char **)NULL);

      if (x != y)
	result = (x > y) ? 1 : -1;
    }
  else if (mhtml_sort_is_caseless)
    result = strcasecmp (string1, string2);
  else
    result = strcmp (string1, string2);

  if (should_free)
    {
      xfree (string1);
      xfree (string2);
    }

  if (result && mhtml_sort_is_descending)
    result = -result;

  return (result);
}

DEFUN (pf_sort, arrayvar &optional sort-fun &key caseless=true
       sortorder=[ascending|descending] numeric=true,
"Sort the contents of the array <var arrayvar>.\n\
\n\
The elements are sorted in place -- this function has no return value.\n\
\n\
If <var caseless=true> is given, then the comparison of the elements of\n\
the array is done without regards to case.\n\
\n\
If <var sortorder=reverse> is given, then the results are returned in\n\
descending order, instead of ascending order.  The default is to order\n\
the elements in ascending order.\n\
\n\
If <var numeric=true> is given, then the elements of <var arrayvar>\n\
are treated as numeric entities, whether they are or not.  The default\n\
is to treat the elements as character strings, which can have\n\
unexpected results when sorting numeric quantities (\"11\" is less\n\
then \"2\" when sorting alphabetically!)\n\
\n\
Finally, you may supply a sorting function, whose name is passed as\n\
<var sort-fun>.  This function will be called on each element just\n\
before comparison, and the results of that function will be used for\n\
the comparison instead of the element itself.  This allows you to\n\
create a collating sort, or to sort on complex weighting features, or\n\
anything else that you can conceive of.\n\
\n\
Examples:\n\
\n\
Given the array:\n\
<unset-var array>\n\
<set-var array[0] = 1\n\
         array[1] = 2\n\
         array[3] = 3\n\
         array[4] = 4\n\
         array[5] = 20>\n\
<example>\n\
<set-var array[0] = 1\n\
         array[1] = 2\n\
         array[3] = 3\n\
         array[4] = 4\n\
         array[5] = 20>\n\
</example>\n\
then,\n\
\n\
<complete-example-global>\n\
<sort array>\n\
<foreach x array> <get-var-once x> </foreach>\n\
</complete-example-global>\n\
while\n\
<complete-example-global>\n\
<sort array numeric=true>\n\
<foreach x array> <get-var-once x> </foreach>\n\
</complete-example-global>\n\
\n\
Sorting strings:\n\
<complete-example-global>\n\
<set-var array[]=\"a\\nb\\nc\\nd\\ne\\nf\\nA\\nB\\nC\\nD\\nE\\nF\">\n\
<sort array sortorder=descending>\n\
<foreach x array> <get-var-once x> </foreach>\n\
</complete-example-global>\n\
\n\
Without regards to case:\n\
<complete-example-global>\n\
<sort array caseless=true>\n\
<foreach x array> <get-var-once x> </foreach>\n\
</complete-example-global>\n\
\n\
Finally, here is an example which sorts a list\n\
of words based upon the percentage of vowels\n\
present in each word, using a sort function\n\
which calculates that value for each string:\n\
\n\
<complete-example>\n\
<defun vowel-percentage string>\n\
  <set-var x =\n\
    <subst-in-string <downcase <get-var-once string>> \"([^aeiou])\" \"\">>\n\
  <percent <string-length <get-var-once x>>\n\
           <string-length <get-var-once string>>>\n\
</defun>\n\
.blank\n\
<set-var words[]=\n\
  <prog\n\
    Brian\n\
    Fox\n\
    sorts\n\
    elegant\n\
    strings\n\
    beautifully>>\n\
.blank\n\
<sort words vowel-percentage numeric=true sortorder=descending>\n\
.blank\n\
<foreach word words>\n\
  <get-var-once word> (<vowel-percentage <get-var-once word>>)<br>\n\
</foreach>\n\
</complete-example>")
{
  char *sortvar = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (sortvar))
    {
      Symbol *sym = symbol_lookup (sortvar);

      /* If there is anything to sort... */
      if ((sym != (Symbol *)NULL) && (sym->values_index != 0))
	{
	  char *sorter = mhtml_evaluate_string (get_positional_arg (vars, 1));
	  int caseless = 0, descending = 0, numeric = 0;
	  char *temp;

	  temp = mhtml_evaluate_string (get_value (vars, "caseless"));
	  if (!empty_string_p (temp))
	    caseless = 1;
	  xfree (temp);

	  temp = mhtml_evaluate_string (get_value (vars, "descending"));
	  if (!empty_string_p (temp))
	    descending = 1;
	  xfree (temp);

	  temp = mhtml_evaluate_string (get_value (vars, "numeric"));
	  if (!empty_string_p (temp))
	    numeric = 1;
	  xfree (temp);

	  /* Support "sortorder=[reverse,ascending,descending]" syntax. */
	  temp = mhtml_evaluate_string (get_value (vars, "sortorder"));

	  if (!empty_string_p (temp))
	    {
	      if ((strcasecmp (temp, "descending") == 0) ||
		  (strcasecmp (temp, "reverse") == 0))
		descending++;
	    }
	  xfree (temp);

	  mhtml_sort_function_name = (char *)NULL;

	  if (!empty_string_p (sorter))
	    mhtml_sort_function_name = sorter;

	  mhtml_sort_is_numeric = numeric;
	  mhtml_sort_is_caseless = caseless;
	  mhtml_sort_is_descending = descending;

	  qsort ((void *)sym->values, sym->values_index, sizeof (char *),
		 sort_with_function);

	  symbol_set_modified (sym);

	  if (sorter) free (sorter);
	}
    }

  if (sortvar) free (sortvar);
}

#if defined (__cplusplus)
}
#endif
