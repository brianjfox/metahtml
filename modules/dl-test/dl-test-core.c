/* dl-test-core.c: -*- C -*-  Test dynamic loading on this system. */

/*  Copyright (c) 1998 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jul  8 12:05:37 1998.  */

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
#include <dlfcn.h>

typedef void Function (char *);

int necessary_symbol = 4;

int
main (int argc, char *argv[])
{
  void *handle = (void *)dlopen ("./dl-test-lib.so", RTLD_LAZY);

  if (handle == (void *)NULL)
    fprintf (stderr, "Error after dlopen(): %s\n", dlerror ());
  else
    {
      Function *func = (Function *)NULL;
      func = (Function *)dlsym (handle, "shared_lib_function");

      if (func == (Function *)NULL)
	{
	  fprintf (stderr, "Error after dlsym(): %s\n", dlerror ());
	  dlclose (handle);
	}
      else
	(*func) ("From within dl-test-core.c");
    }

  return (0);
}
