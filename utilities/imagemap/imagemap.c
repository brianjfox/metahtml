/* imagemap.c: Return the URL for clicked on coordinates. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Fri Aug 11 19:00:19 1995.  */

/* This file is part of <Meta-HTML>(tm), a system for the rapid
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

	http://www.metahtml.com/COPYING
*/

/* The coordinates are passed in argv[1], the map file is in PATH_INFO. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include "pages.h"

static int act_like_server = 0;

#define whitespace(c) ((c == ' ') || (c == '\t'))

static int Xcoord = 0;
static int Ycoord = 0;
static char *default_target = (char *)NULL;

typedef struct
{
  int x;
  int y;
} Point;

typedef struct
{
  char *url;			/* Target URL. */
  int dist;			/* Distance from point. */
} StoredPoint;

static StoredPoint **stored_points = (StoredPoint **)NULL;
static int stored_points_index = 0;
static int stored_points_size = 0;

typedef struct
{
  char *shape;			/* The name of the shape to check. */
  char *target;			/* The target URL to return. */
  Point **points;		/* The points describing this shape. */
} ParsedLine;

typedef int CheckerFun	(const char *url, Point **points);
static int in_rect	(const char *url, Point **points);
static int in_circle	(const char *url, Point **points);
static int in_poly	(const char *url, Point **points);
static int in_point	(const char *url, Point **points);
static int in_default	(const char *url, Point **points);
typedef struct
{
  char *shape;			/* The name of the shape this func handles. */
  int min_points;		/* Number of points this function must have. */
  int max_points;		/* Number of points this function can have. */
  CheckerFun *checker;		/* The function that does the work. */
} Checker;

static Checker checkers[] = {
  { "rect",	2, 2, in_rect },
  { "rectangle",2, 2, in_rect },
  { "circle",	2, 2, in_circle },
  { "poly",	3, 100, in_poly },
  { "polygon",	3, 100, in_poly },
  { "point",	1, 1, in_point },
  { "default",	0, 0, in_default },
  { (char *)0,	0, 0, (CheckerFun *)NULL }
};

static Checker *
find_checker (char *shape)
{
  register int i;
  Checker *result = (Checker *)NULL;

  for (i = 0; checkers[i].shape != (char *)NULL; i++)
    if (strcasecmp (shape, checkers[i].shape) == 0)
      {
	result = &checkers[i];
	break;
      }

  return (result);
}

/* Check for Xcoord and Ycoord within boundary of rectangle.
   POINTS contains the upper-left vertex in the 0th element,
   and the lower-right vertex in the 1st element. */
static int
in_rect (const char *url, Point **points)
{
  if (points == (Point **)NULL)
    return (0);

  return ((Xcoord >= points[0]->x) && (Xcoord <= points[1]->x) &&
	  (Ycoord >= points[0]->y) && (Ycoord <= points[1]->y));
}

/* Check for Xcoord and Ycoord within boundary of circle.
   POINTS contains the center of the circle in the 0th element,
   and a point on the rim of the circle in the 1st element.

   To find out if the point is in the circle, we compare the radius
   of the circle with the radius of the imaginary circle created by
   treating the target point as if it were on the edge of the circle.
   If that radius is less than the radius of the circle in question,
   then the point is on the edge of a circle contained within the
   target circle. */
static int
in_circle (const char *url, Point **points)
{
  int existing_radius;
  int imaginary_radius;

  if (points == (Point **)NULL)
    return (0);

  existing_radius =
    (((points[1]->x - points[0]->x) * (points[1]->x - points[0]->x)) +
     ((points[1]->y - points[0]->y) * (points[1]->y - points[0]->y)));

  imaginary_radius =
    (((Xcoord - points[0]->x) * (Xcoord - points[0]->x)) +
     ((Ycoord - points[0]->y) * (Ycoord - points[0]->y)));

  return (imaginary_radius <= existing_radius);
}

/* Draw a horizontal line from [Xcoord Ycoord] extending infinitely in the
   positive X-axis.  Count the number of times that line crosses the lines
   created by connecting adjacent vertices of the polygon.  If that number
   is even, then [Xcoord Ycoord] is "outside" of the polygon, if odd,
   then "inside". */
