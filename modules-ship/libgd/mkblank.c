/* mkblank.c: -*- C -*-  Make a blank image with specified width, height,
   and bgcolor. */

/*  Copyright (c) 1999 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Wed Mar 24 11:56:10 1999.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

#include "gd.h"

static void parse_rgb (char *string, int *rp, int *gp, int *bp);
static void usage (char *);
static int parse_hex_pair (char *pair_start);

int
main (int argc, char *argv[])
{
  gdImagePtr image;
  int bg = 0;
  char *bg_rgb = "000000";
  int r, g, b;
  int width = 80, height = 12;
  int arg_index = 1;

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if ((strcasecmp (arg, "-w") == 0) || (strcasecmp (arg, "--width") == 0))
	width = atoi (argv[arg_index++]);
      else if ((strcasecmp (arg, "-h") == 0) ||
	       (strcasecmp (arg, "--height") == 0))
	height = atoi (argv[arg_index++]);
      else if ((strcasecmp (arg, "-rgb") == 0) ||
	       (strcasecmp (arg, "--rgb") == 0) ||
	       (strcasecmp (arg, "--bgcolor") == 0))
	bg_rgb = strdup (argv[arg_index++]);
      else
	usage (arg);
    }

  /* Create the image. */
  image = gdImageCreate (width, height);

  parse_rgb (bg_rgb, &r, &g, &b);
  bg = gdImageColorAllocate (image, r, g, b);

  gdImageGif (image, stdout);

  /* Destroy the image in memory. */
  gdImageDestroy (image);

  return (0);
}

static void
usage (char *arg)
{
  if (arg != (char *)NULL)
    fprintf (stderr, "mkblank: Don't understand `%s'!\n", arg);
  fprintf (stderr, "Usage: mkblank --width W --height H --rgb RRGGBB\n");
  exit (1);
}

static int
hex_value (int c)
{
  if (islower (c)) c = toupper (c);
  c = c - '0';
  if (c > 9)
    c = 10 + ((c + '0') - 'A');

  return (c);
}

static void
parse_rgb (char *string, int *rp, int *gp, int *bp)
{
  register int i;

  *rp = *gp = *bp = 0;

  for (i = 0; i < 6 && string[i]; i++)
    {
      switch (i)
	{
	case 0: *rp = 16 * hex_value (string[i]); break;
	case 1: *rp |= hex_value (string[i]); break;
	case 2: *gp = 16 * hex_value (string[i]); break;
	case 3: *gp |= hex_value (string[i]); break;
	case 4: *bp = 16 * hex_value (string[i]); break;
	case 5: *bp |= hex_value (string[i]); break;

	default:
	  break;
	}
    }
}

static int
parse_hex_pair (char *pair_start)
{
  int value = 0;
  int char1, char2;

  char1 = char2 = 0;

  char1 = *pair_start;

  if (char1)
    char2 = (pair_start[1]);

  if (isupper (char1))
    char1 = tolower (char1);

  if (isupper (char2))
    char2 = tolower (char2);

  if (isdigit (char1))
    value = char1 - '0';
  else if ((char1 <= 'f') && (char1 >= 'a'))
    value = 10 + (char1 - 'a');

  if (isdigit (char2))
    value = (value * 16) + (char2 - '0');
  else if ((char2 <= 'f') && (char2 >= 'a'))
    value = (value * 16) + (10 + (char2 - 'a'));

  return (value);
}
