/* mkbg.c: -*- C -*-  DESCRIPTIVE TEXT. */

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
#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"

static void parse_rgb (char *string, int *rp, int *gp, int *bp);
static void usage (void);
static void cleanup (char *string);
static int parse_hex_pair (char *pair_start);
static char **args_passed = (char **)NULL;
static int args_passed_len = 0;

int
main (int argc, char *argv[])
{
  gdImagePtr image;
  int total_width = 1000;
  char *left_rgb = "000000", *right_rgb = "FFFFFF";
  int left_width = 80, page_width = 0, right_width = 1200;
  char *separator_rgb = (char *)NULL, *page_rgb = (char *)NULL;
  int separator_width = 2;
  char *webargs = getenv ("QUERY_STRING");
  int height = 4;
  int right_trans = 0;
  int left_trans = 0;
  char *temp;
  int r, g, b, bg, fg;

  args_passed = argv;
  args_passed_len = argc;

  if (webargs)
    {
      register int i;

      webargs = strdup (webargs);
      cleanup (webargs);

      /* Find the TOTAL-WIDTH parameter. */
      temp = strstr (webargs,  "TOTAL-WIDTH=");
      if (temp)
	total_width = atoi (temp + 12);

      /* Find the LEFT-WIDTH parameter. */
      temp = strstr (webargs,  "LEFT-WIDTH=");
      if (temp)
	left_width = atoi (temp + 11);

      /* Find the RIGHT-WIDTH parameter. */
      temp = strstr (webargs,  "RIGHT-WIDTH=");
      if (temp)
	right_width = atoi (temp + 12);

      /* Check for left or right colors. */
      temp = strstr (webargs, "LEFT-RGB=");
      if (temp)
	{
	  if (temp[9] == '#')
	    temp++;

	  temp = strdup (temp + 9);
	  for (i = 0; temp[i] != '\0' && temp[i] != '\0'; i++);
	    temp[i] = '\0';

	  left_rgb = temp;
	}

      temp = strstr (webargs, "RIGHT-RGB=");
      if (temp)
	{
	  if (temp[10] == '#')
	    temp++;

	  temp = strdup (temp + 10);
	  for (i = 0; temp[i] != '\0' && temp[i] != '\0'; i++);
	    temp[i] = '\0';

	  right_rgb = temp;
	}

      /* Find the LEFT-TRANS parameter. */
      temp = strstr (webargs,  "LEFT-TRANS=");
      if (temp)
	left_trans = 1;

      /* Find the RIGHT-TRANS parameter. */
      temp = strstr (webargs,  "RIGHT-TRANS=");
      if (temp)
	right_trans = 1;

      /* Find out if the image should be given a separator line where the
	 borders connect. */
      temp = strstr (webargs, "SEPARATOR-RGB=");
      if (temp)
	{
	  if (temp[10] == '#')
	    temp++;

	  temp = strdup (temp + 10);
	  for (i = 0; temp[i] != '\0' && temp[i] != '\0'; i++);
	    temp[i] = '\0';

	  separator_rgb = temp;
	}
    }
  else
    {
      int arg_index = 1;

      while (arg_index < argc)
	{
	  char *arg = argv[arg_index++];

	  if ((strcmp (arg, "-lw") == 0) ||
	      (strcmp (arg, "--left-width") == 0))
	    left_width = atoi (argv[arg_index++]);
	  else if ((strcmp (arg, "-rw") == 0) ||
		   (strcmp (arg, "--right-width") == 0))
	    right_width = atoi (argv[arg_index++]);
	  else if ((strcmp (arg, "-pw") == 0) ||
		   (strcmp (arg, "--page-width") == 0))
	    page_width = atoi (argv[arg_index++]);
	  else if ((strcmp (arg, "-lt") == 0) ||
		   (strcmp (arg, "--left-trans") == 0))
	    left_trans = 1;
	  else if ((strcmp (arg, "-rt") == 0) ||
		   (strcmp (arg, "--right-trans") == 0))
	    right_trans = 1;
	  else if ((strcmp (arg, "-rrgb") == 0) ||
		   (strcmp (arg, "--right-rgb") == 0))
	    {
	      temp = argv[arg_index++];
	      if (*temp == '#') temp++;
	      right_rgb = strdup (temp);
	    }
	  else if ((strcmp (arg, "-lrgb") == 0) ||
		   (strcmp (arg, "--left-rgb") == 0))
	    {
	      temp = argv[arg_index++];
	      if (*temp == '#') temp++;
	      left_rgb = strdup (temp);
	    }
	  else if ((strcmp (arg, "-prgb") == 0) ||
		   (strcmp (arg, "--page-rgb") == 0))
	    {
	      temp = argv[arg_index++];
	      if (*temp == '#') temp++;
	      page_rgb = strdup (temp);
	    }
	  else if ((strcmp (arg, "-srgb") == 0) ||
		   (strcmp (arg, "--separator-rgb") == 0))
	    {
	      temp = argv[arg_index++];
	      if (*temp == '#') temp++;
	      separator_rgb = strdup (temp);
	    }
	  else if ((strcmp (arg, "-swidth") == 0) ||
		   (strcmp (arg, "--separator-width") == 0))
	    {
	      temp = argv[arg_index++];
	      separator_width = atoi (temp);
	    }
	  else if ((strcmp (arg, "-h") == 0) ||
		   (strcmp (arg, "--height") == 0))
	    height = atoi (argv[arg_index++]);
	  else
	    usage ();
	}
    }

  /* Create the image. */
  image = gdImageCreate (left_width + page_width + right_width, height);

  parse_rgb (left_rgb, &r, &g, &b);
  bg = gdImageColorAllocate (image, r, g, b);
  parse_rgb (right_rgb, &r, &g, &b);
  fg = gdImageColorAllocate  (image, r, g, b);

  gdImageFilledRectangle (image, 0, 0, left_width, height, bg);
  gdImageFilledRectangle (image, left_width, 0,
			  left_width + page_width + right_width, height, fg);

  if (separator_rgb != (char *)NULL)
    {
      int separator;

      parse_rgb (separator_rgb, &r, &g, &b);
      separator = gdImageColorAllocate (image, r, g, b);
      gdImageFilledRectangle (image, left_width - separator_width, 0,
			      left_width, height, separator);
    }

  if (page_width != 0)
    {
      if (page_rgb != (char *)NULL)
	{
	  int pagecol;

	  parse_rgb (page_rgb, &r, &g, &b);
	  pagecol = gdImageColorAllocate (image, r, g, b);
	  gdImageFilledRectangle (image, left_width, 0,
				  (left_width + page_width - 4),
				  height, pagecol);
	}

      if (separator_rgb != (char *)NULL)
	{
	  int separator;

	  parse_rgb (separator_rgb, &r, &g, &b);
	  separator = gdImageColorAllocate (image, r, g, b);
	  gdImageFilledRectangle (image, left_width + page_width - 4, 0,
				  left_width + page_width,
				  height, separator);
	}
    }

  if (left_trans)
    gdImageColorTransparent (image, bg);
  else if (right_trans)
    gdImageColorTransparent (image, fg);

  if (webargs)
    fprintf (stdout, "Content-Type: image/gif\n\n");

  gdImageGif (image, stdout);

  /* Destroy the image in memory. */
  gdImageDestroy (image);

  return (0);
}

static void usage (void)
{
  register int i;
  fprintf (stderr, "Usage: mkbg --left-width 80 --right-width 920 --left-rgb '#000000' --right-rgb '#FFFFFF'\n");
  
  fprintf (stderr, "Args you passed: ");
  for (i = 1; i < args_passed_len; i++)
    fprintf (stderr, "%s ", args_passed[i]);
  fprintf (stderr, "\n");

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