#define min(x,y) ((x) > (y) ? y : x)
#define max(x,y) ((x) > (y) ? x : y)

static int
in_poly (const char *url, Point **points)
{
  register int i;
  int min_x = -1;
  int max_x = -1;
  int min_y = -1;
  int max_y = -1;
  int result = 0;
  int vertices = 0;

  /* Count vertices. */
  for (vertices = 0; points[vertices] != (Point *)NULL; vertices++);

  /* Close the polygon if it isn't already closed. */
  i = vertices - 1;

  if ((points[i]->x != points[0]->x) || (points[i]->y != points[0]->y))
    {
      i++;
      vertices++;
      points = (Point **)xrealloc (points, ((3 + i) * sizeof (Point *)));
      points[i] = (Point *)xmalloc (sizeof (Point));
      points[i]->x = points[0]->x;
      points[i]->y = points[0]->y;
      points[i + 1] = (Point *)NULL;
    }

#undef debug
#if defined (debug)
  {
    FILE *stream = fopen ("/tmp/imagemaps-suck", "a");
    int indent_len = strlen (url) + 24;
    char *indent_string = (char *)xmalloc (1 + indent_len);
    static int been_here = 0;

    if (!been_here)
      {
	fprintf (stream, "\n----------------------------------\n");
	fprintf (stream, "Xcoord = %d, YCoord = %d\n", Xcoord, Ycoord);
	been_here++;
      }

    memset (indent_string + 2, ' ', indent_len);
    indent_string[indent_len] = '\0';
    indent_string[0] = ',';
    indent_string[1] = '\n';

    fprintf (stream, "\nEntering in_poly (\"%s\", ", url);
    for (i = 0; points[i]; i++)
      fprintf (stream, "[%3d, %3d]%s", points[i]->x, points[i]->y,
	       points[i + 1] ? indent_string : ");\n");

    fclose (stream);
  }
#endif


  /* Now check to see if the point falls within the rectangle which
     encloses the entire polygon.  If not, it certainly isn't inside. */
  for (i = 0; points[i] != (Point *)NULL; i++)
    {
      if ((min_x == -1) || (points[i]->x < min_x))
	min_x = points[i]->x;

      if ((min_y == -1) || (points[i]->y < min_y))
	min_y = points[i]->y;

      if (points[i]->x > max_x) max_x = points[i]->x;
      if (points[i]->y > max_y) max_y = points[i]->y;
    }

#if defined (debug)
  {
    FILE *stream = fopen ("/tmp/imagemaps-suck", "a");
    fprintf (stream, "    Containing rectangle: [%3d, %3d], [%3d, %3d]\n",
	     min_x, min_y, max_x, max_y);
    fclose (stream);
  }
#endif

  /* Is [Xcoord Ycoord] within the rectangle defined by
     [min_x max_y] [max_x min_y]? */
  if ((Xcoord < min_x) || (Xcoord > max_x) ||
      (Ycoord < min_y) || (Ycoord > max_y))
    return (0);

  /* The point falls within the polygon.  Check adjacent vertices. */
  {
    int lines_crossed = 0;
    Point *p2 = points[0];
    Point *p1;

    for (i = 1; points[i] != (Point *)NULL; i++)
      {
	p1 = p2;
	p2 = points[i];

#if defined (debug)
	{
	  FILE *stream = fopen ("/tmp/imagemaps-suck", "a");
	  fprintf (stream, " Checking line segment: [%3d, %3d], [%3d, %3d]\n",
		   p1->x, p1->y, p2->x, p2->y);
	  fclose (stream);
	}
#endif

	min_x = min (p1->x, p2->x);
	max_x = max (p1->x, p2->x);
	min_y = min (p1->y, p2->y);
	max_y = max (p1->y, p2->y);

	/* We need to know if the point falls within the rectangle defined
	   by the maximum vertices of the vector. */
	if ((Xcoord < min_x) || (Xcoord > max_x) ||
	    (Ycoord < min_y) || (Ycoord > max_y))
	  {
	    /* Not within the rectangle.  Great!  If it is to the left
	       of the rectangle, and in between the Y coordinates, then
	       it crosses the line. */
	    if ((Xcoord < min_x) && (Ycoord > min_y) && (Ycoord < max_y))
	      {
#if defined (debug)
		FILE *stream = fopen ("/tmp/imagemaps-suck", "a");
		fprintf (stream, "    Crossed because of rect rule!\n");
		fclose (stream);
#endif

		lines_crossed++;
	      }

	    continue;
	  }

	/* Find the intersection of the line ([-inf, Ycoord], [+inf, Ycoord])
	   and ([p1-x, p1-y], [p2-x, p2-y]).  If the location of the intercept
	   is to the right of Xcoord, then the line will be crossed. */
	{
	  double slope = (double)(p1->y - p2->y) / (double)(p1->x - p2->x);
	  
	  if ((((double)Ycoord - (double)(p1->y - (slope * p1->x))) / slope)
	      >= (double)Xcoord)
	    {
#if defined (debug)
	      FILE *stream = fopen ("/tmp/imagemaps-suck", "a");
	      fprintf (stream, "    Crossed because of slope rule!\n");
	      fclose (stream);
#endif

	      lines_crossed++;
	    }
	}
      }

#if defined (debug)
	{
	  FILE *stream = fopen ("/tmp/imagemaps-suck", "a");
	  fprintf (stream, "Lines Crossed: %d\n\n", lines_crossed);
	  fclose (stream);
	}
#endif

    if (lines_crossed & 0x01)
      result = 1;
  }
  return (result);
}

