# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=mhtml - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to mhtml - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "mhtml - Win32 Release" && "$(CFG)" != "mhtml - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "mhtml.mak" CFG="mhtml - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mhtml - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mhtml - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
CPP=cl.exe
MTL=mktyplib.exe
RSC=rc.exe

!IF  "$(CFG)" == "mhtml - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "obj"
OUTDIR=.\WinRel
INTDIR=.\obj

ALL : "$(OUTDIR)\mhtml.dll"

CLEAN : 
	-@erase "$(INTDIR)\mhtml_engine.obj"
	-@erase "$(INTDIR)\ntrans.obj"
	-@erase "$(OUTDIR)\mhtml.dll"
	-@erase "$(OUTDIR)\mhtml.exp"
	-@erase "$(OUTDIR)\mhtml.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../include" /I "../include/base" /I "../include/frame" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "XP_WIN32" /D "MCC_HTTPD" /YX /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "../include" /I "../include/base" /I\
 "../include/frame" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "XP_WIN32" /D\
 "MCC_HTTPD" /Fp"$(INTDIR)/mhtml.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\obj/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /I "386" /D "NDEBUG"
# ADD MTL /nologo /I "386" /D "NDEBUG"
MTL_PROJ=/nologo /I "386" /D "NDEBUG" 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/mhtml.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ./libhttpd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /force
LINK32_FLAGS=./libhttpd.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/mhtml.pdb" /machine:I386 /force /out:"$(OUTDIR)/mhtml.dll"\
 /implib:"$(OUTDIR)/mhtml.lib" 
LINK32_OBJS= \
	"$(INTDIR)\mhtml_engine.obj" \
	"$(INTDIR)\ntrans.obj"

"$(OUTDIR)\mhtml.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mhtml - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "obj"
OUTDIR=.\WinDebug
INTDIR=.\obj

ALL : "$(OUTDIR)\mhtml.dll"

CLEAN : 
	-@erase "$(INTDIR)\mhtml_engine.obj"
	-@erase "$(INTDIR)\ntrans.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\mhtml.dll"
	-@erase "$(OUTDIR)\mhtml.exp"
	-@erase "$(OUTDIR)\mhtml.ilk"
	-@erase "$(OUTDIR)\mhtml.lib"
	-@erase "$(OUTDIR)\mhtml.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../include" /I "../include/base" /I "../include/frame" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "XP_WIN32" /D "MCC_HTTPD" /YX /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "../include" /I "../include/base"\
 /I "../include/frame" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "XP_WIN32" /D\
 "MCC_HTTPD" /Fp"$(INTDIR)/mhtml.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\obj/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /I "386" /D "_DEBUG"
# ADD MTL /nologo /I "386" /D "_DEBUG"
MTL_PROJ=/nologo /I "386" /D "_DEBUG" 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/mhtml.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 wsock32.lib ./libhttpd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /force
LINK32_FLAGS=wsock32.lib ./libhttpd.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)/mhtml.pdb" /debug /machine:I386 /force\
 /out:"$(OUTDIR)/mhtml.dll" /implib:"$(OUTDIR)/mhtml.lib" 
LINK32_OBJS= \
	"$(INTDIR)\mhtml_engine.obj" \
	"$(INTDIR)\ntrans.obj"

"$(OUTDIR)\mhtml.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "mhtml - Win32 Release"
# Name "mhtml - Win32 Debug"

!IF  "$(CFG)" == "mhtml - Win32 Release"

!ELSEIF  "$(CFG)" == "mhtml - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ntrans.c
DEP_CPP_NTRAN=\
	"..\include\base\net.h"\
	"..\include\base\pool.h"\
	"..\include\base\sem.h"\
	"..\include\base\systems.h"\
	"..\include\frame\http.h"\
	"..\include\version.h"\
	".\../include\base/buffer.h"\
	".\../include\base/ereport.h"\
	".\../include\base/file.h"\
	".\../include\base/pblock.h"\
	".\../include\base/regexp.h"\
	".\../include\base/session.h"\
	".\../include\base/shexp.h"\
	".\../include\base/util.h"\
	".\../include\frame/log.h"\
	".\../include\frame/object.h"\
	".\../include\frame/objset.h"\
	".\../include\frame/protocol.h"\
	".\../include\frame/req.h"\
	".\../include\netsite.h"\
	{$(INCLUDE)}"\sys\stat.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\ntrans.obj" : $(SOURCE) $(DEP_CPP_NTRAN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\mhtml_engine.c
DEP_CPP_MHTML=\
	"..\include\base\net.h"\
	"..\include\base\pool.h"\
	"..\include\base\sem.h"\
	"..\include\base\systems.h"\
	"..\include\version.h"\
	".\../include\base/buffer.h"\
	".\../include\base/ereport.h"\
	".\../include\base/file.h"\
	".\../include\base/pblock.h"\
	".\../include\base/session.h"\
	".\../include\frame/func.h"\
	".\../include\frame/log.h"\
	".\../include\frame/object.h"\
	".\../include\frame/objset.h"\
	".\../include\frame/req.h"\
	".\../include\netsite.h"\
	{$(INCLUDE)}"\sys\stat.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\mhtml_engine.obj" : $(SOURCE) $(DEP_CPP_MHTML) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
