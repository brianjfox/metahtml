/* symbols.c: Functions for manipulating language symbols. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Mon Aug 21 11:12:04 1995.  */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <xmalloc/xmalloc.h>
#include "symbols.h"

/* The Mac only defines "macintosh" in sys/types.h.  Sigh... */
#include <sys/types.h>

#if defined (macintosh)
#  include <mac_port.h>
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

SYMBOL_FUNCTION *symbol_retrieve_hook = (SYMBOL_FUNCTION *)NULL;
SYMBOL_FUNCTION *symbol_intern_hook = (SYMBOL_FUNCTION *)NULL;
SYMBOL_FUNCTION *symbol_free_hook = (SYMBOL_FUNCTION *)NULL;
SYMBOL_MACHFILL_FUNCTION *symbol_machfill_hook =
			(SYMBOL_MACHFILL_FUNCTION *)NULL;
SYMBOL_MACHGET_FUNCTION *symbol_machget_hook =
			(SYMBOL_MACHGET_FUNCTION *)NULL;

/* Set to non-zero when mhtml::flag-newly-interned-symbols is set. */
int mhtml_flag_newly_interned_symbols = 0;

/* Boy, I hate to do this, but I have to. */
#if defined (METAHTML_PROFILER) /* Symbol used because it is true for Mhtml. */

typedef struct
{
  int type;		/* Either user_MACRO, user_SUBST, or user_DEFUN. */
  int flags;		/* Interesting bits about this function. */
  int debug_level;	/* How much debugging to do. */
  char *name;		/* The name of this macro, function, or subst. */
  char *body;		/* The body of this macro function, or subst. */
  char *packname;	/* Default package for the scope of this function. */
  char **named_parameters; /* Variables to bind during function invocation. */
  char **documentation;	/* The first set of comments that follow the defun. */
  void *profile_info;
} UserFunction;
#endif

/* The list of all created packages. */
Package **AllPackages = (Package **)NULL;
int AP_index = 0;
int AP_slots = 0;

/* Default number of slots for a symbol table.  Some callers
   may want to change this if they create a large number of
   symbol tables designed to hold only a few entries.

   Our standard value here is 107, which is good when there are a lot
   of relatively small tables.  Other reasonable primes are:
   29, 227, 307, 577, 797, 991, 1013, 3347, and 4999. */
int symbol_small_prime = 107;

/* The package which is "current".  That is, this is the package to use
   when no package name is supplied.  You can change this with
   symbol_set_default_package (). */
Package *CurrentPackage = (Package *)NULL;

/* An array of recently current packages.  The packages get on this list
   with symbol_push_package (new_package), and are removed with
   symbol_pop_package ().  Deleting a package from the global list side
   effects this PDL -- possibly destroying the usefulness of the carat
   referencing semantics. */
Package **PackagePDL = (Package **)NULL;
static int package_pdl_index = 0;
static int package_pdl_size = 0;

/* #define LowerCaseTranslationTable */

#if !defined (LowerCaseTranslationTable)
/* A translation table for ASCII characters.  This one has only uppercase
   characters in it. */
static char u_stb[128] =
{
  0x00, /* nul */  0x01, /* soh */  0x02, /* stx */  0x03, /* etx */
  0x04, /* eot */  0x05, /* enq */  0x06, /* ack */  0x07, /* bel */
  0x08, /* bs */   0x09, /* ht */   0x0a, /* nl */   0x0b, /* vt */
  0x0c, /* np */   0x0d, /* cr */   0x0e, /* so */   0x0f, /* si */
  0x10, /* dle */  0x11, /* dc1 */  0x12, /* dc2 */  0x13, /* dc3 */
  0x14, /* dc4 */  0x15, /* nak */  0x16, /* syn */  0x17, /* etb */
  0x18, /* can */  0x19, /* em */   0x1a, /* sub */  0x1b, /* esc */
  0x1c, /* fs */   0x1d, /* gs */   0x1e, /* rs */   0x1f, /* us */
  0x20, /* sp */   0x21, /* ! */    0x22, /* " */    0x23, /* # */
  0x24, /* $ */    0x25, /* % */    0x26, /* & */    0x27, /* ' */
  0x28, /* ( */    0x29, /* ) */    0x2a, /* * */    0x2b, /* + */
  0x2c, /* , */    0x2d, /* - */    0x2e, /* . */    0x2f, /* / */
  0x30, /* 0 */    0x31, /* 1 */    0x32, /* 2 */    0x33, /* 3 */
  0x34, /* 4 */    0x35, /* 5 */    0x36, /* 6 */    0x37, /* 7 */
  0x38, /* 8 */    0x39, /* 9 */    0x3a, /* : */    0x3b, /* ; */
  0x3c, /* < */    0x3d, /* = */    0x3e, /* > */    0x3f, /* ? */
  0x40, /* @ */    0x41, /* A */    0x42, /* B */    0x43, /* C */
  0x44, /* D */    0x45, /* E */    0x46, /* F */    0x47, /* G */
  0x48, /* H */    0x49, /* I */    0x4a, /* J */    0x4b, /* K */
  0x4c, /* L */    0x4d, /* M */    0x4e, /* N */    0x4f, /* O */
  0x50, /* P */    0x51, /* Q */    0x52, /* R */    0x53, /* S */
  0x54, /* T */    0x55, /* U */    0x56, /* V */    0x57, /* W */
  0x58, /* X */    0x59, /* Y */    0x5a, /* Z */    0x5b, /* [ */
  0x5c, /* \ */    0x5d, /* ] */    0x5e, /* ^ */    0x5f, /* _ */
  0x60, /* ` */    0x41, /* A */    0x42, /* B */    0x43, /* C */
  0x44, /* D */    0x45, /* E */    0x46, /* F */    0x47, /* G */
  0x48, /* H */    0x49, /* I */    0x4a, /* J */    0x4b, /* K */
  0x4c, /* L */    0x4d, /* M */    0x4e, /* N */    0x4f, /* O */
  0x50, /* P */    0x51, /* Q */    0x52, /* R */    0x53, /* S */
  0x54, /* T */    0x55, /* U */    0x56, /* V */    0x57, /* W */
  0x58, /* X */    0x59, /* Y */    0x5a, /* Z */    0x7b, /* { */
  0x7c, /* | */    0x7d, /* } */    0x7e, /* ~ */    0x7f /*del*/
};

