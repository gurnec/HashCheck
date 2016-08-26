SetCompressor /FINAL /SOLID lzma

!include MUI2.nsh
!include x64.nsh

Unicode true

Name "HashCheck"
OutFile "HashCheckSetup-v2.4.0.38-beta.exe"

RequestExecutionLevel admin
ManifestSupportedOS all

!define MUI_ICON ..\HashCheck.ico
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_CUSTOMFUNCTION_SHOW change_license_font
!insertmacro MUI_PAGE_LICENSE "..\license.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Change to a smaller monospace font to more nicely display the license
;
Function change_license_font
    FindWindow $0 "#32770" "" $HWNDPARENT
    CreateFont $1 "Lucida Console" "7"
    GetDlgItem $0 $0 1000
    SendMessage $0 ${WM_SETFONT} $1 1
FunctionEnd

; These are the same languages supported by HashCheck
;
!insertmacro MUI_LANGUAGE "English" ; the first language is the default
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Catalan"

VIProductVersion "2.4.0.38-beta"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "HashCheck Shell Extension"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "2.4.0.38-beta"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Installer distributed from https://github.com/gurnec/HashCheck/releases"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright © Kai Liu, Christopher Gurnee, Tim Schlueter, et al. All rights reserved."
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Installer (x86/x64) from https://github.com/gurnec/HashCheck/releases"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "2.4.0.38-beta"

; With solid compression, files that are required before the
; actual installation should be stored first in the data block,
; because this will make the installer start faster.
;
!insertmacro MUI_RESERVEFILE_LANGDLL

; The install script
;
Section

    GetTempFileName $0

    ${If} ${RunningX64}
        ${DisableX64FSRedirection}

        ; Install the 64-bit dll
        File /oname=$0 ..\Bin\x64\Release\HashCheck.dll
        ExecWait 'regsvr32 /i /n /s "$0"'
        IfErrors abort_on_error
        Delete $0

        ; One of these 64-bit dlls exists and is undeletable if and
        ; only if it was in use and therefore a reboot is now required
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.0
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.1
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.2
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.3
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.4
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.5
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.6
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.7
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.8
        Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.9

        ${EnableX64FSRedirection}

        ; Install the 32-bit dll (the 64-bit dll handles uninstallation for both)
        File /oname=$0 ..\Bin\Win32\Release\HashCheck.dll
        ExecWait 'regsvr32 /i:"NoUninstall" /n /s "$0"'
        IfErrors abort_on_error
    ${Else}
        ; Install the 32-bit dll
        File /oname=$0 ..\Bin\Win32\Release\HashCheck.dll
        ExecWait 'regsvr32 /i /n /s "$0"'
        IfErrors abort_on_error	
    ${EndIf}

    Delete $0

    ; One of these 32-bit dlls exists and is undeletable if and
    ; only if it was in use and therefore a reboot is now required
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.0
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.1
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.2
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.3
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.4
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.5
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.6
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.7
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.8
    Delete /REBOOTOK $SYSDIR\ShellExt\HashCheck.dll.9

    Return

    abort_on_error:
        Delete $0
        IfSilent +2
        MessageBox MB_ICONSTOP|MB_OK "An unexpected error occurred during installation"
        Quit

SectionEnd

Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd
