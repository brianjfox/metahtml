/* profiler.c: -*- C -*-  Produce profiling information from Meta-HTML. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon May 12 15:29:13 1997.

    This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use of
    the Meta-HTML language.

    Copyright (c) 1995, 1996, Brian J. Fox (bfox@ai.mit.edu).
    Copyright (c) 1996, Universal Access Inc. (http://www.ua.com).

    Meta-HTML is free software; you can redistribute it and/or modify
    it under the terms of the UAI Free Software License as published
    by Universal Access Inc.; either version 1, or (at your option) any
    later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    UAI Free Software License for more details.

    You should have received a copy of the UAI Free Software License
    along with this program; if you have not, you may obtain one by
    writing to:

    Universal Access Inc.
    129 El Paseo Court
    Santa Barbara, CA
    93101  */

#include "modules.h"

#if defined (METAHTML_PROFILER)

#if defined (__cplusplus)
extern "C"
{
#endif
/* 0) #include any files that are specific to your module. */

/* 1) Declare the functions which implement the Meta-HTML functionality. */
static void pf_profiler_dump (PFunArgs);

/* 2) Create a static table which associates function name, type, debug-flags,
      and address of code for each function. */
static PFunDesc ftab[] =
{
  /*   tag	     complex? debug_level	   code    */
  { "PROFILER::DUMP",	0,	 0,		pf_profiler_dump },
  { (char *)NULL,	0,	 0,		(PFunHandler *)NULL }
};

MODULE_INITIALIZE ("profiler", ftab)

static int
sort_names (const void *elt1, const void *elt2)
{
  Symbol *sym1 = *(Symbol **)elt1;
  Symbol *sym2 = *(Symbol **)elt2;

  return (strcmp (sym1->name, sym2->name));
}

static int
sort_types (const void *elt1, const void *elt2)
{
  Symbol *sym1 = *(Symbol **)elt1;
  Symbol *sym2 = *(Symbol **)elt2;

  if (sym1->type == sym2->type)
    return (strcmp (sym1->name, sym2->name));
  else if (sym1->type == symtype_USERFUN)
    return (1);
  else
    return (-1);
}

static int
sort_times (const void *elt1, const void *elt2)
{
  Symbol *sym1 = *(Symbol **)elt1;
  Symbol *sym2 = *(Symbol **)elt2;
  PROFILE_INFO *info1, *info2;

  if (sym1->type == symtype_USERFUN)
    info1 = ((UserFunction *)(sym1->values))->profile_info;
  else
    info1 = ((PFunDesc *)(sym1->values))->profile_info;

  if (sym2->type == symtype_USERFUN)
    info2 = ((UserFunction *)(sym2->values))->profile_info;
  else
    info2 = ((PFunDesc *)(sym2->values))->profile_info;

  if (info1 && !info2) return (1);
  if (info2 && !info1) return (-1);
  if (!info1 && !info2) return (0);
  return (info1->usecs_spent < info2->usecs_spent);
}

static int
sort_calls (const void *elt1, const void *elt2)
{
  Symbol *sym1 = *(Symbol **)elt1;
  Symbol *sym2 = *(Symbol **)elt2;
  PROFILE_INFO *info1, *info2;

  if (sym1->type == symtype_USERFUN)
    info1 = ((UserFunction *)(sym1->values))->profile_info;
  else
    info1 = ((PFunDesc *)(sym1->values))->profile_info;

  if (sym2->type == symtype_USERFUN)
    info2 = ((UserFunction *)(sym2->values))->profile_info;
  else
    info2 = ((PFunDesc *)(sym2->values))->profile_info;

  if (info1 && !info2) return (1);
  if (info2 && !info1) return (-1);
  if (!info1 && !info2) return (0);

  if (info1->times_called == info2->times_called)
    return (strcmp (sym1->name, sym2->name));

  return (info1->times_called < info2->times_called);
}

static int
sort_expense (const void *elt1, const void *elt2)
{
  Symbol *sym1 = *(Symbol **)elt1;
  Symbol *sym2 = *(Symbol **)elt2;
  PROFILE_INFO *info1, *info2;

  if (sym1->type == symtype_USERFUN)
    info1 = ((UserFunction *)(sym1->values))->profile_info;
  else
    info1 = ((PFunDesc *)(sym1->values))->profile_info;

  if (sym2->type == symtype_USERFUN)
    info2 = ((UserFunction *)(sym2->values))->profile_info;
  else
    info2 = ((PFunDesc *)(sym2->values))->profile_info;

  if (info1 && !info2) return (1);
  if (info2 && !info1) return (-1);
  if (!info1 && !info2) return (0);

  return ((info1->usecs_spent / info1->times_called)
	  <
	  (info2->usecs_spent / info2->times_called));
}

static Symbol **
array_concat (Symbol **array_1, Symbol **array_2, int *length)
{
  register int i, max;
  Symbol **result = (Symbol **)NULL;

  for (max = 0; array_1 && array_1[max]; max++);
  for (i = 0; array_2 && array_2[i]; i++);
  max = max + i + 1;

  result = (Symbol **)xmalloc ((1 + max) * sizeof (Symbol *));
  for (max = 0; array_1 && array_1[max]; max++)
    result[max] = array_1[max];

  for (i = 0; array_2 && array_2[i]; i++)
    result[max++] = array_2[i];

  result[max] = (Symbol *)NULL;
  *length = max;
  return (result);
}

#define sort_NAMES	0x0001
#define sort_TIMES	0x0002
#define sort_CALLS	0x0004
#define sort_TYPES	0x0008
#define sort_EXPENSE	0x0010

typedef int SortFun (const void *, const void *);

typedef struct { int type; SortFun *fun; } SORTER;

static SORTER sorters[] =
{
  { sort_NAMES, sort_names },
  { sort_TIMES, sort_times },
  { sort_CALLS, sort_calls },
  { sort_TYPES, sort_types },
  { sort_EXPENSE, sort_expense },
  { 0, (SortFun *)NULL }
};

static SortFun *
find_sortfun (int type)
{
  register int i;
  SortFun *result = sort_names;

  for (i = 0; sorters[i].fun != (SortFun *)NULL; i++)
    if (sorters[i].type == type)
      {
	result = sorters[i].fun;
	break;
      }

  return (result);
}

static void
profiler_dump (Symbol **symbols, char *filename)
{
  register int i;
  FILE *stream = fopen (filename, "w");
  long times_called = 0;
  double time_spent = 0.0;
  int userfuns = 0, sysfuns = 0;

  if (stream != (FILE *)NULL)
    {
      fprintf (stream, "    Type");
      fprintf (stream, "                       ");
      fprintf (stream, "Function Name");
      fprintf (stream, "  ");
      fprintf (stream, "Called     Per     Tot\n");
      fprintf (stream, "   ----------------------------------");
      fprintf (stream, "-------------------------------------");
      fprintf (stream, "\n");

      if (symbols != (Symbol **)NULL)
	{
	  Symbol *sym;

	  for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	    {
	      PROFILE_INFO *info = (PROFILE_INFO *)NULL;

	      switch (sym->type)
		{
		case symtype_USERFUN:
		  info = ((UserFunction *)(sym->values))->profile_info;
		  break;

		case symtype_FUNCTION:
		  info = ((PFunDesc *)(sym->values))->profile_info;
		}

	      if (info != (PROFILE_INFO *)NULL)
		{
		  double secs = info->usecs_spent / 1.0e6;
		  fprintf (stream, "%10s   %31s: %6ld  %03.06f (%03.06f)\n",
			   (sym->type == symtype_USERFUN) ? "USERFUN" : "BUILTIN",
			   sym->name, info->times_called,
			   secs / info->times_called, secs);
		  times_called += info->times_called;
		  time_spent += secs;
		  if (sym->type == symtype_USERFUN)
		    userfuns++;
		  else
		    sysfuns++;
		}
	    }
	}

      fprintf (stream, "   ----------------------------------");
      fprintf (stream, "------------------------------------");
      fprintf (stream, "\n");

      fprintf (stream, "    %d/%d", userfuns, sysfuns);
      fprintf (stream, "                              ");
      fprintf (stream, "%5d", userfuns + sysfuns);
      fprintf (stream, "   ");
      fprintf (stream, "%6ld", times_called);
      fprintf (stream, "            ");
      fprintf (stream, "%03.06f secs\n", time_spent);

      fclose (stream);
    }
  else
    page_syserr ("Couldn't open %s for writing!\n", filename ? filename : "");
}

DEFINE_SECTION (CODE-PROFILING, ,
"The <b>profiler</b> module allows you to get a detailed listing of the\n\
number of times a particular function has executed, the amount of time\n\
spent within each function, and other information which can help you to\n\
tune your Meta-HTML application, and get it running as fast as possible.\n\
\n\
The use of this module is trivial.  First, place\n\
<example>\n\
<set-var mhtml::profile-functions = true>\n\
</example>\n\
at the top of the page that you would like to profile.\n\
\n\
At the bottom of the page, after the last normal instruction, place\n\
<example>\n\
<profiler::dump /tmp/meta.prof>\n\
</example>\n\
\n\
After visiting the page in a browser, the file <i>/tmp/meta.prog</i> will\n\
contain a human readable dump of the profile information.  Your page won't\n\
perceptibly slow down during loading -- the only indication that profiling\n\
is on is the creation of the output file that you specify.", "")

DEFUNX (pf_profiler::dump, filename &key sort=[names|times|calls|expense],
"Write profiling information to <var filename>.  Unlike C profilers,\n\
the output is quite self-explanatory.\n\
\n\
To try it out, do the following in mdb:\n\
<example>\n\
<set-var mhtml::profile-functions = true>\n\
<set-var x=<make-alist foo=bar bar=<make-alist foo=bar>>>\n\
<profiler::dump /tmp/alist.calls sort=calls>\n\
<profiler::dump /tmp/alist.time sort=time>\n\
<profiler::dump /tmp/alist.expense sort=expense>\n\
</example>\n\
\n\
Known bugs:  The total time doesn't really correctly reflect the amount\n\
of time spent processing the entire page -- the time displayed is additive,\n\
so it is always orders of magnitude larger than it should be.  However, the\n\
time displayed for each function call is correct.")
static void
pf_profiler_dump (PFunArgs)
{
  char *filename = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *sort = mhtml_evaluate_string (get_value (vars, "sort"));
  Symbol **user_funcs = symbol_package_symbols ("*user-functions*");
  Symbol **system_funcs =  symbol_package_symbols ("*meta-html*");
  int length;
  Symbol **functions = array_concat (user_funcs, system_funcs, &length);
  int sort_type = sort_CALLS;

  xfree (user_funcs);
  xfree (system_funcs);

  if (empty_string_p (filename))
    {
      xfree (filename);
      filename = strdup ("/tmp/metahtml-profile-info");
    }

  if (empty_string_p (sort))
    {
      xfree (sort);
      sort = strdup ("calls");
    }

  if (strncasecmp (sort, "name", 4) == 0)
    sort_type = sort_NAMES;

  if (strncasecmp (sort, "time", 4) == 0)
    sort_type = sort_TIMES;

  if (strncasecmp (sort, "expense", 4) == 0)
    sort_type = sort_EXPENSE;

  if (strncasecmp (sort, "call", 4) == 0)
    sort_type = sort_CALLS;

  if (strncasecmp (sort, "type", 4) == 0)
    sort_type = sort_TYPES;

  qsort (functions, length, sizeof (Symbol *), find_sortfun (sort_type));

  profiler_dump (functions, filename);

  xfree (sort);
  xfree (filename);
}
#endif /* METAHTML_PROFILER */

#if defined (__cplusplus)
}
#endif
