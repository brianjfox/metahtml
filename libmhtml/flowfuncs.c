/* flowfuncs.c: -*- C -*-  Flow control functions for Meta-HTML */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jun 21 12:02:48 1997.  */

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
/************************************************************/
/*							    */
/*			Flow Control Functions		    */
/*							    */
/************************************************************/

static void pf_if (PFunArgs);
static void pf_ifeq (PFunArgs);
static void pf_ifneq (PFunArgs);
static void pf_when (PFunArgs);
static void pf_var_case (PFunArgs);
static void pf_match_case (PFunArgs);

/* #define PAGE_ITERATOR_MAX_COUNT 2000 [ No longer used.] */
static void pf_while (PFunArgs);

static void pf_break (PFunArgs);
static void pf_return (PFunArgs);
static void pf_with (PFunArgs);

/* Return the parser's idea of the current line number. */
static void pf_current_lineno (PFunArgs);

static void pf_group (PFunArgs);
static void pf_concat (PFunArgs);

static PFunDesc func_table[] =
{
  { "PROG",		0, 0, pf_group },
  { "GROUP",		0, 0, pf_group },
  { "CONCAT",		0, 0, pf_concat },
  { "IF",		0, 0, pf_if },
  { "IFEQ",		0, 0, pf_ifeq },
  { "IFNEQ",		0, 0, pf_ifneq },
  { "WHEN",		1, 0, pf_when },
  { "VAR-CASE",		0, 0, pf_var_case },
  { "MATCH-CASE",	0, 0, pf_match_case },
  { "WHILE",		1, 0, pf_while },
  { "BREAK",		0, 0, pf_break },
  { "RETURN",		0, 0, pf_return },
  { "WITH",		1, 0, pf_with },
  { "*PARSER*::CURRENT-LINENO", 0, 0, pf_current_lineno },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_flowfunc_functions)
DEFINE_SECTION (FLOW-CONTROL, flow control;if statements; while; until,
"<Meta-HTML> contains commands for controlling which of a set\n\
of statements will be executed, and for repetitive execution of a set\n\
of statements.\n\
\n\
Such commands constitute what is called <i>flow control</i>, since\n\
they tell the <meta-html> interpreter where or what the next statement\n\
to interpret resides.\n\
\n\
All of the flow control operators in <meta-html> take a test and some\n\
additional statements; the closest thing to a <b>goto</b> statement in\n\
<meta-html> is the <funref page-operators redirect> command.", "")

DEFUN (pf_group, &rest args,
"Combine all of the material passed into a single Meta-HTML statement.\n\
This is the primitive for grouping multiple statements where only a\n\
single statement is expected.  Whitespace within the group is preserved,\n\
making this command useful for assigning to array variables.\n\
\n\
Some examples:\n\
<example>\n\
<set-var array[] =\n\
  <group this is element 0\n\
         this is element 1\n\
         this is element 2>>\n\
</example>\n\
\n\
<example>\n\
<if <eq this that>\n\
    <group <h2> This is equal to That </h2>>\n\
   <group <h2> This is NOT equal to That </h2>>>\n\
</example>\n\
\n\
Although <code>group</code> is a primitive in Meta-HTML, it could\n\
have been defined as:\n\
<example>\n\
  <defsubst group &body body><get-var-once body></defsubst>\n\
</example>")
{
  if ((body != (PAGE *)NULL) && (!empty_string_p (body->buffer)))
    bprintf_insert (page, start, "%s", body->buffer + 1);
}

DEFUNX (pf_prog, &rest args, "Synonym for <funref FLOW-CONTROL group>.")

DEFUN (pf_concat, &rest args,
"Concatenate all of the arguments given, creating a single token with no\n\
intervening whitespace. This is quite useful for those situations where\n\
intervening whitespace would look bad in the output, but the input source\n\
would be unreadable without any.\n\
\n\
For example:\n\
<example>\n\
<concat <textarea name=label rows=10 cols=40>\n\
        <get-var-once label>\n\
        </textarea>>\n\
</example>")
{
  register int i = 0;
  char *temp;

  while ((temp = get_positional_arg (vars, i)) != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", temp);
      start += strlen (temp);
      i++;
    }
}

static void
count_newlines (char *text)
{
  if (text != (char *)NULL)
    {
      register int i;

      for (i = 0; text[i] != '\0'; i++)
	if (text[i] == '\n') parser_current_lineno++;
    }
}