/* Simply rmember how close we are to this point. */
static int
in_point (const char *url, Point **points)
{
  int this_radius =
    (((Xcoord - points[0]->x) * (Xcoord - points[0]->x)) +
     ((Ycoord - points[0]->y) * (Ycoord - points[0]->y)));
  StoredPoint *storage = (StoredPoint *)xmalloc (sizeof (StoredPoint));

  storage->url = strdup (url);
  storage->dist= this_radius;

  if ((stored_points_index + 2) > stored_points_size)
    stored_points = (StoredPoint **)xrealloc
      (stored_points, (stored_points_size += 10) * sizeof (StoredPoint *));

  stored_points[stored_points_index++] = storage;
  stored_points[stored_points_index] = (StoredPoint *)NULL;

  return (0);
}

static int
in_default (const char *url, Point **points)
{
  default_target = strdup (url);
  return (0);
}

static char *
concat (int count, ...)
{
  BPRINTF_BUFFER *buffer;
  char *result = (char *)NULL;
  va_list args;

  va_start (args, count);

  buffer = bprintf_create_buffer ();

  while (count--)
    {
      char *arg = va_arg (args, char *);

      bprintf (buffer, "%s", arg ? arg : "");
    }

  result = buffer->buffer;
  free (buffer);

  return (result);
}