/* Symbol character translation table. */
static char *stb = u_stb;

#else /* defined (LowerCaseTranslationTable) */
/* A translation table for ASCII characters.  This one has only lowercase
   characters in it. */
static char l_stb[128] =
{
  0x00, /* nul */  0x01, /* soh */  0x02, /* stx */  0x03, /* etx */
  0x04, /* eot */  0x05, /* enq */  0x06, /* ack */  0x07, /* bel */
  0x08, /* bs */   0x09, /* ht */   0x0a, /* nl */   0x0b, /* vt */
  0x0c, /* np */   0x0d, /* cr */   0x0e, /* so */   0x0f, /* si */
  0x10, /* dle */  0x11, /* dc1 */  0x12, /* dc2 */  0x13, /* dc3 */
  0x14, /* dc4 */  0x15, /* nak */  0x16, /* syn */  0x17, /* etb */
  0x18, /* can */  0x19, /* em */   0x1a, /* sub */  0x1b, /* esc */
  0x1c, /* fs */   0x1d, /* gs */   0x1e, /* rs */   0x1f, /* us */
  0x20, /* sp */   0x21, /* ! */    0x22, /* " */    0x23, /* # */
  0x24, /* $ */    0x25, /* % */    0x26, /* & */    0x27, /* ' */
  0x28, /* ( */    0x29, /* ) */    0x2a, /* * */    0x2b, /* + */
  0x2c, /* , */    0x2d, /* - */    0x2e, /* . */    0x2f, /* / */
  0x30, /* 0 */    0x31, /* 1 */    0x32, /* 2 */    0x33, /* 3 */
  0x34, /* 4 */    0x35, /* 5 */    0x36, /* 6 */    0x37, /* 7 */
  0x38, /* 8 */    0x39, /* 9 */    0x3a, /* : */    0x3b, /* ; */
  0x3c, /* < */    0x3d, /* = */    0x3e, /* > */    0x3f, /* ? */
  0x40, /* @ */    0x61, /* a */    0x62, /* b */    0x63, /* c */
  0x64, /* d */    0x65, /* e */    0x66, /* f */    0x67, /* g */
  0x68, /* h */    0x69, /* i */    0x6a, /* j */    0x6b, /* k */
  0x6c, /* l */    0x6d, /* m */    0x6e, /* n */    0x6f, /* o */
  0x70, /* p */    0x71, /* q */    0x72, /* r */    0x73, /* s */
  0x74, /* t */    0x75, /* u */    0x76, /* v */    0x77, /* w */
  0x78, /* x */    0x79, /* y */    0x7a, /* z */    0x5b, /* [ */
  0x5c, /* \ */    0x5d, /* ] */    0x5e, /* ^ */    0x5f, /* _ */
  0x60, /* ` */    0x61, /* a */    0x62, /* b */    0x63, /* c */
  0x64, /* d */    0x65, /* e */    0x66, /* f */    0x67, /* g */
  0x68, /* h */    0x69, /* i */    0x6a, /* j */    0x6b, /* k */
  0x6c, /* l */    0x6d, /* m */    0x6e, /* n */    0x6f, /* o */
  0x70, /* p */    0x71, /* q */    0x72, /* r */    0x73, /* s */
  0x74, /* t */    0x75, /* u */    0x76, /* v */    0x77, /* w */
  0x78, /* x */    0x79, /* y */    0x7a, /* z */    0x7b, /* { */
  0x7c, /* | */    0x7d, /* } */    0x7e, /* ~ */    0x7f /*  del*/
};

