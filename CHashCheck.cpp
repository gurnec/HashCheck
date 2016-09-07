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

CHashCheck::CHashCheck( )
{
    InterlockedIncrement(&g_cRefThisDll);
    m_cRef = 1;
    m_hList = NULL;
    if (g_uWinVer >= 0x0600)
    {
        m_hbitmapMenu    = (HBITMAP)LoadImage(g_hModThisDll, MAKEINTRESOURCE(IDI_MENUBITMAP),    IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
        m_hbitmapMenuSep = (HBITMAP)LoadImage(g_hModThisDll, MAKEINTRESOURCE(IDI_MENUBITMAPSEP), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
    }
    else
        m_hbitmapMenu = m_hbitmapMenuSep = NULL;
}

CHashCheck::~CHashCheck()
{
    InterlockedDecrement(&g_cRefThisDll);
    SLRelease(m_hList);
    if (m_hbitmapMenu)
        DeleteObject(m_hbitmapMenu);
    if (m_hbitmapMenuSep)
        DeleteObject(m_hbitmapMenuSep);
}

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
	m_cItems = 0;

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
					if (SLAddStringI(m_hList, szPath))
						m_cItems++;
				}
			}

			GlobalUnlock(medium.hGlobal);
		}

		ReleaseStgMedium(&medium);
	}

	// If there was any failure, the list would be empty...
	return(m_cItems ? S_OK : E_INVALIDARG);
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

    WORD maxIdOffsetPlusOne = 0;  // what must be returned as per the QueryContextMenu specs

    if (! InsertMenu(hmenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL))
        goto done;

    MENUITEMINFO mii;
    mii.cbSize     = sizeof(mii);
    mii.fMask      = MIIM_FTYPE | MIIM_ID | MIIM_STRING;  // which mii members are set
    if (g_uWinVer >= 0x0600)  // prior to Vista, 32-bit bitmaps w/alpha channels don't render correctly in menus
        mii.fMask |= MIIM_BITMAP;
    mii.fType      = MFT_STRING;  // specifies the dwTypeData member is a pointer to a nul-terminated string

	// Load the localized menu text
	TCHAR szMenuText[MAX_STRINGMSG];
	LoadString(g_hModThisDll, IDS_HS_MENUTEXT, szMenuText, countof(szMenuText));  // "Create Chec&ksum file..."

    mii.wID        = idCmdFirst + VERB_CHECKSUM_ID;
    mii.dwTypeData = szMenuText;
    mii.hbmpItem   = m_hbitmapMenu;
    if (mii.wID > idCmdLast || ! InsertMenuItem(hmenu, indexMenu++, TRUE, &mii))
        goto done;
    maxIdOffsetPlusOne = max(maxIdOffsetPlusOne, VERB_CHECKSUM_ID + 1);

    // If multiple items (or a folder) were selected
    if (m_cItems > 1 || m_cItems && PathIsDirectory((LPTSTR)SLGetData(m_hList)))
    {
        // Load the localized menu text
        LoadString(g_hModThisDll, IDS_HS_MENUTEXT_SEP, szMenuText, countof(szMenuText));  // "Create se&parate Checksum files..."

        mii.wID        = idCmdFirst + VERB_CHECKSUM_SEPARATE_ID;
        mii.dwTypeData = szMenuText;
        mii.hbmpItem   = m_hbitmapMenuSep;
        if (mii.wID > idCmdLast || ! InsertMenuItem(hmenu, indexMenu++, TRUE, &mii))
            goto done;
        maxIdOffsetPlusOne = max(maxIdOffsetPlusOne, VERB_CHECKSUM_SEPARATE_ID + 1);
    }

    InsertMenu(hmenu, indexMenu, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);

    done:
	return(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, maxIdOffsetPlusOne));
}

STDMETHODIMP CHashCheck::InvokeCommand( LPCMINVOKECOMMANDINFO pici )
{
    BOOL bSeparateFiles;
    if (IS_INTRESOURCE(pici->lpVerb))
    {
        if ((UINT_PTR)pici->lpVerb > VERB_CHECKSUM_MAX_ID)
            return(E_FAIL);
        bSeparateFiles = (UINT_PTR)pici->lpVerb == VERB_CHECKSUM_SEPARATE_ID;
    }
    else
    {
        LPCMINVOKECOMMANDINFOEX picix =
            pici->cbSize == sizeof(LPCMINVOKECOMMANDINFOEX) ?
            (LPCMINVOKECOMMANDINFOEX)pici : NULL;
        if (picix && (picix->fMask & CMIC_MASK_UNICODE))
        {
            if (StrCmpICW(picix->lpVerbW, VERB_CHECKSUM_W) == 0)
                bSeparateFiles = FALSE;
            else if (StrCmpICW(picix->lpVerbW, VERB_CHECKSUM_SEPARATE_W) == 0)
                bSeparateFiles = TRUE;
            else
                return(E_FAIL);
        }
        else
        {
            if (StrCmpICA(pici->lpVerb, VERB_CHECKSUM_A) == 0)
                bSeparateFiles = FALSE;
            else if (StrCmpICA(pici->lpVerb, VERB_CHECKSUM_SEPARATE_A) == 0)
                bSeparateFiles = TRUE;
            else
                return(E_FAIL);
        }
    }

	// Hand things over to HashSave, where all the work is done...
	HashSaveStart(pici->hwnd, m_hList, bSeparateFiles);

	// HaveSave has AddRef'ed and now owns our list
	SLRelease(m_hList);
	m_hList = NULL;

	return(S_OK);
}

STDMETHODIMP CHashCheck::GetCommandString( UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax )
{
	static const  CHAR szVerbA[] = VERB_CHECKSUM_A;
	static const WCHAR szVerbW[] = VERB_CHECKSUM_W;
	static const  CHAR szVerbSepA[] = VERB_CHECKSUM_SEPARATE_A;
	static const WCHAR szVerbSepW[] = VERB_CHECKSUM_SEPARATE_W;

	if (idCmd > VERB_CHECKSUM_MAX_ID || cchMax < countof(szVerbSepW))
		return(E_INVALIDARG);

	switch (uFlags)
	{
		// The help text (status bar text) should not contain any of the
		// characters added for the menu access keys.

		case GCS_HELPTEXTA:
		{
			if (idCmd == 0)
				LoadStringA(g_hModThisDll, IDS_HS_MENUTEXT, (LPSTR)pszName, cchMax);
			else
				LoadStringA(g_hModThisDll, IDS_HS_MENUTEXT_SEP, (LPSTR)pszName, cchMax);

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
			if (idCmd == 0)
				LoadStringW(g_hModThisDll, IDS_HS_MENUTEXT, (LPWSTR)pszName, cchMax);
			else
				LoadStringW(g_hModThisDll, IDS_HS_MENUTEXT_SEP, (LPWSTR)pszName, cchMax);

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
			if (idCmd == 0)
				SSStaticCpyA((LPSTR)pszName, szVerbA);
			else
				SSStaticCpyA((LPSTR)pszName, szVerbSepA);
			return(S_OK);
		}

		case GCS_VERBW:
		{
			if (idCmd == 0)
				SSStaticCpyW((LPWSTR)pszName, szVerbW);
			else
				SSStaticCpyW((LPWSTR)pszName, szVerbSepW);
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
