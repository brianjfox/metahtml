/* odbcdefs.h: -*- C -*-  . */

/* Author: Henry Minsky (hqm@ua.com) */

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

#if !defined (_ODBCDEFS_H_)
#define _ODBCDEFS_H_

#if defined (OPENLINK_UDBC_CLIENT)
# define SQLRETCODE int

/* C to SQL datatype mapping */
# define SQL_SIGNED_OFFSET		(-20)
# define SQL_UNSIGNED_OFFSET		(-22)

# define SQL_C_SLONG			(SQL_C_LONG  + SQL_SIGNED_OFFSET)
# define SQL_C_SSHORT			(SQL_C_SHORT + SQL_SIGNED_OFFSET)


/* options for SQLSetConnectOption/SQLGetConnectOption */
# define SQL_CURRENT_QUALIFIER		109
# define SQL_ODBC_CURSORS		110
# define SQL_QUIET_MODE			111
# define SQL_PACKET_SIZE 		112

/* SQLGetInfo infor number */
# define SQL_NON_NULLABLE_COLUMNS	75
# define SQL_DRIVER_HLIB		76
# define SQL_QUALIFIER_LOCATION		114

#endif /* OPENLINK_UDBC_CLIENT */

/* Below are declarations for ODBC calls which we use. 
   The list below does not cover the complete ODBC
   spec,  so add declarations as you need them.   */
RETCODE SQLAllocConnect (HENV henv, HDBC FAR *phdbc);
RETCODE SQLAllocEnv (HENV FAR *phenv);
RETCODE SQLAllocStmt (HDBC hdbc, HSTMT FAR *phstmt);
RETCODE SQLExecDirect (HSTMT hstmt, UCHAR FAR  *szSqlStr, SDWORD cbSqlStr);

RETCODE SQLDriverConnect (HDBC hdbc,
			  PTR hwnd,
			  UCHAR FAR *szConnStrIn,
			  SWORD cbConnStrIn,
			  UCHAR FAR *szConnStrOut,
			  SWORD	cbConnStrOutMax,
			  SWORD FAR *pcbConnStrOut,
			  UWORD	fDriverCompletion);

RETCODE SQLDisconnect (HDBC hdbc);

RETCODE SQLError (HENV henv,
		  HDBC hdbc,
		  HSTMT hstmt,
		  UCHAR FAR *szSqlState,
		  SDWORD FAR *pfNativeError,
		  UCHAR FAR *szErrorMsg,
		  SWORD cbErrorMsgMax,
		  SWORD FAR *pcbErrorMsg);

RETCODE SQLPrepare (HSTMT hstmt, UCHAR FAR *szSqlStr, SDWORD cbStr);
RETCODE SQLExecute (HSTMT hstmt);
RETCODE SQLFetch (HSTMT hstmt);
RETCODE SQLFreeConnect (HDBC hdbc);
RETCODE SQLFreeEnv (HENV henv);
RETCODE SQLFreeStmt (HSTMT hstmt, UWORD fOption);

RETCODE SQLGetData (HSTMT hstmt, UWORD icol, SWORD fCType, PTR rgbValue,
		    SDWORD cbValueMax, SDWORD FAR *pcbValue);

RETCODE SQLDescribeCol (HSTMT hstmt,
			UWORD icol,
			UCHAR FAR *vcColName,
			SWORD FAR cbColNameMax,
			SWORD FAR *cbColName,
			SWORD FAR *fSqlType,
			UDWORD FAR *cbColDef,
			SWORD FAR *ibScale,
			SWORD FAR *fNullable);

RETCODE SQLRowCount (HSTMT hstmt, SDWORD FAR *pcrow);

RETCODE SQLColAttributes (HSTMT hstmt,
			  UWORD icol,
			  UWORD fDescType,
			  PTR rgbDesc,
			  SWORD cbDescMax,
			  SWORD FAR *pcbDesc,
			  SDWORD FAR *pfDesc);

RETCODE SQLTables (HSTMT hstmt,
		   UCHAR FAR *szCatalogName,
		   SWORD cbCatalogName,
		   UCHAR FAR *szSchemaName,
		   SWORD cbSchemaName,
		   UCHAR FAR *szTableName,
		   SWORD cbTableName,
		   UCHAR FAR *szTableType,
		   SWORD cbTableType);

RETCODE SQLColumns (HSTMT hstmt,
		    UCHAR FAR *szCatalogName,
		    SWORD cbCatalogName,
		    UCHAR FAR *szSchemaName,
		    SWORD cbSchemaName,
		    UCHAR FAR *szTableName,
		    SWORD cbTableName,
		    UCHAR FAR *szColumnName,
		    SWORD cbColumnName);

RETCODE SQLGetTypeInfo (HSTMT hstmt, SWORD fSqlType);

RETCODE SQLDataSources (HENV henv,
			UWORD fDirection,
			UCHAR FAR *szDSN,
			SWORD cbDSNMax,
			SWORD FAR *pcbDSN,
			UCHAR FAR *szDescription,
			SWORD cbDescriptionMax,
			SWORD FAR *pcbDescription);

RETCODE SQLTransact (HENV henv, HDBC hdbc, UWORD fType);
RETCODE SQLCancel (HSTMT hstmt);
RETCODE SQLNumResultCols (HSTMT hstmt, SWORD FAR *pcol);

RETCODE SQLBindCol (HSTMT hstmt, 
		    UWORD icol,
		    SWORD fCType,
		    PTR rgbValue,
		    SDWORD cbValueMax,
		    SDWORD FAR * pcbValue);

RETCODE SQLGetInfo (HDBC hdbc,
		    UWORD fInfoType,
		    PTR rgbInfoValue,
		    SWORD cbInfoValueMax,
		    SWORD FAR * pcbInfoValue);

RETCODE SQLGetConnectOption (HDBC hdbc, UWORD fOption, PTR pvParam);
RETCODE SQLSetConnectOption (HDBC hdbc, UWORD fOption, UDWORD pvParam);

#endif /* _ODBCDEFS_H_ */