/* Symbol character translation table. */
static char *stb = l_stb;
#endif /* LowerCaseTranslationTable */


#define symchar(character) stb[(character & 0x7f)]

#if 0

/* Non-zero means translation tables are initialized. */
static int stb_initialized = 0;

static void
initialize_symbol_translations (void)
{
  if (!stb_initialized)
    {
      register int i;

      for (i = 0; i < 128; i++)
	u_stb[i] = l_stb[i] = (char)i;

      for (i = 'A'; i <= 'Z'; i++)
	l_stb[i] = i + 32;

      for (i = 'a'; i <= 'z'; i++)
	u_stb[i] = i - 32;

      stb_initialized = 1;
    }
}
#endif

#if !defined (symbol_xmalloc)
#  define symbol_xmalloc xmalloc
#endif

#if !defined (symbol_xrealloc)
#  define symbol_xrealloc xrealloc
#endif

#if !defined (symbol_strdup)
#  define symbol_strdup strdup
#endif

/* Create a symbol table with ROWS slots for symbol lists. */
static SymbolTable *
symbol_create_table (int rows)
{
  register int i;
  SymbolTable *newtab = (SymbolTable *)symbol_xmalloc (sizeof (SymbolTable));

  if (rows == 0)
    rows = symbol_small_prime;

  newtab->symbol_list = (SymbolList **)
    symbol_xmalloc (rows * sizeof (SymbolList *));
  newtab->rows = rows;
  newtab->entries = 0;

  for (i = 0; i < rows; i++)
    newtab->symbol_list[i] = (SymbolList *)NULL;

  return (newtab);
}

static int
symbol_match_name (Symbol *symbol, char *name)
{
  register int i;

  for (i = 0; (symchar (name[i]) == symbol->name[i]) && name[i] != '\0'; i++);

  return ((name[i] + symbol->name[i]) == 0);
}

/* Return the index of the row in TABLE which will contain the symbol
   named NAME. */
static int
symbol_hash (SymbolTable *table, char *name)
{
  register unsigned int i, j;

  for (i = 0, j = 0; name[i] != '\0'; i++)
    j = (j << 2) + symchar (name[i]);

  return ((j & ~(0xffffffff << 31)) % table->rows);
}

/* Set the default package.  This is the package to use when
   the name of the package is "". */
void
symbol_set_default_package (Package *package)
{
  if (package)
    CurrentPackage = package;
}

/* Locate and return the existing package NAME. */
Package *
symbol_lookup_package (char *name)
{
  register int i = 0;
  Package *package = (Package *)NULL;

  if (name != (char *)NULL)
    {
      if (name[0] == '\0')
	{
	  package = CurrentPackage;
	}
      else if (AllPackages != (Package **)NULL)
	{
	  int name_len = strlen (name);

	  /* Special case magic package names.  If the name consists of
	     all carats, then it is relative to the list of most recently
	     accessed packages.  The number of carats signifies the offset
	     from the current package, not in order of recently created
	     packages, but in the order that the packages were most recently
	     current. */
	  for (i = 0; i < name_len; i++)
	    if (name[i] != '^')
	      break;

	  /* If the package name was not a magic one, just look it up. */
	  if (i != name_len)
	    {
	      for (i = 0; i < AP_index; i++)
		if ((AllPackages[i]->name_len == name_len) &&
		    (symbol_match_name ((Symbol *)AllPackages[i], name)))
		  {
		    package = AllPackages[i];
		    break;
		  }
	    }
	  else
	    {
	      /* Find the Nth most recently current package. */
	      i = package_pdl_index - i;
	      if (i > -1)
		package = PackagePDL[i];
	    }
	}
    }
  return (package);
}

/* Push CurrentPackage onto the Package PDL, and make NEW_PACKAGE 
   the current package. */
void
symbol_push_package (Package *new_package)
{
  if (package_pdl_index + 2 >= package_pdl_index)
    PackagePDL = (Package **)symbol_xrealloc
      (PackagePDL, package_pdl_size += (10 * sizeof (Package *)));

  PackagePDL[package_pdl_index++] = CurrentPackage;
  PackagePDL[package_pdl_index] = (Package *)NULL;
  symbol_set_default_package (new_package);
}

/* Make the top of the Package PDL be the current package, and shrink
   the PDL. */
void
symbol_pop_package (void)
{
  if (package_pdl_index)
    {
      Package *package = PackagePDL[--package_pdl_index];

      PackagePDL[package_pdl_index] = (Package *)NULL;
      symbol_set_default_package (package);
    }
}

void
package_pdl_remove (Package *package)
{
  register int i;

  if (package_pdl_index)
    {
      for (i = package_pdl_index - 1; i > -1; i--)
	if (package == PackagePDL[i])
	  {
	    /* Move subsequent packages back. */
	    while ((PackagePDL[i] = PackagePDL[i + 1]) != (Package *)NULL)
	      i++;

	    package_pdl_index--;

	    break;
	  }
    }
}

