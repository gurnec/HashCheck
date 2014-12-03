/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "CHashCheck.hpp"
#include "HashCheckUI.h"
#include "HashCheckOptions.h"

STDMETHODIMP CHashCheck::QueryInterface( REFIID riid, LPVOID *ppv )
{
	if (IsEqualIID(riid, IID_IUnknown))
	{
		*ppv = this;
	}
	else if (IsEqualIID(riid, IID_IShellExtInit))
	{
		*ppv = (LPSHELLEXTINIT)this;
	}
	else if (IsEqualIID(riid, IID_IContextMenu))
	{
		*ppv = (LPCONTEXTMENU)this;
	}
	else if (IsEqualIID(riid, IID_IShellPropSheetExt))
	{
		*ppv = (LPSHELLPROPSHEETEXT)this;
	}
	else if (IsEqualIID(riid, IID_IDropTarget))
	{
		*ppv = (LPDROPTARGET)this;
	}
	else
	{
		*ppv = NULL;
		return(E_NOINTERFACE);
	}

	AddRef();
	return(S_OK);
}

STDMETHODIMP CHashCheck::Initialize( LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdtobj, HKEY hkeyProgID )
{
	// We'll be needing a buffer, and let's double it just to be safe
	TCHAR szPath[MAX_PATH << 1];

	// Make sure that we are working with a fresh list
	SLRelease(m_hList);
	m_hList = SLCreate();

	// This indent exists to facilitate diffing against the CmdOpen source
	{
		FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM medium;

		if (!pdtobj || pdtobj->GetData(&format, &medium) != S_OK)
			return(E_INVALIDARG);

		if (HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal))
		{
			UINT uDrops = DragQueryFile(hDrop, -1, NULL, 0);

			for (UINT uDrop = 0; uDrop < uDrops; ++uDrop)
			{
				if (DragQueryFile(hDrop, uDrop, szPath, countof(szPath)))
				{
					SLAddStringI(m_hList, szPath);
				}
			}

			GlobalUnlock(medium.hGlobal);
		}

		ReleaseStgMedium(&medium);
	}


	// If there was any failure, the list would be empty...
	return((SLCheck(m_hList)) ? S_OK : E_INVALIDARG);
}

STDMETHODIMP CHashCheck::QueryContextMenu( HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
{
	if (uFlags & (CMF_DEFAULTONLY | CMF_NOVERBS))
		return(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0));

	// Ugly hack: work around a bug in Windows 5.x that causes a spurious
	// separator to be added when invoking the context menu from the Start Menu
	if (g_uWinVer < 0x0600 && !(uFlags & (0x20000 | CMF_EXPLORE)) && GetModuleHandleA("explorer.exe"))
		return(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0));

	// Load the menu display settings
	HASHCHECKOPTIONS opt;
	opt.dwFlags = HCOF_MENUDISPLAY;
	OptionsLoad(&opt);

	// Do not show if the settings prohibit it
	if (opt.dwMenuDisplay == 2 || (opt.dwMenuDisplay == 1 && !(uFlags & CMF_EXTENDEDVERBS)))
		return(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0));

	// Load the localized menu text
	TCHAR szMenuText[MAX_STRINGMSG];
	LoadString(g_hModThisDll, IDS_HS_MENUTEXT, szMenuText, countof(szMenuText));

	if (InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst, szMenuText))
		return(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 1));

	return(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0));
}

STDMETHODIMP CHashCheck::InvokeCommand( LPCMINVOKECOMMANDINFO pici )
{
	// Ignore string verbs (high word must be zero)
	// The only valid command index is 0 (low word must be zero)
	if (pici->lpVerb)
		return(E_INVALIDARG);

	// Hand things over to HashSave, where all the work is done...
	HashSaveStart(pici->hwnd, m_hList);

	// HaveSave has AddRef'ed and now owns our list
	SLRelease(m_hList);
	m_hList = NULL;

	return(S_OK);
}

