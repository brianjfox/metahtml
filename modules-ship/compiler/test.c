/* test.c: -*- C -*-  */

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

#include "compiler/compile.h"

/* extern void initialize_compiler_tags (Package *package); */

extern void mh_tag_disassemble (mh_tag_t tag, FILE *file);

extern mh_object_t mh_welcome_to_the_machine (mh_tag_t     tag,
					      mh_object_t *args,
					      size_t       args_count);

extern mh_object_t
mh_welcome_to_the_machine_with_time (mh_tag_t        tag,
				     mh_object_t    *args,
				     size_t          args_count,
				     struct timeval *tm);

extern void 
mh_parse_install (void);

extern void
mh_parse_show (mh_parse_t parse);

extern void
mh_expand_install (void);

extern void 
mh_core_show (mh_core_t core);

static double
timeval_to_seconds (struct timeval *tm)
{
  return (tm->tv_sec + ((double) tm->tv_usec) / 1e6);
}

extern void
mh_machine_fset_function_install (mh_tag_t tag)
{
}

void
mh_repl (void)
{
  char buf [1024];
  mh_tag_t tag;
  mh_object_t    result;

  mh_parse_t    parse    = MH_PARSE_EMPTY;
  mh_core_t     core_exp = MH_CORE_NULL;
  mh_core_t     core_opt = MH_CORE_NULL;

  struct timeval compile_time;
  struct timeval machine_time;

  fprintf (stdout, "\nMBC: Interactive Byte-Compiler for Meta-HTML\n");
  fprintf (stdout, "Dingo Down!\n");

  do 
    {
      fprintf (stdout, "(mbc) ");
      fflush (stdout);

      if (NULL == fgets (buf, 1024, stdin))
	break;

      /* Avoid "" input; Axe the trailing '\n' */
      {
	unsigned int size = strlen (buf);
	if (size == 1)
	  continue;
	else if ('\n' == buf[size-1])
	  buf[size-1]=0;
      }

      tag = mh_compile_with_intermediates_and_time
	(mh_string_new (buf, 0),
	 &parse, &core_exp, &core_opt,
	 &compile_time);

      if (parse)
	{
	  printf ("\n\nParse:");
	  mh_parse_show (parse);
	}

      if (core_exp)
	{
	  printf ("\n\nCore:");
	  mh_core_show (core_exp);
	}

      if (core_opt)
	{
	  printf ("\n\nCore (Optimized):");
	  mh_core_show (core_opt);
	  printf ("\n");
	}

      if (!tag)
	{
	  fprintf (stdout, "\"\"\n");
	  continue;
	}

      
      /* Disassemble */
      mh_tag_disassemble (tag, stdout);

      /* And the 'evaluator' */
      result = mh_welcome_to_the_machine_with_time
	(tag, (mh_object_t *) NULL, 0, &machine_time);

      fprintf (stdout, "\n;; Compile: %.3f, Machine: %.3f (ms)\n",
	       1000 * timeval_to_seconds (&compile_time),
	       1000 * timeval_to_seconds (&machine_time));
      mh_object_write (result, true, stdout);
      fprintf (stdout, "\n");
    }
  while (1);

  puts ("Dingo Done!\n");
}

/*
 *
 *
 *
 */

/************************************************************************
 *
 * 
 *
 */
#include "machine/parse.h"

extern void
mh_parse_show (mh_parse_t parse);

extern void
mh_parse_and_show (string_t str)
{
  mh_parse_t parse;
  string_t   parse_to_str;
  
  printf ("\n\nTest Parse : \"%s\"\nTest Result:", str);
  parse = mh_parse ("test", str);
  if (MH_PARSE_EMPTY != parse)
    {
      mh_parse_show (parse);

      parse_to_str = mh_parse_to_string (parse);
      printf ("\nTest String: \"%s\"\n",  parse_to_str);
      if (0 != strcmp (str, parse_to_str))
	{
	  printf
	    ("\n\n!!!!!!!! String and Parse_to_String differ !!!!!!!!!!\n\n");
	}
      free (parse_to_str);
    }
}

static string_t parse_tests[] =
{
  "<defun foo>abc</defun>",
  "<abc def",
  "abc[\"HTML]def",
  "abc['HTML\"]def",
  "abc['HTML']def",
  "abc[\"HTML\"]def",
  "This is a <get-var test> of [\"MetaHTML\"].",
  "Here is another ;;; a comment\nexample.",
  "<defun foo>abc</defun>",
  "<defun foo><set-var bar=\"10\">The cost is <get-var bar> dollars.</defun>",
  "one <get-var <get-var foo>  > three",
  "one <concat <get-var foo> abc <get-var bar>  > three",
  "<get-var foo>",
  "<get-var <get-var bar>>",
  "<abc>",
  "<abc def>",
  "<abc> def",
  "<abc def",
  "<abc <hij> def>",
  "<  abc <hij> def  >",
  "<abc <hij def>",
  "abc <def <xyz> <>> hij",
  "abc <def <xyz><>> hij",
  "<<<<abc>>>>",
  "<abc>>>>",
  "abc def",
  "abc <get-var def> hij",
  "abc <def>hij<<xyz foo  =   123> zzz>555",
  "abc <def <hij <klm",
  "1.2 3.4 5.6",
  "<add 0 1.2 3.4 5.6 7>",
  NULL
};