/* Create a package named NAME suitable for storing symbols in.
   If the package already exists, return that package.
   A non-zero value for SMALL_PRIME creates the table with that many
   hash buckets, instead of using the default value.
   You can create an anonymous package by passing NULL as the name. */
Package *
symbol_get_package_hash (char *name, int small_prime)
{
  register int i = 0;
  Package *package = (Package *)NULL;

  if (name != (char *)NULL)
    package = symbol_lookup_package (name);

  if (package == (Package *)NULL)
    {
      package = (Package *)symbol_xmalloc (sizeof (Package));
      package->name = name ? symbol_strdup (name) : name;
      package->name_len = name ? strlen (name) : 0;
      package->table = symbol_create_table (small_prime);

      if ((AP_index + 2) > AP_slots)
	AllPackages = (Package **) symbol_xrealloc
	  (AllPackages, (AP_slots += 10) * sizeof (Package *));

      AllPackages[AP_index++] = package;
      AllPackages[AP_index] = (Package *)NULL;

      /* Fix up the name. */
      for (i = 0; i < package->name_len; i++)
	package->name[i] = symchar (package->name[i]);
    }

  return (package);
}

/* Create a package named NAME suitable for storing symbols in.
   If the package already exists, return that package.
   You can create an anonymous package by passing NULL as the name. */
Package *
symbol_get_package (char *name)
{
  return (symbol_get_package_hash (name, 0));
}

/* Destroy the package PACKAGE, freeing all of the space that it
   was using.  Delete the package from the list of AllPackages. */
void
symbol_destroy_package (Package *package)
{
  register int i;

  if (package != (Package *)NULL)
    {
      if (package == CurrentPackage)
	CurrentPackage = (Package *)NULL;

      if (AllPackages != (Package **)NULL)
	{
	  for (i = AP_index; i > -1; i--)
	    if (package == AllPackages[i])
	      {
		/* Move subsequent packages back. */
		while ((AllPackages[i] = AllPackages[i + 1])
		       != (Package *)NULL)
		  i++;

		AP_index--;

		break;
	      }
	}

      /* Remove this package from the PackagePDL. */
      package_pdl_remove (package);

      /* Now, simply free the package contents. */
      if (package->name) free (package->name);
      for (i = 0; i < package->table->rows; i++)
	{
	  if (package->table->symbol_list[i] != (SymbolList *)NULL)
	    {
	      SymbolList *list = package->table->symbol_list[i];

	      while (list)
		{
		  Symbol *sym = list->symbol;
		  SymbolList *thissym = list;
		  list = list->next;

		  symbol_free (sym);
		  free (thissym);
		}

	    }
	}

      free (package->table->symbol_list);
      free (package->table);
      free (package);
    }
}

/* Return the package of IDENTIFIER, a full identifier for a symbol.
   Identifiers have two portions: the package part and the symbol name
   part.  The two parts are separated by a pair of colons, as in
   PACKAGE::SYMBOL. */
Package *
symbol_package (char *identifier)
{
  Package *package = CurrentPackage;
  char *colon = strstr (identifier, "::");

  if (colon != (char *)NULL)
    {
      register int i;
      char *packname = (char *)symbol_xmalloc (1 + (colon - identifier));

      for (i = 0; i != (colon - identifier); i++)
	packname[i] = symchar (identifier[i]);

      packname[i] = '\0';
      package = symbol_get_package (packname);
      free (packname);
    }

  return (package);
}

Symbol *
symbol_lookup_in_package (Package *package, char *name)
{
  SymbolTable *table = (SymbolTable *)NULL;
  Symbol *result = (Symbol *)NULL;

  if (package)
    table = package->table;

  if (table)
    {
      int row = symbol_hash (table, name);
      SymbolList *list = table->symbol_list[row];
      int name_len = strlen (name);

      while (list)
	{
	  if ((name_len == list->symbol->name_len) &&
	      (symbol_match_name (list->symbol, name) == 1))
	    {
	      result = list->symbol;
	      break;
	    }
	  else
	    list = list->next;
	}
    }

  if ((symbol_retrieve_hook != (SYMBOL_FUNCTION *)NULL) &&
      (result != (Symbol *)NULL))
    (*symbol_retrieve_hook) (result);

  return (result);
}

/* Remove from PACKAGE the symbol specified by NAME. */
Symbol *
symbol_remove_in_package (Package *package, char *name)
{
  SymbolTable *table = (SymbolTable *)NULL;
  Symbol *result = (Symbol *)NULL;

  if (package)
    table = package->table;

  if (table)
    {
      int row = symbol_hash (table, name);
      SymbolList *list = table->symbol_list[row];
      int name_len = strlen (name);
      SymbolList *prev = (SymbolList *)NULL;

      while (list)
	{
	  if ((name_len == list->symbol->name_len) &&
	      (symbol_match_name (list->symbol, name) == 1))
	    {
	      result = list->symbol;

	      if (prev)
		prev->next = list->next;
	      else
		table->symbol_list[row] = list->next;

	      free (list);
	      table->entries--;
	      break;
	    }
	  else
	    {
	      prev = list;
	      list = list->next;
	    }
	}
    }
  return (result);
}

