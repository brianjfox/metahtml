/* recolor.c: -*- C -*-  DESCRIPTIVE TEXT. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Mar 24 12:09:39 1997.  */

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

static void
usage (void)
{
  fprintf (stderr, "Usage: recolor IMAGE FRFGFB:TRTGTB ...\n");
  exit (1);
}


int
main (int argc, char *argv[])
{
  FILE *stream;
  gdImagePtr image;
  int arg_index = 2;

  if (argc < 2)
    usage ();

  if (argc < 3) exit (0);

  stream = fopen (argv[1], "r");

  if (!stream) usage ();

  image = gdImageCreateFromGif (stream);
  fclose (stream);

  if (!image)
    usage ();

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];
      int from_red, from_green, from_blue;
      int to_red, to_green, to_blue;
      int color;

      sscanf (arg, "%02x%02X%02x:%02x%02x%02x",
	      &from_red, &from_green, &from_blue,
	      &to_red, &to_green, &to_blue);

#if defined (DEBUG)
      fprintf (stderr, "Request: Change %02X%02X%02X to %02X%02X%02X -> ",
	       from_red, from_green, from_blue, to_red, to_green, to_blue);
#endif
      color = gdImageColorExact (image, from_red, from_green, from_blue);
      if (color == -1)
	color = gdImageColorClosest (image, from_red, from_green, from_blue);

#if defined (DEBUG)
      fprintf (stderr, "Color Index %d is actually: %02X%02X%02X to %02X%02X%02X \n",
	       color,
	       image->red[color], image->green[color], image->blue[color],
	       to_red, to_green, to_blue);
#endif
      
      if (color > -1)
	{
	  image->red[color] = to_red;
	  image->green[color] = to_green;
	  image->blue[color] = to_blue;
	}
    }

  stream = fopen (argv[1], "w");
  gdImageGif (image, stream);
  fclose (stream);

  return (0);
}



