/* createdoc.c: -*- C -*-  Create documentation for the builtin functions. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jul 19 15:48:27 1997.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1995, 2001, Brian J. Fox (bfox@ai.mit.edu).

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

#include "mhtmlstd.h"

#if defined (__cplusplus)
extern "C"
{
#endif

typedef struct
{
  char *name;
  char *source_filename;
  int lineno;
  char *section;
  int type;
  char **reqargs;
  char **keyargs;
  char **optargs;
  char *restarg;
  char *documentation;
} funinfo;

typedef struct
{
  char *name;
  char *source_filename;
  int lineno;
  char *keywords;
  char *short_info;
  char *long_info;
} secinfo;

#define user_DEFUN 1
#define user_MACRO 2
#define user_DEFVAR 3

#undef whitespace
#define whitespace(x) ((x == ' ') || (x == '\t') || (x == '\n'))
#define skip_whitespace \
   while (whitespace (buffer[i])) { if (buffer[i] == '\n') lineno++; i++; }

static funinfo **info_array = (funinfo **)NULL;
static int info_array_slots = 0;
static int info_array_index = 0;

static secinfo **secinfo_array = (secinfo **)NULL;
static int secinfo_slots = 0;
static int secinfo_index = 0;

static char **source_files = (char **)NULL;
static int sf_slots = 0;
static int sf_index = 0;

static char *current_section = (char *)NULL;
static char *output_filename = (char *)NULL;
static char *sections_filename = (char *)NULL;
static char **sections = (char **)NULL;
static int sections_index = 0;
static int sections_slots = 0;

static void
set_current_section (char *name)
{
  register int i;

  current_section = (char *)NULL;

  /* Kill leading whitespace. */
  while (whitespace (*name)) name++;

  /* Kill trailing whitespace. */
  for (i = strlen (name) - 1; i > -1; i--)
    if (whitespace (name[i])) name[i] = '\0';

  /* Canonicalize the name of this section. */
  for (i = 0; name[i] != '\0'; i++)
    {
      if (islower (name[i])) name[i] = toupper (name[i]);
      if (whitespace (name[i])) name[i] = '-';
    }

  for (i = 0; i < sections_index; i++)
    if (strcmp (name, sections[i]) == 0)
      {
	current_section = sections[i];
	break;
      }

  if (!current_section)
    {
      if (sections_index + 2 > sections_slots)
	sections = (char **)xrealloc
	  (sections, (sections_slots += 10) * sizeof (char *));

      current_section = strdup (name);
      sections[sections_index++] = current_section;
      sections[sections_index] = (char *)NULL;
    }
}

static void
output_array (FILE *stream, char *name, char **values)
{

  if (values != (char **)NULL)
    {
      fprintf (stream, "(\"%s\"", name);

      if (values[0])
	{
	  register int i;

	  if (!values[1])
	    fprintf (stream, " .");

	  for (i = 0; values[i] != (char *)NULL; i++)
	    fprintf (stream, " \"%s\"", values[i]);
	}

      fprintf (stream, ")");
    }
}

