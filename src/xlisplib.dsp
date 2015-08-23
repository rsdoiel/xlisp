# Microsoft Developer Studio Project File - Name="xlisplib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=xlisplib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xlisplib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xlisplib.mak" CFG="xlisplib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xlisplib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xlisplib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xlisplib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\xlisp.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy /y release\xlisp.lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "xlisplib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\xlisp.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy /y debug\xlisp.lib ..\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "xlisplib - Win32 Release"
# Name "xlisplib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\msstuff.c
# End Source File
# Begin Source File

SOURCE=.\xlansi.c
# End Source File
# Begin Source File

SOURCE=.\xlapi.c
# End Source File
# Begin Source File

SOURCE=.\xlcom.c
# End Source File
# Begin Source File

SOURCE=.\xldbg.c
# End Source File
# Begin Source File

SOURCE=.\xldmem.c
# End Source File
# Begin Source File

SOURCE=.\xlfasl.c
# End Source File
# Begin Source File

SOURCE=.\xlftab.c
# End Source File
# Begin Source File

SOURCE=.\xlfun1.c
# End Source File
# Begin Source File

SOURCE=.\xlfun2.c
# End Source File
# Begin Source File

SOURCE=.\xlfun3.c
# End Source File
# Begin Source File

SOURCE=.\xlimage.c
# End Source File
# Begin Source File

SOURCE=.\xlinit.c
# End Source File
# Begin Source File

SOURCE=.\xlint.c
# End Source File
# Begin Source File

SOURCE=.\xlio.c
# End Source File
# Begin Source File

SOURCE=.\xlitersq.c
# End Source File
# Begin Source File

SOURCE=.\xlmain.c
# End Source File
# Begin Source File

SOURCE=.\xlmath.c
# End Source File
# Begin Source File

SOURCE=.\xlobj.c
# End Source File
# Begin Source File

SOURCE=.\xlosint.c
# End Source File
# Begin Source File

SOURCE=.\xlprint.c
# End Source File
# Begin Source File

SOURCE=.\xlread.c
# End Source File
# Begin Source File

SOURCE=.\xlsym.c
# End Source File
# Begin Source File

SOURCE=.\xlterm.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
