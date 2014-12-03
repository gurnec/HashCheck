/**
 * Registry Helper Functions
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __REGHELPERS_H__
#define __REGHELPERS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <shlwapi.h>

HKEY WINAPI RegOpen( HKEY, LPCTSTR, LPCTSTR );
BOOL WINAPI RegDelete( HKEY, LPCTSTR, LPCTSTR );
BOOL WINAPI RegSetSZ( HKEY, LPCTSTR, LPCTSTR );
BOOL WINAPI RegSetDW( HKEY, LPCTSTR, DWORD );
BOOL WINAPI RegGetSZ( HKEY, LPCTSTR, LPTSTR, DWORD );
BOOL WINAPI RegGetDW( HKEY, LPCTSTR, LPDWORD );

#ifdef __cplusplus
}
#endif

#endif