static void
output_function (FILE *stream, funinfo *info)
{
  register int i;
  static char *funname = (char *)NULL;
  int funname_size = 0;
  int l = strlen (info->name);

  /* Get the function name. */
  if (l + 2 > funname_size)
    funname = (char *)xrealloc (funname, funname_size += (l + 10));

  if (strncasecmp (info->name, "pf_", 3) == 0)
    strcpy (funname, info->name + 3);
  else
    strcpy (funname, info->name);

  for (i = 0; funname[i]; i++)
    if (islower (funname[i]))
      funname[i] = toupper (funname[i]);
    else if ((funname[i] == '_') && (info->type != user_DEFVAR))
      funname[i] = '-';

  /* Start the alist. */
  fprintf (stream, "(\"%s\" (", funname);

  /* Print the pretty name of this function. */
  for (i = 0; funname[i]; i++)
    if (isupper (funname[i]))
      funname[i] = tolower (funname[i]);

  fprintf (stream, "(\"NAME\" . \"%s\")", funname);

  /* Print the file and line number. */
  fprintf (stream, "(\"SOURCE-FILE\" . \"%s\")(\"LINE-NUMBER\" . %d)",
	   info->source_filename, info->lineno);

  /* Print the section. */
  fprintf (stream, "(\"SECTION\" . \"%s\")", info->section);

  /* Print the type. */
  switch (info->type)
    {
    case user_DEFUN:
      fprintf (stream, "(\"FUNTYPE\" . \"simple\")");
      break;
    case user_MACRO:
      fprintf (stream, "(\"FUNTYPE\" . \"complex\")");
      break;
    case user_DEFVAR:
      fprintf (stream, "(\"FUNTYPE\" . \"variable\")");
      break;

    default:
      fprintf (stream, "(\"FUNTYPE\" . \"BROKEN-DOCUMENTATION\")");
    }

  /* Print the required arguments. */
  output_array (stream, "REQ-ARGS", info->reqargs);

  /* Print the &optional arguments. */
  output_array (stream, "OPT-ARGS", info->optargs);

  /* Print the &key arguments. */
  output_array (stream, "KEY-ARGS", info->keyargs);

  /* Print the &rest arguments. */
  if (info->restarg)
    fprintf (stream, "(\"REST-ARGS\" . \"%s\")", info->restarg);

  /* Print the description.  Right now, it is always the short-desc. */
  fprintf (stream, "(\"SHORT-DESC\" . \"%s\")", info->documentation);

  /* End this alist. */
  fprintf (stream, "))");
}

static void
output_section (FILE *stream, secinfo *info)
{
  /* Start the alist. */
  fprintf (stream, "(\"%s\"", info->name);

  /* Print the file and line number. */
  fprintf (stream, "\n  ((\"SOURCE-FILE\" . \"%s\")\n  (\"LINE-NUMBER\" . %d)",
	   info->source_filename, info->lineno);

  /* Print the section. */
  fprintf (stream, "\n  (\"SECTION\" . \"%s\")", info->name);

  /* Print the keywords arguments. */
  fprintf (stream, "\n  (\"KEYWORDS\" . \"%s\")", info->keywords);

  /* Print the short info. */
  fprintf (stream, "\n  (\"SHORT-DESC\" . %s)",
	   info->short_info ? info->short_info : "\"\"");

  /* Print the long info. */
  fprintf (stream, "\n  (\"LONG-DESC\" . %s)",
	   info->long_info ? info->long_info : "\"\"");

  /* End this alist. */
  fprintf (stream, "))");
}

static void
output_documentation (void)
{
  FILE *stream;

  if (output_filename)
    stream = fopen (output_filename, "w");
  else
    stream = stdout;

  if (info_array_index)
    {
      register int i;
      funinfo *info;

      for (i = 0; (info = info_array[i]) != (funinfo *)NULL; i++)
	output_function (stream, info);
    }

  if (stream != stdout)
    fclose (stream);

  /* Now, output the sections. */
  if (sections_filename)
    stream = fopen (sections_filename, "w");
  else
    stream = stdout;

  if (secinfo_index)
    {
      register int i;
      secinfo *info;

      for (i = 0; (info = secinfo_array[i]) != (secinfo *)NULL; i++)
	output_section (stream, info);
    }

  if (stream != stdout)
    fclose (stream);
}

