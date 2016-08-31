/**
 * HashCheck Shell Extension
 * Original work copyright (C) Kai Liu.  All rights reserved.
 * Modified work copyright (C) 2014, 2016 Christopher Gurnee.  All rights reserved.
 * Modified work copyright (C) 2016 Tim Schlueter.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "globals.h"
#include "CHashCheck.hpp"
#include "CHashCheckClassFactory.hpp"
#include "RegHelpers.h"
#include "libs/WinHash.h"
#include "libs/Wow64.h"
#include <Strsafe.h>

 // Table of formerly supported Hash file extensions to be removed during install
LPCTSTR szFormerHashExtsTab[] = {
    _T(".md4")
};

// Bookkeeping globals (declared as extern in globals.h)
HMODULE g_hModThisDll;
CREF g_cRefThisDll;

// Activation context cache (declared as extern in globals.h)
volatile BOOL g_bActCtxCreated;
HANDLE g_hActCtx;

// Major and minor Windows version (declared as extern in globals.h)
UINT16 g_uWinVer;

// Prototypes for the self-registration/install/uninstall helper functions
STDAPI DllRegisterServerEx( LPCTSTR );
HRESULT Install( BOOL, BOOL );
HRESULT Uninstall( );
BOOL WINAPI InstallFile( LPCTSTR, LPTSTR, LPTSTR );


#if defined(_USRDLL) && defined(_DLL)
#pragma comment(linker, "/entry:DllMain")
#endif
extern "C" BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			g_hModThisDll = hInstance;
			g_cRefThisDll = 0;
			g_bActCtxCreated = FALSE;
			g_hActCtx = INVALID_HANDLE_VALUE;
			g_uWinVer = SwapV16(LOWORD(GetVersion()));
			#ifndef _WIN64
			if (g_uWinVer < 0x0501) return(FALSE);
			#endif
			#ifdef _DLL
			DisableThreadLibraryCalls(hInstance);
			#endif
			break;

		case DLL_PROCESS_DETACH:
			if (g_bActCtxCreated && g_hActCtx != INVALID_HANDLE_VALUE)
				ReleaseActCtx(g_hActCtx);
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;
	}

	return(TRUE);
}

STDAPI DllCanUnloadNow( )
{
	return((g_cRefThisDll == 0) ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID *ppv )
{
	*ppv = NULL;

	if (IsEqualIID(rclsid, CLSID_HashCheck))
	{
		LPCHASHCHECKCLASSFACTORY lpHashCheckClassFactory = new CHashCheckClassFactory;
		if (lpHashCheckClassFactory == NULL) return(E_OUTOFMEMORY);

		HRESULT hr = lpHashCheckClassFactory->QueryInterface(riid, ppv);
		lpHashCheckClassFactory->Release();
		return(hr);
	}

	return(CLASS_E_CLASSNOTAVAILABLE);
}

STDAPI DllRegisterServerEx( LPCTSTR lpszModuleName )
{
	HKEY hKey;
	TCHAR szBuffer[MAX_PATH << 1];

	// Register class
	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("CLSID\\%s"), CLSID_STR_HashCheck, TRUE))
	{
		RegSetSZ(hKey, NULL, CLSNAME_STR_HashCheck);
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("CLSID\\%s\\InprocServer32"), CLSID_STR_HashCheck, TRUE))
	{
		RegSetSZ(hKey, NULL, lpszModuleName);
		RegSetSZ(hKey, TEXT("ThreadingModel"), TEXT("Apartment"));
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	// Register context menu handler
	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("AllFileSystemObjects\\ShellEx\\ContextMenuHandlers\\%s"), CLSNAME_STR_HashCheck, TRUE))
	{
		RegSetSZ(hKey, NULL, CLSID_STR_HashCheck);
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	// Register property sheet handler
	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("AllFileSystemObjects\\ShellEx\\PropertySheetHandlers\\%s"), CLSNAME_STR_HashCheck, TRUE))
	{
		RegSetSZ(hKey, NULL, CLSID_STR_HashCheck);
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	// Register the HashCheck program ID
	if (hKey = RegOpen(HKEY_CLASSES_ROOT, PROGID_STR_HashCheck, NULL, TRUE))
	{
		LoadString(g_hModThisDll, IDS_FILETYPE_DESC, szBuffer, countof(szBuffer));
		RegSetSZ(hKey, NULL, szBuffer);

		StringCchPrintf(szBuffer, countof(szBuffer), TEXT("@\"%s\",-%u"), lpszModuleName, IDS_FILETYPE_DESC);
		RegSetSZ(hKey, TEXT("FriendlyTypeName"), szBuffer);

		RegSetSZ(hKey, TEXT("AlwaysShowExt"), TEXT(""));
		RegSetSZ(hKey, TEXT("AppUserModelID"), APP_USER_MODEL_ID);

		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("%s\\DefaultIcon"), PROGID_STR_HashCheck, TRUE))
	{
		StringCchPrintf(szBuffer, countof(szBuffer), TEXT("%s,-%u"), lpszModuleName, IDI_FILETYPE);
		RegSetSZ(hKey, NULL, szBuffer);
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("%s\\shell\\open\\DropTarget"), PROGID_STR_HashCheck, TRUE))
	{
		// The DropTarget will be the primary way in which we are invoked
		RegSetSZ(hKey, TEXT("CLSID"), CLSID_STR_HashCheck);
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("%s\\shell\\open\\command"), PROGID_STR_HashCheck, TRUE))
	{
		// This is a legacy fallback used only when DropTarget is unsupported
		StringCchPrintf(szBuffer, countof(szBuffer), TEXT("rundll32.exe \"%s\",HashVerify_RunDLL %%1"), lpszModuleName, IDS_FILETYPE_DESC);
		RegSetSZ(hKey, NULL, szBuffer);
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("%s\\shell\\edit\\command"), PROGID_STR_HashCheck, TRUE))
	{
		RegSetSZ(hKey, NULL, TEXT("notepad.exe %1"));
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	if (hKey = RegOpen(HKEY_CLASSES_ROOT, TEXT("%s\\shell"), PROGID_STR_HashCheck, TRUE))
	{
		RegSetSZ(hKey, NULL, TEXT("open"));
		RegCloseKey(hKey);
	} else return(SELFREG_E_CLASS);

	// The actual association of .sfv/.md5/.sha1/.sha256/.sha512/.sha3-256/.sha3-512/.asc files with our program ID
	// will be handled by DllInstall, not DllRegisterServer.

	// Register approval
	if (hKey = RegOpen(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), NULL, TRUE))
	{
		RegSetSZ(hKey, CLSID_STR_HashCheck, CLSNAME_STR_HashCheck);
		RegCloseKey(hKey);
	}

	return(S_OK);
}

STDAPI DllRegisterServer( )
{
	TCHAR szCurrentDllPath[MAX_PATH << 1];
	GetModuleFileName(g_hModThisDll, szCurrentDllPath, countof(szCurrentDllPath));
	return(DllRegisterServerEx(szCurrentDllPath));
}

STDAPI DllUnregisterServer( )
{
	HKEY hKey;
	BOOL bClassRemoved = TRUE;
	BOOL bApprovalRemoved = FALSE;

	// Unregister class
	if (!RegDelete(HKEY_CLASSES_ROOT, TEXT("CLSID\\%s"), CLSID_STR_HashCheck))
		bClassRemoved = FALSE;

	// Unregister handlers
	if (!Wow64CheckProcess())
	{
		/**
		 * Registry reflection sucks; it means that if we try to unregister the
		 * Wow64 extension, we'll also unregister the Win64 extension; the API
		 * to disable reflection seems to only affect changes in value, not key
		 * removals. :( This hack will disable the deletion of certain HKCR
		 * keys in the case of 32-on-64, and it should be pretty safe--unless
		 * the user had installed only the 32-bit extension without the 64-bit
		 * extension on Win64 (which should be a very rare scenario), there
		 * should be no undesirable effects to using this hack.
		 **/

		if (!RegDelete(HKEY_CLASSES_ROOT, TEXT("AllFileSystemObjects\\ShellEx\\ContextMenuHandlers\\%s"), CLSNAME_STR_HashCheck))
			bClassRemoved = FALSE;

		if (!RegDelete(HKEY_CLASSES_ROOT, TEXT("AllFileSystemObjects\\ShellEx\\PropertySheetHandlers\\%s"), CLSNAME_STR_HashCheck))
			bClassRemoved = FALSE;

		if (!RegDelete(HKEY_CLASSES_ROOT, PROGID_STR_HashCheck, NULL))
			bClassRemoved = FALSE;
	}

	// Unregister approval
	if (hKey = RegOpen(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), NULL, FALSE))
	{
		LONG lResult = RegDeleteValue(hKey, CLSID_STR_HashCheck);
		bApprovalRemoved = (lResult == ERROR_SUCCESS || lResult == ERROR_FILE_NOT_FOUND);
		RegCloseKey(hKey);
	}

	if (!bClassRemoved) return(SELFREG_E_CLASS);
	if (!bApprovalRemoved) return(S_FALSE);

	return(S_OK);
}