static PAGE *
imagemap_this_url (const char *url)
{
  int length;
  PAGE *page;
  char *server_name = getenv ("SERVER_NAME");
  char *sid = getenv ("SID");
  char *port = getenv ("SERVER_PORT");
  char *new_url;
  BPRINTF_BUFFER *urlbuff = bprintf_create_buffer ();
  BPRINTF_BUFFER *prefix_buff = bprintf_create_buffer ();

  /* If the client passed the cookie in the mime header, then it will
     continue to do so, so don't pass the cookie around in the URL. */
  if (getenv ("HTTP_COOKIE") != (char *)NULL)
    sid = (char *)NULL;

  if (port != (char *)NULL)
    {
      if (strcmp (port, "80") == 0)
	port = (char *)NULL;
      else
	port = concat (2, ":", port);
    }

  if (server_name == (char *)NULL)
    {
      char hostname[100];
      gethostname (hostname, 100);
      server_name = strdup (hostname);
    }

  if (port) server_name = concat (2, server_name, port);

  bprintf (urlbuff, "%s", url);

  /* Allow the target URL to contain Meta text which expands to the
     clicked on point. */
  {
    char xpos[20], ypos[20];

    sprintf (xpos, "%d", Xcoord);
    sprintf (ypos, "%d", Ycoord);

    page_subst_in_page (urlbuff, "\\$X", xpos);
    page_subst_in_page (urlbuff, "\\$Y", ypos);
  }

  /* Does this url require a prefix? */
  if (strchr (url, ':') == (char *)NULL)
    {
      bprintf (prefix_buff, "http://%s", server_name);

      /* If the URL didn't start with a '/', then it is relative.
	 Either insert the relative prefix (if there is one), or 
	 a slash. */
      if (*url != '/')
	{
	  char *referer = getenv ("HTTP_REFERER");

	  if (referer != (char *)NULL)
	    {
	      register int i;
	      int slashes_seen = 0;

	      for (i = 0; referer[i]; i++)
		if (referer[i] == '/')
		  {
		    slashes_seen++;
		    if (slashes_seen == 3)
		      break;
		  }

	      if (slashes_seen == 3)
		{
		  char *rel_prefix = strdup (referer + i);
		  char *temp = strrchr (rel_prefix, '/');

		  if (temp != (char *)NULL)
		    *temp = 0;

		  bprintf (prefix_buff, "%s", rel_prefix);
		  free (rel_prefix);
		}
	    }
	  bprintf (prefix_buff, "/");
	}
    }

  /* Insert the prefix if it exists. */
  if (prefix_buff->buffer)
    bprintf_insert (urlbuff, 0, "%s", prefix_buff->buffer);

  /* Maybe substitute for SID? */
  if (sid != (char *)NULL)
    page_subst_in_page (urlbuff, "\\$SID", concat (2, sid, "/"));
  else
    page_subst_in_page (urlbuff, "\\$SID", sid);

  new_url = urlbuff->buffer;
  page = page_create_page ();

  bprintf (page, "<html>");
  bprintf (page, "<head><title>Image Map Redirection</title></head>");
  bprintf (page, "<body>");
  bprintf (page, "<h1>Image Map Redirection</h1><p>");
  bprintf (page, "This document can be found at ");
  bprintf (page, "<a href=\"%s\">%s</a>.<p>", new_url, new_url);
  bprintf (page, "You see this message because your browser doesn't support ");
  bprintf (page, "automatic redirection handling.");
  bprintf (page, "</body>");
  bprintf (page, "</html>\n");
  length = page->bindex;
  bprintf_insert (page, 0, "Content-length: %d\n\n", length);
  bprintf_insert (page, 0, "Content-type: text/html\n");
  bprintf_insert (page, 0, "Date: %s\n",
		  http_date_format ((long) time ((time_t *)0)));
  bprintf_insert (page, 0, "Location: %s\n", new_url);
  if (act_like_server)
    bprintf_insert (page, 0, "HTTP/1.0 302 Found\n");
  return (page);
}

static ParsedLine *
parse_line (const char *line)
{
  ParsedLine *parsed = (ParsedLine *)NULL;
  char *shape = (char *)NULL;
  char *target = (char *)NULL;
  Point **points = (Point **)NULL;

  /* The "while" is so we can use "break". */
  while (line != (char *)NULL)
    {
      register int i, start;
      int points_index = 0;
      int points_size = 0;

      /* Skip leading whitespace. */
      for (i = 0; whitespace (line[i]); i++);

      start = i;

      if ((line[start] == '#') || (line[i] == '\0'))
	break;

      /* Snarf first argument. */
      for (i = start; (line[i] != '\0') && (!whitespace (line[i])); i++);

      if (start == i)
	break;

      shape = (char *)xmalloc (1 + (i - start));
      strncpy (shape, line + start, i - start);
      shape[i - start] = '\0';

      /* We have the shape name.  Skip more whitespace. */
      for (start = i; whitespace (line[start]); start++);

      /* Get the name of the target URL. */
      for (i = start; (line[i] != '\0') && (!whitespace (line[i])); i++);

      if (start == i)
	break;

      target = (char *)xmalloc (1 + (i - start));
      strncpy (target, line + start, i - start);
      target[i - start] = '\0';

      /* We have the name of the target URL.  Skip more whitespace. */
      for (start = i; whitespace (line[start]); start++);

      /* This is enough information to create a structure.  Do it. */
      parsed = (ParsedLine *)xmalloc (sizeof (ParsedLine));
      parsed->shape = shape;
      parsed->target = target;

      /* Collect sets of points. */
      while (line[start] != '\0')
	{
	  char numbuff[40];
	  int x, y;

	  /* Get the X coordinate. */
	  for (i = start; isdigit (line[i]); i++);

	  /* Ignore ridiculous values. */
	  if ((i == start) || ((i - start) > 39))
	    break;

	  strncpy (numbuff, line + start, i - start);
	  numbuff[i - start] = '\0';

	  x = atoi (numbuff);

	  /* Skip non-digits. */
	  for (start = i;
	       (line[start] != '\0') && (!isdigit (line[start]));
	       start++);

	  if ((i == start) || (line[start] == '\0'))
	    break;

	  /* Get the Y coordinate. */
	  for (i = start; isdigit (line[i]); i++);

	  /* Ignore ridiculous values. */
	  if ((i == start) || ((i - start) > 39))
	    break;

	  strncpy (numbuff, line + start, i - start);
	  numbuff[i - start] = '\0';

	  y = atoi (numbuff);

	  /* Skip non-digits. */
	  for (start = i;
	       (line[start] != '\0') && (!isdigit (line[start]));
	       start++);

	  /* We have successfully gotten an X and Y coordinate.
	     Create a new point. */
	  if ((points_index + 2) >= points_size)
	    points = (Point **)xrealloc
	      (points, (points_size += 3) * sizeof (Point *));

	  points[points_index] = (Point *)xmalloc (sizeof (Point));
	  points[points_index]->x = x;
	  points[points_index]->y = y;
	  points_index++;
	  points[points_index] = (Point *)NULL;
	}

      parsed->points = points;
      break;
    }

  /* If we didn't make a parsed out line, free the values we might
     have partially parsed. */
  if (parsed == (ParsedLine *)NULL)
    {
      if (target) free (target);
      if (shape) free (shape);
    }

  return (parsed);
}