static string_t compile_tests[] =
{
  "<if <get-var xyx> true false>",
  "<if <get-var xyx> \"true\" \"false\">",
  "<if \"abc\" <get-var xyz>>",
  "<if \"abc\" <not xyz>>",
  "<prog \"abc\" <not xyz>>",
  "<set-var xyz=1><and <get-var xyz> \"abc\">",
  "<or \"abc\" <get-var xyz>>",
#if 0
  "<set-var xyz=1><while <get-var xyz>>abc<set-var xyz=\"\">def</while>",
  "abc<set-var xyz=1><while <get-var xyz>>abc<abc><set-var xyz=\"\"><def></while>xyz" ,
#endif
  "<when foo>[\"abc\"]</when>",
  "<add \"1.2\" \"3.8\">" ,
  "<add 1.2 3.8>" ,
  "<set-var abc=1.2><add abc 3.8>",
  "<set-var num=3.8><add 1.2 num>" ,
  "<set-var num=8.8><add 1.2 <get-var num>>" ,
  "<random \"10\">" ,
  "<downcase \"Ed Gamble Jr.\">" ,
  "<set-var who=\"Ed Gamble Jr.\"><downcase who>" ,
  "<get-var foo bar>" ,
  "012 <random \"10\"> 345",
  "012 <random 10> 345",
  "012 <br> 345",
  "<defun FOO xyz>abc <get-var xyz></defun>",
  "<defun FOO xyz abc>abc <get-var xyz></defun>",
  "<defun FOO xyz &unevalled abc &key key &rest rst>abc <get-var xyz></defun>",
#if 0
  "123<get-var xyz><defun FOO xyz>abc <get-var xyz></defun><FOO \"def\">",
#endif
  "<make-alist foo=10 bar=20>",
  "<alist-get-var <make-alist bar=10> bar>",
  "<alist-set-var <make-alist bar=10> bar=100>",
  "<set-var xxx=<make-alist bar=10>><alist-set-var xxx bar=100><get-var xxx>",
  "<set-var default::www=10><get-var default::www>",
  "<set-var tester::www=10><get-var tester::www>",
  "<set-var tester::www=10 www=100><get-var tester::www www>",
#if 0
  "<defun xxx &key abc><get-var abc></defun><xxx abc=10>",
#endif
  "<defun xxx &key abc><get-var abc[]></defun>",
#if 0
  "<alist-set-var pack::foo bar=100>",
#endif
  NULL
};

extern void 
mh_compile_tests_install (void)
{
#if 0
  MH_SYMBOL_VALUE (mh_symbol_intern ("xyz")) = 
    "value-of-symbol-'xyz'";

  MH_SYMBOL_VALUE (mh_symbol_intern ("val")) = 
    "The-value-of-Symbol-'val'";

  MH_SYMBOL_VALUE (mh_symbol_intern ("num")) = "3.14159";

  /* Super Hack */
  MH_SYMBOL_VALUE (mh_symbol_intern ("false")) = "false";
#endif
}

void mh_compile_from_parse (mh_string_t string)
{
  mh_core_t     core;
  mh_tag_t tag;
  mh_parse_t    actual_parse;

  printf ("
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

COMPILE: \n  \"%s\"", MH_STRING_CHARS (string));

  printf ("\n\nParse:");
  actual_parse = mh_parse ("main", MH_STRING_CHARS (string));
  mh_parse_show (actual_parse);

  /* Use the automatic parse */
  core = mh_expand ("main", actual_parse);
  printf ("\n\nCore:");
  mh_core_show (core);
  
  mh_optimize (core);
  printf ("\n\nCore (Optimized):");
  mh_core_show (core);
  printf ("\n");

  tag = mh_generate (core);

  /* Disassemble */
  mh_tag_disassemble (tag, stdout);
  
  {
    mh_object_t object =
      mh_welcome_to_the_machine (tag, (mh_object_t *) NULL, 0);

    mh_object_write (object, true, stdout);
    printf ("\n");
  }

}

extern int
main (void)
{
  string_t *tests;

  mh_object_init ();
  mh_expand_install ();

  tests = parse_tests;
  while (*tests)
    mh_parse_and_show (*tests++);

  mh_compile_tests_install ();
  tests = compile_tests;
  while (*tests)
    mh_compile_from_parse (mh_string_new (*tests++, 0));

  mh_repl();
  return 1;
}


