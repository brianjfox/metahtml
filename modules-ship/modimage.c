/* modimage.c: -*- C -*-  Module which allows the use of the GD library. */

/*  Author: Brian J. Fox (bfox@ai.mit.edu) Tue Mar 30 18:44:20 1999.

    This file is part of <Meta-HTML>(tm), a system for the rapid
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
#include "modules.h"

#if defined (__cplusplus)
extern "C"
{
#endif

#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#include <gdfontl.h>

/* #include <mtables.c> */
#define costScale 1024
#define sintScale 1024
extern int cost[], sint[];

static void pf_image_create (PFunArgs);
static void pf_image_delete (PFunArgs);
static void pf_image_write (PFunArgs);
static void pf_image_set_pixel (PFunArgs);
static void pf_image_get_pixel (PFunArgs);
static void pf_image_text (PFunArgs);
static void pf_image_line (PFunArgs);
static void pf_image_fill (PFunArgs);
static void pf_image_rect (PFunArgs);
static void pf_image_arc (PFunArgs);
static void pf_image_poly (PFunArgs);
static void pf_image_copy (PFunArgs);
static void pf_image_info (PFunArgs);
static void pf_image_transparent (PFunArgs);

static PFunDesc ftab[] =
{
  /*   tag           complex? debug_level          code    */
  { "IMAGE::CREATE",	0,	0,	pf_image_create },
  { "IMAGE::DELETE",	0,	0,	pf_image_delete },
  { "IMAGE::WRITE",	0,	0,	pf_image_write },
  { "IMAGE::SET-PIXEL",	0,	0,	pf_image_set_pixel },
  { "IMAGE::GET-PIXEL",	0,	0,	pf_image_get_pixel },
  { "IMAGE::TEXT",	0,	0,	pf_image_text },
  { "IMAGE::LINE",	0,	0,	pf_image_line },
  { "IMAGE::FILL",	0,	0,	pf_image_fill },
  { "IMAGE::RECT",	0,	0,	pf_image_rect },
  { "IMAGE::ARC",	0,	0,	pf_image_arc },
  { "IMAGE::POLY",	0,	0,	pf_image_poly },
  { "IMAGE::COPY",	0,	0,	pf_image_copy },
  { "IMAGE::TRANSPARENT",0,	0,	pf_image_transparent },
  { "IMAGE::INFO",	0,	0,	pf_image_info },

  { (char *)NULL,       0,       0,             (PFunHandler *)NULL }
};

/* 3)  Insert the following statement, with the name of your module file
   as a string.  This allows Meta-HTML to install the functions in your
   module when it is loaded. */
MODULE_INITIALIZE ("modimage", ftab)

/* 4) Write a blurb about what this module does.  The text here will make
   it directly into the documentation, as a section of its own, and the
   functions that you declare with DEFUN, DEFMACRO, etc., will be documented
   in that section. */
DEFINE_SECTION (IMAGE-MODULE, image-keywords; more keywords,
"Functions which allow the creation of GIF images.
You create an image with <var IMAGE::CREATE>, and you can then draw lines,
arcs, and points into the image.  When you are done creating the image,
you call <var IMAGE::RETRIEVE> to get the GIF image in a binary variable.

Finally, when you are totally done with the image, you call <var IMAGE::DELETE>
to make the image go away.", "")

/* An IMAGE object in Meta-HTML under this GD-based library is a variable
   name which encodes the location of a data structure within our code. */
static gdImage **images = (gdImage **)NULL;
static int image_slots = 0;

/* Create and allocate an image with width X and height Y. */
static int
allocate_image (int x, int y)
{
  register int i;
  register int slot = -1;

  for (i = 0; i < image_slots; i++)
    if (images[i] == (gdImage *)NULL)
      {
	slot = i;
	break;
      }

  if (slot < 0)
    {
      images = (gdImage **)xrealloc
	(images, (image_slots += 10) * sizeof (gdImage *));
      slot = i;
      for (i = slot; i < image_slots; i++)
	images[i] = (gdImage *)NULL;
    }

  images[slot] = gdImageCreate (x, y);

  return (slot);
}

static void
deallocate_image (int which)
{
  if (which < image_slots)
    if (images[which] != (gdImage *)NULL)
      {
	gdImageDestroy (images[which]);
	images[which] = (gdImage *)NULL;
      }
}

static int
dereference_image_var (char *for_whom, char *varname)
{
  int which = -1;

  if (!empty_string_p (varname))
    {
      char *value = pagefunc_get_variable (varname);

      if (!empty_string_p (value) && (integer_p (value, 10)))
	{
	  which = atoi (value);

	  if (which > -1)
	    {
	      if (which > image_slots)
		which = -1;
	      else if (images[which] == (gdImage *)NULL)
		which = -1;

	      if (which == -1)
		pagefunc_set_variable (varname, "(Deallocated Image)");
	    }
	}
    }
  else
    page_debug ("IMAGE::%s Missing IMAGE variable", for_whom);
  return (which);
}

static int
get_image_arg_1 (char *for_whom, Package *vars, int pos)
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, pos));
  int which = dereference_image_var (for_whom, varname);
  xfree (varname);
  return (which);
}

