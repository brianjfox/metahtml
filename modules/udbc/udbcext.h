/*
 *  udbcext.h
 *
 *  $Id: udbcext.h,v 1.1 2001/10/28 15:33:10 bfox Exp $
 *
 *  OpenLink ODBC extensions and changes to the SAG specifications
 *  Driver specific constants.
 *
 *  (C)Copyright 1993, 1994, 1995, 1996 OpenLink Software.
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
/**************************************************************************
 *
 *  Modification History
 *
 *  When	Who	What
 *  -----------------------------------------------------------------------
 *  8/1/96 	MPE	Created File. 
 *  17/2/96	MPE	Added extensions from sqlext.h
 *
 */

#ifndef _UDBCEXT_H
#define _UDBCEXT_H

/*
 * OpenLink API Extensions
 */

#define SQL_GETLASTSERIAL	1048L
#define SQL_GETLASTROWID	1049L

/* SQLBindParameter extensions */
#define SQL_DEFAULT_PARAM            (-5)
#define SQL_IGNORE                   (-6)
#define SQL_LEN_DATA_AT_EXEC_OFFSET  (-100)
#define SQL_LEN_DATA_AT_EXEC(length) (-length+SQL_LEN_DATA_AT_EXEC_OFFSET)

#endif /* _UDBCEXT_H */
