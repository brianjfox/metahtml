/* thumbnail.c: -*- C -*-  Return a thumbnail of IMAGE. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Jan 22 08:15:35 1996.  */

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

static void usage (void);
static void calc_aspect (int x_in, int y_in, int dx_in, int *dy_in);
static void cleanup (char *string);
static int parse_hex_pair (char *pair_start);


int
main (int argc, char *argv[])
{
  gdImagePtr source, dest;
  int width = 100, height = 100;
  int constrain_width = 0;
  int constrain_height = 0;
  int do_trans = 0;
  int calc_aspect_only = 0;
  char *file = (char *)NULL;
  char *webargs = getenv ("QUERY_STRING");

  if (webargs)
    {
      register int i;
      char *temp;

      webargs = strdup (webargs);
      cleanup (webargs);

      /* Find the WIDTH parameter. */
      temp = strstr (webargs,  "WIDTH=");
      if (width)
	{
	  constrain_width = 1;
	  width = atoi (temp + 6);
	}

      /* Find the HEIGHT parameter. */
      temp = strstr (webargs, "HEIGHT=");
      if (temp)
	{
	  constrain_height = 1;
	  height = atoi (temp + 7);
	}

      /* Find the FILE parameter. */
      file = strstr (webargs, "FILE=");
      if (!file) usage ();
      file = strdup (file + 5);

      /* Find the end of the filename. */
      for (i = 0; ((file[i] != '\0') && (file[i] != '&')); i++);
      file[i] = '\0';
    }
  else
    {
      int arg_index = 1;

      while (arg_index < argc)
	{
	  char *arg = argv[arg_index++];

	  if ((strcmp (arg, "-w") == 0) ||
	      (strcmp (arg, "--width") == 0))
	    {
	      constrain_width = 1;
	      width = atoi (argv[arg_index++]);
	    }
	  else if ((strcmp (arg, "-h") == 0) ||
		   (strcmp (arg, "--height") == 0))
	    {
	      constrain_height = 1;
	      height = atoi (argv[arg_index++]);
	    }
	  else if ((strcmp (arg, "-t") == 0) ||
		   (strcmp (arg, "--transparent") == 0))
	    {
	      do_trans = 1;
	    }
	  else if ((strcmp (arg, "-a") == 0) ||
		   (strcmp (arg, "--aspect-only") == 0))
	    {
	      calc_aspect_only = 1;
	    }
	  else if (arg[0] != '-')
	    {
	      if (file != (char *)NULL)
		usage ();
	      else
		file = arg;
	    }
	  else
	    usage ();
	}
    }

  if (file == (char *)NULL)
    usage ();

  /* Get the source image. */
  {
    FILE *stream = fopen (file, "r");

    if (!stream)
      {
	perror (file);
	exit (2);
      }

    source = gdImageCreateFromGif (stream);
    fclose (stream);

    if (!source)
      {
	fprintf (stderr, "Couldn't create GIF from %s!\n", file);
	exit (2);
      }
  }

  /* Decide how to constrain the image.  If neither width nor height
     was specified, constrain the height based on the width.  If one
     was specified, constrain the other one.  If both were specified,
     simply forget about keeping the aspect ratio the same. */
  if (!constrain_width && !constrain_height)
    constrain_width = 1;

  if (constrain_width && !constrain_height)
    {
      calc_aspect (gdImageSX (source), gdImageSY (source), width, &height);
    }

  if (constrain_height && !constrain_width)
    {
      calc_aspect (gdImageSY (source), gdImageSX (source), height, &width);
    }

  if (calc_aspect_only)
    {
      fprintf (stdout, "width=%d height=%d\n", width, height);
    }
  else
    {
      int trans = gdImageGetTransparent (source);

      /* Create the destination placeholder. */
      dest = gdImageCreate (width, height);
      gdImageFilledRectangle (dest, 0, 0, width, height, trans);

      gdImageCopyResized (dest, source, 0, 0, 0, 0, width, height,
			  gdImageSX (source), gdImageSY (source));

      if (webargs)
	fprintf (stdout, "Content-Type: image/gif\n\n");

      gdImageGif (dest, stdout);

      if (do_trans)
	gdImageColorTransparent (dest, trans);

      /* Destroy the image in memory. */
      gdImageDestroy (dest);
    }

  gdImageDestroy (source);

  return (0);
}

static void usage (void)
{
  fprintf (stderr, "Usage: thumbnail [--width W] [--height H] file\n");
  exit (1);
}

static void
calc_aspect (int x_in, int y_in, int dx_in, int *dy_in)
{
  double x, y, dx, dy;
  double ratio;

  x = (double) x_in;
  y = (double) y_in;
  dx = (double) dx_in;

  if (y == 0)
    return;

  ratio = x / y;

  if (ratio == 0.0)
    return;

  dy = dx / ratio;

  *dy_in = (int)dy;
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
