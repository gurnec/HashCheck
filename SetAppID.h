/**
 * Sets the Application User Model ID for windows or processes on NT 6.1+
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __SETAPPID_H__
#define __SETAPPID_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

VOID WINAPI SetAppIDForWindow( HWND hWnd, BOOL fEnable );
VOID WINAPI SetAppIDForProcess( );

#ifdef __cplusplus
}
#endif

#endif
