/* logging.c: Logging for the server. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Tue Nov 14 19:14:25 1995.  */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
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

#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include <lockname.h>
#include "logging.h"

#if !defined (HAVE_TYPE_TIME_T)
typedef long time_t;
#endif

#if defined (macintosh)
extern char *strdup (const char *string);
#  define os_open(name, flags, mode) open (name, flags)
#endif

#if defined (__CYGWIN32__)
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

#if !defined (SEEK_END)
# define SEEK_END 2
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

extern char *pagefunc_get_variable (char *);

LOG_MAPPING **mhttpd_logfiles = (LOG_MAPPING **)NULL;
static int logfiles_size = 0;
static int logfiles_index = 0;

static LOG_MAPPING *
make_log_mapping (void)
{
  LOG_MAPPING *mapping = (LOG_MAPPING *)xmalloc (sizeof (LOG_MAPPING));

  memset (mapping, 0, sizeof (LOG_MAPPING));
  return (mapping);
}

static void
free_mapping (LOG_MAPPING *mapping)
{
  if (mapping != (LOG_MAPPING *)NULL)
    {
      if (mapping->logfile) free (mapping->logfile);
      free (mapping);
    }
}

static LOG_MAPPING *
mhttpd_find_logfile (int which)
{
  LOG_MAPPING *mapping = (LOG_MAPPING *)NULL;

  if (mhttpd_logfiles)
    {
      register int i;

      for (i = 0; (mapping = mhttpd_logfiles[i]) != (LOG_MAPPING *)NULL; i++)
	if (which == mapping->which)
	  break;
    }

  return (mapping);
}

static void
mhttpd_add_logfile (int which, char *name)
{
  LOG_MAPPING *mapping = make_log_mapping ();

  mapping->logfile = strdup (name);
  mapping->which = which;

  if (logfiles_index +2 > logfiles_size)
    mhttpd_logfiles = (LOG_MAPPING **)xrealloc
      (mhttpd_logfiles, (logfiles_size += 5) * sizeof (LOG_MAPPING *));

  mhttpd_logfiles[logfiles_index++] = mapping;
  mhttpd_logfiles[logfiles_index] = (LOG_MAPPING *)NULL;
}

void
mhttpd_set_logfile (int which, char *name)
{
  LOG_MAPPING *mapping = mhttpd_find_logfile (which);

  if (mapping)
    {
      free (mapping->logfile);
      mapping->logfile = strdup (name ? name : "");
    }
  else
    {
      mhttpd_add_logfile (which, name ? name : "");
    }
}

char *
mhttpd_get_logfile (int which)
{
  LOG_MAPPING *mapping = mhttpd_find_logfile (which);

  if (mapping)
    return (mapping->logfile);
  else
    return ((char *)NULL);
}

/* Given TICKS (the number of seconds since the epoch), return a string
   representing that date in the format the logfiles mandate.  The
   string returned comes from a static buffer.  The caller must manually
   save it away if it is not to be used immediately.
   [06/Nov/1995:05:00:22 -0800] */
char *
mhttpd_date_format (long ticks)
{
  struct tm *date = localtime ((const time_t *)&ticks);
  static char result[100];
  int hours_from_gmt = 0;
  char zone[10];
  static char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

#if defined (__CYGWIN32__) || defined (hpux)
  {
    struct timeval tv;
    struct timezone tz;
    gettimeofday (&tv, &tz);
    hours_from_gmt = ((tz.tz_minuteswest / 60) / 60);
  }
#else
#  if defined (__linux__) || defined (Solaris) || defined (sgi) || defined (_AIX) || defined (_SCO_DS)
  hours_from_gmt = (timezone / 60) / 60;
#  else
  hours_from_gmt = (date->tm_gmtoff / 60) / 60;
#  endif
#endif

  sprintf (zone, "%05d", -(100 * hours_from_gmt));

  sprintf (result, "%02d/%s/%04d:%02d:%02d:%02d %s",
	   date->tm_mday, months[date->tm_mon], date->tm_year + 1900,
	   date->tm_hour, date->tm_min, date->tm_sec, zone);

  return (result);
}

static int
lock_logfile (char *path)
{
  char lockfile[1024];
  int lock;

  sprintf (lockfile, "%s.lck", path);
  lock = os_open (lockfile, O_WRONLY, 0666);

  if (lock == -1)
    lock = os_open (lockfile, O_CREAT | O_WRONLY, 0666);

  if (lock != -1)
    LOCKFILE (lock);

  return (lock);
}

static void
unlock_logfile (char *path, int lock)
{
  if (lock != -1)
    {
      UNLOCKFILE (lock);
      close (lock);
    }
}

void
mhttpd_log_verbatim (int which, char *format, ...)
{
  LOG_MAPPING *mapping = mhttpd_find_logfile (which);

  if (mapping)
    {
      int lock;

      if ((mapping->logfile == (char *)NULL) || (mapping->logfile[0] == '\0'))
	lock = -1;
      else
	lock = lock_logfile (mapping->logfile);

      if (lock != -1)
	{
	  int fd;

	  fd = os_open (mapping->logfile, O_CREAT | O_WRONLY | O_APPEND, 0666);

	  if (fd != -1)
	    {
	      BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
	      va_list args;

	      va_start (args, format);

	      vbprintf (buffer, format, args);
	      /* lseek (fd, (off_t)0, SEEK_END); */
	      write (fd, buffer->buffer, buffer->bindex);
	      bprintf_free_buffer (buffer);
	      close (fd);
	    }

	  unlock_logfile (mapping->logfile, lock);
	}
    }
}

void
mhttpd_access_log (char *name, char *format, ...)
{
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  time_t ticks = time ((time_t *)0);
  char *date_string = mhttpd_date_format (ticks);
  va_list args;
  va_start (args, format);

  bprintf (buffer, "%s - - [%s] ", name, date_string);
  vbprintf (buffer, format, args);
  mhttpd_log_verbatim (log_ACCESS, "%s - -\n", buffer->buffer);
  bprintf_free_buffer (buffer);
}

void
mhttpd_log (int which, char *format, ...)
{
  BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
  char *server_name = pagefunc_get_variable ("mhtml::server-name");
  char *server_port = pagefunc_get_variable ("mhtml::server-port");
  time_t ticks = time ((time_t *)0);
  char *date_string = mhttpd_date_format (ticks);
  va_list args;

  va_start (args, format);

  if (!server_name) server_name = "[No mhtml::server-name!]";
  if (!server_port) server_port = "80";

  bprintf (buffer, "%s:%s - - [%s] ", server_name, server_port, date_string);
  vbprintf (buffer, format, args);
  mhttpd_log_verbatim (which, "%s\n", buffer->buffer);
  bprintf_free_buffer (buffer);
}

#if defined (__cplusplus)
}
#endif