static void
split_arguments (funinfo *info, char *argstring)
{
  register int i = 0;
  char **args = (char **)NULL;
  int slots = 0, idx = 0;
  int start = 0;
  int gathering_required = 1;
  int gathering_optional = 0;
  int gathering_keys = 0;
  int done = 0;

  while (!done)
    {
      while (whitespace (argstring[i])) i++;
      if (!argstring[i]) break;

      start = i;
      while (argstring[i] && !whitespace (argstring[i])) i++;

      {
	int len = i - start;
	char *arg = (char *)xmalloc (1 + len);
	strncpy (arg, argstring + start, len);
	arg[len] = '\0';

	if (*arg == '&')
	  {
	    if (gathering_required)
	      {
		gathering_required = 0;
		info->reqargs = args;
	      }
	    else if (gathering_optional)
	      {
		gathering_optional = 0;
		info->optargs = args;
	      }
	    else if (gathering_keys)
	      {
		gathering_keys = 0;
		info->keyargs = args;
	      }
	    
	    /* No matter what, we had best taken care of the existing ARGS. */
	    args = (char **)NULL;
	    slots = idx = 0;
	      
	    if (strcmp (arg, "&optional") == 0)
	      {
		gathering_optional = 1;
		continue;
	      }
	    else if (strcmp (arg, "&key") == 0)
	      {
		gathering_keys = 1;
		continue;
	      }
	    else if (strcmp (arg, "&rest") == 0)
	      {
		info->restarg = strdup (argstring + i + 1);
		break;
	      }
	  }

	if (idx + 2 > slots)
	  args = (char **)xrealloc (args, (slots += 4) * sizeof (char *));

	args[idx++] = arg;
	args[idx] = (char *)NULL;
      }
    }

  /* If there are any arguments left, put them where they belong. */
  if (idx > 0)
    {
      if (gathering_required)
	{
	  gathering_required = 0;
	  info->reqargs = args;
	}
      else if (gathering_optional)
	{
	  gathering_optional = 0;
	  info->optargs = args;
	}
      else if (gathering_keys)
	{
	  gathering_keys = 0;
	  info->keyargs = args;
	}
    }
}

static char *
gobble_string (char *buffer, int *start, int *length)
{
  register int i = *start;
  register int c;
  int quoted = 0;
  char *result;
  int lineno = 0;
  int done = 0;

  skip_whitespace;
  *start = i;

  while (!done)
    {
      c = buffer[i];

      switch (c)
	{
	case '\0': done = 1; break;
	case '\n': lineno++; break;
	case '\\': i++; break;
	case '"':
	  quoted = !quoted;
	  break;

	case ',':
	case ')':
	  if (!quoted)
	    {
	      done = 1;
	      continue;
	    }
	}
      i++;
    }
    
    *length = i - *start;
    result = (char *)xmalloc (1 + *length);
    strncpy (result, buffer + *start, *length);
    result[*length] = '\0';
    *start += (*length);

    return (result);
}

