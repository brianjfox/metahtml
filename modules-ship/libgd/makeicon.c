/* make-icon.c: -*- C -*-  Write some text into a button image. */

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
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontt.h"
#include "gdfontl.h"
#include "gdfontg.h"

#define CENTER 0
#define LEFT   1
#define RIGHT  2
#define EXACT  3

static void parse_rgb (char *string, int *rp, int *gp, int *bp);
static void usage (void);
static void cleanup (char *string);
static int parse_hex_pair (char *pair_start);
static int text_position = CENTER;
static int text_xpos = 10;
static int text_ypos = 10;

int
main (int argc, char *argv[])
{
  gdImagePtr image;
  char *image_filename = (char *)NULL;
  char *label = (char *)NULL;
  char *fg_rgb = (char *)NULL;
  char *bg_rgb = (char *)NULL;
  char *text_rgb = (char *)NULL;
  int r, g, b;
  int transparent = 0;
  char *webargs = getenv ("QUERY_STRING");
  char *temp;

  if (webargs)
    {
      register int i;

      webargs = strdup (webargs);
      cleanup (webargs);

      /* Find the TEXT part of the string. */
      label = strstr (webargs, "LABEL=");
      if (!label) usage ();
      label = strdup (label + 6);

      /* Find the end of the text. */
      for (i = 0; ((label[i] != '\0') && (label[i] != '&')); i++);
      label[i] = '\0';

      /* Find the FILENAME part of the string. */
      temp = strstr (webargs, "FILENAME=");
      if (!temp) usage ();
      image_filename = strdup (temp + 9);

      /* Find the end of the filename. */
      for (i = 0; ((image_filename[i] != '\0') &&
		   (image_filename[i] != '&')); i++);
      image_filename[i] = '\0';

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

      /* Check for background transparent. */
      temp = strstr (webargs, "TRANSPARENT=");
      if (temp) transparent = 1;
    }
  else
    {
      int arg_index = 1;

      while (arg_index < argc)
	{
	  char *arg = argv[arg_index++];

	  if ((strcmp (arg, "-fg") == 0) || (strcmp (arg, "--fg") == 0))
	    {
	      temp = argv[arg_index++];
	      if (temp[0] == '#')
		temp++;

	      fg_rgb = strdup (temp);
	    }
	  else if ((strcmp (arg, "-bg") == 0) || (strcmp (arg, "--bg") == 0))
	    {
	      temp = argv[arg_index++];
	      if (temp[0] == '#')
		temp++;

	      bg_rgb = strdup (temp);
	    }
	  else if ((strcmp (arg, "-trgb") == 0) ||
		   (strcmp (arg, "--text-rgb") == 0))
	    {
	      temp = argv[arg_index++];
	      if (temp[0] == '#')
		temp++;

	      text_rgb = strdup (temp);
	    }
	  else if ((strcmp (arg, "-i") == 0) || (strcmp (arg, "--image") == 0))
	    {
	      image_filename = argv[arg_index++];
	    }
	  else if (arg[0] != '-')
	    {
	      static int label_size = 0;
	      static int label_index = 0;
	      int l = strlen (arg);

	      if (l + label_index + 4 > label_size)
		label = (char *)realloc (label, (label_size += (64 + l)));

	      if (label_index != 0)
		label[label_index++] = ' ';

	      strcpy (label + label_index, arg);
	      label_index += l;
	      label[label_index] = '\0';
	    }
	  else
	    usage ();
	}
    }

  if (!image_filename)
    usage ();
  else
    {
      /* Create the image. */
      FILE *stream = fopen (image_filename, "r");

      if (!stream)
	{
	  fprintf (stderr, "Error reading %s for input!\n", image_filename);
	  usage ();
	}
      image = gdImageCreateFromGif (stream);
      fclose (stream);
    }

  if (!image)
    {
      fprintf (stderr, "Couldn't get a gif out of %s!\n", image_filename);
      usage ();
    }

  if (bg_rgb)
    {
      parse_rgb (bg_rgb, &r, &g, &b);
      image->red[0] = r;
      image->green[0] = g;
      image->blue[0] = b;
    }

  if (fg_rgb)
    {
      parse_rgb (fg_rgb, &r, &g, &b);
      image->red[1] = r;
      image->green[1] = g;
      image->blue[1] = b;
    }

  if (label != (char *)NULL)
    {
      int width = gdImageSX (image);
      int height = gdImageSY (image);
      int text = 1;
      gdFontPtr f = gdFontMediumBold;

      if (text_rgb)
	{
	  parse_rgb (text_rgb, &r, &g, &b);
	  text = gdImageColorAllocate (image, r, g, b);
	}
      else
	text = 1;

      switch (text_position)
	{
	case CENTER:
	  if (width < height)
	    gdImageStringUp
	      (image, f, ((width / 2) - (f->h / 2)), height - 4, label, text);
	  else
	    gdImageString
	      (image, f, 20, ((height / 2) - (f->h / 2)), label, text);
	  break;

	case RIGHT:
	  {
	    
	  }
	break;

	case LEFT:
	  {
	  }
	break;

	case EXACT:
	  gdImageString (image, f, text_xpos, text_ypos, label, text);
	  break;
	}
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
  fprintf (stderr, "Usage: make-icon --image BASEIMAGE -fg COLOR -bg COLOR -trgb COLOR [Words for Label]\n");
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