DEFUN (pf_if, test &optional then else,
"First <var test> is evaluated. If the result does not contain only\n\
whitespace characters the <var then> clause is evaluated, otherwise,\n\
the <var else> clause is evaluated. Although <Meta-HTML> has the\n\
relational operator <funref relational-operators or>, you can\n\
efficiently test for the presence of any of a group of variables\n\
with code similar to the following:\n\
<example>\n\
<if <get-var foo bar>\n\
   \"Either FOO or BAR is present\"\n\
  \"Neither FOO nor BAR is present\">\n\
</example>")
{
  char *test_clause = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *then_clause = get_positional_arg (vars, 1);
  char *else_clause = get_positional_arg (vars, 2);
  char *consequence;

  if (!empty_string_p (test_clause))
    {
      consequence = then_clause;
      count_newlines (else_clause);
    }
  else
    {
      consequence = else_clause;
      count_newlines (then_clause);
    }

  if (consequence != (char *)NULL)
    bprintf_insert (page, start, "%s", consequence);

  xfree (test_clause);
}

DEFUN (pf_ifeq, this that &optional then else &key caseless=true,
"The <var this> and <var that> clauses are evaluated.\n\
If the results are text-wise identical, then the <var then>\n\
clause is evaluated, otherwise, the <var else> clause is\n\
evaluated.  If <var caseless=true> is given, the text-wise\n\
comparison of the values is done with no regard to upper and lower\n\
case distinctions.")
{
  char *left_clause = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *right_clause = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *then_clause = get_positional_arg (vars, 2);
  char *else_clause = get_positional_arg (vars, 3);
  int caseless_p = var_present_p (vars, "CASELESS");
  char *consequence;

  if (((empty_string_p (left_clause)) && (empty_string_p (right_clause))) ||
      ((left_clause && right_clause) &&
       (((!caseless_p) && (strcmp (left_clause, right_clause) == 0)) ||
	((caseless_p) && (strcasecmp (left_clause, right_clause) == 0)))))
    {
      consequence = then_clause;
      count_newlines (else_clause);
    }
  else
    {
      consequence = else_clause;
      count_newlines (then_clause);
    }

  if (consequence != (char *)NULL)
    bprintf_insert (page, start, "%s", consequence);

  xfree (left_clause);
  xfree (right_clause);
}

DEFUN (pf_ifneq, this that &optional then else &key caseless=true,
"The <var this> and <var that> clauses are evaluated.\n\
If the results are not text-wise identical, then the <var then>\n\
clause is evaluated, otherwise, the <var else> clause is\n\
evaluated.  If <var caseless=true> is given, the text-wise\n\
comparison of the values is done with no regard to upper and lower\n\
case distinctions.")
{
  char *left_clause = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *right_clause = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *then_clause = get_positional_arg (vars, 2);
  char *else_clause = get_positional_arg (vars, 3);
  int caseless_p = var_present_p (vars, "CASELESS");
  char *consequence = (char *)NULL;

  if (((empty_string_p (left_clause)) && (empty_string_p (right_clause))) ||
      ((left_clause && right_clause) &&
       (((!caseless_p) && (strcmp (left_clause, right_clause) == 0)) ||
	((caseless_p) && (strcasecmp (left_clause, right_clause) == 0)))))
    {
      consequence = else_clause;
      count_newlines (then_clause);
    }
  else
    {
      consequence = then_clause;
      count_newlines (else_clause);
    }

  if (consequence != (char *)NULL)
    bprintf_insert (page, start, "%s", consequence);

  xfree (left_clause);
  xfree (right_clause);
}

DEFMACRO (pf_when, test,
"Evaluate <var test>.  If the result is a non-empty string,\n\
then execute the <var body> statements.  This is a cleaner way to\n\
handle optional multiple statement execution rather than dealing with\n\
quoting everything inside of an <funref FLOW-CONTROL if> form.")
{
  char *test = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (test))
    bprintf_insert (page, start, "%s", body->buffer);

  if (test) free (test);
}

