/* keymaps.c -- Functions and keymaps for the GNU Readline library. */

/* Copyright (C) 1988,1989 Free Software Foundation, Inc.

   This file is part of GNU Readline, a library for reading lines
   of text with interactive input and history editing.

   Readline is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 1, or (at your option) any
   later version.

   Readline is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline; see the file COPYING.  If not, write to the Free
   Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */
#define READLINE_LIBRARY

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdlib.h>
#include "rlconf.h"
#include "keymaps.h"
#include "emacs_keymap.c"

#if defined (VI_MODE)
#include "vi_keymap.c"
#endif

#if defined (__cplusplus)
extern "C"
{
#endif
extern int rl_do_lowercase_version (int, int);
extern int rl_rubout (int, int), rl_insert (int, int);

extern char *xmalloc (int), *xrealloc (void *, int);

/* **************************************************************** */
/*								    */
/*		      Functions for manipulating Keymaps.	    */
/*								    */
/* **************************************************************** */


/* Return a new, empty keymap.
   Free it with free() when you are done. */
Keymap
rl_make_bare_keymap (void)
{
  register int i;
  Keymap keymap = (Keymap)xmalloc (KEYMAP_SIZE * sizeof (KEYMAP_ENTRY));

  for (i = 0; i < KEYMAP_SIZE; i++)
    {
      keymap[i].type = ISFUNC;
      keymap[i].function = (Function *)NULL;
    }

  for (i = 'A'; i < ('Z' + 1); i++)
    {
      keymap[i].type = ISFUNC;
      keymap[i].function = (Function *)rl_do_lowercase_version;
    }

  return (keymap);
}

/* Return a new keymap which is a copy of MAP. */
Keymap
rl_copy_keymap (Keymap map)
{
  register int i;
  Keymap temp = rl_make_bare_keymap ();

  for (i = 0; i < KEYMAP_SIZE; i++)
    {
      temp[i].type = map[i].type;
      temp[i].function = map[i].function;
    }
  return (temp);
}

/* Return a new keymap with the printing characters bound to rl_insert,
   the uppercase Meta characters bound to run their lowercase equivalents,
   and the Meta digits bound to produce numeric arguments. */
Keymap
rl_make_keymap (void)
{
  register int i;
  Keymap newmap;

  newmap = rl_make_bare_keymap ();

  /* All ASCII printing characters are self-inserting. */
  for (i = ' '; i < 127; i++)
    newmap[i].function = (Function *)rl_insert;

  newmap[TAB].function = (Function *)rl_insert;
  newmap[RUBOUT].function = (Function *)rl_rubout;
  newmap[CTRL('H')].function = (Function *)rl_rubout;

#if KEYMAP_SIZE > 128
  /* Printing characters in some 8-bit character sets. */
  for (i = 128; i < 160; i++)
    newmap[i].function = (Function *)rl_insert;

  /* ISO Latin-1 printing characters should self-insert. */
  for (i = 160; i < 256; i++)
    newmap[i].function = (Function *)rl_insert;
#endif /* KEYMAP_SIZE > 128 */

  return (newmap);
}

/* Free the storage associated with MAP. */
void
rl_discard_keymap (Keymap map)
{
  int i;

  if (!map)
    return;

  for (i = 0; i < KEYMAP_SIZE; i++)
    {
      switch (map[i].type)
	{
	case ISFUNC:
	  break;

	case ISKMAP:
	  rl_discard_keymap ((Keymap)map[i].function);
	  break;

	case ISMACR:
	  free ((char *)map[i].function);
	  break;
	}
    }
}

#if defined (STATIC_MALLOC)

/* **************************************************************** */
/*								    */
/*			xmalloc and xrealloc ()		     	    */
/*								    */
/* **************************************************************** */

static void memory_error_and_abort (void);

static char *
xmalloc (int bytes)
{
  char *temp = (char *)malloc (bytes);

  if (!temp)
    memory_error_and_abort ();
  return (temp);
}

static char *
xrealloc (char *pointer, int bytes)
{
  char *temp;

  if (!pointer)
    temp = (char *)malloc (bytes);
  else
    temp = (char *)realloc (pointer, bytes);

  if (!temp)
    memory_error_and_abort ();
  return (temp);
}

static void
memory_error_and_abort (void)
{
  fprintf (stderr, "readline: Out of virtual memory!\n");
  abort ();
}
#endif /* STATIC_MALLOC */

#if defined (__cplusplus)
}
#endif
