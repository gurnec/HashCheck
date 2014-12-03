/**
 * Registry Helper Functions
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "RegHelpers.h"
#include "libs\Wow64.h"
#include "libs\SimpleString.h"

#define countof(x) (sizeof(x)/sizeof(x[0]))

HKEY WINAPI RegOpen( HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpSubst )
{
	HKEY hKeyRet = NULL;
	TCHAR szJoinedKey[0x7F];
	if (lpSubst) wnsprintf(szJoinedKey, countof(szJoinedKey), lpSubKey, lpSubst);

	if (RegCreateKeyEx(hKey, (lpSubst) ? szJoinedKey : lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKeyRet, NULL) == ERROR_SUCCESS)
	{
		Wow64DisableRegReflection(hKeyRet);
		return(hKeyRet);
	}

	return(NULL);
}

BOOL WINAPI RegDelete( HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpSubst )
{
	LONG lResult;
	TCHAR szJoinedKey[0x7F];
	if (lpSubst) wnsprintf(szJoinedKey, countof(szJoinedKey), lpSubKey, lpSubst);

	lResult = SHDeleteKey(hKey, (lpSubst) ? szJoinedKey : lpSubKey);
	return(lResult == ERROR_SUCCESS || lResult == ERROR_FILE_NOT_FOUND);
}

BOOL WINAPI RegSetSZ( HKEY hKey, LPCTSTR lpValueName, LPCTSTR lpData )
{
	return(RegSetValueEx(hKey, lpValueName, 0, REG_SZ, (LPBYTE)lpData, ((DWORD)SSLen(lpData) + 1) * sizeof(lpData[0])) == ERROR_SUCCESS);
}

BOOL WINAPI RegSetDW( HKEY hKey, LPCTSTR lpValueName, DWORD dwData )
{
	return(RegSetValueEx(hKey, lpValueName, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData)) == ERROR_SUCCESS);
}

BOOL WINAPI RegGetSZ( HKEY hKey, LPCTSTR lpValueName, LPTSTR lpData, DWORD cbData )
{
	DWORD dwType;

	if ( RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE)lpData, &cbData) == ERROR_SUCCESS &&
	     dwType == REG_SZ && cbData >= sizeof(TCHAR) )
	{
		// The registry does not check for proper null termination, so in order
		// to prevent crashes in the case of a malformed registry entry, make
		// sure that, at the very least, the final character is a null
		*((LPTSTR)((LPBYTE)lpData + cbData - sizeof(TCHAR))) = 0;
		return(TRUE);
	}
	else
	{
		lpData[0] = 0;
		return(FALSE);
	}
}

BOOL WINAPI RegGetDW( HKEY hKey, LPCTSTR lpValueName, LPDWORD lpdwData )
{
	DWORD cbData = sizeof(DWORD);
	DWORD dwType;

	if ( RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE)lpdwData, &cbData) == ERROR_SUCCESS &&
	     dwType == REG_DWORD && cbData == sizeof(DWORD) )
	{
		return(TRUE);
	}
	else
	{
		*lpdwData = 0;
		return(FALSE);
	}
}