static void
gather_documentation (char *filename)
{
  struct stat finfo;

  if ((stat (filename, &finfo)) != -1)
    {
      int fd = open (filename, O_RDONLY, 0666);

      if (fd != -1)
	{
	  register int i;
	  char *buffer = (char *)xmalloc (1 + ((int)finfo.st_size));
	  int lineno = 1;
	  
	  read (fd, buffer, (int)finfo.st_size);
	  close (fd);
	  buffer[(int)finfo.st_size] = '\0';

	  /* Find occurrences of DOC_SECTION, DEFUN, DEFMACRO, DEFVAR,
	     DEFUNX, DEFMACROX, and DEFINE_SECTION. */
	  for (i = 0; buffer[i] != '\0'; i++)
	    {
	      if (buffer[i] == '\n')
		{
		  static char section_buffer[512];

		  lineno++;

		  if (strncmp (buffer + i + 1, "DEFVAR", 6) == 0)
		    {
		      funinfo *info = (funinfo *)xmalloc (sizeof (funinfo));
		      int varstart, varlen;

		      memset (info, 0, sizeof (funinfo));

		      info->lineno = lineno;
		      info->source_filename = source_files[sf_index - 1];
		      info->section = current_section;
		      info->type = user_DEFVAR;

		      /* Find starting open paren. */
		      while (buffer[i] != '(')
			{
			  if (buffer[i] == '\n') lineno++;
			  i++;
			}

		      i++;
		      skip_whitespace;
		      varstart = i;

		      /* Gobble until comma or close paren, make that word
			 the current variable name. */
		      while ((buffer[i] != ',') && (buffer[i] != ')')) i++;
		      varlen = i - varstart;
		      info->name = (char *)xmalloc (1 + varlen);
		      strncpy (info->name, buffer + varstart, varlen);
		      info->name[varlen] = '\0';

		      /* Gather documentation for this variable. */
		      if (buffer[i] == ',') i++;
		      skip_whitespace;
		      varstart = i;

		      {
			char *temp;
			temp = gobble_string (buffer, &varstart, &varlen);
			if (*temp == '"')
			  {
			    register int j = strlen (temp) - 1;
			    while (whitespace (temp[j])) j--;
			    if (temp[j] == '"') temp[j--] = '\0';
			    while (whitespace (temp[j])) j--;
			    temp[++j] = '\0';
			    
			    info->documentation = strdup (temp + 1);
			    free (temp);
			  }
			else info->documentation = temp;
		      }

		      i = varstart;

		      if (info_array_index + 2 > info_array_slots)
			info_array = (funinfo **)xrealloc
			  (info_array, (info_array_slots += 50)
			   * sizeof (funinfo *));
		      info_array[info_array_index++] = info;
		      info_array[info_array_index] = (funinfo *)NULL;
		    }

		  if (strncmp (buffer + i + 1, "DOC_SECTION", 11) == 0)
		    {
		      int secstart, seclen;

		      /* Find starting open paren. */
		      while (buffer[i] != '(') i++;
		      i++;
		      secstart = i;

		      /* Gobble until close paren, make that word the current
			 section name. */
		      while (buffer[i] != ')') i++;
		      seclen = i - secstart;
		      strncpy (section_buffer, buffer + secstart, seclen);
		      section_buffer[seclen] = '\0';
		      set_current_section (section_buffer);
		      continue;
		    }

		  if (strncmp (buffer + i + 1, "DEFINE_SECTION", 14) == 0)
		    {
		      secinfo *info = (secinfo *)xmalloc (sizeof (secinfo));
		      int secstart, seclen;

		      memset (info, 0, sizeof (secinfo));

		      info->lineno = lineno;
		      info->source_filename = source_files[sf_index - 1];

		      /* Find starting open paren. */
		      while (buffer[i] != '(')
			{
			  if (buffer[i] == '\n') lineno++;
			  i++;
			}

		      i++;
		      skip_whitespace;
		      secstart = i;

		      /* Gobble until comma or close paren, make that word
			 the current section name. */
		      while ((buffer[i] != ',') && (buffer[i] != ')')) i++;
		      seclen = i - secstart;
		      strncpy (section_buffer, buffer + secstart, seclen);
		      section_buffer[seclen] = '\0';
		      set_current_section (section_buffer);
		      info->name = strdup (current_section);

		      /* Gather keywords for this section. */
		      if (buffer[i] == ',') i++;
		      skip_whitespace;
		      secstart = i;

		      while (buffer[i] != ',') i++;
		      seclen = i - secstart;
		      info->keywords = (char *)xmalloc (1 + seclen);
		      strncpy (info->keywords, buffer + secstart, seclen);
		      info->keywords[seclen] = '\0';

		      /* Gather short documentation for this section. */
		      if (buffer[i] == ',') i++;
		      skip_whitespace;
		      secstart = i;

		      info->short_info =
			gobble_string (buffer, &secstart, &seclen);

		      /* Gather long documentation for this section. */
		      i = secstart;
		      if (buffer[i] == ',') i++;
		      skip_whitespace;
		      secstart = i;

		      info->long_info =
			gobble_string (buffer, &secstart, &seclen);

		      /* Add this section to our list. */
		      if (secinfo_index + 2 > secinfo_slots)
			secinfo_array = (secinfo **)xrealloc
			  (secinfo_array, (secinfo_slots += 10)
			   * sizeof (secinfo *));

		      secinfo_array[secinfo_index++] = info;
		      secinfo_array[secinfo_index] = '\0';
		      continue;
		    }

		  if (strncmp (buffer + i + 1, "DEF", 3) == 0)
		    {
		      funinfo *info = (funinfo *)xmalloc (sizeof (funinfo));
		      int start, len;

		      memset (info, 0, sizeof (funinfo));
		      info->lineno = lineno;
		      info->source_filename = source_files[sf_index - 1];
		      info->section = current_section;

		      if (strncmp (buffer + i + 1, "DEFUN", 5) == 0)
			info->type = user_DEFUN;
		      else
			info->type = user_MACRO;

		      /* Find starting open paren. */
		      i++;
		      while (buffer[i] != '(')
			{
			  if (buffer[i] == '\n') lineno++;
			  i++;
			}

		      i++;

		      skip_whitespace;
		      start = i;

		      /* Get the function name. */
		      while (buffer[i] != ',') i++;
		      len = i - start;
		      info->name = (char *)xmalloc (1 + len);
		      strncpy (info->name, buffer + start, len);
		      info->name[len] = '\0';
		      i++;
		      skip_whitespace;
		      start = i;

		      /* Next comes the arguments of the function. */
		      while (buffer[i] != ',')
			{
			  if (buffer[i] == '\n') lineno++;
			  i++;
			}
		      buffer[i] = '\0';
		      split_arguments (info, buffer + start);
		      i++;
		      skip_whitespace;
		      i++;
		      start = i;

		      /* Finally, the documentation string. */
		      {
			register int c;
			int quoted = 1;

			while ((c = buffer[i]) != '\0')
			  {
			    switch (c)
			      {
			      case '\n': lineno++; break;
			      case '\\': i++; break;
			      case '"':
				quoted = !quoted;

				if (!quoted)
				  {
				    buffer[i] = '\0';
				    i--;
				  }
				break;
			      }
			    i++;
			  }

			len = i - start;
			i++;
		      }

		      info->documentation = (char *)xmalloc (1 + len);
		      strncpy (info->documentation, buffer + start, len);
		      info->documentation[len] = '\0';

		      if (info_array_index + 2 > info_array_slots)
			info_array = (funinfo **)xrealloc
			  (info_array, (info_array_slots += 50)
			   * sizeof (funinfo *));
		      info_array[info_array_index++] = info;
		      info_array[info_array_index] = (funinfo *)NULL;
		    }
		}
	    }
	}
    }
}

