/*
 *  libudbc.h
 *
 *  $Id: libudbc.h,v 1.1 2001/10/28 15:33:10 bfox Exp $
 *
 *  OpenLink Universal Database Connectivity
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
 *
 *  NOTE:
 *   This is a beta release of the UDBC SDK.
 *   The structure of those header files will be cleaned up in the future.
 */

#ifndef _LIBUDBC_H
#define _LIBUDBC_H

#define UDBC_PROTOTYPES		/* define this for ANSI C function prototypes */

#define UDBC_SAG	0	/* If 1, include SAG definitions */
#define UDBC_ODBC	1	/* If 1, include ODBC definitions */
#define UDBC_OPL	1	/* If 1, include OpenLink extentions */

/*
 *  Map the SAG functions to the OpenLink SDK functions
 */
#if UDBC_SAG

#include <udbc/udbcsag.h>	/* UDBC / SAG definitions */

#define AllocConnect		_UDBC_AllocConnect
#define AllocEnv		_UDBC_AllocEnv
#define AllocStmt		_UDBC_AllocStmt
#define BindCol			_UDBC_BindCol
#define BindParam		_UDBC_BindParam
#define Cancel			_UDBC_Cancel
#define ColAttribute		_UDBC_ColAttribute
#define Connect			_UDBC_Connect
#define DescribeCol		_UDBC_DescribeCol
#define Disconnect		_UDBC_Disconnect
#define Error			_UDBC_Error
#define ExecDirect		_UDBC_ExecDirect
#define Execute			_UDBC_Execute
#define Fetch			_UDBC_Fetch
#define FreeConnect		_UDBC_FreeConnect
#define FreeEnv			_UDBC_FreeEnv
#define FreeStmt		_UDBC_FreeStmt
#define GetCol			_UDBC_GetCol
#define GetCursorName		_UDBC_GetCursorName
#define NumResultCols		_UDBC_NumResultCols
#define Prepare			_UDBC_Prepare
#define RowCount		_UDBC_RowCount
#define SetCursorName		_UDBC_SetCursorName
#define SetParamValue		_UDBC_SetParamValue
#define Transact		_UDBC_Transact

#endif	/* UDBC_SAG */


/*
 *  Map the Microsoft ODBC functions to the OpenLink SDK functions
 */
#if UDBC_ODBC

#include <udbc/udbcodbc.h>	/* UDBC / ODBC definitions */

#ifndef macintosh

#define SQLAllocConnect		_UDBC_AllocConnect
#define SQLAllocEnv		_UDBC_AllocEnv
#define SQLAllocStmt		_UDBC_AllocStmt
#define SQLBindCol		_UDBC_BindCol
#define SQLBindParameter	_UDBC_BindParameter
#define SQLBrowseConnect	_UDBC_BrowseConnect
#define SQLCancel		_UDBC_Cancel
#define SQLColAttributes	_UDBC_ColAttribute
#define SQLColumnPrivileges	_UDBC_ColumnPrivileges
#define SQLColumns		_UDBC_Columns
#define SQLConnect		_UDBC_Connect
#define SQLDataSources		_UDBC_DataSources
#define SQLDescribeCol		_UDBC_DescribeCol
#define SQLDescribeParam	_UDBC_DescribeParam
#define SQLDisconnect		_UDBC_Disconnect
#define SQLDrivers		_UDBC_Drivers
#define SQLDriverConnect	_UDBC_DriverConnect
#define SQLError		_UDBC_Error
#define SQLExecDirect		_UDBC_ExecDirect
#define SQLExecute		_UDBC_Execute
#define SQLExtendedFetch	_UDBC_ExtendedFetch
#define SQLFetch		_UDBC_Fetch
#define SQLForeignKeys		_UDBC_ForeignKeys
#define SQLFreeConnect		_UDBC_FreeConnect
#define SQLFreeEnv		_UDBC_FreeEnv
#define SQLFreeStmt		_UDBC_FreeStmt
#define SQLGetConnectOption	_UDBC_GetConnectOption
#define SQLGetCursorName	_UDBC_GetCursorName
#define SQLGetData		_UDBC_GetCol
#define SQLGetFunctions		_UDBC_GetFunctions
#define SQLGetInfo		_UDBC_GetInfo
#define SQLGetStmtOption	_UDBC_GetStmtOption
#define SQLGetTypeInfo		_UDBC_GetTypeInfo
#define SQLMoreResults		_UDBC_MoreResults
#define SQLNativeSql		_UDBC_NativeSql
#define SQLNumParams		_UDBC_NumParams
#define SQLNumResultCols	_UDBC_NumResultCols
#define SQLParamData		_UDBC_ParamData
#define SQLParamOptions		_UDBC_ParamOptions
#define SQLPrepare		_UDBC_Prepare
#define SQLPrimaryKeys		_UDBC_PrimaryKeys
#define SQLProcedureColumns	_UDBC_ProcedureColumns
#define SQLProcedures		_UDBC_Procedures
#define SQLPutData		_UDBC_PutData
#define SQLRowCount		_UDBC_RowCount
#define SQLSetConnectOption	_UDBC_SetConnectOption
#define SQLSetCursorName	_UDBC_SetCursorName
#define SQLSetParam		_UDBC_SetParamValue
#define SQLSetPos		_UDBC_SetPos
#define SQLSetScrollOptions	_UDBC_SetScrollOptions
#define SQLSetStmtOption	_UDBC_SetStmtOption
#define SQLSpecialColumns	_UDBC_SpecialColumns
#define SQLStatistics		_UDBC_Statistics
#define SQLTablePrivileges	_UDBC_TablePrivileges
#define SQLTables		_UDBC_Tables
#define SQLTransact		_UDBC_Transact

#endif /* macintosh */

#endif	/* UDBC_ODBC */

#ifdef UDBC_OPL
#include <udbc/udbcext.h>
#endif

#include <udbc/udbcprto.h>	/* Function prototypes */

#endif
