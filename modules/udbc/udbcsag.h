/*
 *  udbcsag.h
 *
 *  $Id: udbcsag.h,v 1.1 2001/10/28 15:33:12 bfox Exp $
 *
 *  OpenLink Universal Database Connectivity
 *  SAG Interface Definition
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

#ifndef _UDBCSAG_H
#define _UDBCSAG_H

#ifndef _UDBCTYPE_H
#include <udbc/udbctype.h>
#endif

/*
 *  SAG typedefs
 */
typedef UCHAR		SQLCHAR;
typedef SDWORD		SQLINTEGER;
typedef UDWORD		SQLUINTEGER;
typedef SWORD		SQLSMALLINT;
typedef UWORD		SQLUSMALLINT;
typedef double		SQLDOUBLE;
typedef float		SQLREAL;
typedef PTR		SQLPOINTER;

typedef SDWORD	 	SQLHENV;	/* environment handle */
typedef SDWORD	 	SQLHDBC;	/* connection handle */
typedef SDWORD	 	SQLHSTMT;	/* statement handle */

/*
 *  Function return type
 */
typedef int 		SQLRETURN;

/*
 *  Non fatal error indications
 */
#define SQL_NULL_DATA			(-1)

/*
 *  Return values from functions
 */
#define SQL_SUCCESS			0
#define SQL_SUCCESS_WITH_INFO		1
#define SQL_NO_DATA			100
#define SQL_ERROR			(-1)
#define SQL_INVALID_HANDLE		(-2)

/*
 *  Flag for null-terminated string
 */
#define SQL_NTS				(-3)

/*
 *  Maximum message length
 */
#define SQL_MAX_MESSAGE_LENGTH		512

#define SQL_SQLSTATE_SIZE		5

/*
 *  Maximum identifier length
 */
#define SQL_MAX_DSN_LENGTH		32
#define SQL_MAX_ID_LENGTH		18

/*
 *  SQL data type codes, as defined in the X/Open SQL specification
 */
#define SQL_CHAR			1
#define SQL_NUMERIC			2
#define SQL_DECIMAL			3
#define SQL_INTEGER			4
#define SQL_SMALLINT			5
#define SQL_FLOAT			6
#define SQL_REAL			7
#define SQL_DOUBLE			8
#define SQL_VARCHAR			12

/*
 *  Application buffer types
 */
#define SQLBUF_CHAR			SQL_CHAR
#define SQLBUF_LONG			SQL_INTEGER
#define SQLBUF_SHORT			SQL_SMALLINT
#define SQLBUF_FLOAT			SQL_REAL
#define SQLBUF_DOUBLE			SQL_DOUBLE
#define SQLBUF_DEFAULT			99

/*
 *  DescribeCol() description of NULLABLE attribute
 */
#define SQL_NO_NULLS			0
#define SQL_NULLABLE			1
#define SQL_NULLABLE_UNKNOWN		2

/*
 *  FreeStmt() options
 */
#define SQL_CLOSE			0
#define SQL_DROP			1
#define SQL_UNBIND			2
#define SQL_RESET_PARAMS		3

/*
 *  Transact() options
 */
#define SQL_COMMIT			0
#define SQL_ROLLBACK			1

/*
 *  Error() options
 */
#define SQL_NULL_HENV			0
#define SQL_NULL_HDBC			0
#define SQL_NULL_HSTMT			0

/*
 *  ColAttribute options
 */
#define SQL_COLUMN_COUNT		0
#define SQL_COLUMN_NAME			1
#define SQL_COLUMN_TYPE			2
#define SQL_COLUMN_LENGTH		3
#define SQL_COLUMN_PRECISION		4
#define SQL_COLUMN_SCALE		5
#define SQL_COLUMN_DISPLAY_SIZE		6
#define SQL_COLUMN_NULLABLE		7

#endif /* _UDBCSAG_H */
