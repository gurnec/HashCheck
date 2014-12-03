/**
 * Wow64 Wrapper
 * Last modified: 2008/12/13
 **/

#include "Wow64.h"

#ifndef _WIN64

#define GetProc(m, n) ((PVOID)GetProcAddress(GetModuleHandleA(m), n))

UINT WOW64API Wow64GetSystemDirectory( LPTSTR lpBuffer, UINT uSize )
{
	typedef UINT (WINAPI *PFNGETSYSTEMWOW64DIRECTORY)( LPTSTR lpBuffer, UINT uSize );

	PFNGETSYSTEMWOW64DIRECTORY pfnGetSystemWow64Directory =
		GetProc("KERNEL32.dll", "GetSystemWow64Directory");

	if (pfnGetSystemWow64Directory)
		return(pfnGetSystemWow64Directory(lpBuffer, uSize));

	return(0);
}

VOID WOW64API Wow64DisableFsRedir( )
{
	typedef BOOL (WINAPI *PFNWOW64DISABLEWOW64FSREDIRECTION)( PVOID *OldValue );

	PFNWOW64DISABLEWOW64FSREDIRECTION pfnWow64DisableWow64FsRedirection =
		GetProc("KERNEL32.dll", "Wow64DisableWow64FsRedirection");

	if (pfnWow64DisableWow64FsRedirection)
	{
		PVOID pvOldValue;
		pfnWow64DisableWow64FsRedirection(&pvOldValue);
	}
}

VOID WOW64API Wow64DisableFsRedirEx( PVOID *OldValue )
{
	typedef BOOL (WINAPI *PFNWOW64DISABLEWOW64FSREDIRECTION)( PVOID *OldValue );

	PFNWOW64DISABLEWOW64FSREDIRECTION pfnWow64DisableWow64FsRedirection =
		GetProc("KERNEL32.dll", "Wow64DisableWow64FsRedirection");

	if (pfnWow64DisableWow64FsRedirection)
		pfnWow64DisableWow64FsRedirection(OldValue);
}

VOID WOW64API Wow64RevertFsRedir( PVOID OldValue )
{
	typedef BOOL (WINAPI *PFNWOW64REVERTWOW64FSREDIRECTION)( PVOID OldValue );

	PFNWOW64REVERTWOW64FSREDIRECTION pfnWow64RevertWow64FsRedirection =
		GetProc("KERNEL32.dll", "Wow64RevertWow64FsRedirection");

	if (pfnWow64RevertWow64FsRedirection)
		pfnWow64RevertWow64FsRedirection(OldValue);
}

BOOL WOW64API Wow64CheckProcess( )
{
	typedef BOOL (WINAPI *PFNISWOW64PROCESS)( HANDLE hProcess, PBOOL Wow64Process );

	PFNISWOW64PROCESS pfnIsWow64Process =
		GetProc("KERNEL32.dll", "IsWow64Process");

	BOOL bIsWow64;

	if (pfnIsWow64Process && pfnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		return(bIsWow64);

	return(FALSE);
}

VOID WOW64API Wow64DisableRegReflection( HKEY hKey )
{
	typedef LONG (WINAPI *PFNREGDISABLEREFLECTIONKEY)( HKEY hBase );

	PFNREGDISABLEREFLECTIONKEY pfnRegDisableReflectionKey =
		GetProc("ADVAPI32.dll", "RegDisableReflectionKey");

	if (pfnRegDisableReflectionKey && hKey)
		pfnRegDisableReflectionKey(hKey);
}

#endif