static int
get_image_arg (char *for_whom, Package *vars)
{
  return (get_image_arg_1 (for_whom, vars, 0));
}

static int
integer_val (char *string)
{
  double first = strtod (string, (char **)NULL);
  return ((int) first);
}

DEFUNX(image::create, imagevar &key width height src,
"Creates a new image with the specified width and height, and makes
<var imagevar> be a receptacle for that image.

If you pass <var src=/www/docs/images/foo.gif>, then <var foo.gif> will
be loaded into the image variable instead of an empty image.")

static void
pf_image_create (PFunArgs)
{
  char *width_arg = mhtml_evaluate_string (get_value (vars, "WIDTH"));
  char *height_arg = mhtml_evaluate_string (get_value (vars, "HEIGHT"));
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *srcfile = mhtml_evaluate_string (get_value (vars, "SRC"));
  int width = 100, height = 26;
  int slot;
  char *result = (char *)NULL;

  if (!empty_string_p (varname))
    {
      if (!empty_string_p (srcfile))
	{
	  FILE *stream = fopen (srcfile, "r");
	  gdImage *image = (gdImage *)NULL;

	  if (stream != (FILE *)NULL)
	    {
	      image = gdImageCreateFromGif (stream);
	      fclose (stream);
	    }

	  if (image == (gdImage *)NULL)
	    {
	      page_debug ("IMAGE::CREATE Couldn't load %s: %s",
			  srcfile, strerror (errno));
	      return;
	    }

	  slot = allocate_image (1, 1);
	  gdImageDestroy (images[slot]);
	  images[slot] = image;
	}
      else
	{
	  if (!empty_string_p (width_arg))
	    width = integer_val (width_arg);

	  if (!empty_string_p (height_arg))
	    height = integer_val (height_arg);

	  slot = allocate_image (width, height);
	}

      mhtml_set_numeric_variable (varname, slot);
      result = pagefunc_get_variable (varname);
    }
  else
    page_debug ("IMAGE::CREATE Missing IMAGE variable");

  xfree (varname);
  xfree (width_arg);
  xfree (height_arg);
  xfree (srcfile);

#if 0
  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
#endif
}

DEFUNX (image::info, imagevar,
"Returns an alist representing information about the image in <var imagevar>.
The alist contains <var width>, <var height>, <var total-colors>, and
<var colors>.  <var colors> is an array of the color values which appear in
the image.")

#define alist_set(name, num)					\
   do								\
     {								\
       sprintf (numbuff, "%d", num);				\
       forms_set_tag_value_in_package (p, name, numbuff);	\
     } while (0)
    
static void
pf_image_info (PFunArgs)
{
  int slot = get_image_arg ("INFO", vars);
  char *result = (char *)NULL;

  if (slot >= 0)
    {
      register int i;
      gdImage *image = images[slot];
      Package *p = symbol_get_package ((char *)NULL);
      Symbol *colors = symbol_intern_in_package (p, "colors");
      char numbuff[20];

      alist_set ("width", image->sx);
      alist_set ("height", image->sy);
      alist_set ("total-colors", image->colorsTotal);
      if (image->transparent > -1)
	alist_set ("transparent", image->transparent);

      for (i = 0; i < gdImageColorsTotal (image); i++)
	{
	  sprintf (numbuff, "#%02X%02X%02X ",
		   gdImageRed (image, i),
		   gdImageGreen (image, i),
		   gdImageBlue (image, i));
	  symbol_add_value (colors, numbuff);
	}

      result = package_to_alist (p, 0);
    }

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }
}

DEFUNX (image::delete, imagevar,
"Reclaims any space that is currently being used by <var imagevar>.")

static void
pf_image_delete (PFunArgs)
{
  int slot = get_image_arg ("DELETE", vars);

  if (slot > -1)
    {
      deallocate_image (slot);
      slot = get_image_arg ("DEALLOCATION FORCE", vars);
    }
}

static int
hex_value (int c)
{
  if (islower (c)) c = toupper (c);
  if (strchr ("0123456789ABCDEF", c) != (char *)NULL)
    {
      c = c - '0';
      if (c > 9)
	c = 10 + ((c + '0') - 'A');
    }
  else
    c = -1;

  return (c);
}

static int
parse_rgb (char *rgb, int *r, int *g, int *b)
{
  register int result = -1;

  *r = -1; *g = -1; *b = -1;

  if (rgb != (char *)NULL)
    {
      while (whitespace (*rgb)) rgb++;
      while (*rgb == '#') rgb++;

      if (strchr (rgb, ',') != (char *)NULL)
	{
	  /* This is RGB in decimal, values separated by commas. */
	  sscanf (rgb, "%d,%d,%d", r, g, b);
	}
      else
	{
	  register int i;
	  /* This is RGB in hex. */

	  for (i = 0; i < 6 && rgb[i]; i++)
	    {
	      switch (i)
		{
		case 0: *r = 16 * hex_value (rgb[i]); break;
		case 1: *r |= hex_value (rgb[i]); break;
		case 2: *g = 16 * hex_value (rgb[i]); break;
		case 3: *g |= hex_value (rgb[i]); break;
		case 4: *b = 16 * hex_value (rgb[i]); break;
		case 5: *b |= hex_value (rgb[i]); break;

		default:
		  break;
		}
	    }
	}
    }

  if ((*r < 0) || (*g < 0) || (*b < 0))
    result = -1;
  else
    result = 0;

  return (result);
}

static int
rgb_to_index (gdImage *image, char *rgb, int allocate_p)
{
  int r, g, b, pixel = -1;

  if (parse_rgb (rgb, &r, &g, &b) < 0)
    pixel = -1;
  else
    {
      pixel = gdImageColorExact (image, r, g, b);

      if (pixel == -1)
	pixel = gdImageColorAllocate (image, r, g, b);

      if (pixel == -1)
	pixel = gdImageColorClosest (image, r, g, b);
    }

  return (pixel);
}
  
DEFUNX (image::set-pixel, imagevar &key x y color,
"Set the pixel in <var imagevar> at location <var x>, <var y> to the color
<var color>.")
static void
pf_image_set_pixel (PFunArgs)
{
  int slot = get_image_arg ("SET-PIXEL", vars);

  if (slot > -1)
    {
      gdImage *image = images[slot];
      char *x_arg = mhtml_evaluate_string (get_value (vars, "X"));
      char *y_arg = mhtml_evaluate_string (get_value (vars, "Y"));
      char *c_arg = mhtml_evaluate_string (get_value (vars, "COLOR"));

      if ((!empty_string_p (x_arg) && number_p (x_arg)) &&
	  (!empty_string_p (y_arg) && number_p (y_arg)) &&
	  (!empty_string_p (c_arg)))
	{
	  int x = integer_val (x_arg);
	  int y = integer_val (y_arg);
	  int c = -1;

	  /* Convert the color into an index.  If this image doesn't already
	     have this color, add it now. */
	  c = rgb_to_index (image, c_arg, 1);

	  gdImageSetPixel (image, x, y, c);
	}
      else
	page_debug ("<image::set-pixel ..> requires all of X, Y, and COLOR");

      xfree (x_arg);
      xfree (y_arg);
      xfree (c_arg);
    }
}

DEFUNX (image::get-pixel, imagevar &key x y,
"Returns the color of the pixel at <var x>, <var y> in <var imagevar>")

static void
pf_image_get_pixel (PFunArgs)
{
  int slot = get_image_arg ("GET-PIXEL", vars);
  char *result = (char *)NULL;
  char buffer[20];

  if (slot > -1)
    {
      gdImage *image = images[slot];
      char *x_arg = mhtml_evaluate_string (get_value (vars, "X"));
      char *y_arg = mhtml_evaluate_string (get_value (vars, "Y"));

      if ((!empty_string_p (x_arg) && number_p (x_arg)) &&
	  (!empty_string_p (y_arg) && number_p (y_arg)))
	{
	  int x = integer_val (x_arg);
	  int y = integer_val (y_arg);
	  int c = gdImageGetPixel (image, x, y);
	  if (c > -1)
	    {
	      sprintf (buffer, "#%02X%02X%02X",
		       image->red[c], image->green[c], image->blue[c]);
	      result = buffer;
	    }
	}
      else
	page_debug ("<image::get-pixel ..> requires both X and Y");

      xfree (x_arg);
      xfree (y_arg);
    }

  if (result != (char *)NULL)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
    }
}

DEFUNX (image::text, imagevar text &key x y color size align report,
"Write text on the image in <var imagevar> at position <var x>, <var y>,
in the color <var color>.
<var align> can be one of \"right\", \"center\", or \"left\", and defaults
to \"center\".
<var size> ranges frome 1 to 6 and defaults to 3.

If <var report=true> is specified, the text isn't drawn, but all of the
information about the drawing is returned as an alist.")

static void
pf_image_text (PFunArgs)
{
  int slot = get_image_arg ("TEXT", vars);

  if (slot > -1)
    {
      char *text = mhtml_evaluate_string (get_positional_arg (vars, 1));
      char *report_arg = mhtml_evaluate_string (get_value (vars, "REPORT"));
      Package *info = (Package *)NULL;

      if (!empty_string_p (report_arg))
	{
	  info = symbol_get_package ((char *)NULL);
	  symbol_intern_in_package (info, "left");
	  symbol_intern_in_package (info, "right");
	  symbol_intern_in_package (info, "height");
	  symbol_intern_in_package (info, "width");
	}
      xfree (report_arg);

      if (!empty_string_p (text))
	{
	  char *x_arg = mhtml_evaluate_string (get_value (vars, "X"));
	  char *y_arg = mhtml_evaluate_string (get_value (vars, "Y"));
	  char *c_arg = mhtml_evaluate_string (get_value (vars, "COLOR"));
	  char *size_arg = mhtml_evaluate_string (get_value (vars, "SIZE"));
	  char *align_arg = mhtml_evaluate_string (get_value (vars, "ALIGN"));
	  gdFontPtr font;
	  int x, y, c, size;

	  if (!empty_string_p (x_arg) && number_p (x_arg))
	    x = integer_val (x_arg);
	  else
	    x = gdImageSX (images[slot]) / 2;

	  if (!empty_string_p (y_arg) && number_p (y_arg))
	    y = integer_val (y_arg);
	  else
	    y = gdImageSY (images[slot]) / 2;
	  
	  /* Convert the color into an index.  If this image doesn't already
	     have this color, add it now. */
	  if (empty_string_p (c_arg))
	    c = rgb_to_index (images[slot], "000000", 1);
	  else
	    c = rgb_to_index (images[slot], c_arg, 1);

	  if (!empty_string_p (size_arg) && integer_p (size_arg, 10))
	    size = atoi (size_arg);
	  else
	    size = 3;

	  switch (size)
	    {
	    case 1:
	      font = gdFontTiny;
	      break;

	    case 2:
	      font = gdFontSmall;
	      break;

	    case 3:
	      font = gdFontMediumBold;
	      break;

	    case 4:
	      font = gdFontLarge;

	    default:
	      font = gdFontMediumBold;
	    }

	  /* Finally, handle the alignment. */
	  {
	    char *align = "center";
	    int char_width = font->w;
	    int char_height = font->h;
	    int len = strlen (text);

	    /* Adjust Y coordinate to be the center of the character. */
	    y -= char_height / 2;

	    if (!empty_string_p (align_arg))
	      align = align_arg;

	    if (strcasecmp (align, "left") == 0)
	      {
		/* When we're aligned left, do nothing special. */
	      }
	    else if (strcasecmp (align, "right") == 0)
	      {
		/* Make the last character of the string end on X. */
		x -= char_width * len;
	      }
	    else if (strcasecmp (align, "center") == 0)
	      {
		/* Make the center character of the string land on X. */
		x -= ((char_width * len) / 2);
	      }

	    if (info != (Package *)NULL)
	      {
		mhtml_set_numeric_variable_in_package
		  (info, "width", (len * char_width));

		mhtml_set_numeric_variable_in_package
		  (info, "char-width", char_width);

		mhtml_set_numeric_variable_in_package
		  (info, "height", char_height);

		mhtml_set_numeric_variable_in_package
		  (info, "char-height", char_height);

		mhtml_set_numeric_variable_in_package (info, "left", x);
		mhtml_set_numeric_variable_in_package
		  (info, "right", x + (len * char_width));
	      }
	  }

	  if (info == (Package *)NULL)
	    {
	      /* Okay, draw the damn string already! */
	      gdImageString (images[slot], font, x, y, text, c);
	    }
	  else
	    {
	      char *result = package_to_alist (info, 0);
	      int len = (strlen (result));
	      symbol_destroy_package (info);

	      bprintf_insert (page, start, "%s", result);
	      *newstart += len;
	    }

	  xfree (x_arg);
	  xfree (y_arg);
	  xfree (c_arg);
	  xfree (size_arg);
	  xfree (align_arg);
	}

      xfree (text);
    }
}

DEFUNX (image::line, imagevar &key x1 y1 x2 y2 color brush,
"Draw a line in <var imagevar> from (<var x1>, <var y1>) to
(<var x2>, <var y2>) in the color <var color>.
<var brush>, if supplied, is another image created with
<funref image-module image::create>, that will be used as the brush to
paint the line.  In that case, the <var color> argument is ignored.")
static void
pf_image_line (PFunArgs)
{
  int slot = get_image_arg ("LINE", vars);

  if (slot > -1)
    {
      gdImage *image = images[slot];
      char *x1_arg = mhtml_evaluate_string (get_value (vars, "X1"));
      char *y1_arg = mhtml_evaluate_string (get_value (vars, "Y1"));
      char *x2_arg = mhtml_evaluate_string (get_value (vars, "X2"));
      char *y2_arg = mhtml_evaluate_string (get_value (vars, "Y2"));
      char *c_arg = mhtml_evaluate_string (get_value (vars, "COLOR"));
      char *brush_arg = mhtml_evaluate_string (get_value (vars, "BRUSH"));

      if ((!empty_string_p (x1_arg) && number_p (x1_arg)) &&
	  (!empty_string_p (x2_arg) && number_p (x2_arg)) &&
	  (!empty_string_p (y1_arg) && number_p (y1_arg)) &&
	  (!empty_string_p (y2_arg) && number_p (y2_arg)))
	{
	  int x1 = integer_val (x1_arg);
	  int y1 = integer_val (y1_arg);
	  int x2 = integer_val (x2_arg);
	  int y2 = integer_val (y2_arg);
	  int c = 0;
	  char *temp_color;

	  /* Convert the color into an index.  If this image
	     doesn't already have this color, add it now. */
	  if (!empty_string_p (c_arg))
	    temp_color = c_arg;
	  else
	    temp_color = "000000";

	  c = rgb_to_index (image, temp_color, 1);

	  if (!empty_string_p (brush_arg))
	    {
	      int brush_slot = dereference_image_var ("LINE", brush_arg);
	      gdImage *brush = brush_slot > -1 ? images[brush_slot] : NULL;

	      if (brush != (gdImage *)NULL)
		gdImageSetBrush (image, brush);
	    }

	  gdImageLine (image, x1, y1, x2, y2, c);

	  if (!empty_string_p (brush_arg))
	    gdImageSetBrush (image, (gdImage *)NULL);
	}
      else
	page_debug
	  ("<image::line ..> requires all of X1, Y1, X2, Y2 and COLOR");

      xfree (x1_arg);
      xfree (x2_arg);
      xfree (y1_arg);
      xfree (y2_arg);
      xfree (c_arg);
      xfree (brush_arg);
    }
}

DEFUNX (image::fill, imagevar &key x y color border,
"Fill an area of the image in <var imagevar> with the color specified by
<var color>.  The filling starts at the point specified by (<var x>, <var y>),
and continues in all directions bounded by pixels which are not the same
color as the color at (<var x>, <var y>), or, optionally, which are not the
same color as the color specified by <var border>.")

static void
pf_image_fill (PFunArgs)
{
  int slot = get_image_arg ("FILL", vars);

  if (slot > -1)
    {
      gdImage *image = images[slot];
      char *x_arg = mhtml_evaluate_string (get_value (vars, "X"));
      char *y_arg = mhtml_evaluate_string (get_value (vars, "Y"));
      char *c_arg = mhtml_evaluate_string (get_value (vars, "COLOR"));
      char *b_arg = mhtml_evaluate_string (get_value (vars, "BORDER"));

      if ((!empty_string_p (x_arg) && number_p (x_arg)) &&
	  (!empty_string_p (y_arg) && number_p (y_arg)) &&
	  (!empty_string_p (c_arg)))
	{
	  int x = integer_val (x_arg);
	  int y = integer_val (y_arg);
	  int c = -1;

	  /* Convert the color into an index.  If this image doesn't already
	     have this color, add it now. */
	  c = rgb_to_index (image, c_arg, 1);

	  if (!empty_string_p (b_arg))
	    {
	      int b = rgb_to_index (image, b_arg, 1);

	      gdImageFillToBorder (image, x, y, b, c);
	    }
	  else
	    {
	      gdImageFill (image, x, y, c);
	    }
	}
      else
	page_debug ("<image::fill ..> requires all of X, Y, and COLOR");

      xfree (x_arg);
      xfree (y_arg);
      xfree (c_arg);
      xfree (b_arg);
    }
}

DEFUNX (image::rect, imagevar &key x1 y1 x2 y2 color fill,
"Draw a rectangle with the border lines in the color <var color> and perhaps
filled with the color <var fill>.  The rectangle is drawn with the upper-left
corner specified by (<var x1> <var y1>) and the bottom-right corner specified
by (<var x2>, <var y2>).")

static void
pf_image_rect (PFunArgs)
{
  int slot = get_image_arg ("RECT", vars);

  if (slot > -1)
    {
      gdImage *image = images[slot];
      char *x1_arg = mhtml_evaluate_string (get_value (vars, "X1"));
      char *y1_arg = mhtml_evaluate_string (get_value (vars, "Y1"));
      char *x2_arg = mhtml_evaluate_string (get_value (vars, "X2"));
      char *y2_arg = mhtml_evaluate_string (get_value (vars, "Y2"));
      char *c_arg = mhtml_evaluate_string (get_value (vars, "color"));
      char *filled_arg = mhtml_evaluate_string (get_value (vars, "fill"));
      int filled = !empty_string_p (filled_arg);
      int fill_color = -1;

      if ((!empty_string_p (x1_arg) && number_p (x1_arg)) &&
	  (!empty_string_p (x2_arg) && number_p (x2_arg)) &&
	  (!empty_string_p (y1_arg) && number_p (y1_arg)) &&
	  (!empty_string_p (y2_arg) && number_p (y2_arg)) &&
	  (!empty_string_p (c_arg)))
	{
	  int x1 = integer_val (x1_arg);
	  int y1 = integer_val (y1_arg);
	  int x2 = integer_val (x2_arg);
	  int y2 = integer_val (y2_arg);
	  int c = -1;

	  /* Convert the color into an index.  If this image doesn't already
	     have this color, add it now. */
	  c = rgb_to_index (image, c_arg, 1);

	  if (filled)
	    {
	      fill_color = rgb_to_index (image, filled_arg, 1);
	      if (fill_color == -1)
		fill_color = c;

	      gdImageFilledRectangle (image, x1, y1, x2, y2, fill_color);
	    }

	    gdImageRectangle (image, x1, y1, x2, y2, c);
	}
      else
	page_debug
	  ("<image::rect ..> requires all of X1, Y1, X2, Y2, and COLOR");

      xfree (x1_arg);
      xfree (x2_arg);
      xfree (y1_arg);
      xfree (y2_arg);
      xfree (c_arg);
      xfree (filled_arg);
    }
}

DEFUNX (image::arc, imagevar &key x y width height start end color fill,
"Draws a partial ellipse centered at the point specified by <var x> and
<var y>, with a width of <var width> and height of <var height>.

The arguments of <var start> and <var end> are given in degrees, and specify
the starting and ending points on the curve.

The following code draws a red circle with a radius of 50 pixels where
the exact center of the circle appears at 100,100:
<example>
<image::arc image x=100 y=100 width=50 height=50 start=0 end=360 color=FF0000>
</example>")

static void
pf_image_arc (PFunArgs)
{
  int slot = get_image_arg ("ARC", vars);

  if (slot > -1)
    {
      gdImage *image = images[slot];
      char *x_arg = mhtml_evaluate_string (get_value (vars, "X"));
      char *y_arg = mhtml_evaluate_string (get_value (vars, "Y"));
      char *w_arg = mhtml_evaluate_string (get_value (vars, "WIDTH"));
      char *h_arg = mhtml_evaluate_string (get_value (vars, "HEIGHT"));
      char *s_arg = mhtml_evaluate_string (get_value (vars, "START"));
      char *e_arg = mhtml_evaluate_string (get_value (vars, "END"));
      char *c_arg = mhtml_evaluate_string (get_value (vars, "COLOR"));
      char *filled_arg = mhtml_evaluate_string (get_value (vars, "FILL"));
      int filled = !empty_string_p (filled_arg);
      int fill_color = -1;

      if ((!empty_string_p (x_arg) && number_p (x_arg)) &&
	  (!empty_string_p (y_arg) && number_p (y_arg)) &&
	  (!empty_string_p (w_arg) && number_p (w_arg)) &&
	  (!empty_string_p (h_arg) && number_p (h_arg)) &&
	  (!empty_string_p (s_arg) && number_p (s_arg)) &&
	  (!empty_string_p (e_arg) && number_p (e_arg)) &&
	  (!empty_string_p (c_arg)))
	{
	  int x = integer_val (x_arg);
	  int y = integer_val (y_arg);
	  int w = integer_val (w_arg);
	  int h = integer_val (h_arg);
	  int s = 270 + integer_val (s_arg);
	  int e = 270 + integer_val (e_arg);
	  int c = -1;

	  /* Convert the color into an index.  If this image doesn't already
	     have this color, add it now. */
	  c = rgb_to_index (image, c_arg, 1);

	  if (filled)
	    {
	      fill_color = rgb_to_index (image, filled_arg, 1);
	      if (fill_color == -1)
		fill_color = c;
	    }

	  if (debug_level > 5)
	    page_debug ("<IMAGE::ARC x=%d y=%d w=%d h=%d s=%d e=%d c=%d>",
			x, y, w, h, s, e, c);
	  gdImageArc (image, x, y, w, h, s, e, c);

	  if (filled && (s != e))
	    {
	      int w2 = w / 2;
	      int h2 = h / 2;
	      int lx, ly;

	      while (s > e) e += 360;

	      /* Draw the lines from the center to the endpoints of the arc. */
	      lx = ((long)cost[s % 360] * (long)w2 / costScale) + x;
	      ly = ((long)sint[s % 360] * (long)h2 / sintScale) + y;
	      gdImageLine (image, x, y, lx, ly, c);

	      lx = ((long)cost[e % 360] * (long)w2 / costScale) + x;
	      ly = ((long)sint[e % 360] * (long)h2 / sintScale) + y;
	      gdImageLine (image, x, y, lx, ly, c);

	      /* Get a point inside the sealed arc. */
	      w2--; h2--;
	      lx = ((long)cost[(s + 1) % 360] * (long)w2 / costScale) + x;
	      ly = ((long)sint[(s + 1) % 360] * (long)h2 / sintScale) + y;

	      /* Now fill this area. */
	      gdImageFillToBorder (image, lx, ly, c, fill_color);
	    }
	}
      else
	page_debug
	  ("<image::arc ..> requires all of X, Y, WIDTH, HEIGHT, START, END, and COLOR");

      xfree (x_arg);
      xfree (y_arg);
      xfree (w_arg);
      xfree (h_arg);
      xfree (s_arg);
      xfree (e_arg);
      xfree (c_arg);
      xfree (filled_arg);
    }
}

DEFUNX (image::write, imagevar filename,
"Writes the contents of the image in <var imagevar> to the file specified
by <var filename>.  Returns \"true\" if the image was successfully written.")
static void
pf_image_write (PFunArgs)
{
  int slot = get_image_arg ("WRITE", vars);
  char *result = (char *)NULL;

  if (slot != -1)
    {
      char *output = mhtml_evaluate_string (get_positional_arg (vars, 1));

      if (!empty_string_p (output))
	{
	  FILE *stream = fopen (output, "wb");

	  if (stream != (FILE *)NULL)
	    {
	      gdImageGif (images[slot], stream);
	      fclose (stream);
	      result = "true";
	    }
	  else
	    page_debug ("IMAGE::WRITE Couldn't open %s: %s",
			output, strerror (errno));
	}
      else
	page_debug ("IMAGE::WRITE requires a filename to write the image to");

      xfree (output);
    }
  if (result != (char *)NULL)
    bprintf_insert (page, start, "%s", result);
}

DEFUNX (image::transparent, imagevar &key color,
"Makes <var color> be the transparent one for this image.")

static void
pf_image_transparent (PFunArgs)
{
  int slot = get_image_arg ("SET-TRANSPARENT", vars);

  if (slot > -1)
    {
      gdImage *image = images[slot];
      char *c_arg = mhtml_evaluate_string (get_value (vars, "COLOR"));

      if (!empty_string_p (c_arg))
	{
	  int c = -1;

	  /* Convert the color into an index.  If this image doesn't already
	     have this color, add it now. */
	  c = rgb_to_index (image, c_arg, 1);

	  if (c != -1)
	    gdImageColorTransparent (image, c);
	}
    }
}

static void
pf_image_poly (PFunArgs)
{
}

DEFUNX (image::copy, src-image dst-image &key src-x src-y dst-x dst-y src-width src-height dst-width dst-height,
"Copies bits from <var src-image> to <var dst-image>.
Both images must exist.
If a different width or height is specified for the destination, the image is
resized to fit the specified values.")
static void
pf_image_copy (PFunArgs)
{
  int source_slot = get_image_arg_1 ("COPY", vars, 0);
  int dest_slot = get_image_arg_1 ("COPY", vars, 1);

  if ((source_slot != -1) && (dest_slot != -1))
    {
      gdImage *source = images[source_slot];
      gdImage *dest = images[dest_slot];
      char *srcx_arg = mhtml_evaluate_string
	(get_one_of (vars, "SOURCE-X", "SRC-X", (char *)NULL));
      char *srcy_arg = mhtml_evaluate_string
	(get_one_of (vars, "SOURCE-Y", "SRC-Y", (char *)NULL));
      char *dstx_arg = mhtml_evaluate_string
	(get_one_of (vars, "DEST-X", "DST-X", (char *)NULL));
      char *dsty_arg = mhtml_evaluate_string
	(get_one_of (vars, "DEST-Y", "DST-Y", (char *)NULL));
      char *srcw_arg = mhtml_evaluate_string
	(get_one_of (vars, "SOURCE-WIDTH", "SRC-W", "SRC-WIDTH", "WIDTH",
		     (char *)NULL));
      char *srch_arg = mhtml_evaluate_string
	(get_one_of (vars, "SOURCE-HEIGHT", "SRC-H", "SRC-HEIGHT", "HEIGHT",
		     (char *)NULL));
      char *dstw_arg = mhtml_evaluate_string
	(get_one_of (vars, "DEST-WIDTH", "DST-W", "DST-WIDTH", (char *)NULL));
      char *dsth_arg = mhtml_evaluate_string
	(get_one_of (vars, "DEST-HEIGHT", "DST-H", "DST-HEIGHT",
		     (char *)NULL));
      int src_x, src_y, dst_x, dst_y, src_w, dst_w, src_h, dst_h;

      if (!empty_string_p (srcx_arg) && (number_p (srcx_arg)))
	src_x = integer_val (srcx_arg);
      else
	src_x = 0;

      if (!empty_string_p (dstx_arg) && (number_p (dstx_arg)))
	dst_x = integer_val (dstx_arg);
      else
	dst_x = src_x;

      if (!empty_string_p (srcy_arg) && (number_p (srcy_arg)))
	src_y = integer_val (srcy_arg);
      else
	src_y = 0;

      if (!empty_string_p (dsty_arg) && (number_p (dsty_arg)))
	dst_y = integer_val (dsty_arg);
      else
	dst_y = src_y;

      if (!empty_string_p (srcw_arg) && (number_p (srcw_arg)))
	src_w = integer_val (srcw_arg);
      else
	src_w = gdImageSX (source);

      if (!empty_string_p (dstw_arg) && (number_p (dstw_arg)))
	dst_w = integer_val (dstw_arg);
      else
	dst_w = gdImageSX (dest);

      if (!empty_string_p (srch_arg) && (number_p (srch_arg)))
	src_h = integer_val (srch_arg);
      else
	src_h = gdImageSY (source);

      if (!empty_string_p (dsth_arg) && (number_p (dsth_arg)))
	dst_h = integer_val (dsth_arg);
      else
	dst_h = gdImageSY (dest);

      gdImageCopyResized (dest, source,
			  dst_x, dst_y, src_x, src_y,
			  dst_w, dst_h, src_w, src_h);
      xfree (srcx_arg); xfree (srcy_arg);
      xfree (srcw_arg); xfree (srch_arg);
      xfree (dstx_arg); xfree (dsty_arg);
      xfree (dstw_arg); xfree (dsth_arg);
    }
}

#if defined (__cplusplus)
}
#endif