/* Intern in PACKAGE, the symbol named NAME, and return the interned symbol.
   If the symbol is already present, then return that. */
Symbol *
symbol_intern_in_package (Package *package, char *name)
{
  Symbol *symbol = symbol_lookup_in_package (package, name);

  if (symbol == (Symbol *)NULL)
    {
      register int i;
      SymbolTable *table = package->table;
      int row = symbol_hash (table, name);
      SymbolList *list = table->symbol_list[row];
      SymbolList *newlist = (SymbolList *)symbol_xmalloc (sizeof (SymbolList));

      symbol = (Symbol *)symbol_xmalloc (sizeof (Symbol));
      memset (symbol, 0, sizeof (Symbol));
      symbol->flags = 0;
      if (mhtml_flag_newly_interned_symbols != 0) symbol->flags |= sym_FLAGGED;
      symbol->name_len = strlen (name);
      symbol->name = (char *)symbol_xmalloc (1 + symbol->name_len);

      for (i = 0; i < symbol->name_len; i++)
	symbol->name[i] = symchar (name[i]);

      symbol->name[i] = '\0';

      symbol->values = (char **)NULL;
      symbol->values_index = 0;
      symbol->values_slots = 0;
      symbol->type = symtype_STRING;
      symbol_set_modified (symbol);
      symbol->package = (void *)package;
      symbol->notifier = (int *)NULL;

#if defined (METAHTML_COMPILER)
      symbol->machine = (void *)NULL;
#endif

      newlist->symbol = symbol;
      newlist->next = list;
      table->symbol_list[row] = newlist;
      table->entries++;

      if (symbol_intern_hook != (SYMBOL_FUNCTION *)NULL)
	(*symbol_intern_hook) (symbol);

    }
  else if (symbol_retrieve_hook != (SYMBOL_FUNCTION *)NULL)
    (*symbol_retrieve_hook) (symbol);

  return (symbol);
}

/* Associate SYMBOL with NOTIFIER, the address of an integer. */
void
symbol_notify_value (Symbol *symbol, int *address)
{
  if (symbol != (Symbol *)NULL)
    {
      symbol->notifier = address;
      *address = (symbol->values_index != 0);
    }
}

/* Return the symbol name portion of the full IDENTIFIER. */
static char *
symbol_part (char *identifier)
{
  char *symbol_name = strstr (identifier, "::");

  if (symbol_name)
    symbol_name += 2;
  else
    symbol_name = identifier;

  return (symbol_name);
}

/* Intern the symbol named NAME, and return that symbol.
   This interns in the package specified in the symbol name given,
   so that `foo::bar' interns BAR in the FOO package, and `bar'
   interns BAR in the current package. */
Symbol *
symbol_intern (char *name)
{
  Package *package = symbol_package (name);
  char *symbol_name = symbol_part (name);
  Symbol *symbol;

  /* The PACKAGE will be NULL if this is the first call ever to
     a symbol interning or package function, and the name contained
     no package part.  In that case, create the default package. */
  if (package == (Package *)NULL)
    {
      if (!CurrentPackage)
	CurrentPackage = symbol_get_package_hash (DEFAULT_PACKAGE_NAME, 577);

      package = CurrentPackage;
    }

  symbol = symbol_intern_in_package (package, symbol_name);

  return (symbol);
}

/* Find in PACKAGE for NAME, and return the associated symbol. */
Symbol *
symbol_lookup (char *name)
{
  Package *package = symbol_package (name);
  char *symbol_name = symbol_part (name);
  Symbol *symbol;

  /* The PACKAGE will be NULL if this is the first call ever to
     a symbol interning or package function, and the name contained
     no package part.  In that case, create the default package. */
  if (package == (Package *)NULL)
    {
      package = symbol_get_package_hash (DEFAULT_PACKAGE_NAME, 577);
      CurrentPackage = package;
    }

  symbol = symbol_lookup_in_package (package, symbol_name);

  return (symbol);
}

/* Get the values of NAME. */
char **
symbol_get_values (char *name)
{
  Package *package = symbol_package (name);
  char *symbol_name = symbol_part (name);
  Symbol *symbol;
  char **values = (char **)NULL;

  /* The PACKAGE will be NULL if this is the first call ever to
     a symbol interning or package function, and the name contained
     no package part.  In that case, create the default package. */
  if (package == (Package *)NULL)
    {
      package = symbol_get_package_hash (DEFAULT_PACKAGE_NAME, 577);
      CurrentPackage = package;
    }

  symbol = symbol_lookup_in_package (package, symbol_name);

  if ((symbol != (Symbol *)NULL))
    values = symbol->values;

  return (values);
}