STDMETHODIMP CHashCheck::GetCommandString( UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax )
{
	static const  CHAR szVerbA[] =  "cksum";
	static const WCHAR szVerbW[] = L"cksum";

	if (idCmd != 0 || cchMax < countof(szVerbW))
		return(E_INVALIDARG);

	switch (uFlags)
	{
		// The help text (status bar text) should not contain any of the
		// characters added for the menu access keys.

		case GCS_HELPTEXTA:
		{
			LoadStringA(g_hModThisDll, IDS_HS_MENUTEXT, (LPSTR)pszName, cchMax);

			LPSTR lpszSrcA = (LPSTR)pszName;
			LPSTR lpszDestA = (LPSTR)pszName;

			while (*lpszSrcA && *lpszSrcA != '(' && *lpszSrcA != '.')
			{
				if (*lpszSrcA != '&')
				{
					*lpszDestA = *lpszSrcA;
					++lpszDestA;
				}

				++lpszSrcA;
			}

			*lpszDestA = 0;
			return(S_OK);
		}

		case GCS_HELPTEXTW:
		{
			LoadStringW(g_hModThisDll, IDS_HS_MENUTEXT, (LPWSTR)pszName, cchMax);

			LPWSTR lpszSrcW = (LPWSTR)pszName;
			LPWSTR lpszDestW = (LPWSTR)pszName;

			while (*lpszSrcW && *lpszSrcW != L'(' && *lpszSrcW != L'.')
			{
				if (*lpszSrcW != L'&')
				{
					*lpszDestW = *lpszSrcW;
					++lpszDestW;
				}

				++lpszSrcW;
			}

			*lpszDestW = 0;
			return(S_OK);
		}

		case GCS_VERBA:
		{
			SSStaticCpyA((LPSTR)pszName, szVerbA);
			return(S_OK);
		}

		case GCS_VERBW:
		{
			SSStaticCpyW((LPWSTR)pszName, szVerbW);
			return(S_OK);
		}
	}

	return(E_INVALIDARG);
}

STDMETHODIMP CHashCheck::AddPages( LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam )
{
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USEREFPARENT | PSP_USETITLE;
	psp.hInstance = g_hModThisDll;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_HASHPROP);
	psp.pszTitle = MAKEINTRESOURCE(IDS_HP_TITLE);
	psp.pfnDlgProc = HashPropDlgProc;
	psp.lParam = (LPARAM)m_hList;
	psp.pfnCallback = HashPropCallback;
	psp.pcRefParent = (PUINT)&g_cRefThisDll;

	if (ActivateManifest(FALSE))
	{
		psp.dwFlags |= PSP_USEFUSIONCONTEXT;
		psp.hActCtx = g_hActCtx;
	}

	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);

	if (hPage && !pfnAddPage(hPage, lParam))
		DestroyPropertySheetPage(hPage);

	// HashProp has AddRef'ed and now owns our list
	SLRelease(m_hList);
	m_hList = NULL;

	return(S_OK);
}

STDMETHODIMP CHashCheck::Drop( LPDATAOBJECT pdtobj, DWORD grfKeyState, POINTL pt, PDWORD pdwEffect )
{
	FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM medium;

	UINT uThreads = 0;

	if (pdtobj && pdtobj->GetData(&format, &medium) == S_OK)
	{
		if (HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal))
		{
			UINT uDrops = DragQueryFile(hDrop, -1, NULL, 0);
			UINT cchPath;
			LPTSTR lpszPath;

			// Reduce the likelihood of a race condition when trying to create
			// an activation context by creating it before creating threads
			ActivateManifest(FALSE);

			for (UINT uDrop = 0; uDrop < uDrops; ++uDrop)
			{
				if ( (cchPath = DragQueryFile(hDrop, uDrop, NULL, 0)) &&
				     (lpszPath = (LPTSTR)malloc((cchPath + 1) * sizeof(TCHAR))) )
				{
					InterlockedIncrement(&g_cRefThisDll);

					HANDLE hThread;

					if ( (DragQueryFile(hDrop, uDrop, lpszPath, cchPath + 1) == cchPath) &&
					     (!(GetFileAttributes(lpszPath) & FILE_ATTRIBUTE_DIRECTORY)) &&
					     (hThread = CreateThreadCRT(HashVerifyThread, lpszPath)) )
					{
						// The thread should free lpszPath, not us
						CloseHandle(hThread);
						++uThreads;
					}
					else
					{
						free(lpszPath);
						InterlockedDecrement(&g_cRefThisDll);
					}
				}
			}

			GlobalUnlock(medium.hGlobal);
		}

		ReleaseStgMedium(&medium);
	}

	if (uThreads)
	{
		// DROPEFFECT_LINK would work here as well; it really doesn't matter
		*pdwEffect = DROPEFFECT_COPY;
		return(S_OK);
	}
	else
	{
		// We shouldn't ever be hitting this case
		*pdwEffect = DROPEFFECT_NONE;
		return(E_INVALIDARG);
	}
}
