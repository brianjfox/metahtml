/* modstat.c: -*- C -*-  Module handles combinations, permutations.. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Sun Aug  9 09:28:03 1998.

    This file is part of <Meta-HTML>(tm), a system for the rapid
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

#include "modules.h"

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_create_combination (PFunArgs);
static void pf_next_combination (PFunArgs);
static void pf_free_combination (PFunArgs);

static PFunDesc ftab[] =
{
  { "STAT::CREATE-COMBINATION",	0, 0, pf_create_combination },
  { "STAT::NEXT-COMBINATION",	0, 0, pf_next_combination },
  { "STAT::FREE-COMBINATION",	0, 0, pf_free_combination },
  { (char *)NULL,		0, 0, (PFunHandler *)NULL }
};

MODULE_INITIALIZE ("modstat", ftab)
DEFINE_SECTION (STATISTICS-MODULE, statistics;permutation;combination,
"Functions for generating combinations and permutations, one by one.", "")

typedef struct
{
  char **src_set;		/* The source of the elements is an array. */
  int *v, *y, *z;
  int sset_slots;		/* Number of slots allocated. */
  int dset_slots;		/* Number of slots allocated. */
  int sset_index;		/* Number supplied as input. */
  int dset_index;		/* Number desired for output. */
  int start_new;
} COMBINATION;

static void
free_combination (COMBINATION *c)
{
  if (c != (COMBINATION *)NULL)
    {
      if (c->src_set != (char **)NULL)
	symbol_free_array (c->src_set);

      xfree (c->v); xfree (c->y); xfree (c->z);
    }

  free (c);
}

static COMBINATION *
create_combination (char **array)
{
  register int i;
  COMBINATION *c = (COMBINATION *)xmalloc (sizeof (COMBINATION));

  memset (c, 0, sizeof (c));

  for (i = 0; ((array != (char **)NULL) && (array[i] != (char *)NULL)); i++);
  c->sset_slots = i;
  c->src_set = (char **)xmalloc ((1 + c->sset_slots) * sizeof (char *));
  c->v = (int *)xmalloc ((1 + c->sset_slots) * sizeof (int *));
  c->y = (int *)xmalloc ((1 + c->sset_slots) * sizeof (int *));
  c->z = (int *)xmalloc ((1 + c->sset_slots) * sizeof (int *));

  for (i = 0; ((array != (char **)NULL) && (array[i] != (char *)NULL)); i++)
    c->src_set[i] = strdup (array[i]);

  c->src_set[i] = (char *)NULL;
  c->sset_index = i;
  c->start_new = 1;

  return (c);
}

static char **
next_combination (COMBINATION *c)
{
  char **dest = (char **)NULL;

  if (c->dset_index > c->sset_index)
    return (dest);
  else
    {
      register int i, j;

      if (c->start_new)
	{
	  c->start_new = 0;
	  c->dset_index++;

	  if (c->dset_index > c->sset_index)
	    return (dest);

	  for (i = 0; i < c->sset_slots; i++)
	    c->v[i] = c->y[i] = c->z[i] = 0;

	  for (i = 0; i < c->dset_index; i++)
	    {
	      c->y[i] = c->v[i] = i;
	      c->z[i] = c->sset_index - (c->dset_index - i);
	    }
	}

      /* Here is a combination. */
      dest = (char **)xmalloc ((1 + (c->dset_index)) * sizeof (char *));
      for (i = 0; i < c->dset_index; i++)
	dest[i] = c->src_set[c->v[i]];
      dest[i] = (char *)NULL;

      /* Make the next combination. */
      for (i = c->dset_index - 1; i >= 0; i--)
	if (c->v[i] < c->z[i])
	  break;

      if (i < 0)
	c->start_new = 1;
      else
	{
	  int stx = c->v[i] + 1;
	  for (j = i; j < c->dset_index; j++)
	    {
	      c->v[j] = stx;
	      stx++;
	    }
	}
    }
  return (dest);
}

DEFUN (pf_create_combination, &key mincol=x &rest args,
"Create a combination structure, and return a pointer to it.")
{
  COMBINATION *c = (COMBINATION *)NULL;

  if (get_positional_arg (vars, 0) != (char *)NULL)
    {
      register int i;
      char **array = (char **)NULL;
      char *arg;

      i = 0;
      while ((arg = get_positional_arg (vars, i)) != (char *)NULL) i++;
      array = (char **)xmalloc ((1 + i) * sizeof (char *));

      i = 0;
      while ((arg = get_positional_arg (vars, i)) != (char *)NULL)
	{
	  array[i] = mhtml_evaluate_string (arg);
	  i++;
	}

      array[i] = (char *)NULL;

      c = create_combination (array);

      /* Hey, did the user supply a mincol=x keyword argument? */
      {
	char *mincol_arg = mhtml_evaluate_string (get_value (vars, "mincol"));
	if (!empty_string_p (mincol_arg))
	  c->dset_index = atoi (mincol_arg) - 1;
	if (c->dset_index < 0)
	  c->dset_index = c->sset_index;

	xfree (mincol_arg);
      }

      /* Return the address of this combination pointer. */
      bprintf_insert (page, start, "%0lX", c);
    }
}

DEFUN (pf_next_combination, combination,
"Return the next combination of <var combination>, or the empty array if\n\
there are no more combinations to process.")
{
  char *address_string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  COMBINATION *c;
  char **array = (char **)NULL;

  if (!empty_string_p (address_string))
    {
      char *endptr;
      c = (COMBINATION *)strtoul (address_string, &endptr, 16);
      if (*endptr == '\0')
	array = next_combination (c);
    }

  if (array != (char **)NULL)
    {
      register int i;

      for (i = 0; array[i] != (char *)NULL; i++)
	{
	  int l = strlen (array[i]);
	  bprintf_insert (page, start, "%s\n", array[i]);
	  start += l + 1;
	}
      free (array);
      *newstart = start;
    }
}

DEFUN (pf_free_combination, combination,
"Free any memory associated with <var combination>, which must be a value\n\
returned from <tag stat::create-combination>.")
{
  char *address_string = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (address_string))
    {
      COMBINATION *c;
      char *endptr;

      c = (COMBINATION *)strtoul (address_string, &endptr, 16);
      if (*endptr == '\0')
	{
	  free_combination (c);
	  bprintf_insert (page, start, "true");
	  *newstart += 4;
	}
    }
}

  