int
main (int argc, char *argv[])
{
  register int i;
  struct stat finfo;

  for (i = 1; i < argc; i++)
    {
      char *filename = strdup (argv[i]);
      char *temp = strrchr (filename, '.');
      int failed = 0;

      if (strcmp (filename, "-o") == 0)
	{
	  i++;
	  output_filename = argv[i];
	  continue;
	}

      if (strcmp (filename, "-s") == 0)
	{
	  i++;
	  sections_filename = argv[i];
	  continue;
	}

      if (temp)
	{
	  strcpy (temp, ".c");

	  if (stat (filename, &finfo) == -1)
	    {
	      strcpy (temp, ".cc");
	      if (stat (filename, &finfo) == -1)
		{
		  strcpy (temp, ".C");
		  if (stat (filename, &finfo) == -1)
		    {
		      strcpy (temp, ".mhtml");
		      if (stat (filename, &finfo) == -1)
			{
			  fprintf (stderr,
				   "createdoc: Couldn't find any of: ");
			  *temp = '\0';
			  fprintf (stderr, "%s.c, %s.cc, %s.C or %s.mhtml\n",
				   filename, filename, filename, filename);
			  failed++;
			}
		    }
		}
	    }
	}

      if (!failed)
	{
	  if (sf_index + 2 > sf_slots)
	    source_files = (char **)xrealloc
	      (source_files, (sf_slots += 10) * sizeof (char *));

	  source_files[sf_index++] = strdup (filename);
	  source_files[sf_index] = (char *)NULL;

	  gather_documentation (filename);
	}
    }

  output_documentation ();
  return (0);
}

#if defined (__cplusplus)
}
#endif
