/**
 * Sets the Application User Model ID for windows or processes on NT 6.1+
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "globals.h"
#include "libs\SimpleString.h"
#include <propidl.h>


/*******************************************************************************
 *
 *
 * In order to build without requiring the installation of the NT 6.1 PSDK, it
 * is necessary to manually define a number of things...
 *
 *
 ******************************************************************************/


#ifndef PROPERTYKEY_DEFINED
#define PROPERTYKEY_DEFINED
typedef struct {
	GUID fmtid;
	DWORD pid;
} PROPERTYKEY;

typedef const PROPERTYKEY *REFPROPERTYKEY;
#endif

#ifndef __propsys_h__
typedef interface IPropertyStore IPropertyStore;

interface IPropertyStore
{
	CONST_VTBL struct IPropertyStoreVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IPropertyStore *This, REFIID riid, void **ppvObject );
		ULONG ( STDMETHODCALLTYPE *AddRef )( IPropertyStore *This );
		ULONG ( STDMETHODCALLTYPE *Release )( IPropertyStore *This );
		HRESULT ( STDMETHODCALLTYPE *GetCount )( IPropertyStore *This, DWORD *cProps );
		HRESULT ( STDMETHODCALLTYPE *GetAt )( IPropertyStore *This, DWORD iProp, PROPERTYKEY *pkey );
		HRESULT ( STDMETHODCALLTYPE *GetValue )( IPropertyStore *This, REFPROPERTYKEY key, PROPVARIANT *pv );
		HRESULT ( STDMETHODCALLTYPE *SetValue )( IPropertyStore *This, REFPROPERTYKEY key, PROPVARIANT *pv );
		HRESULT ( STDMETHODCALLTYPE *Commit )( IPropertyStore *This );

		END_INTERFACE

	} *lpVtbl;
};
#endif

static const IID IID_IPropertyStore = { 0x886d8eeb, 0x8cf2, 0x4446, { 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99 } };
static const PROPERTYKEY PKEY_AppUserModel_PreventPinning = { { 0x9f4c2855, 0x9f79, 0x4b39, { 0xa8, 0xd0, 0xe1, 0xd4, 0x2d, 0xe1, 0xd5, 0xf3 } }, 9 };
static const PROPERTYKEY PKEY_AppUserModel_ID = { { 0x9f4c2855, 0x9f79, 0x4b39, { 0xa8, 0xd0, 0xe1, 0xd4, 0x2d, 0xe1, 0xd5, 0xf3 } }, 5 };

typedef HRESULT (WINAPI *PFNSCPEAUMID)( PCWSTR AppID );
typedef HRESULT (WINAPI *PFNSHGPSFW)( HWND hwnd, REFIID riid, void** ppv );


/*******************************************************************************
 *
 *
 * SetAppIDForWindow
 *
 *
 ******************************************************************************/


VOID WINAPI SetAppIDForWindow( HWND hWnd, BOOL fEnable )
{
	if (g_uWinVer >= 0x601)
	{
		IPropertyStore *pps;

		PFNSHGPSFW pfnSHGetPropertyStoreForWindow = (PFNSHGPSFW)
			GetProcAddress(GetModuleHandleA("SHELL32.dll"), "SHGetPropertyStoreForWindow");

		if ( pfnSHGetPropertyStoreForWindow &&
		     SUCCEEDED(pfnSHGetPropertyStoreForWindow(hWnd, &IID_IPropertyStore, &pps)) )
		{
			PROPVARIANT propvars[2];

			// PropVariantInit is just a macro for a zeroing memset
			ZeroMemory(propvars, sizeof(propvars));

			if (fEnable)
			{
				// InitPropVariantFrom* functions were introduced in NT6, and
				// should be avoided because of the extra SDK dependency and
				// because of the extra COM memory allocation (and cleanup),
				// which is unnecessary since we are running in-process.

				propvars[0].vt = VT_BOOL;
				propvars[0].boolVal = TRUE;

				propvars[1].vt = VT_LPWSTR;
				propvars[1].pwszVal = APP_USER_MODEL_ID;
			}

			pps->lpVtbl->SetValue(pps, &PKEY_AppUserModel_PreventPinning, &propvars[0]);
			pps->lpVtbl->SetValue(pps, &PKEY_AppUserModel_ID, &propvars[1]);

			pps->lpVtbl->Release(pps);
		}
	}
}


/*******************************************************************************
 *
 *
 * SetAppIDForProcess
 *
 *
 ******************************************************************************/


VOID WINAPI SetAppIDForProcess( )
{
	if (g_uWinVer >= 0x601)
	{
		PFNSCPEAUMID pfnSetCurrentProcessExplicitAppUserModelID = (PFNSCPEAUMID)
			GetProcAddress(GetModuleHandleA("SHELL32.dll"), "SetCurrentProcessExplicitAppUserModelID");

		if (pfnSetCurrentProcessExplicitAppUserModelID)
			pfnSetCurrentProcessExplicitAppUserModelID(APP_USER_MODEL_ID);
	}
}
