/* 
 * fcgiappmisc.h --
 *
 *      Functions implemented by fcgiapp.h that aren't needed
 *      by normal applications, but may be useful to special
 *      applications.
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: fcgiappmisc.h,v 1.1 2001/10/28 15:29:31 bfox Exp $
 */

#ifndef _FCGIAPPMISC_H
#define _FCGIAPPMISC_H

#include "fcgiapp.h"         /* for FCGX_Stream */

#if defined (c_plusplus) || defined (__cplusplus)
extern "C" {
#endif

#ifdef _WIN32
#ifndef DLLAPI
#define DLLAPI __declspec(dllimport)
#endif
#else
#define DLLAPI
#endif

DLLAPI FCGX_Stream *CreateWriter(
        int socket,
        int requestId,
        int bufflen,
        int streamType);

DLLAPI void FreeStream(FCGX_Stream **stream);

#if defined (__cplusplus) || defined (c_plusplus)
} /* terminate extern "C" { */
#endif

#endif	/* _FCGIAPPMISC_H */
