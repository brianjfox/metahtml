/* mktrans.c: -*- C -*-  DESCRIPTIVE TEXT. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sat Mar 16 16:37:34 1996.  */

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
#include "gd.h"

int
main (int argc, char *argv[])
{
  register int i = 1;
  int transcol = -2;

  while (i < argc)
    {
      char *arg = argv[i++];

      if (strcmp (arg, "-i") == 0)
	{
	  transcol = atoi (argv[i++]);
	}
      else
	{
	  FILE *stream = fopen (arg, "r");
	  gdImagePtr image = (gdImagePtr)0;

	  if (stream != (FILE *)NULL)
	    {
	      image = gdImageCreateFromGif (stream);
	      fclose (stream);
	    }
	  else
	    {
	      fprintf (stderr, "Cannot read image: %s\n", arg);
	      continue;
	    }

	  if (image != (gdImagePtr)0)
	    {
	      int color =
		transcol == -2 ? gdImageGetPixel (image, 0, 0) : transcol;

	      gdImageColorTransparent (image, color);

	      stream = fopen (arg, "w");

	      if (stream != (FILE *)NULL)
		{
		  gdImageGif (image, stream);
		  fclose (stream);
		}
	      else
		{
		  fprintf (stderr, "Cannot write image: %s\n", arg);
		}
	    }
	}
    }
  return (0);
}
