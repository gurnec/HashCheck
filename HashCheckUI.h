/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __HASHCHECKUI_H__
#define __HASHCHECKUI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

HANDLE __fastcall CreateThreadCRT( PVOID pThreadProc, PVOID pvParam );

// HashSave
VOID WINAPI HashSaveStart( HWND hWndOwner, HSIMPLELIST hListInput );

// HashProp
UINT CALLBACK HashPropCallback( HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
INT_PTR CALLBACK HashPropDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

// HashVerify
DWORD WINAPI HashVerifyThread( PTSTR pszPath );

// Activation context functions
ULONG_PTR WINAPI ActivateManifest( BOOL bActivate );

__forceinline VOID WINAPI DeactivateManifest( ULONG_PTR uCookie )
{
	if (uCookie)
		DeactivateActCtx(0, uCookie);
}

#ifdef __cplusplus
}
#endif

#endif
