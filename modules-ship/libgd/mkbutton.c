/* mkbutton.c: -*- C -*-  Make a button image. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Thu Jun 20 17:20:15 1996.  */

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
#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"

static void parse_rgb (char *string, int *rp, int *gp, int *bp);
static void usage (void);
static void cleanup (char *string);
static int parse_hex_pair (char *pair_start);

int
main (int argc, char *argv[])
{
  gdImagePtr image;
  int bg, fg;
  char *bg_rgb = "7070e0";
  char *fg_rgb = "000000";
  int r, g, b;
  int width = 100, height = 10;
  char *text = (char *)NULL;
  char *webargs = getenv ("QUERY_STRING");

  if (webargs)
    {
      register int i;
      char *temp;

      webargs = strdup (webargs);
      cleanup (webargs);

      /* Find the WIDTH parameter. */
      temp = strstr (webargs,  "WIDTH=");
      if (!temp) usage ();
      width = atoi (temp + 6);

      /* Find the HEIGHT parameter. */
      temp = strstr (webargs, "HEIGHT=");
      if (!temp) usage ();
      height = atoi (temp + 7);

      /* Find the TEXT part of the string. */
      text = strstr (webargs, "TEXT=");
      if (!text) usage ();
      text = strdup (text + 5);

      /* Find the end of the text. */
      for (i = 0; ((text[i] != '\0') && (text[i] != '&')); i++);
      text[i] = '\0';

      /* Check for fg or bg colors. */
      temp = strstr (webargs, "FG=");
      if (temp)
	{
	  if (temp[3] == '#')
	    temp++;

	  temp = strdup (temp + 3);
	  for (i = 0; temp[i] != '\0' && temp[i] != '\0'; i++);
	    temp[i] = '\0';

	  fg_rgb = temp;
	}

      temp = strstr (webargs, "BG=");
      if (temp)
	{
	  if (temp[3] == '#')
	    temp++;

	  temp = strdup (temp + 3);
	  for (i = 0; temp[i] != '\0' && temp[i] != '\0'; i++);
	    temp[i] = '\0';

	  bg_rgb = temp;
	}

      if ((width == 0) || (height == 0))
	usage ();
    }
  else
    {
      int arg_index = 0;

      while (arg_index < argc)
	{
	  char *arg = argv[arg_index++];

	  if ((strcmp (arg, "-w") == 0) || (strcmp (arg, "--width") == 0))
	    width = atoi (argv[arg_index++]);
	  else if ((strcmp (arg, "-h") == 0) ||
		   (strcmp (arg, "--height") == 0))
	    height = atoi (argv[arg_index++]);
	  else if (arg[0] != '-')
	    {
	      static int text_size = 0;
	      static int text_index = 0;
	      int l = strlen (arg);

	      if (l + text_index + 4 > text_size)
		text = (char *)realloc (text, (text_size += (200 + l)));

	      if (text_index != 0)
		text[text_index++] = ' ';

	      strcpy (text + text_index, arg);
	      text_index += l;
	      text[text_index] = '\0';
	    }
	  else
	    usage ();
	}
    }

  /* Create the image. */
  image = gdImageCreate (width, height);

  parse_rgb (bg_rgb, &r, &g, &b);
  bg = gdImageColorAllocate (image, r, g, b);
  parse_rgb (fg_rgb, &r, &g, &b);
  fg = gdImageColorAllocate  (image, r, g, b);

  if (text != (char *)NULL)
    {
      gdFontPtr f = gdFontMediumBold;

      if (width < height)
	gdImageStringUp
	  (image, f, ((width / 2) - (f->h / 2)), height - 4, text, fg);
      else
	gdImageString (image, f, 4, ((height / 2) - (f->h / 2)), text, fg);
    }

  if (webargs)
    fprintf (stdout, "Content-Type: image/gif\n\n");

  gdImageGif (image, stdout);

  /* Destroy the image in memory. */
  gdImageDestroy (image);

  return (0);
}

static void usage (void)
{
  fprintf (stderr, "Usage: makebar width height [text]\n");
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

/* Do the `%FF' and `+' hacking on string.  We can do this hacking in
   place, since the resultant string cannot be longer than the input
   string. */
static void
cleanup (char *string)
{
  register int i, j, len;
  char *dest;

  len = strlen (string);
  dest = (char *)alloca (1 + len);

  for (i = 0, j = 0; i < len; i++)
    {
      switch (string[i])
	{
	case '%':
	  dest[j++] = parse_hex_pair (string + i + 1);
	  i += 2;
	  break;

	case '+':
	  dest[j++] = ' ';
	  break;

	default:
	  dest[j++] = string[i];
	}
    }

  dest[j] = '\0';
  strcpy (string, dest);
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
