/* mhtmlstd.h: -*- C -*-  DESCRIPTIVE TEXT. */

/*  Copyright (c) 1996 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Fri Jan  3 15:32:04 1997.  */

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

#if !defined (_MHTMLSTD_H_)
#define _MHTMLSTD_H_

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#if defined (__unix)
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if defined (__linux__)
#  define _NO_CTYPE 1
#endif
#include <ctype.h>

#include <sys/types.h>
#include <errno.h>
#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif

#if defined (HAVE_FCNTL_H)
#  include <fcntl.h>
#else
#  if defined (HAVE_SYS_FCNTL_H)
#    include <sys/fcntl.h>
#  endif
#endif
#if defined (HAVE_BSTRING_H)
#  include <bstring.h>
#endif
#include <sys/ioctl.h>
#if defined (Solaris) && defined (HAVE_SYS_TTOLD_H)
#  include <sys/ttold.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#if 0
#if defined (__WINNT__)		/* CYGWIN defines sig_t in netdb.h. */
#  if !defined (HAVE_TYPE_SIG_T)
#    define HAVE_TYPE_SIG_T 1
#  endif
#endif
#endif

#include <sys/wait.h>

#if defined (TIME_WITH_SYS_TIME)
#  include <sys/time.h>
#  include <time.h>
#else
#  if defined (HAVE_SYS_TIME_H)
#    include <sys/time.h>
#  else
#    if defined (HAVE_TIME_H)
#      include <time.h>
#    endif
#  endif
#endif

#include <sys/stat.h>

#if defined (HAVE_DIRENT_H)
#  include <dirent.h>
#  define D_NAMELEN(x) strlen((x)->d_name)
#else
#  define dirent direct
#  define D_NAMELEN(dirent) (dirent)->d_namlen
#  if defined (HAVE_SYS_NDIR_H)
#    include <sys/ndir.h>
#  endif
#  if defined (HAVE_SYS_DIR_H)
#    include <sys/dir.h>
#  endif
#  if defined (HAVE_NDIR_H)
#    include <ndir.h>
#  endif
#endif

#if defined (NOTDEF)
    #if defined (sgi) || defined (Solaris)
    #define d_namlen d_reclen
    #endif

    #if defined (__linux__) && !defined (d_namlen)
    #define d_namlen d_reclen
    #endif

    #if defined (__WINNT__)
    #  define D_NAMELEN(x) strlen ((x)->d_name)
    #else
    #  define D_NAMELEN(x) (x)->d_namlen
    #endif
#endif /* NOTDEF */

#include "lockname.h"
#if !defined (HAVE_SRANDOM)
#  include <math.h>
#  define srandom(seed) srand (seed)
#  define random() rand()
#endif
#if defined (HAVE_GETPWNAM)
#include <pwd.h>
#endif
#include <signal.h>

#include <setjmp.h>
#if !defined (NO_REGEX_H)
#  if !defined (HAVE_REGEX_H)
#    include <regex/regex.h>
#  else
#    include <regex.h>
#  endif
#endif /* NO_REGEX_H */
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include <tcp/tcp.h>
#include <wisper/wisp.h>

#if defined (HAVE_SYS_SELECT_H)
#  include <sys/select.h>
#endif

#if !defined (HAVE_TYPE_SIG_T)
#if defined (__cplusplus)
extern "C" typedef RETSIGTYPE (*sig_t) (int);
#else
typedef RETSIGTYPE (*sig_t) (int);
#endif /* CPLUSPLUS */
#endif /* HAVE_TYPE_SIG_T */

#if !defined (HAVE_TYPE_TIME_T)
typedef long time_t;
#endif

#if defined (macintosh)
extern char *strdup (const char *string);
#  define os_open(name, flags, mode) open (name, flags)
#endif

#if defined (__WINNT__)
#  if !defined (O_BINARY)
#    define O_BINARY 0
#  endif
#  define os_open(name, flags, mode) open (name, flags | O_BINARY, mode)
#endif

#if !defined (os_open)
#  define os_open(name, flags, mode) open (name, flags, mode)
#endif

#if !defined (errno)
extern int errno;
#endif

#if !defined (xfree)
#  define xfree(x) if (x) free (x)
#endif

#endif /* !_MHTMLSTD_H_ */