static void
free_points (Point **points)
{
  if (points != (Point **)NULL)
    {
      register int i;

      for (i = 0; points[i] != (Point *)NULL; i++)
	free (points[i]);

      free (points);
    }
}

static void
free_parsed_line (ParsedLine *parsed_line)
{
  if (parsed_line != (ParsedLine *)NULL)
    {
      if (parsed_line->shape != (char *)NULL)
	free (parsed_line->shape);

      if (parsed_line->target != (char *)NULL)
	free (parsed_line->target);

      free_points (parsed_line->points);

      free (parsed_line);
    }
}

static PAGE *
imagemap_error_page (char *format, ...)
{
  BPRINTF_BUFFER *loser_path;
  BPRINTF_BUFFER *message;
  PAGE *page;
  char *temp;
  va_list args;

  va_start (args, format);

  message = bprintf_create_buffer ();
  bprintf (message, "imagemap: ");
  vbprintf (message, format, args);
  bprintf (message, "\n");

  loser_path = bprintf_create_buffer ();
  bprintf (loser_path, "../imagemap-loser.mhtml");
  page = page_read_template (loser_path->buffer);

  if (page == (PAGE *)NULL)
    {
      page = page_create_page ();
      bprintf (page, "<html><head><title>Imagemap Error Page</title></head>\n");
      bprintf (page, "<hr><h1>Imagemap Error</h1><hr><p>");
      bprintf (page, "<h3><ERROR-MESSAGE></h3><p>");
      bprintf (page,
	       "An error occured while processing the imagemap request ");
      if ((temp = getenv ("HTTP_REFERER")) != (char *)NULL)
	bprintf (page, "from <a href=\"`%s'\">%s</a>, ", temp, temp);

      bprintf (page, "which means that your link could not be followed.<p>");
      bprintf (page, "You should mail this error page to ");

      if ((temp = getenv ("SERVER_NAME")) != (char *)NULL)
	bprintf (page, "<a href=\"mailto:webmaster@%s\">webmaster@%s</a> ",
		 temp, temp);
      else
	bprintf (page, "the <i><b>webmaster</b></i> of this site ");

      bprintf (page, "in the hopes that she or he will fix the problem.");
    }

  page_subst_in_page (page, "<ERROR-MESSAGE>", message->buffer);

  if (act_like_server)
    bprintf_insert (page, 0, "HTTP/1.0 200 OK\n");

  page_insert_http_header (page, page_IS_EXPIRED);
  return (page);
}
      
/* Return a relocation page, an error page, or a NULL page if there was
   no error, but there was also no default which could be used. */
