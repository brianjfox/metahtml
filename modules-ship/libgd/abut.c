/* abut.c: -*- C -*-  Abut images, vertically, or horizontally. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sat May  3 06:30:24 1997.  */

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
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>
#include "gd.h"

#define whitespace(x) ((x) == ' ' || (x) == '\t' || (x) == '\n')
#define HORIZONTAL 1
#define VERTICAL   2

static int orientation = HORIZONTAL;
static int pixel_spacing = 0;

static gdImagePtr *input_images = (gdImagePtr *)NULL;
static int input_images_index = 0;
static int input_images_slots = 0;

static char *rotated = (char *)NULL;
static char *separator_rgb = (char *)NULL;

static void parse_rgb (char *string, int *rp, int *gp, int *bp);
static void cleanup (char *string);
static int parse_hex_pair (char *pair_start);
static void memory_error_and_abort (char *fname);
static void decode_args (char *string, int *argcp, char ***argvp);

/* The name of this program, as taken from argv[0]. */
static char *progname = (char *)NULL;

/* Becomes non-zero when passed QUERY_STRING from the web. */
static int called_from_web_server = 0;

/* Return a pointer to free()able block of memory large enough
   to hold BYTES number of bytes.  If the memory cannot be allocated,
   print an error message and abort. */
static void *
xmalloc (unsigned int bytes)
{
  void *temp = (void *)malloc (bytes);

  if (!temp)
    memory_error_and_abort ("xmalloc");
  return (temp);
}

static void *
xrealloc (void *pointer, unsigned int bytes)
{
  void *temp;

  if (!pointer)
    temp = (void *)malloc (bytes);
  else
    temp = (void *)realloc (pointer, bytes);

  if (!temp)
    memory_error_and_abort ("xrealloc");
  return (temp);
}

static void
memory_error_and_abort (char *fname)
{
  fprintf (stderr, "%s: Out of virtual memory!\n", fname);
  abort ();
}


static void
usage (int status)
{
  fprintf (stderr,
	   "Usage: %s [--vertical --horizontal --spacing] gif-file...\n",
	   progname);

  if (status)
    exit (status);
}


static void
file_error (char *filename)
{
  fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror (errno));
  exit (1);
} 

static void
parse_program_arguments (int argc, char *argv[])
{
  register int arg_index = 1;
  char *webargs = getenv ("PATH_INFO");

  progname = argv[0];

  if (!webargs || !*webargs)
    webargs = getenv ("QUERY_STRING");

  if (webargs && *webargs)
    {
#if defined (DEBUG)
      fprintf (stderr, "Webargs = `%s'\n", webargs);
#endif
      called_from_web_server = 1;
      decode_args (webargs, &argc, &argv);
    }

#if defined (DEBUG)
  {
    register int i;

    fprintf (stderr, "Arglist: ");
    for (i = 0; i < argc; i++)
      fprintf (stderr, "`%s' ", argv[i]);
    fprintf (stderr, "\n");
  }
#endif

  while (arg_index < argc)
    {
      char *arg = argv[arg_index++];

      if (strcasecmp (arg, "--help") == 0)
	{
	  usage (0);
	  exit (0);
	}
      else if ((strcmp (arg, "-v") == 0) || (strcmp (arg, "--vertical") == 0))
	{
	  orientation = VERTICAL;
	  continue;
	}
      else if ((strcmp (arg, "-h") == 0) ||
	       (strcmp (arg, "--horizontal") == 0))
	{
	  orientation = HORIZONTAL;
	  continue;
	}
      else if ((strcmp (arg, "-r") == 0) || (strcmp (arg, "--rotate") == 0))
	{
	  rotated = argv[arg_index++];
	  continue;
	}
      else if ((strcmp (arg, "-s") == 0) || (strcmp (arg, "--spacing") == 0))
	{
	  pixel_spacing = atoi (argv[arg_index++]);
	  continue;
	}
      else if (strcmp (arg, "-srgb") == 0)
	{
	  char *temp = argv[arg_index++];
	  if (*temp == '#') temp++;
	  separator_rgb = strdup (temp);
	  continue;
	}
      else if (strncmp (arg, "--", 2) != 0)
	{
	  FILE *stream = fopen (arg, "rb");

	  if (stream != (FILE *)NULL)
	    {
	      gdImagePtr image = gdImageCreateFromGif (stream);

	      if (image != (gdImagePtr)NULL)
		{
		  if (input_images_index + 2 > input_images_slots)
		    input_images = (gdImagePtr *)
		      xrealloc
		      (input_images,
		       (input_images_slots += 20) * sizeof (gdImagePtr));

		  input_images[input_images_index++] = image;
		}
	      else
		{
		  fclose (stream);
		  file_error (arg);
		}
	    }
	  else
	    file_error (arg);
	}
      else
	usage (1);
    }
}