DEFUN (pf_break, ,
"Unconditionally and immediately stop the execution of the nearest\n\
surrounding <funref FLOW-CONTROL while> or <funref ARRAYS foreach>.\n\
\n\
Example usage:\n\
<example>\n\
<while true>\n\
  ;;; Check to see if the user has changed the file.\n\
  <if <file-newer? <get-var foo.c> <get-var foo.o>>\n\
    <break>>\n\
\n\
  ;;; Not changed yet, so do some more in the background.\n\
  <process-chunk <get-var chunk-num>>\n\
  <increment chunk-num>\n\
</while>\n\
</example>")
{
  page->attachment = (void *)bprintf_create_buffer ();
  count_newlines (page->buffer + start);
  page->bindex = start;
  page->buffer[start] = '\0';
}

DEFUN (pf_return, &rest args,
"Unconditionally and immediately stop the execution of the current\n\
function, macro, <funref FLOW-CONTROL while> or <funref ARRAYS\n\
foreach> statement, and return the evaluated <var args>.\n\
\n\
Example usage:\n\
<complete-example>\n\
<define-function countdown start stop>\n\
  <if <eq start stop>\n\
      <return BlastOff!>>\n\
  <get-var start>, \n\
  <countdown <sub start 1> <get-var stop>>\n\
</define-function>\n\
<countdown 10 4>\n\
</complete-example>")
{
  register int i = 0;
  char *result = (char *)NULL;

  if (body->buffer != (char *)NULL)
    {
      while (whitespace (body->buffer[i])) i++;
      result = mhtml_evaluate_string (body->buffer + i);
      page->bindex = start;
      page->buffer[start] = '\0';
      bprintf (page, "%s", result ? result : "");
      *newstart += strlen (result ? result : "");
      xfree (result);
    }

  page->attachment = (void *)bprintf_create_buffer ();
}

DEFVAR (mhtml::iteration-limit,
"The use of this variable has been deprecated.  <funref flow-control while>\n\
loops last until either the <var test-clause> is met, or a\n\
<funref flow-control break> statement is seen.")

DEFMACRO (pf_while, test,
"<var test> is evaluated.  If the result is a non-empty string, then\n\
the <var body> statements are evaluated, and the process is repeated.")
{
  char *test = get_positional_arg (vars, 0);
  int lineno = parser_current_lineno;

  while (1)
    {
      char *result = mhtml_evaluate_string (test);
      int empty = empty_string_p (result);
      PAGE *code;

      xfree (result);

      if (empty)
	break;

      code = page_copy_page (body);
      page_process_page_internal (code);

      if (code != (PAGE *)NULL)
	{
	  int broken = (code->attachment != (void *)NULL);

	  if (code->bindex != 0)
	    {
	      bprintf_insert (page, start, "%s", code->buffer);
	      start += (code->bindex);
	      parser_current_lineno = lineno;

	      *newstart = start;
	    }

	  page_free_page (code);

	  if (broken)
	    break;
	}
    }
}

static int
case_comp (char *s1, char *s2, int regexp_p)
{
  int result = -1;

  if (!regexp_p)
    result = strcasecmp (s1, s2);
  else
    {
      regex_t re;
      regmatch_t offsets[8];
      int matched = 0;

      regcomp (&re, s2, REG_EXTENDED | REG_ICASE);
      matched = (regexec (&re, s1, 8, offsets, 0) == 0);

      if (matched)
	result = 0;

      regfree (&re);
    }

  return (result);
}

static void
pf_var_case_internal (PFunArgs, int regexp_p)
{
  register int i = 0;
  char **names = get_vars_names (vars);
  char **vals = get_vars_vals (vars);
  static char *nullval = "";
  char *default_action = (char *)NULL;
  int clause_found = 0;

  if (names != (char **)NULL)
    {
      while (1)
	{
	  char *name = (char *)NULL;
	  char *case_value = (char *)NULL;
	  char *page_value = (char *)NULL;
	  char *action = (char *)NULL;

	  if ((names[i] == (char *)NULL) || (names[i + 1] == (char *)NULL))
	    break;

	  name = mhtml_evaluate_string (names[i]);
	  case_value = mhtml_evaluate_string (vals[i]);
	  page_value = pagefunc_get_variable (name);
	  action = names[i + 1];
	  i += 2;

	  if (name != (char *)NULL)
	    {
	      /* Check for special "default" case. */
	      if (strcasecmp (name, "default") == 0)
		{
		  default_action = action;
		  if (case_value) free (case_value);
		  if (page_value) free (page_value);
		  free (name);
		  continue;
		}
	      free (name);
	    }

	  /* Check the value against the page value. */
	  if (empty_string_p (page_value))
	    {
	      page_value = nullval;
	    }

	  if (empty_string_p (case_value))
	    {
	      if (case_value) free (case_value);
	      case_value = nullval;
	    }

	  if ((page_value == case_value) ||
	      (case_comp (page_value, case_value, regexp_p) == 0))
	    {
	      clause_found = 1;
	      if (action != (char *)NULL)
		bprintf_insert (page, start, "%s", action);

	      if (case_value != nullval) free (case_value);
	      break;
	    }

	  if (case_value != nullval) free (case_value);
	}

      if (default_action && !clause_found)
	bprintf_insert (page, start, "%s", default_action);
    }
}

