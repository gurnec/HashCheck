# Microsoft Developer Studio Project File - Name="HashCheck" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# USEKIT 3

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HashCheck.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HashCheck.mak" CFG="HashCheck - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HashCheck - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "HashCheck - Win64 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

CPP=cl.exe
RSC=rc.exe
LINK32=link.exe

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Target_Dir ""

!IF "$(CFG)" == "HashCheck - Win32 Release"

# PROP Output_Dir "bin.x86-32"
# PROP Intermediate_Dir "obj.x86-32"
# ADD CPP /nologo /MD /W3 /G6 /GF /EHsc /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "_WIN32_WINNT=0x0501" /D "SL_ENABLE_LGBLK" /D "SL_SMBLK_SIZE=0x1000" /c
# ADD RSC /l 0x409 /d "NDEBUG" /d "_M_IX86"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib shlwapi.lib comctl32.lib uxtheme.lib winhash.lib qsort_s_uptr.lib /nologo /LIBPATH:libs\x86-32 /DLL /SUBSYSTEM:WINDOWS,5.1 /OSVERSION:5.1 /MACHINE:IX86 /RELEASE /OPT:REF /OPT:ICF /OPT:NOWIN98 /MERGE:.rdata=.text /IGNORE:4078

!ELSEIF "$(CFG)" == "HashCheck - Win64 Release"

# PROP Output_Dir "bin.x86-64"
# PROP Intermediate_Dir "obj.x86-64"
# ADD CPP /nologo /MD /W3 /Wp64 /GS- /GF /EHsc /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "_WIN32_WINNT=0x0502" /D "SL_ENABLE_LGBLK" /D "SL_SMBLK_SIZE=0x1000" /c
# ADD RSC /l 0x409 /d "NDEBUG" /d "_M_AMD64"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib shlwapi.lib comctl32.lib uxtheme.lib winhash.lib qsort_s_uptr.lib /nologo /LIBPATH:libs\x86-64 /DLL /SUBSYSTEM:WINDOWS,5.2 /OSVERSION:5.2 /MACHINE:AMD64 /RELEASE /OPT:REF /OPT:ICF /OPT:NOWIN98 /MERGE:.rdata=.text /IGNORE:4078

!ENDIF

# Begin Target
# Name "HashCheck - Win32 Release"
# Name "HashCheck - Win64 Release"

# Begin Group "Source Files"
# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File
SOURCE=.\HashCheck.cpp
# End Source File
# Begin Source File
SOURCE=.\CHashCheck.cpp
# End Source File
# Begin Source File
SOURCE=.\CHashCheckClassFactory.cpp
# End Source File
# Begin Source File
SOURCE=.\HashCheckCommon.c
# End Source File
# Begin Source File
SOURCE=.\HashCalc.c
# End Source File
# Begin Source File
SOURCE=.\HashSave.c
# End Source File
# Begin Source File
SOURCE=.\HashProp.c
# End Source File
# Begin Source File
SOURCE=.\HashVerify.c
# End Source File
# Begin Source File
SOURCE=.\HashCheckOptions.c
# End Source File
# Begin Source File
SOURCE=.\RegHelpers.c
# End Source File
# Begin Source File
SOURCE=.\SetAppID.c
# End Source File
# Begin Source File
SOURCE=.\UnicodeHelpers.c
# End Source File
# Begin Source File
SOURCE=.\HashCheck.def
# End Source File
# End Group

# Begin Group "Header Files"
# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File
SOURCE=.\CHashCheck.hpp
# End Source File
# Begin Source File
SOURCE=.\CHashCheckClassFactory.hpp
# End Source File
# Begin Source File
SOURCE=.\HashCheckUI.h
# End Source File
# Begin Source File
SOURCE=.\HashCheckCommon.h
# End Source File
# Begin Source File
SOURCE=.\HashCalc.h
# End Source File
# Begin Source File
SOURCE=.\HashCheckOptions.h
# End Source File
# Begin Source File
SOURCE=.\GetHighMSB.h
# End Source File
# Begin Source File
SOURCE=.\RegHelpers.h
# End Source File
# Begin Source File
SOURCE=.\SetAppID.h
# End Source File
# Begin Source File
SOURCE=.\UnicodeHelpers.h
# End Source File
# Begin Source File
SOURCE=.\globals.h
# End Source File
# Begin Source File
SOURCE=.\version.h
# End Source File
# End Group

# Begin Group "Resource Files"
# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File
SOURCE=.\HashCheck.rc
# End Source File
# End Group

# Begin Group "Libraries"
# PROP Default_Filter ""
# Begin Source File
SOURCE=.\libs\WinIntrinsics.h
# End Source File
# Begin Source File
SOURCE=.\libs\SimpleString.h
# End Source File
# Begin Source File
SOURCE=.\libs\SimpleString.c
# End Source File
# Begin Source File
SOURCE=.\libs\SimpleList.h
# End Source File
# Begin Source File
SOURCE=.\libs\SimpleList.c
# End Source File
# Begin Source File
SOURCE=.\libs\SwapIntrinsics.h
# End Source File
# Begin Source File
SOURCE=.\libs\SwapIntrinsics.c
# End Source File
# Begin Source File
SOURCE=.\libs\IsFontAvailable.h
# End Source File
# Begin Source File
SOURCE=.\libs\IsFontAvailable.c
# End Source File
# Begin Source File
SOURCE=.\libs\WinHash.h
# End Source File
# Begin Source File
SOURCE=.\libs\WinHash.c
# End Source File
# Begin Source File
SOURCE=.\libs\Wow64.h
# End Source File
# Begin Source File
SOURCE=.\libs\Wow64.c
# End Source File
# End Group

# End Target
# End Project
