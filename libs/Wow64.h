/**
 * Wow64 Wrapper
 * Last modified: 2008/12/13
 **/

#ifndef __WOW64_H__
#define __WOW64_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#ifndef _WIN64

#define WOW64API __fastcall

UINT WOW64API Wow64GetSystemDirectory( LPTSTR lpBuffer, UINT uSize );
VOID WOW64API Wow64DisableFsRedir( );
VOID WOW64API Wow64DisableFsRedirEx( PVOID *OldValue );
VOID WOW64API Wow64RevertFsRedir( PVOID OldValue );
BOOL WOW64API Wow64CheckProcess( );
VOID WOW64API Wow64DisableRegReflection( HKEY hKey );

#else

#define Wow64GetSystemDirectory   GetSystemWow64Directory
#define Wow64DisableFsRedir()
#define Wow64DisableFsRedirEx(a)
#define Wow64RevertFsRedir(a)
#define Wow64CheckProcess()       FALSE
#define Wow64DisableRegReflection RegDisableReflectionKey

#endif

#ifdef __cplusplus
}
#endif

#endif
