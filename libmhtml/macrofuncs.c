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
"<meta-html> contains a powerful <i>macro</i> facility, which allows\n\
you to define your own commands.  Such commands are first-class\n\
objects in <meta-html>; they may even supersede the compiled in\n\
definitions.\n\
\n\
There are two types of macros that you can define.  One type is a\n\
<i>complex-tag</i>; it consists of an opening tag, a body, and a\n\
closing tag.  The other type is a <i>simple-tag</i>; it only has an\n\
opening tag.\n\
\n\
You create a macro by using one of the macro-defining commands.  In\n\
the body of the definition, special keywords can be placed, which\n\
affect what is produced when the macro is invoked.  As a macro writer,\n\
you have logical access to the arguments passed to the macro in the\n\
opening tag, and for complex-tags, you have access to the body which\n\
appears between the opening and closing tags.  ",

"In the opening tag of the macro defining command, several special\n\
<i>meta-arguments</i> may be used to affect the binding method used at\n\
invocation time to bind the passed parameters to the formal arguments.\n\
\n\
<ul> <li> <b>&optional</b><br> Indicates that the following named\n\
parameter is optional, and does not have to be supplied.  While at\n\
this time Meta-HTML does not complain if there are missing arguments\n\
at invocation time, it is likely that the byte-compiler will require\n\
function calls to match the formal parameters of the defined function.\n\
\n\
<example>\n\
<defun func x &optional y> <get-var x>, <get-var y> </defun>\n\
</example>\n\
\n\
<li> <b>&key</b><br> Indicates that the following named parameters\n\
will be bound by the caller placing the name of the parameter followed\n\
by an equals sign, and the value of that parameter in the opening tag\n\
of the function call.  Thus, keyword arguments may appear in any order\n\
in the calling function.  Here is an example of defining a tag called\n\
<tag image> which will add the width and hieght if they are not\n\
already present:\n\
\n\
<example>\n\
<defun image &key src width height>\n\
  <if <or <not <get-var width>>\n\
	  <not <get-var height>>>\n\
      <find-image-xy <get-var src> width height>>\n\
  <img src=\"<get-var src>\" width=<get-var width> height=<get-var height>>\n\
</defun>\n\
</example>\n\
\n\
<li> <b>&rest</b><br> Gobbles up any remaining arguments to the\n\
function, collecting them in the named parameter which follows the\n\
<i>&rest</i>.  The arguments may be gathered into a single string, or\n\
into an array, with one argument per slot.  This is controlled by\n\
writing the formal parameter name either with or without sqaure\n\
braces: (i.e., <code>foo[]</code> or <code>foo</code>).\n\
\n\
<example>\n\
<defun func req-arg &rest rest-args[]>\n\
  <ol>\n\
    <foreach x rest-args>\n\
      <li> <get-var x> </li>\n\
    </foreach>\n\
  </ol>\n\
</defun>\n\
</example>\n\
\n\
<li> <b>&body</b><br> Causes the following named parameter to be bound\n\
to the body of the invoked function or macro.  For <tag defun> and\n\
<tag defsubst>, this is all of the material which appeared in the\n\
opening tag, while for <tag defmacro> and <tag defweakmacro>, this is\n\
all of the material that appeared between the opening and closing\n\
tags.\n\
\n\
<example>\n\
<defmacro with-debugging-output &body body>\n\
  <with-open-stream debug-stream /tmp/debug-output mode=append>\n\
    <stream-put debug-stream <get-var body>>\n\
  </with-open-stream>\n\
</defmacro>\n\
</example>\n\
\n\
<li> <b>&unevalled</b><br> Modifies the binding rule of a formal\n\
parameter such that the material which is bound is not evaluated\n\
before the binding takes place.  This is almost equivalent to using\n\
the <b>%0</b> ... <b>%9</b>, or <b>%body</b> textual substitutions,\n\
but the arguments are bound to variables instead of pure textual\n\
substitution.  Here is how one might write a function which takes an\n\
expression, and produces the expression and the evaluation of the\n\
expression as output:\n\
\n\
<example>\n\
<defun debug-expr &body &unevalled qbody &body body>\n\
  <get-var-once qbody> EVALS TO: <get-var-once body>\n\
</defun>\n\
</example>\n\
\n\
Such an invocation might look like:\n\
<example>\n\
  <set-var x=4 y=5>\n\
  <debug-expr <add x y>>\n\
</example>\n\
\n\
which would produce:\n\
<example>\n\
  <add x y> EVALS TO: 9\n\
</example>\n\
</ul>\n\
\n\
Here is a ridiculous function, which uses all of the special\n\
meta-parameters:\n\
<example>\n\
<defsubst func req &optional opt &key k1 &unevalled k2 &body b &rest args[]>\n\
   REQ: <get-var-once req>,\n\
   OPT: <get-var-once opt>\n\
    K1: <get-var-once k1>\n\
    K2: <get-var-once k2>\n\
  BODY: <get-var-once b>\n\
  REST: <foreach arg args><get-var-once arg> </foreach>\n\
</defsubst>\n\
</example>\n\
\n\
And, here are examples of calling that function:\n\
\n\
Example 1:\n\
<example>\n\
<set-var key-1-arg=key-1>\n\
<func required k2=\"Unevalled\" opt-arg k1=<get-var key-1-arg> rest0 rest1>\n\
   REQ: required,\n\
   OPT: opt-arg\n\
    K1: key-1\n\
    K2: Unevalled\n\
  BODY: required k2=\"Unevalled\" opt-arg k1=key-1 rest0 rest1\n\
  REST: rest0 rest1\n\
</example>\n\
Example 2:\n\
<example>\n\
<func k2=<get-var k1> required rest0 rest1>\n\
   REQ: required,\n\
   OPT: rest0\n\
    K1: \n\
    K2: <get-var k1>\n\
  BODY: k2= required rest0 rest1\n\
  REST: rest1\n\
</example>\n\
\n\
Notice how in the second example, our optional parameter <b>opt</b>\n\
got bound to the second non-keyword argument <code>rest0</code>!")

