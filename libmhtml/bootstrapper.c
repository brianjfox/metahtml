/* bootstrapper.c: -*- C -*-  Load Meta-HTML defuns from bootstrap_code. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Wed Jan 29 10:49:30 1997.  */

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

#include "language.h"
#include "symdump.h"

#if defined (__cplusplus)
extern "C"
{
#endif

extern unsigned char bootstrap_code[];
extern int bootstrap_code_len;

extern unsigned char system_preload_code[];
extern int system_preload_code_len;
extern void initialize_external_functions (Package *package);

extern char *mhtml_load_module (char *module_name, char *mode_arg,
				int no_init_p, char *initfunc_name,
				int allow_output_p);

/* Try to load the machine *before* loading anything else,
   especially the internals. */
static void
load_vm (void)
{
  char *loaded_p = mhtml_load_module ("modmachine", (char *)NULL, 0,
				      (char *)NULL, 0);
  xfree (loaded_p);
}

void
bootstrap_metahtml (int call_initializer_p)
{
  if (bootstrap_code_len)
    {
      int sd = symdump_open_string_data ();
      void *ignore = (void *)xmalloc (4 * bootstrap_code_len);
      free (ignore);

      if (sd != -1)
	{
	  Package *pack;
	  char *temp;

	  symdump_write_string_data
	    (sd, bootstrap_code_len, &bootstrap_code[0]);
	  symdump_seek_string_data (sd, 0);
	  symdump_set_string_data_buffer_size (sd, bootstrap_code_len);
	  while ((pack = symbol_load_package (sd)) != (Package *)NULL);
	  if (mhtml_user_keywords == (Package *)NULL)
	    mhtml_user_keywords =
	      symbol_get_package_hash ("*user-functions*", 577);
	  bprintf_free_buffer (symdump_close_string_data (sd));

	  /* Always call <bootstrapper::system-initialize>. */
	  temp = mhtml_evaluate_string ("<bootstrapper::system-initialize>");
	  xfree (temp);

	  /* Optionally call <bootstrapper::initialize> */
	  if (call_initializer_p)
	    {
	      temp = mhtml_evaluate_string ("<bootstrapper::initialize>");
	      xfree (temp);
	    }
	}
    }
}

void
mhtml_system_preload (int call_initializer_p)
{
  if (mhtml_function_package == (Package *)NULL)
    {
      mhtml_function_package = symbol_get_package_hash ("*meta-html*", 577);
      initialize_external_functions (mhtml_function_package);
    }

  load_vm ();

  if (system_preload_code_len)
    {
      int sd = symdump_open_string_data ();
      void *ignore = (void *)xmalloc (4 * system_preload_code_len);
      free (ignore);

      if (sd != -1)
	{
	  Package *pack;
	  char *temp;

	  symdump_write_string_data
	    (sd, system_preload_code_len, &system_preload_code[0]);
	  symdump_seek_string_data (sd, 0);
	  symdump_set_string_data_buffer_size (sd, system_preload_code_len);
	  while ((pack = symbol_load_package (sd)) != (Package *)NULL);
	  if (mhtml_user_keywords == (Package *)NULL)
	    mhtml_user_keywords =
	      symbol_get_package_hash ("*user-functions*", 577);
	  bprintf_free_buffer (symdump_close_string_data (sd));

	  /* Optionally call <system-preload::initialize> */
	  if (call_initializer_p)
	    {
	      temp = mhtml_evaluate_string ("<system-preload::initialize>");
	      xfree (temp);
	    }
	}
    }
}

#if defined (__cplusplus)
}
#endif