DEFUN (pf_var_case, &optional name=value consequent... default
       default-consequent,
"For each <var name=value> pair, the value of <var name> is\n\
string-wise compared with <var value>.  If they are identical, then\n\
the corresponding <var consequent> code is performed, and its value is\n\
the return value of the <example code><var-case></example> form.\n\
\n\
If none of the clauses match, and there is a <code>default</code>\n\
clause, then the <var default-consequent> is evaluated, and its return\n\
value is the return value of the <example code><var-case></example>\n\
form.\n\
\n\
<code>var-case</code> is especially useful as a `traffic\n\
switch' to select one of several actions based on a user button\n\
press.\n\
\n\
For example:\n\
\n\
<example>\n\
<var-case\n\
   action=\"Save Files\"      <save-files <get-var posted::files[]>>\n\
   action=\"Delete Files\"    <delete-files <get-var posted::files[]>>\n\
   action=\"Rename Files\"    <redirect\n\
                                 rename-files.mhtml?<cgi-encode files>>>\n\
</example>")
{
  pf_var_case_internal (PassPFunArgs, 0);
}

DEFUN (pf_match_case, &optional name=regexp consequent... default
       default-consequent,
"For each <var name=value> pair, the value of <var name> is\n\
compared with the regular expression <var regexp>.  If the expression matches,\n\
then the corresponding <var consequent> code is performed, and its value is\n\
the return value of the <example code><match-case></example> form.\n\
\n\
If none of the clauses match, and there is a <code>default</code>\n\
clause, then the <var default-consequent> is evaluated, and its return\n\
value is the return value of the <example code><match-case></example>\n\
form.\n\
\n\
<code>match-case</code> is especially useful as a `traffic\n\
switch' to select one of several actions based on a user button\n\
press.")
{
  pf_var_case_internal (PassPFunArgs, 1);
}

DEFMACRO (pf_with, &optional var=val...,
"Execute <var body> in an environment where <var var> has the value\n\
<var val>.  Execution takes place in the current package.  After\n\
execution, the value of <var var> is restored to the value that it\n\
had before encountering the <code>with</code> macro.\n\
\n\
<complete-example>\n\
<set-var x=hello>\n\
<with x=1 z=<get-var foo>> <get-var x> </with>\n\
<get-var x>\n\
</complete-example>")
{
  register int i;
  Package *newvars = symbol_get_package ((char *)NULL);
  char **names = get_vars_names (vars);
  char **vals = get_vars_vals (vars);

  /* Evalaute the variable values in the order the user typed them in. */
  for (i = 0; ((names != (char **)NULL) && (names[i] != (char *)NULL)); i++)
    {
      char *evalled_name = mhtml_evaluate_string (names[i]);
      char *canonical_name = symbol_canonical_name (evalled_name);
      char *value = mhtml_evaluate_string (vals[i]);
  
      forms_set_tag_value_in_package (newvars, evalled_name, value);
      xfree (canonical_name);
      xfree (evalled_name);
      xfree (value);
    }

  /* Call the (let ((...))) code with arguments almost like ours -- having
     replaced the input variables package with a new one containing the
     evaluated values. */
  vars = newvars;
  mhtml_let (PassPFunArgs);
  symbol_destroy_package (newvars);
}

static void
pf_current_lineno (PFunArgs)
{
  char *arg = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (arg))
    parser_current_lineno = atoi (arg);
  else
    bprintf_insert (page, start, "%d", parser_current_lineno);

  xfree (arg);
}

#if defined (__cplusplus)
}
#endif