STDAPI DllInstall( BOOL bInstall, LPCWSTR pszCmdLine )
{
	// To install into System32\ShellExt
	// regsvr32.exe /i /n HashCheck.dll
	//
	// To install without registering an uninstaller
	// regsvr32.exe /i:"NoUninstall" /n HashCheck.dll
	//
	// To install in-place (without copying the .dll anywhere)
	// regsvr32.exe /i:"NoCopy" /n HashCheck.dll
	//
	// To install with both options above
	// regsvr32.exe /i:"NoUninstall NoCopy" /n HashCheck.dll
	//
	// To uninstall
	// regsvr32.exe /u /i /n HashCheck.dll
	//
	// To install/uninstall silently
	// regsvr32.exe /s ...
	//
	// DllInstall can also be invoked from a RegisterDlls INF section or from
	// a UnregisterDlls INF section, if the registration flags are set to 2.
	// Consult the documentation for RegisterDlls/UnregisterDlls for details.

	return( (bInstall) ?
		Install(pszCmdLine == NULL || StrStrIW(pszCmdLine, L"NoUninstall") == NULL,
                pszCmdLine == NULL || StrStrIW(pszCmdLine, L"NoCopy")      == NULL) :
		Uninstall()
	);
}

HRESULT Install( BOOL bRegisterUninstaller, BOOL bCopyFile )
{
	TCHAR szCurrentDllPath[MAX_PATH << 1];
	GetModuleFileName(g_hModThisDll, szCurrentDllPath, countof(szCurrentDllPath));

	TCHAR szSysDir[MAX_PATH + 0x20];
	UINT uSize = GetSystemDirectory(szSysDir, MAX_PATH);

	if (uSize && uSize < MAX_PATH)
	{
		LPTSTR lpszPath = szSysDir;
		LPTSTR lpszPathAppend = lpszPath + uSize;

		if (*(lpszPathAppend - 1) != TEXT('\\'))
			*lpszPathAppend++ = TEXT('\\');

		LPTSTR lpszTargetPath = (bCopyFile) ? lpszPath : szCurrentDllPath;

		if ( (!bCopyFile || InstallFile(szCurrentDllPath, lpszTargetPath, lpszPathAppend)) &&
		     DllRegisterServerEx(lpszTargetPath) == S_OK )
		{
			HKEY hKey, hKeySub;

			// Associate file extensions
			for (UINT i = 0; i < countof(g_szHashExtsTab); ++i)
			{
				if (hKey = RegOpen(HKEY_CLASSES_ROOT, g_szHashExtsTab[i], NULL, TRUE))
				{
					RegSetSZ(hKey, NULL, PROGID_STR_HashCheck);
					RegSetSZ(hKey, TEXT("PerceivedType"), TEXT("text"));

					if (hKeySub = RegOpen(hKey, TEXT("PersistentHandler"), NULL, TRUE))
					{
						RegSetSZ(hKeySub, NULL, TEXT("{5e941d80-bf96-11cd-b579-08002b30bfeb}"));
						RegCloseKey(hKeySub);
					}

					RegCloseKey(hKey);
				}
			}

            // Disassociate former file extensions; see the comment in DllUnregisterServer for
            // why this step is skipped for Wow64 processes
            if (!Wow64CheckProcess())
            {
                for (UINT i = 0; i < countof(szFormerHashExtsTab); ++i)
                {
                    HKEY hKey;

                    if (hKey = RegOpen(HKEY_CLASSES_ROOT, szFormerHashExtsTab[i], NULL, FALSE))
                    {
                        TCHAR szTemp[countof(PROGID_STR_HashCheck)];
                        RegGetSZ(hKey, NULL, szTemp, sizeof(szTemp));
                        if (_tcscmp(szTemp, PROGID_STR_HashCheck) == 0)
                            RegDeleteValue(hKey, NULL);
                        RegCloseKey(hKey);
                    }
                }
            }

			// Uninstaller entries
			RegDelete(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s"), CLSNAME_STR_HashCheck);

			if (bRegisterUninstaller && (hKey = RegOpen(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s"), CLSNAME_STR_HashCheck, TRUE)))
			{
				TCHAR szUninstall[MAX_PATH << 1];
				StringCchPrintf(szUninstall, countof(szUninstall), TEXT("regsvr32.exe /u /i /n \"%s\""), lpszTargetPath);

				static const TCHAR szURLFull[] = TEXT("https://github.com/gurnec/HashCheck/issues");
				TCHAR szURLBase[countof(szURLFull)];
				SSStaticCpy(szURLBase, szURLFull);
				szURLBase[35] = 0; // strlen("https://github.com/gurnec/HashCheck")

				RegSetSZ(hKey, TEXT("DisplayIcon"), lpszTargetPath);
				RegSetSZ(hKey, TEXT("DisplayName"), TEXT(HASHCHECK_NAME_STR));
				RegSetSZ(hKey, TEXT("DisplayVersion"), TEXT(HASHCHECK_VERSION_STR));
				RegSetDW(hKey, TEXT("EstimatedSize"), 896);
				RegSetSZ(hKey, TEXT("HelpLink"), szURLFull);
				RegSetDW(hKey, TEXT("NoModify"), 1);
				RegSetDW(hKey, TEXT("NoRepair"), 1);
				RegSetSZ(hKey, TEXT("UninstallString"), szUninstall);
				RegSetSZ(hKey, TEXT("URLInfoAbout"), szURLBase);
				RegSetSZ(hKey, TEXT("URLUpdateInfo"), TEXT("https://github.com/gurnec/HashCheck/releases/latest"));
				RegCloseKey(hKey);
			}

			return(S_OK);

		} // if copied & registered

	} // if valid sysdir

	return(E_FAIL);
}

HRESULT Uninstall( )
{
	HRESULT hr = S_OK;

	TCHAR szCurrentDllPath[MAX_PATH << 1];
	TCHAR szTemp[MAX_PATH << 1];

	LPTSTR lpszFileToDelete = szCurrentDllPath;
	LPTSTR lpszTempAppend = szTemp + GetModuleFileName(g_hModThisDll, szTemp, countof(szTemp));

    StringCbCopy(szCurrentDllPath, sizeof(szCurrentDllPath), szTemp);

#ifdef _WIN64
    // If this 64-bit dll was installed to the default location,
    // uninstall the 32-bit dll if it exists in its default location

    TCHAR lpszDefInstallPath[MAX_PATH + 0x20];
    UINT uSize = GetSystemDirectory(lpszDefInstallPath, MAX_PATH);

    if (uSize && uSize < MAX_PATH)
    {
        LPTSTR lpszPathAppend = lpszDefInstallPath + uSize;

        if (*(lpszPathAppend - 1) != TEXT('\\'))
            *lpszPathAppend++ = TEXT('\\');

        static const TCHAR szFolderAndFilename[] = TEXT("ShellExt") TEXT("\\") TEXT(HASHCHECK_FILENAME_STR);
        SSStaticCpy(lpszPathAppend, szFolderAndFilename);

        // If this 64-bit dll was installed to the default location
        if (StrCmpI(szCurrentDllPath, lpszDefInstallPath) == 0)
        {
            TCHAR lpszSystemWow64[MAX_PATH + 0x20];
            uSize = GetSystemWow64Directory(lpszSystemWow64, MAX_PATH);

            if (uSize && uSize < MAX_PATH)
            {
                LPTSTR lpszSystemWow64Append = lpszSystemWow64 + uSize;

                if (*(lpszSystemWow64Append - 1) != TEXT('\\'))
                    SSCpy2Ch(lpszSystemWow64Append++, TEXT('\\'), 0);

                StringCbCopyEx(lpszDefInstallPath, sizeof(lpszDefInstallPath), lpszSystemWow64, &lpszPathAppend, NULL, 0);

                SSStaticCpy(lpszPathAppend, szFolderAndFilename);

                // If the 32-bit dll exists in its default location
                if (PathFileExists(lpszDefInstallPath))
                {
                    static const TCHAR szRegsvr32[] = TEXT("regsvr32.exe");
                    SSStaticCpy(lpszSystemWow64Append, szRegsvr32);
                    // the lpszSystemWow64 buffer now contains the full regsvr32.exe path

                    TCHAR lpszCommandLine[MAX_PATH + 0x20];
                    LPTSTR lpszCommandLineAppend;
                    
                    static const TCHAR szCommandOpts[] = TEXT("regsvr32.exe /u /i /n /s ");
                    lpszCommandLineAppend = SSStaticCpy(lpszCommandLine, szCommandOpts) - 1;

                    StringCbCopy(lpszCommandLineAppend, sizeof(lpszCommandLine)-sizeof(szCommandOpts), lpszDefInstallPath);

                    STARTUPINFO si;
                    memset(&si, 0, sizeof(si));
                    si.cb = sizeof(si);

                    PROCESS_INFORMATION pi;
                    memset(&pi, 0, sizeof(pi));

                    if (!CreateProcess(lpszSystemWow64, lpszCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                        return E_FAIL;

                    DWORD dwExit;
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    GetExitCodeProcess(pi.hProcess, &dwExit);
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);

                    if (dwExit != 0)
                        return E_FAIL;
                }
            }
        }
    }
#endif

	// Rename the DLL prior to scheduling it for deletion
	*lpszTempAppend++ = TEXT('.');
	SSCpy2Ch(lpszTempAppend, 0, 0);

	for (TCHAR ch = TEXT('0'); ch <= TEXT('9'); ++ch)
	{
		*lpszTempAppend = ch;

		if (MoveFileEx(szCurrentDllPath, szTemp, MOVEFILE_REPLACE_EXISTING))
		{
			lpszFileToDelete = szTemp;
			break;
		}
	}

	// Schedule the DLL to be deleted at shutdown/reboot
	if (!MoveFileEx(lpszFileToDelete, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) hr = E_FAIL;

	// Unregister
	if (DllUnregisterServer() != S_OK) hr = E_FAIL;

	// Disassociate file extensions; see the comment in DllUnregisterServer for
	// why this step is skipped for Wow64 processes
	if (!Wow64CheckProcess())
	{
		for (UINT i = 0; i < countof(g_szHashExtsTab); ++i)
		{
			HKEY hKey;

			if (hKey = RegOpen(HKEY_CLASSES_ROOT, g_szHashExtsTab[i], NULL, FALSE))
			{
                RegGetSZ(hKey, NULL, szTemp, sizeof(szTemp));
                if (_tcscmp(szTemp, PROGID_STR_HashCheck) == 0)
                    RegDeleteValue(hKey, NULL);
                RegCloseKey(hKey);
			}
		}
	}

	// We don't need the uninstall strings any more...
	RegDelete(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s"), CLSNAME_STR_HashCheck);

	return(hr);
}

BOOL WINAPI InstallFile( LPCTSTR lpszSource, LPTSTR lpszDest, LPTSTR lpszDestAppend )
{
	static const TCHAR szShellExt[] = TEXT("ShellExt");
	static const TCHAR szDestFile[] = TEXT("\\") TEXT(HASHCHECK_FILENAME_STR);

	SSStaticCpy(lpszDestAppend, szShellExt);
	lpszDestAppend += countof(szShellExt) - 1;

	// Create directory if necessary
	if (! PathFileExists(lpszDest))
		CreateDirectory(lpszDest, NULL);

	SSStaticCpy(lpszDestAppend, szDestFile);
	lpszDestAppend += countof(szDestFile) - 1;

	// No need to copy if the source and destination are the same
	if (StrCmpI(lpszSource, lpszDest) == 0)
		return(TRUE);

	// If the destination file does not already exist, just copy
	if (! PathFileExists(lpszDest))
		return(CopyFile(lpszSource, lpszDest, FALSE));

	// If destination file exists and cannot be overwritten
	TCHAR szTemp[MAX_PATH + 0x20];
	SIZE_T cbDest = BYTEDIFF(lpszDestAppend, lpszDest);
	LPTSTR lpszTempAppend = (LPTSTR)BYTEADD(szTemp, cbDest);

	StringCbCopy(szTemp, sizeof(szTemp), lpszDest);
	*lpszTempAppend++ = TEXT('.');
	SSCpy2Ch(lpszTempAppend, 0, 0);

	for (TCHAR ch = TEXT('0'); ch <= TEXT('9'); ++ch)
	{
		if (CopyFile(lpszSource, lpszDest, FALSE))
			return(TRUE);

		*lpszTempAppend = ch;

		if (MoveFileEx(lpszDest, szTemp, MOVEFILE_REPLACE_EXISTING))
			MoveFileEx(szTemp, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}

	return(FALSE);
}