static void
gif_return (gdImagePtr gif)
{
  FILE *out = stdout;

  if (called_from_web_server)
    {
#if defined (DEBUG)
      fprintf (stderr, "Called from Web Server!\n");
#endif /* DEBUG */
      fprintf (out, "Expires: Monday, 05-Feb-96 15:04:14 GMT\n");
      fprintf (out, "Content-Type: image/gif\n");
    }

  gdImageGif (gif, out);
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

static void
decode_args (char *string, int *argcp, char ***argvp)
{
  char *temp;

  /* Find the WEBARGS parameter. */
  temp = strchr (string, '=');
  if (temp)
    {
      register int i;
      int argc = *argcp;
      int slots = argc + 10;
      char **argv = (char **)xmalloc (slots * sizeof (char *));

      temp++;
      temp = strdup (temp);
      cleanup (temp);

      /* Now split the string into arguments at whitespace. */
      i = 0;
      while (temp[i])
	{
	  int start = i;
	  char *arg;

	  while (temp[i] != '\0' && !whitespace (temp[i])) i++;
	  arg = (char *)xmalloc (1 + (i - start));
	  strncpy (arg, temp + start, (i - start));
	  arg[i - start] = '\0';

	  if (argc + 2 > slots)
	    argv = (char **)xrealloc (argv, ((slots += 10) * sizeof (char *)));

	  argv[argc++] = arg;
	  argv[argc] = (char *)NULL;

	  while (whitespace (temp[i])) i++;
	}

      *argcp = argc;
      *argvp = argv;
    }
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

  if (strchr (string, ',') != (char *)NULL)
    {
      /* This is RGB in decimal, values separated by commas. */
      sscanf (string, "%d,%d,%d", rp, gp, bp);
    }
  else
    {
      /* This is RGB in hex. */

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
}

static gdImagePtr
rotate_image (gdImagePtr image, int degrees)
{
  register int x, y;
  int width = gdImageSX (image);
  int height = gdImageSY (image);

  if (degrees == -90)  degrees = 270;
  if (degrees == -270) degrees = 90;
  if (degrees == -180) degrees = 180;

  if (degrees < 90)
    return (image);
  else if (degrees == 180)
    {
      gdImagePtr rot = gdImageCreate (width, height);

      for (x = 0; x < width; x++)
	for (y = 0; y < height; y++)
	    {
	      int pixel = gdImageGetPixel (image, x, y);
	      int r = gdImageRed (image, pixel);
	      int g = gdImageGreen (image, pixel);
	      int b = gdImageBlue (image, pixel);
	      int newcol;

	      newcol = gdImageColorExact (rot, r, g, b);
	      if (newcol < 0) newcol = gdImageColorAllocate (rot, r, g, b);
	      gdImageSetPixel (rot, width - x, height - y, newcol);
	    }
      return (rot);
    }
  else
    {
      gdImagePtr rot = gdImageCreate (height, width);

      for (x = 0; x < width; x++)
	for (y = 0; y < height; y++)
	    {
	      int pixel = gdImageGetPixel (image, x, y);
	      int r = gdImageRed (image, pixel);
	      int g = gdImageGreen (image, pixel);
	      int b = gdImageBlue (image, pixel);
	      int newcol;

	      newcol = gdImageColorExact (rot, r, g, b);
	      if (newcol < 0) newcol = gdImageColorAllocate (rot, r, g, b);

	      if (degrees == 90)
		gdImageSetPixel (rot, height - y, x, newcol);
	      else
		gdImageSetPixel (rot, y, width - x, newcol);
	    }
	return (rot);
      }
}

int
main (int argc, char *argv[])
{
  /* Get the args. */
  parse_program_arguments (argc, argv);

  if (input_images_index == 0)
    usage (1);
  else
    {
      register int i;
      gdImagePtr base_image;
      int max_width = 0, max_height = 0;
      int last_offset = 0;
      int r, g, b, trans;
      int sr, sg, sb;
      int separator = -1;

      for (i = 0; i < input_images_index; i++)
	{
	switch (orientation)
	  {
	  case HORIZONTAL:
	    max_width += gdImageSX (input_images[i]);
	    if (i) max_width += pixel_spacing;

	    if (gdImageSY (input_images[i]) > max_height)
	      max_height = gdImageSY (input_images[i]);
	    break;

	  case VERTICAL:
	    if (gdImageSX (input_images[i]) > max_width)
	      max_width = gdImageSX (input_images[i]);

	    max_height += gdImageSY (input_images[i]);
	    if (i) max_height += pixel_spacing;
	  break;
	  }
	}

      base_image = gdImageCreate (max_width, max_height);
      r = 254, g = 254, b = 254;
      trans = gdImageColorAllocate (base_image, r, g, b);
      gdImageFilledRectangle (base_image, 0, 0, max_width, max_height, trans);
      gdImageColorTransparent (base_image, trans);

      if (separator_rgb != (char *)NULL)
	{
	  parse_rgb (separator_rgb, &sr, &sg, &sb);
	  separator = gdImageColorAllocate (base_image, sr, sg, sb);
	}

      /* Okay, we have an image which is as large as the largest image,
	 and which is filled in with a completely transparent color.
	 Start overlaying the images onto this base. */
      for (i = 0; i < input_images_index; i++)
	{
	  gdImagePtr image = input_images[i];
	  register int start_x = 0, start_y = 0;
	  register int x, y;
	  int xlimit = gdImageSX (image);
	  int ylimit = gdImageSY (image);

	  switch (orientation)
	    {
	    case HORIZONTAL:
	      start_y = (max_height - gdImageSY (image)) / 2;
	      start_x = last_offset;
	      last_offset += gdImageSX (image) + pixel_spacing;

	      if (i != 0 && separator != -1)
		gdImageFilledRectangle
		  (base_image,
		   (start_x - ((pixel_spacing / 2) + 1)), 0,
		   (start_x - ((pixel_spacing / 2) - 1)), max_height,
		   separator);

	      break;

	    case VERTICAL:
	      start_x = (max_width - gdImageSX (image)) / 2;
	      start_y = last_offset;
	      last_offset += gdImageSY (image) + pixel_spacing;

	      if (i != 0 && separator != -1)
		gdImageFilledRectangle
		  (base_image,
		   0, (start_y - ((pixel_spacing / 2) + 1)),
		   max_width, (start_y - ((pixel_spacing / 2) - 1)),
		   separator);

	      break;
	    }

	  for (y = 0; y < ylimit; y++)
	    for (x = 0; x < xlimit; x++)
	      {
		int pixel_color = gdImageGetPixel (image, x, y);
		int base_color =
		  gdImageColorExact (base_image,
				     gdImageRed (image, pixel_color),
				     gdImageGreen (image, pixel_color),
				     gdImageBlue (image, pixel_color));

		if (base_color == -1)
		  base_color =
		    gdImageColorAllocate (base_image,
					  gdImageRed (image, pixel_color),
					  gdImageGreen (image, pixel_color),
					  gdImageBlue (image, pixel_color));
		/* Set pixel here if not transparent color. */
		if (base_color != trans)
		  gdImageSetPixel (base_image, start_x + x, start_y + y,
				   base_color);
	      }
	}

      if (rotated)
	base_image = rotate_image (base_image, atoi (rotated));

      gif_return (base_image);
    }
  return (0);
}