/* Remove the symbol specified by NAME.  The removed symbol is returned. */
Symbol *
symbol_remove (char *name)
{
  Package *package = symbol_package (name);
  Symbol *symbol = (Symbol *)NULL;

  if (package != (Package *)NULL)
    symbol = symbol_remove_in_package (package, symbol_part (name));

  return (symbol);
}

/* Return an array of pointers to every symbol in the package named by NAME.
   The actual symbols are returned, so modifications can be made to them
   directly.  Beware!  Changing the name of a symbol in the returned array
   will probably make the symbol impossible to find by normal lookup
   methods, so don't do it. */
Symbol **
symbol_package_symbols (char *name)
{
  Package *package = CurrentPackage;

  if (name)
    {
      if (name[0] == '\0')
	package = symbol_get_package_hash (DEFAULT_PACKAGE_NAME, 577);
      else
	package = symbol_get_package (name);
    }
  
  return (symbols_of_package (package));
}

/* Return an array of pointers to every symbol in the package PACKAGE.
   The actual symbols are returned, so modifications can be made to them
   directly.  Beware!  Changing the name of a symbol in the returned array
   will probably make the symbol impossible to find by normal lookup
   methods, so don't do it. */
Symbol **
symbols_of_package (Package *package)
{   
  Symbol **symbols = (Symbol **)NULL;

  if (package != (Package *)NULL)
    {
      SymbolTable *table = package->table;

      if (table->entries)
	{
	  register int i, j;

	  symbols = (Symbol **)symbol_xmalloc
	    ((1 + table->entries) * sizeof (Symbol *));

	  for (i = 0, j = 0; i < table->rows; i++)
	    {
	      SymbolList *list = table->symbol_list[i];

	      while (list)
		{
		  symbols[j++] = list->symbol;
		  list = list->next;
		}
	    }

	  symbols[j] = (Symbol *)NULL;
	}
    }

  return (symbols);
}

/* Free the individual pointers in ARRAY, and the ARRAY itself. */
void
symbol_free_array (char **array)
{
  if (array != (char **)NULL)
    {
      register int i;

      for (i = 0; array[i] != (char *)NULL; i++)
	free (array[i]);

      free (array);
    }
}

/* Return the number of entries in ARRAY. */
int
symbol_array_length (char **array)
{
  register int i = 0;

  if (array != (char **)NULL)
    for (i = 0; array[i] != (char *)NULL; i++);

  return (i);
}

/* Create and return a copy of ARRAY. */
char **
symbol_copy_array (char **array)
{
  int len = symbol_array_length (array);
  char **copy = (char **)NULL;

  if (len)
    {
      register int i;

      copy = (char **)symbol_xmalloc ((1 + len) * sizeof (char *));
      for (i = 0; i < len; i++)
	copy[i] = symbol_strdup (array[i]);
      copy[i] = (char *)NULL;
    }

  return (copy);
}

/* Create a Datablock object from DATA and LENGTH. */
Datablock *
datablock_create (char *data, int length)
{
  Datablock *block = (Datablock *)symbol_xmalloc (sizeof (Datablock));

  memset ((char *)block, 0, sizeof (Datablock));
  block->length = length;

  if (length != 0)
    {
      block->data = (char *)symbol_xmalloc (length);
      memcpy (block->data, data, length);
    }

  return (block);
}

/* Return a duplicate of BLOCK. */
Datablock *
datablock_copy (Datablock *block)
{
  Datablock *newblock = datablock_create (block->data, block->length);
  return (newblock);
}

/* Free BLOCK and contents. */
void
datablock_free (Datablock *block)
{
  if (block != (Datablock *)NULL)
    {
      if (block->length != 0)
	free (block->data);

      free (block);
    }
}

/* Free all of the data associated with SYMBOL. */
void
symbol_free (Symbol *symbol)
{
  if (symbol != (Symbol *)NULL)
    {
      if (symbol->notifier)
	*(symbol->notifier) = 0;


      switch (symbol->type)
	{
	case symtype_STRING:
	  if (symbol_free_hook != (SYMBOL_FUNCTION *)NULL)
	    (*symbol_free_hook) (symbol);
	  free (symbol->name);
	  if (symbol->preserved_name) free (symbol->preserved_name);
	  symbol_free_array (symbol->values);
	  free (symbol);
	  break;

	case symtype_BINARY:
	  if (symbol_free_hook != (SYMBOL_FUNCTION *)NULL)
	    (*symbol_free_hook) (symbol);
	  free (symbol->name);
	  if (symbol->preserved_name) free (symbol->preserved_name);
	  datablock_free ((Datablock *)symbol->values);
	  free (symbol);
	  break;

	case symtype_FUNCTION:
	case symtype_USERFUN:
	  break;
	}
    }
}