DEFMACROX (pf_defsubst, name &optional named-parameters &key
	  package=packname whitespace=delete,
"A synonym for <funref macro-commands define-tag>.")

DEFMACRO (pf_define_tag, name &optional named-parameters &key
	  package=packname whitespace=delete,
"Define <var name> as a simple tag.  Within <var body>, the values of\n\
<code>%0...%9</code> are defined to be the positional arguments that\n\
were found in the opening tag of the invocation, and\n\
<code>%body</code> is all of that material in a single string.\n\
\n\
If any <var named-parameter>s are supplied, the values that were\n\
passed in the opening tag are evaluated and bound to the named\n\
parameters.\n\
\n\
A keyword argument of <var package-name> wraps the entire body of the\n\
macro in an <funref packages in-package> statement.\n\
\n\
The keyword argument <var whitespace> can be set to the string\n\
<code>delete</code> to remove whitespace from the starts and ends of\n\
lines in the subst definition before it is stored.  This effectively\n\
concatenates all of the lines of the subst definition into a single\n\
long line.\n\
\n\
Also see <funref macro-commands define-function> and <funref\n\
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
 "Define <var name> as a complex tag. At invocation time, various\n\
substitutions are made within <var body>.  Specifically, if the text\n\
string is:\n\
\n\
<ul>\n\
<li><b>%0</b>,<b>%1</b>, and so on, upto <b>%9</b> are replaced\n\
with the exact text of the positional arguments that were found in\n\
the opening tag of the invocation\n\
\n\
<li><b>%attributes</b> is replaced by all of the arguments which\n\
appeared in the opening tag.\n\
\n\
<li><b>%body</b> is replaced with the exact text of the material\n\
that appeared between the opening and closing tags\n\
\n\
<li><b>%qbody</b> is similar to <b>%body</b>, but the string is\n\
first surrounded by double quotes, and double quote characters which\n\
appear in the string are escaped.\n\
\n\
<li><b>%xbody</b> is replaced with the evaluation of the material\n\
that appeared between the opening and closing tags\n\
</ul>\n\
\n\
If any <var named-parameter>s are supplied, the values that were\n\
passed in the opening tag are evaluated and bound to the named\n\
parameters.\n\
\n\
A keyword argument of <var package-name> wraps the entire body of the\n\
macro in an <funref packages in-package> statement.\n\
\n\
The keyword argument <var whitespace> can be set to the string\n\
<code>delete</code> to remove whitespace from the starts and ends of\n\
lines in the macro definition before it is stored.  This effectively\n\
concatenates all of the lines of the macro definition into a single\n\
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
"<code>defweakmacro</code> is exactly like <funref macro-commands\n\
define-container>, with one exception: at invocation time, the closing\n\
tag does not have to be present -- in that case, the invocation is\n\
treated as if the definition were a <funref macro-commands defsubst>.\n\
\n\
This facility exists primarily to allow the redefinition of standard\n\
HTML constructs which allow the closing tag to be missing, and yet,\n\
still inexplicably operate correctly.\n\
\n\
For example, the <example code><p></example> tag is often used without\n\
its closing counterpart of <example code></p></example>.  If you\n\
wished to redefine <example code><p></example> to do something special\n\
when a closing tag was found, you might write the following\n\
definition:\n\
\n\
<example>\n\
<defweakmacro p>\n\
  <verbatim><P></verbatim>\n\
  <when %qbody> Look ma! %body See? </when>\n\
  <verbatim></P></verbatim>\n\
</defweakmacro>\n\
</example>\n\
\n\
then, a simple <example code><P></example> would produce\n\
<example code><P></P></example code>, while a complex invocation, such as:\n\
<example>\n\
<P> this is a list </P>\n\
</example>\n\
produces\n\
<example>\n\
  <P> Look ma!  this is a list See? </P>\n\
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
"Define <var name> as a simple tag.\n\
\n\
The only differences between <funref macro-commands define-function>\n\
and <funref macro-commands define-tag> are:\n\
<ol>\n\
<li> The <i>whitespace=delete</i> option is assumed.\n\
<li> The <var named-parameter>s are evaluated in the context of the\n\
caller, not of the definition of the defun.\n\
<li> By default, a local package is wrapped around the invocation of\n\
the defined function.  This can be changed by the use of the <var\n\
package=packname> keyword.\n\
</ol>\n\
\n\
<example>\n\
<define-function factorial num>\n\
   <if <lt num 2> 1\n\
      <mul num <factorial <sub num 1>>>>\n\
</define-function>\n\
.blank\n\
<factorial 5> --> 120\n\
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
"Remove the definition of a user-defined <funref macros defun>,\n\
<funref macros defmacro> or <funref macros defsubst>.  For every <var\n\
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
