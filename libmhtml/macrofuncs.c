/* macrofuncs.c: -*- C -*-  Functions which allow the definition of
   Meta-HTML functions and macros. */

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

static void pf_define_container (PFunArgs);
static void pf_define_tag (PFunArgs);
static void pf_define_function (PFunArgs);
static void pf_defweakmacro (PFunArgs);
static void pf_undef (PFunArgs);

/************************************************************/
/*							    */
/*		   Macro Manipulation Functions		    */
/*							    */
/************************************************************/

static PFunDesc func_table[] =
{
  { "DEFINE-CONTAINER",	1, 0, pf_define_container },
  { "DEFINE-TAG",	1, 0, pf_define_tag },
  { "DEFINE-FUNCTION",	1, 0, pf_define_function },
  { "DEFMACRO",		1, 0, pf_define_container },
  { "DEFSUBST",		1, 0, pf_define_tag },
  { "DEFUN",		1, 0, pf_define_function },
  { "DEFWEAKMACRO",	1, 0, pf_defweakmacro },
  { "UNDEF",		0, 0, pf_undef },

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_macro_functions)
DEFINE_SECTION (MACRO-COMMANDS, macros; functions; new operators, 
"<meta-html> contains a powerful <i>macro</i> facility, which allows
you to define your own commands.  Such commands are first-class
objects in <meta-html>; they may even supersede the compiled in
definitions.

There are two types of macros that you can define.  One type is a
<i>complex-tag</i>; it consists of an opening tag, a body, and a
closing tag.  The other type is a <i>simple-tag</i>; it only has an
opening tag.

You create a macro by using one of the macro-defining commands.  In
the body of the definition, special keywords can be placed, which
affect what is produced when the macro is invoked.  As a macro writer,
you have logical access to the arguments passed to the macro in the
opening tag, and for complex-tags, you have access to the body which
appears between the opening and closing tags.  ",

"In the opening tag of the macro defining command, several special
<i>meta-arguments</i> may be used to affect the binding method used at
invocation time to bind the passed parameters to the formal arguments.

<ul> <li> <b>&optional</b><br> Indicates that the following named
parameter is optional, and does not have to be supplied.  While at
this time Meta-HTML does not complain if there are missing arguments
at invocation time, it is likely that the byte-compiler will require
function calls to match the formal parameters of the defined function.

<example>
<defun func x &optional y> <get-var x>, <get-var y> </defun>
</example>

<li> <b>&key</b><br> Indicates that the following named parameters
will be bound by the caller placing the name of the parameter followed
by an equals sign, and the value of that parameter in the opening tag
of the function call.  Thus, keyword arguments may appear in any order
in the calling function.  Here is an example of defining a tag called
<tag image> which will add the width and hieght if they are not
already present:

<example>
<defun image &key src width height>
  <if <or <not <get-var width>>
	  <not <get-var height>>>
      <find-image-xy <get-var src> width height>>
  <img src=\"<get-var src>\" width=<get-var width> height=<get-var height>>
</defun>
</example>

<li> <b>&rest</b><br> Gobbles up any remaining arguments to the
function, collecting them in the named parameter which follows the
<i>&rest</i>.  The arguments may be gathered into a single string, or
into an array, with one argument per slot.  This is controlled by
writing the formal parameter name either with or without sqaure
braces: (i.e., <code>foo[]</code> or <code>foo</code>).

<example>
<defun func req-arg &rest rest-args[]>
  <ol>
    <foreach x rest-args>
      <li> <get-var x> </li>
    </foreach>
  </ol>
</defun>
</example>

<li> <b>&body</b><br> Causes the following named parameter to be bound
to the body of the invoked function or macro.  For <tag defun> and
<tag defsubst>, this is all of the material which appeared in the
opening tag, while for <tag defmacro> and <tag defweakmacro>, this is
all of the material that appeared between the opening and closing
tags.

<example>
<defmacro with-debugging-output &body body>
  <with-open-stream debug-stream /tmp/debug-output mode=append>
    <stream-put debug-stream <get-var body>>
  </with-open-stream>
</defmacro>
</example>

<li> <b>&unevalled</b><br> Modifies the binding rule of a formal
parameter such that the material which is bound is not evaluated
before the binding takes place.  This is almost equivalent to using
the <b>%0</b> ... <b>%9</b>, or <b>%body</b> textual substitutions,
but the arguments are bound to variables instead of pure textual
substitution.  Here is how one might write a function which takes an
expression, and produces the expression and the evaluation of the
expression as output:

<example>
<defun debug-expr &body &unevalled qbody &body body>
  <get-var-once qbody> EVALS TO: <get-var-once body>
</defun>
</example>

Such an invocation might look like:
<example>
  <set-var x=4 y=5>
  <debug-expr <add x y>>
</example>

which would produce:
<example>
  <add x y> EVALS TO: 9
</example>
</ul>

Here is a ridiculous function, which uses all of the special
meta-parameters:
<example>
<defsubst func req &optional opt &key k1 &unevalled k2 &body b &rest args[]>
   REQ: <get-var-once req>,
   OPT: <get-var-once opt>
    K1: <get-var-once k1>
    K2: <get-var-once k2>
  BODY: <get-var-once b>
  REST: <foreach arg args><get-var-once arg> </foreach>
</defsubst>
</example>

And, here are examples of calling that function:

Example 1:
<example>
<set-var key-1-arg=key-1>
<func required k2=\"Unevalled\" opt-arg k1=<get-var key-1-arg> rest0 rest1>
   REQ: required,
   OPT: opt-arg
    K1: key-1
    K2: Unevalled
  BODY: required k2=\"Unevalled\" opt-arg k1=key-1 rest0 rest1
  REST: rest0 rest1
</example>
Example 2:
<example>
<func k2=<get-var k1> required rest0 rest1>
   REQ: required,
   OPT: rest0
    K1: 
    K2: <get-var k1>
  BODY: k2= required rest0 rest1
  REST: rest1
</example>

Notice how in the second example, our optional parameter <b>opt</b>
got bound to the second non-keyword argument <code>rest0</code>!")

DEFMACROX (pf_defsubst, name &optional named-parameters &key
	  package=packname whitespace=delete,
"A synonym for <funref macro-commands define-tag>.")

DEFMACRO (pf_define_tag, name &optional named-parameters &key
	  package=packname whitespace=delete,
"Define <var name> as a simple tag.  Within <var body>, the values of
<code>%0...%9</code> are defined to be the positional arguments that
were found in the opening tag of the invocation, and
<code>%body</code> is all of that material in a single string.

If any <var named-parameter>s are supplied, the values that were
passed in the opening tag are evaluated and bound to the named
parameters.

A keyword argument of <var package-name> wraps the entire body of the
macro in an <funref packages in-package> statement.

The keyword argument <var whitespace> can be set to the string
<code>delete</code> to remove whitespace from the starts and ends of
lines in the subst definition before it is stored.  This effectively
concatenates all of the lines of the subst definition into a single
long line.

Also see <funref macro-commands define-function> and <funref
macro-commands define-container>.")
{
  char *temp = get_positional_arg (vars, 0);
  char *subst_name = mhtml_evaluate_string (temp);
  char *subst_body = body ? body->buffer : "";

  if (!empty_string_p (subst_name))
    mhtml_add_user_function (user_SUBST, subst_name, subst_body, vars);

  if (subst_name && subst_name != temp)
    free (subst_name);
}

DEFMACROX (pf_defmacro, name &optional named-parameters
	  &key package=packname whitespace=delete,
"A synonym for <funref macro-commands define-container>.")

DEFMACRO (pf_define_container, name &optional named-parameters
	  &key package=packname whitespace=delete,
 "Define <var name> as a complex tag. At invocation time, various
substitutions are made within <var body>.  Specifically, if the text
string is:

<ul>
<li><b>%0</b>,<b>%1</b>, and so on, upto <b>%9</b> are replaced
with the exact text of the positional arguments that were found in
the opening tag of the invocation

<li><b>%attributes</b> is replaced by all of the arguments which
appeared in the opening tag.

<li><b>%body</b> is replaced with the exact text of the material
that appeared between the opening and closing tags

<li><b>%qbody</b> is similar to <b>%body</b>, but the string is
first surrounded by double quotes, and double quote characters which
appear in the string are escaped.

<li><b>%xbody</b> is replaced with the evaluation of the material
that appeared between the opening and closing tags
</ul>

If any <var named-parameter>s are supplied, the values that were
passed in the opening tag are evaluated and bound to the named
parameters.

A keyword argument of <var package-name> wraps the entire body of the
macro in an <funref packages in-package> statement.

The keyword argument <var whitespace> can be set to the string
<code>delete</code> to remove whitespace from the starts and ends of
lines in the macro definition before it is stored.  This effectively
concatenates all of the lines of the macro definition into a single
long line.")
{
  char *temp = get_positional_arg (vars, 0);
  char *subst_name = mhtml_evaluate_string (temp);
  char *subst_body = body ? body->buffer : "";

  if (!empty_string_p (subst_name))
    mhtml_add_user_function (user_MACRO, subst_name, subst_body, vars);

  if (subst_name && subst_name != temp)
    free (subst_name);
}

DEFMACRO (pf_defweakmacro, name  &optional named-parameters
	  &key package=packname whitespace=delete,
"<code>defweakmacro</code> is exactly like <funref macro-commands
define-container>, with one exception: at invocation time, the closing
tag does not have to be present -- in that case, the invocation is
treated as if the definition were a <funref macro-commands defsubst>.

This facility exists primarily to allow the redefinition of standard
HTML constructs which allow the closing tag to be missing, and yet,
still inexplicably operate correctly.

For example, the <example code><p></example> tag is often used without
its closing counterpart of <example code></p></example>.  If you
wished to redefine <example code><p></example> to do something special
when a closing tag was found, you might write the following
definition:

<example>
<defweakmacro p>
  <verbatim><P></verbatim>
  <when %qbody> Look ma! %body See? </when>
  <verbatim></P></verbatim>
</defweakmacro>
</example>

then, a simple <example code><P></example> would produce
<example code><P></P></example code>, while a complex invocation, such as:
<example>
<P> this is a list </P>
</example>
produces
<example>
  <P> Look ma!  this is a list See? </P>
</example>")
{
  char *temp = get_positional_arg (vars, 0);
  char *subst_name = mhtml_evaluate_string (temp);
  char *subst_body = body ? body->buffer : "";

  if (!empty_string_p (subst_name))
    {
      UserFunction *uf;
      mhtml_add_user_function (user_MACRO, subst_name, subst_body, vars);
      uf = mhtml_find_user_function (subst_name);
      uf->flags |= user_WEAK_MACRO;
    }

  if (subst_name && subst_name != temp)
    free (subst_name);
}

DEFMACROX (pf_defun, name  &optional named-parameters
	  &key package=packname whitespace=delete,
"A synonym for <funref macro-commands define-function>.")

DEFMACRO (pf_define_function, name  &optional named-parameters
	  &key package=packname whitespace=delete,
"Define <var name> as a simple tag.

The only differences between <funref macro-commands define-function>
and <funref macro-commands define-tag> are:
<ol>
<li> The <i>whitespace=delete</i> option is assumed.
<li> The <var named-parameter>s are evaluated in the context of the
caller, not of the definition of the defun.
<li> By default, a local package is wrapped around the invocation of
the defined function.  This can be changed by the use of the <var
package=packname> keyword.
</ol>

<example>
<define-function factorial num>
   <if <lt num 2> 1
      <mul num <factorial <sub num 1>>>>
</define-function>
.blank
<factorial 5> --> 120
</example>")
{
  char *temp = get_positional_arg (vars, 0);
  char *subst_name = mhtml_evaluate_string (temp);
  char *subst_body = body ? body->buffer : "";

  if (!empty_string_p (subst_name))
    mhtml_add_user_function (user_DEFUN, subst_name, subst_body, vars);

  if (subst_name && subst_name != temp)
    free (subst_name);
}

DEFUN (pf_undef, &optional name...,
"Remove the definition of a user-defined <funref macros defun>,
<funref macros defmacro> or <funref macros defsubst>.  For every <var
name> that has been defined in this way, the definition is removed.")
{
  register int i;
  char *name;

  if (!mhtml_user_keywords)
    return;

  for (i = 0; (name = get_positional_arg (vars, i)) != (char *)NULL; i++)
    {
      char *varname = name;

      varname = mhtml_evaluate_string (name);

      if (varname)
	{
	  UserFunction *uf = mhtml_find_user_function (varname);
	  Symbol *sym;

	  if (uf)
	    {
	      make_gcable ((char *)uf->body);
	      make_gcable ((char *)uf->name);
	      make_gcable ((char *)uf);
	    }

	  sym = symbol_remove_in_package (mhtml_user_keywords, varname);

	  if (sym)
	    {
	      sym->values= (char **)NULL;
	      sym->values_index = 0;
	      symbol_free (sym);
	    }

	  free (varname);
	}
    }
}

#if defined (__cplusplus)
}
#endif
