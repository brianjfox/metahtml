/* gifq.c: -*- C -*-  Query a gif about various parmeters. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jun 19 12:05:42 1996.  */

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

static void usage (char *);
extern int errno;

int
main (int argc, char *argv[])
{
  FILE *stream;
  gdImagePtr image;
  int arg_index = 2;

  if (argc < 3)
    usage ("Less than 3 arguments");

  if ((stream = fopen (argv[1], "r")) == (FILE *)NULL)
    usage (strerror (errno));

  image = gdImageCreateFromGif (stream);
  fclose (stream);

  if (!image)
    usage ("Couldn't create internal image");

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if (strcasecmp (arg, "width") == 0)
	fprintf (stdout, "%d\n", gdImageSX (image));
      else if (strcasecmp (arg, "height") == 0)
	fprintf (stdout, "%d\n", gdImageSY (image));
      else if (strcasecmp (arg, "transparent") == 0)
	fprintf (stdout, "%d\n", gdImageGetTransparent (image));
      else if (strcasecmp (arg, "interlaced") == 0)
	fprintf (stdout, "%d\n", gdImageGetInterlaced (image));
      else if (strcasecmp (arg, "border-width") == 0)
	{
	  register int x;
	  int start_pixel = gdImageGetPixel (image, 0, 0);

	  for (x = 1; x < gdImageSX (image); x++)
	    if (gdImageGetPixel (image, x, 0) != start_pixel)
	      break;

	  fprintf (stdout, "%d\n", x);
	}
      else if (strcasecmp (arg, "colors") == 0)
	{
	  register int i;

	  for (i = 0; i < gdImageColorsTotal (image); i++)
	    fprintf (stdout, "#%02X%02X%02X ",
		     gdImageRed (image, i),
		     gdImageGreen (image, i),
		     gdImageBlue (image, i));
	  fprintf (stdout, "\n");
	}
      else
	usage ("Incorrect argument passed");
    }

  /* Destroy the image in memory. */
  gdImageDestroy (image);

  return (0);
}

static void
usage (char *optional_message)
{
  fprintf (stderr, "Usage: gifq IMAGE [width | height | transparent | interlaced | border-width | colors]...\n");
  if (optional_message != (char *)NULL)
    fprintf (stderr, "Message: %s\n", optional_message);
  exit (1);
}

