/*
 *  udbctype.h
 *
 *  $Id: udbctype.h,v 1.1 2001/10/28 15:33:12 bfox Exp $
 *
 *  OpenLink Universal Database Connectivity
 *  Machine / operating system dependent type definitions for UDBC
 *
 *  (C)Copyright 1993, 1994 OpenLink Software.
 *  All Rights Reserved.
 *
 *  The copyright above and this notice must be preserved in all
 *  copies of this source code.  The copyright above does not
 *  evidence any actual or intended publication of this source code.
 *
 *  This is unpublished proprietary trade secret of OpenLink Software.
 *  This source code may not be copied, disclosed, distributed, demonstrated
 *  or licensed except as authorized by OpenLink Software.
 */

#ifndef _UDBCTYPE_H
#define _UDBCTYPE_H

#if defined (WIN16)
# ifndef FAR
#  define FAR		far
# endif
# ifndef PASCAL
#  define PASCAL	pascal
# endif
# define UDBC_API	PASCAL FAR
#elif defined (WIN32)
# ifndef FAR
#  define FAR
# endif
# define UDBC_API	__stdcall
#elif defined (macintosh)
# define FAR
# define UDBC_API	pascal
#else
# define FAR
# define UDBC_API
#endif

#ifdef VAXC
# define signed
#endif

/*
 *  API declaration data types
 */
typedef unsigned char	UCHAR;
typedef signed short	SWORD;
typedef unsigned short	UWORD;
typedef signed long	SDWORD;
typedef unsigned long	UDWORD;
typedef signed long	SLONG;
typedef unsigned long	ULONG;
typedef signed short	SSHORT;
typedef unsigned short	USHORT;
typedef void FAR *	PTR;
#if !defined(_Windows) && !defined(__OS2__)
typedef short		BOOL;
#endif

typedef void (*udbc_message_handler) (char *);

#endif
