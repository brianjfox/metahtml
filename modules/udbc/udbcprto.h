/*
 *  udbcprto.h
 *
 *  $Id: udbcprto.h,v 1.1 2001/10/28 15:33:11 bfox Exp $
 *
 *  OpenLink Universal Database Connectivity
 *  Prototypes
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

#ifndef _UDBCPRTO_H
#define _UDBCPRTO_H

#ifndef macintosh
# define UDBCFUN SQLRETURN UDBC_API
#else
# define UDBCFUN UDBC_API SQLRETURN
#endif

/*
 *  Function prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif

  UDBCFUN _UDBC_SetMessageHandler (
#ifdef UDBC_PROTOTYPES
    udbc_message_handler userHandler
#endif
  );

  UDBCFUN _UDBC_AllocConnect (
#ifdef UDBC_PROTOTYPES
    HENV henv,
    HDBC FAR * hdbc
#endif
  );

  UDBCFUN _UDBC_AllocEnv (
#ifdef UDBC_PROTOTYPES
    HENV FAR * henv
#endif
  );

  UDBCFUN _UDBC_AllocStmt (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    HSTMT FAR * hstmt
#endif
  );

  UDBCFUN _UDBC_BindCol (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD icol,
    SWORD fBufType,
    PTR rgbValue,
    SDWORD cbValueMax,
    SDWORD FAR * cbValue
#endif
  );

  UDBCFUN _UDBC_BindParam (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD ipar,
    SWORD fBufType,
    SWORD fSQLType,
    SDWORD cbParamDef,
    SWORD ibScale,
    PTR rgbValue,
    SDWORD FAR * cbValue
#endif
  );

  UDBCFUN _UDBC_BindParameter (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD ipar,
    SWORD fParamType,
    SWORD fCType,
    SWORD fSqlType,
    UDWORD cbColDef,
    SWORD ibScale,
    PTR rgbValue,
    SDWORD cbValueMax,
    SDWORD FAR * pcbValue
#endif
  );

  UDBCFUN _UDBC_Cancel (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt
#endif
  );

  UDBCFUN _UDBC_ColAttribute (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD icol,
    UWORD fDescType,
    PTR rgbDesc,
    SWORD cbDescMax,
    SWORD FAR * cbDesc,
    SDWORD FAR * fDesc
#endif
  );

  UDBCFUN _UDBC_Connect (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    UCHAR FAR * vcServer,
    SWORD cbServer,
    UCHAR FAR * vcUID,
    SWORD cbUID,
    UCHAR FAR * vcAuthStr,
    SWORD cbAuthStr
#endif
  );

  UDBCFUN _UDBC_DescribeCol (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD icol,
    UCHAR FAR * vcColName,
    SWORD FAR cbColNameMax,
    SWORD FAR * cbColName,
    SWORD FAR * fSqlType,
    UDWORD FAR * cbColDef,
    SWORD FAR * ibScale,
    SWORD FAR * fNullable
#endif
  );

  UDBCFUN _UDBC_Disconnect (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc
#endif
  );

  UDBCFUN _UDBC_Error (
#ifdef UDBC_PROTOTYPES
    HENV henv,
    HDBC hdbc,
    HSTMT hstmt,
    UCHAR FAR * vcSqlState,
    SDWORD FAR * fNativeError,
    UCHAR FAR FAR * vcErrorMsg,
    SWORD cbErrorMsgMax,
    SWORD FAR * cbErrorMsg
#endif
  );

  UDBCFUN _UDBC_ExecDirect (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcSqlStr,
    SDWORD cbSqlStr
#endif
  );

  UDBCFUN _UDBC_Execute (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt
#endif
  );

  UDBCFUN _UDBC_Fetch (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt
#endif
  );

  UDBCFUN _UDBC_FreeConnect (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc
#endif
  );

  UDBCFUN _UDBC_FreeEnv (
#ifdef UDBC_PROTOTYPES
    HENV henv
#endif
  );

  UDBCFUN _UDBC_FreeStmt (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD fOption
#endif
  );

  UDBCFUN _UDBC_GetCol (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD icol,
    SWORD fCType,
    PTR rgbValue,
    SDWORD cbValueMax,
    SDWORD FAR * cbValue
#endif
  );

  UDBCFUN _UDBC_GetCursorName (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcCursor,
    SWORD cbCursorMax,
    SWORD FAR * cbCursor
#endif
  );

  UDBCFUN _UDBC_MoreResults (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt
#endif
  );

  UDBCFUN _UDBC_NumResultCols (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    SWORD FAR * ccol
#endif
  );

  UDBCFUN _UDBC_ParamOptions (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UDWORD crow,
    UDWORD FAR * irow
#endif
  );

  UDBCFUN _UDBC_Prepare (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcSqlStr,
    SDWORD cbSqlStr
#endif
  );

  UDBCFUN _UDBC_RowCount (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    SDWORD FAR * crow
#endif
  );

  UDBCFUN _UDBC_SetCursorName (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcCursor,
    SWORD cbCursor
#endif
  );

  UDBCFUN _UDBC_SetParamValue (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD ipar,
    SWORD fBufType,
    SWORD fSqlType,
    UDWORD cbParDef,
    SWORD ibScale,
    PTR rgbValue,
    SDWORD FAR * cbValue
#endif
  );

  UDBCFUN _UDBC_Transact (
#ifdef UDBC_PROTOTYPES
    HENV henv,
    HDBC hdbc,
    UWORD fType
#endif
  );

  UDBCFUN _UDBC_BrowseConnect (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    UCHAR FAR * vcConnStrIn,
    SWORD cbConnStrIn,
    UCHAR FAR * vcConnStrOut,
    SWORD cbConnStrOutMax,
    SWORD FAR * cbConnStrOut
#endif
  );

  UDBCFUN _UDBC_ColumnPrivileges (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * vcTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * vcTableName,
    SWORD cbTableName,
    UCHAR FAR * vcColumnName,
    SWORD cbColumnName
#endif
  );

  UDBCFUN _UDBC_Columns (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * vcTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * vcTableName,
    SWORD cbTableName,
    UCHAR FAR * vcColumnName,
    SWORD cbColumnName
#endif
  );

#ifndef UDBC_STRICT_SAG

  UDBCFUN _UDBC_DataSources (
#ifdef UDBC_PROTOTYPES
    HENV henv,
    UWORD fDirection,
    UCHAR FAR * vcDSN,
    SWORD cbDSNMax,
    SWORD FAR * cbDSN,
    UCHAR FAR * vcDescription,
    SWORD cbDescriptionMax,
    SWORD FAR * cbDescription
#endif
  );

  UDBCFUN _UDBC_DescribeParam (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD icol,
    SWORD FAR * fSqlType,
    UDWORD FAR * cbColDef,
    SWORD FAR * ibScale,
    SWORD FAR * fNullable
#endif
  );

  UDBCFUN _UDBC_DriverConnect (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
#ifdef WIN16
    HWND hwnd,
#else
    PTR hwnd,
#endif
    UCHAR FAR * vcConnStrIn,
    SWORD cbConnStrIn,
    UCHAR FAR * vcConnStrOut,
    SWORD cbConnStrOutMax,
    SWORD FAR * cbConnStrOut,
    UWORD fDriverCompletion
#endif
  );

  UDBCFUN _UDBC_Drivers (
#ifdef UDBC_PROTOTYPES
    HENV henv,
    UWORD fDirection,
    UCHAR FAR * szDriverDesc,
    SWORD cbDriverDescMax,
    SWORD FAR * pcbDriverDesc,
    UCHAR FAR * szDriverAttributes,
    SWORD cbDrvrAttrMax,
    SWORD FAR * pcbDrvrAttr
#endif
);


  UDBCFUN _UDBC_ExtendedFetch (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD fFetchType,
    SDWORD irow,
    UDWORD FAR * crow,
    UWORD FAR * rgfRowStatus
#endif
  );

  UDBCFUN _UDBC_ForeignKeys (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcPkTableQualifier,
    SWORD cbPkTableQualifier,
    UCHAR FAR * vcPkTableOwner,
    SWORD cbPkTableOwner,
    UCHAR FAR * vcPkTableName,
    SWORD cbPkTableName,
    UCHAR FAR * vcFkTableQualifier,
    SWORD cbFkTableQualifier,
    UCHAR FAR * vcFkTableOwner,
    SWORD cbFkTableOwner,
    UCHAR FAR * vcFkTableName,
    SWORD cbFkTableName
#endif
  );

  UDBCFUN _UDBC_GetConnectOption (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    UWORD fOption,
    PTR pvParam
#endif
  );

  UDBCFUN _UDBC_GetFunctions (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    UWORD fFunction,
    UWORD FAR * fExists
#endif
  );

  UDBCFUN _UDBC_GetInfo (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    UWORD fInfoType,
    PTR rgbInfoValue,
    SWORD cbInfoValueMax,
    SWORD FAR * cbInfoValue
#endif
  );

  UDBCFUN _UDBC_GetStmtOption (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD fOption,
    PTR pvParam
#endif
  );

  UDBCFUN _UDBC_GetTypeInfo (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    SWORD fSqlType
#endif
  );

  UDBCFUN _UDBC_MoreResults (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt
#endif
  );

  UDBCFUN _UDBC_NativeSql (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    UCHAR FAR * vcSqlStrIn,
    SDWORD cbSqlStrIn,
    UCHAR FAR * vcSqlStr,
    SDWORD cbSqlStrMax,
    SDWORD FAR * cbSqlStr
#endif
  );

  UDBCFUN _UDBC_NumParams (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    SWORD FAR * cpar
#endif
  );

  UDBCFUN _UDBC_ParamData (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    PTR FAR * rgbValue
#endif
  );

  UDBCFUN _UDBC_ParamOptions (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UDWORD crow,
    UDWORD FAR * irow
#endif
  );

  UDBCFUN _UDBC_PrimaryKeys (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * vcTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * vcTableName,
    SWORD cbTableName
#endif
  );

  UDBCFUN _UDBC_ProcedureColumns (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcProcQualifier,
    SWORD cbProcQualifier,
    UCHAR FAR * vcProcOwner,
    SWORD cbProcOwner,
    UCHAR FAR * vcProcName,
    SWORD cbProcName,
    UCHAR FAR * vcColumnName,
    SWORD cbColumnName
#endif
  );

  UDBCFUN _UDBC_Procedures (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcProcQualifier,
    SWORD cbProcQualifier,
    UCHAR FAR * vcProcOwner,
    SWORD cbProcOwner,
    UCHAR FAR * vcProcName,
    SWORD cbProcName
#endif
  );

  UDBCFUN _UDBC_SetPos (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD irow,
    UWORD fRefresh,
    UWORD fLock
#endif
  );

  UDBCFUN _UDBC_PutData (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    PTR rgbValue,
    SDWORD cbValue
#endif
  );

  UDBCFUN _UDBC_SetConnectOption (
#ifdef UDBC_PROTOTYPES
    HDBC hdbc,
    UWORD fOption,
    UDWORD vParam
#endif
  );

  UDBCFUN _UDBC_SetScrollOptions (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD fConcurrency,
    SDWORD crowKeyset,
    UWORD crowRowset
#endif
  );

  UDBCFUN _UDBC_SetStmtOption (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD fOption,
    UDWORD vParam
#endif
  );

  UDBCFUN _UDBC_SpecialColumns (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UWORD fColType,
    UCHAR FAR * vcTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * vcTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * vcTableName,
    SWORD cbTableName,
    UWORD fScope,
    UWORD fNullable
#endif
  );

  UDBCFUN _UDBC_Statistics (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * vcTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * vcTableName,
    SWORD cbTableName,
    UWORD fUnique,
    UWORD fAccuracy
#endif
  );

  UDBCFUN _UDBC_TablePrivileges (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * vcTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * vcTableName,
    SWORD cbTableName
#endif
  );

  UDBCFUN _UDBC_Tables (
#ifdef UDBC_PROTOTYPES
    HSTMT hstmt,
    UCHAR FAR * vcTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * vcTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * vcTableName,
    SWORD cbTableName,
    UCHAR FAR * vcTableType,
    SWORD cbTableType
#endif
  );

#endif	/* UDBC_STRICT_SAG */

#ifdef __cplusplus
};
#endif

#endif
