/* prototypes.h: -*- C -*-  List of prototypes for Solaris. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Sat Aug 31 21:09:02 1996. */

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

extern void free (void *);
extern void srandom (unsigned int);
extern long int random (void);
extern char *crypt (const char *key, const char *salt);
extern int gethostname (char *name, int namelen);
#if defined (SOLARIS_VERSION_4)
extern char *initstate (unsigned int, char *, int);
extern char *setstate (char *);
#endif