void
symbol_reset (Symbol *symbol)
{
  switch (symbol->type)
    {
    case symtype_STRING:
      symbol_free_array (symbol->values);
      break;

    case symtype_BINARY:
      datablock_free ((Datablock *)symbol->values);
      break;

    case symtype_FUNCTION:
    case symtype_USERFUN:
      return;
    }

  symbol->flags = 0;
  symbol->type = symtype_STRING;
  symbol_set_modified (symbol);
  symbol->values_index = 0;
  symbol->values_slots = 0;
  symbol->values = (char **)NULL;
}

int
symbol_get_flags (Symbol *symbol)
{
  int result = 0;

  if (symbol != (Symbol *)NULL)
    result = symbol->flags;

  return (result);
}

int
symbol_get_flag (Symbol *symbol, int flag)
{
  int result = 0;

  if (symbol != (Symbol *)NULL)
    result = (symbol->flags & flag);

  return (result);
}

void
symbol_set_flag (Symbol *symbol, int flag)
{
  if (symbol != (Symbol *)NULL) symbol->flags |= flag;
}

void
symbol_clear_flag (Symbol *symbol, int flag)
{
  if (symbol != (Symbol *)NULL) symbol->flags &= ~flag;
}

/* Copy SYMBOL into PACKAGE.  Both SYMBOL and PACKAGE are real data items,
   not names of those items.  Returns the copied symbol. */
Symbol *
symbol_copy (Symbol *symbol, Package *package)
{
  Symbol *copy;

  copy = symbol_remove_in_package (package, symbol->name);

  if (copy != (Symbol *)NULL)
    symbol_free (copy);

  copy = symbol_intern_in_package (package, symbol->name);
  copy->type = symbol->type;
  copy->flags = symbol->flags;
  symbol_set_modified (copy);
  copy->values_index = symbol->values_index;
  copy->values_slots = symbol->values_slots;

  switch (symbol->type)
    {
    case symtype_STRING:
      copy->values = symbol_copy_array (symbol->values);
      copy->values_slots = copy->values_index;
      break;

    case symtype_FUNCTION:
      copy->values = symbol->values;
      break;

#if defined (METAHTML_PROFILER)
    case symtype_USERFUN:
      {
	UserFunction *newfun = (UserFunction *)
	  symbol_xmalloc (sizeof (UserFunction));
	UserFunction *old = (UserFunction *)symbol->values;
	newfun->type = old->type;
	newfun->flags= old->flags;
	newfun->debug_level = 0;
	newfun->name = old->name ? symbol_strdup (old->name) : (char *)NULL;
	newfun->body = old->body ? symbol_strdup (old->body) : (char *)NULL;
	newfun->packname =
	  old->packname ? symbol_strdup (old->packname) : (char *)NULL;
	if (old->named_parameters)
	  newfun->named_parameters = symbol_copy_array (old->named_parameters);
	else
	  newfun->named_parameters = (char **)NULL;

	if (old->documentation)
	  newfun->documentation = symbol_copy_array (old->documentation);
	else
	  newfun->documentation = (char **)NULL;

	newfun->profile_info = (void *)NULL;

	copy->values = (char **)newfun;
      }
      break;
#else
    case symtype_USERFUN:
      copy->values = symbol->values;
      break;
#endif /* !METAHTML */

    case symtype_BINARY:
      copy->values = (char **)datablock_copy ((Datablock *)symbol->values);
      break;
    }

#if defined (METAHTML_COMPILER)
  copy->machine = (void *)NULL;
#endif

  return (copy);
}

/* Change the name of SYMBOL to NEWNAME. */
Symbol *
symbol_rename (Symbol *symbol, char *newname)
{
  Symbol *sym, *newsym;
  Package *pack = (Package *)symbol->package;

  sym = symbol_remove_in_package (pack, symbol->name);
  newsym = symbol_remove_in_package (pack, newname);
  symbol_free (newsym);
  newsym = symbol_intern_in_package (pack, newname);
  newsym->values = sym->values;
  newsym->values_index = sym->values_index;
  newsym->values_slots = sym->values_slots;
  newsym->type = sym->type;
  newsym->flags = sym->flags;
  symbol_set_modified (newsym);
  free (sym->name);
  if (sym->preserved_name) free (sym->preserved_name);
  free (sym);

  return (newsym);
}

/* Move SYMBOL from its current package into PACKAGE.
   Both SYMBOL and PACKAGE are real data items, not names of those items.
   Returns the moved symbol. */
Symbol *
symbol_move (Symbol *symbol, Package *package)
{
  int row;
  SymbolList *list = (SymbolList *)symbol_xmalloc (sizeof (SymbolList));

  row = symbol_hash (package->table, symbol->name);
  symbol_remove_in_package ((Package *)symbol->package, symbol->name);
  symbol_free (symbol_remove_in_package (package, symbol->name));
  list->next = package->table->symbol_list[row];
  package->table->symbol_list[row] = list;
  package->table->entries++;
  list->symbol = symbol;
  symbol->package = (void *)package;

  return (symbol);
}