static PAGE *
process_file (char *filename)
{
  static char buffer[1024];
  FILE *stream = fopen (filename, "r");
  PAGE *page = (PAGE *)NULL;
  char *temp;
  int line_number = 1;

  if (!stream)
    page = imagemap_error_page ("The mapfile is missing: `%s'", filename);

  while ((page == (PAGE *)NULL) &&
	 ((temp = fgets (buffer, 1023, stream)) != (char *)NULL))
    {
      int len = strlen (buffer);

      if ((len != 0) && (buffer[len - 1] == '\n'))
	{
	  buffer[len - 1] = '\0';
	  len--;
	}

      line_number++;

      if (len != 0)
	{
	  ParsedLine *parsed = parse_line (buffer);

	  if (parsed != (ParsedLine *)NULL)
	    {
	      Checker *checker = find_checker (parsed->shape);

	      if (checker == (Checker *)NULL)
		{
		  page = imagemap_error_page
		    ("In file %s, at line %d: Invalid shape `%s'\n",
		     filename, line_number, parsed->shape);
		  continue;
		}
	      else
		{
		  register int i;

		  for (i = 0; parsed->points && parsed->points[i]; i++);

		  if ((i < checker->min_points) || (i > checker->max_points))
		    {
		      page = imagemap_error_page
			("in file %s, at line %d: Incorrect number of points (%d) for shape %s\n",
			 filename, line_number, i, checker->shape);
		      continue;
		    }
		  else
		    {
		      if ((*(checker->checker))
			  (parsed->target, parsed->points) == 1)
			page = imagemap_this_url (parsed->target);
		    }
		}
	      free_parsed_line (parsed);
	    }
	}
    }

  /* No answer found yet.  If there are stored points, take the minimum. */
  if (page == (PAGE *)NULL)
    {
      if (stored_points != (StoredPoint **)NULL)
	{
	  register int i;
	  int min_dist = stored_points[0]->dist;
	  char *url = stored_points[0]->url;

	  for (i = 0; stored_points[i] != (StoredPoint *)NULL; i++)
	    if (stored_points[i]->dist < min_dist)
	      {
		min_dist = stored_points[i]->dist;
		url = stored_points[i]->url;
	      }

	  page = imagemap_this_url (url);
	}
      else if (default_target != (char *)NULL)
	page = imagemap_this_url (default_target);
    }

  return (page);
}

/* In our build-page server, we gaurantee that the current directory will
   be the directory which contains the image of this program.  So, PATH_INFO
   is relative to this directory at all times.  That is to say, you can
   assume that "../$PATH_INFO" is the way to find the imagemap file. */
int
main (int argc, char *argv[])
{
  char *path_info = getenv ("PATH_INFO");
  char *coords = getenv ("QUERY_STRING");
  PAGE *page = (PAGE *)NULL;
  char *newpath;

  if ((strstr (argv[0], "nph-") != (char *)NULL) ||
      (getenv ("MHTML_NPH_ENGINE") != (char *)NULL))
    act_like_server++;

  if ((coords == (char *)NULL) && (argc > 1))
    coords = argv[1];

  if (coords != (char *)NULL)
    sscanf (coords, "%d,%d", &Xcoord, &Ycoord);
  else
    page = imagemap_error_page ("Missing coordintates on invocation.");

  if (path_info)
    {
      char *temp;
      path_info = strdup (path_info);
      temp = strchr (path_info, '?');
      if (temp) *temp = '\0';
    }
  else
    path_info = strdup (" CAN'T TOUCH THIS!  ");

  if (page == (PAGE *)NULL)
    {
      char *document_directory = getenv ("WEBBASEDIR");

      if (document_directory)
	{
	  newpath = (char *)xmalloc
	    (2 + strlen (document_directory) + strlen (path_info));
	  sprintf (newpath, "%s%s", document_directory, path_info);
	}
      else
	{
	  newpath = (char *)xmalloc (4 + strlen (path_info));
	  sprintf (newpath, "..%s", path_info);
	}

      page = process_file (newpath);
    }

  if (page == (PAGE *)NULL)
    {
      /* If we get here, the map file existed, but specified no default. */
      page = imagemap_error_page
	("Location (%d, %d)  not handled by `%s', and no `default' target.",
	 Xcoord, Ycoord, path_info);
    }
  
  fprintf (stdout, "%s", page->buffer);
  return (0);
}
