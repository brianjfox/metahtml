/* coloring-book.c: -*- C -*-  Given the name of a GIF file, and a
   list of points/rgb values, return the image with those points
   filled to the surrounding border by the RGB value. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Fri Jan 31 09:38:43 1997.  */


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

typedef struct
{
  int x, y;			/* Location to operate on. */
  int r, g, b;			/* Color to stick there. */
} Point;

typedef struct
{
  int count;
  int slots;
  Point **points;
} Pointlist;

static void cleanup (char *string);
static int parse_hex_pair (char *pair_start);
static void memory_error_and_abort (char *fname);
static void decode_args (char *string, int *argcp, char ***argvp);
static void color_in_image (gdImagePtr image, Pointlist *points);

static Point *parse_point (char *text);

/* The name of this program, as taken from argv[0]. */
static char *progname = (char *)NULL;

/* Becomes non-zero when passed QUERY_STRING from the web. */
static int called_from_web_server = 0;

static Pointlist *all_points = (Pointlist *)NULL;
static char *the_gif_filename = (char *)NULL;

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
add_to_pointlist (Point *point, Pointlist **listp)
{
  Pointlist *list = *listp;

  if (list == (Pointlist *)NULL)
    {
      list = (Pointlist *)xmalloc (sizeof (Pointlist));
      memset (list, 0, sizeof (Pointlist));
    }

  if ((list->count + 2) > list->slots)
    list->points = (Point **) xrealloc
    (list->points, (list->slots += 4) * sizeof (Point *));

  list->points[list->count++] = point;
  list->points[list->count] = (Point *)NULL;

  *listp = list;
}

static void
usage (int status)
{
  fprintf (stderr, "Usage: %s GIF --point x,y,r,g,b ...\n", progname);

  if (status)
    exit (status);
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
      char *next= (arg_index < argc) ? (argv[arg_index]) : (char *)NULL;

      if ((strcasecmp (arg, "--point") == 0) || (strcmp (arg, "-p") == 0))
	{
	  if (next == (char *)NULL)
	    usage (1);
	  else
	    {
	      Point *point;

	      point = parse_point (next);

	      add_to_pointlist (point, &all_points);
	      arg_index++;
	    }
	}
      else if (strcasecmp (arg, "--help") == 0)
	{
	  usage (0);
	  exit (0);
	}
      else if (!strncmp (arg, "--", 2) == 0)
	{
	  if (the_gif_filename != (char *)NULL)
	    usage (1);
	  else
	    the_gif_filename = strdup (arg);
	}
      else
	usage (1);
    }
}

static Point *
parse_point (char *args)
{
  register int i, start = 0;
  Point *point = (Point *)xmalloc (sizeof (Point));
  char *text = strdup (args);

  memset (point, 0, sizeof (point));

  for (i = 0; text[i] != '\0'; i++)
    if (text[i] == ',')
      {
	text[i] = ' ';
	start = i + 1;
      }

  sscanf (text, "%d %d", &point->x, &point->y);
  point->r = parse_hex_pair (text + start);
  point->g = parse_hex_pair (text + start + 2);
  point->b = parse_hex_pair (text + start + 4);

  free (text);
  return (point);
}
  
static void
file_error (char *filename)
{
  fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror (errno));
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

  gdImageInterlace (gif, 1);

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

static void
color_in_image (gdImagePtr image, Pointlist *points)
{

  if (image && points)
    {
      register int i;
      int border = gdImageColorClosest (image, 0, 0, 0);

      for (i = 0; i < points->count; i++)
	{
	  Point *point = points->points[i];
	  int color = gdImageColorAllocate
	    (image, point->r, point->g, point->b);

	  gdImageFillToBorder (image, point->x, point->y, border, color);
	}
    }
}
	  
int
main (int argc, char *argv[])
{
  gdImagePtr image;
  FILE *stream;

  /* Get the args. */
  parse_program_arguments (argc, argv);

  stream = fopen (the_gif_filename, "rb");

  if (!stream)
    {
      file_error (the_gif_filename);
      exit (2);
    }

  image = gdImageCreateFromGif (stream);
  fclose (stream);

  color_in_image (image, all_points);
  gif_return (image);
  return (0);
}