/* Add to SYMBOL, the value VALUE.  The modified symbol is returned. */
Symbol *
symbol_add_value (Symbol *symbol, char *value)
{
  if (value != (char *)NULL)
    {
      switch (symbol->type)
	{
	case symtype_STRING:
	  if (symbol->values_index + 2 > symbol->values_slots)
	    symbol->values = (char **)symbol_xrealloc
	    (symbol->values, (symbol->values_slots += 10) * sizeof (char *));

	  symbol->values[symbol->values_index++] = symbol_strdup (value);
	  symbol->values[symbol->values_index] = (char *)NULL;
	  break;

	case symtype_FUNCTION:
	case symtype_USERFUN:
	  break;

	case symtype_BINARY:
	  symbol->values = (char **)datablock_copy ((Datablock *)value);
	  break;
	}
    }

  if (symbol->notifier)
    *(symbol->notifier) = (symbol->values_index != 0);

  symbol_set_modified (symbol);

  return (symbol);
}

/* Return the full print name of SYMBOL. */
char *
symbol_full_name (Symbol *symbol)
{
  char *sname = symbol->preserved_name ? symbol->preserved_name : symbol->name;
  Package *pack = (Package *)symbol->package;
  char *name;

  if (pack->name != (char *)NULL)
    {
      name = (char *)symbol_xmalloc (3 + pack->name_len + symbol->name_len);
      strcpy (name, pack->name);
      name[pack->name_len + 0] = ':';
      name[pack->name_len + 1] = ':';
      strcpy (name + pack->name_len + 2, sname);
    }
  else
    name = symbol_strdup (sname);

  return (name);
}

/* Copy all of the symbols from FROM into TO. */
void
symbol_copy_package (Package *from, Package *to)
{
  Symbol **symbols = symbols_of_package (from);

  if (symbols)
    {
      register int i;
      Symbol *sym;

      for (i = 0; (sym = symbols[i]) != (Symbol *)NULL; i++)
	symbol_copy (sym, to);
    }
}

/* Store into the symbol referenced by NAME, the array of character strings
   pointed to by ARRAY. */
void
symbol_store_array (char *name, char **array)
{
  Symbol *sym;

  if ((sym = symbol_remove (name)) != (Symbol *)NULL)
    symbol_free (sym);

  /* Only assign when there really is anything to assign. */
  if (array != (char **)NULL)
    {
      register int i;

      for (i = 0; array[i] != (char *)NULL; i++);

      sym = symbol_intern (name);
      sym->values = array;
      sym->values_index = i;
      sym->values_slots = i;
      symbol_set_modified (sym);
    }
}

/* Return the canonicalized version of NAME.  This just strips out
   array brackets.  Always returns a new string, unless the input
   string is NULL. */
char *
symbol_canonical_name (char *name)
{
  char *result = (char *)NULL;

  if (name != (char *)NULL)
    {
      register int i;

      result = symbol_strdup (name);

      for (i = 0; ((name[i] != '\0') && (name[i] != '[')); i++);

      /* If there is an open-bracket here, see if there is a close. */
      if (name[i] == '[')
	{
	  int zero = i;

	  for (; ((name[i] != '\0') && (name[i] != ']')); i++);
	  if (name[i] == ']')
	    result[zero] = '\0';
	}
    }

  return (result);
}

#if defined (TEST)

int
main (int argc, char *argv[])
{
  register int i, j;

  for (i = 1; i < argc; i++)
    symbol_intern (argv[i]);

  for (j = 0; j < AP_index; j++)
    {
      Package *pack = AllPackages[j];
      char *packname = pack->name;
      Symbol **symbols = symbol_package_symbols (packname);
      int max_row = 0;
      int min_row = 10000;
      
      fprintf (stdout, "Distribution of package `%s': \n   ", packname);
      for (i = 0; i < pack->table->rows; i++)
	{
	  int count = 0;
	  SymbolList *list = pack->table->symbol_list[i];

	  if (!(i % 8)) fprintf (stdout, "\n   ");
	  fprintf (stdout, "%03d: ", i);

	  while (list)
	    {
	      count++;
	      list = list->next;
	    }

	  if (count > max_row) max_row = count;
	  if (count < min_row) min_row = count;

	  fprintf (stdout, "%02d%s", count,
		   (i + 1) == pack->table->rows ? ".\n" : ", ");
	}
      fprintf (stdout, "Most in any single row: %d, Least: %d\n\n",
	       max_row, min_row);

      fprintf (stdout, "Symbols in package `%s' (%d):\n",
	       packname, pack->table->entries);

      for (i = 0; symbols && symbols[i]; i++)
	fprintf (stdout, "   `%s'\n", symbol_full_name (symbols[i]));

      fprintf (stdout, " (Total of %d symbols)\n\n",
	       pack->table->entries);

      free (symbols);
    }

  /* Test the remove algorithm. */


  return (0);
}
#endif /* TEST */

#if defined (__cplusplus)
}
#endif
