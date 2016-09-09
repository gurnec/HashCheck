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
#include "HashCheckCommon.h"
#include "HashCalc.h"
#include "SetAppID.h"
#include "UnicodeHelpers.h"
#include "libs/WinHash.h"
#include <Strsafe.h>
#include <assert.h>

static const TCHAR SAVE_DEFAULT_NAME[] = TEXT("checksums");

// Due to the stupidity of the x64 compiler, the code emitted for the non-inline
// function is not as efficient as it is on x86
#ifdef _M_IX86
#undef SSChainNCpy2
#define SSChainNCpy2 SSChainNCpy2F
#endif



/*============================================================================*\
	Function declarations
\*============================================================================*/

// Path processing
VOID WINAPI HashCalcWalkDirectory( PHASHCALCCONTEXT phcctx, PTSTR pszPath, UINT cchPath );
__forceinline BOOL WINAPI IsSpecialDirectoryName( PCTSTR pszPath );
__forceinline BOOL WINAPI IsDoubleSlashPath( PCTSTR pszPath );

// Save helpers
__forceinline VOID WINAPI HashCalcSetSavePrefix( PHASHCALCCONTEXT phcctx, PTSTR pszSave );



/*============================================================================*\
	Path processing
\*============================================================================*/

// Checks if pszPath of length cchPathLen (excluding the nul terminator) ends in a recognized checksum extension
__inline BOOL WINAPI HasChecksumExt( LPCTSTR pszPath, UINT cchPathLen )
{
    // array of lengths in TCHARS (w/o the terminating nul) of extensions (e.g 5 for _T(".sha1")), excluding .asc
    static UINT cchHashExtsLenTab[NUM_HASHES] = {
#define HASH_CALC_EXT_LEN_op(alg) countof(HASH_EXT_##alg) - 1,
        FOR_EACH_HASH(HASH_CALC_EXT_LEN_op)
    };

    for (UINT i = 0; i < NUM_HASHES; i++)
    {
        if (cchPathLen > cchHashExtsLenTab[i] && _tcscmp(pszPath + cchPathLen - cchHashExtsLenTab[i], g_szHashExtsTab[i]) == 0)
            return(TRUE);
    }
    return(FALSE);
}

BOOL WINAPI HashCalcPrepare( PHASHCALCCONTEXT phcctx )
{
	PTSTR pszPrev = NULL;
	PTSTR pszCurrent, pszCurrentEnd;
	UINT cbCurrent, cchCurrent;

	SLReset(phcctx->hListRaw);

	while (pszCurrent = SLGetDataAndStepEx(phcctx->hListRaw, &cbCurrent))
	{
		pszCurrentEnd = BYTEADD(pszCurrent, cbCurrent);
		cchCurrent = cbCurrent / sizeof(TCHAR) - 1;

		// Get rid of the trailing slash if there is one
		if (cchCurrent && *(pszCurrentEnd - 1) == TEXT('\\'))
		{
			*(--pszCurrentEnd) = 0;
			--cchCurrent;
			cbCurrent -= sizeof(TCHAR);
		}

		if (pszPrev == NULL)
		{
			// Initialize the cchPrefix (and cchMax) for the first time; since
			// we have stripped away the trailing slash (if there was one), we
			// are guaranteed that cchPrefix < cchCurrent for the first run,
			// and that cchPrefix < cchPrev for all other iterations

			PTSTR pszTail = StrRChr(pszCurrent, pszCurrentEnd, TEXT('\\'));

			if (pszTail)
				phcctx->cchPrefix = (UINT)(pszTail - pszCurrent) + 1;
			else
				phcctx->cchPrefix = 0;

			// For "\\" paths, we cannot cut off any of the first two slashes
			if (phcctx->cchPrefix == 2 && IsDoubleSlashPath(pszCurrent))
				phcctx->cchPrefix = 0;

			phcctx->cchMax = cchCurrent;
		}
		else
		{
			// Or, just update cchPrefix

			UINT i, j = 0, k = 0;

			for (i = 0; i < cchCurrent && i < phcctx->cchPrefix; ++i)
			{
				if (pszCurrent[i] != pszPrev[i])
					break;

				if (pszCurrent[i] == TEXT('\\'))
				{
					j = i + 1;
					++k;
				}
			}

			// For "\\" paths, we cannot cut off any of the first two slashes
			if (cchCurrent >= 2 && IsDoubleSlashPath(pszCurrent) && k < 3)
				phcctx->cchPrefix = 0;
			else
				phcctx->cchPrefix = j;
		}

		if (cchCurrent && phcctx->hList)
		{
			// Finally, we can do the actual work that's needed!

			if (PathIsDirectory(pszCurrent))
			{
				if (cchCurrent < MAX_PATH_BUFFER - 2)
				{
					memcpy(phcctx->scratch.sz, pszCurrent, cbCurrent);
					HashCalcWalkDirectory(phcctx, phcctx->scratch.sz, cchCurrent);
				}
			}
			else
			{
                if (phcctx->bSeparateFiles && phcctx->wIfExists != IFEXISTS_TREATNORMAL &&
                    HasChecksumExt(pszCurrent, cchCurrent))
                {
                    continue;
                }

				PHASHCALCITEM pItem = SLAddItem(phcctx->hList, NULL, sizeof(HASHCALCITEM) + cbCurrent +
                    (phcctx->bSeparateFiles ? MAX_FILE_EXT_LEN * sizeof(TCHAR) : 0));

				if (pItem)
				{
                    pItem->results.dwFlags = 0;
					pItem->cchPath = cchCurrent;
					memcpy(pItem->szPath, pszCurrent, cbCurrent);

					if (phcctx->cchMax < cchCurrent)
						phcctx->cchMax = cchCurrent;

					++phcctx->cTotal;
				}
			}
		}

        if (phcctx->status == PAUSED)
            WaitForSingleObject(phcctx->hUnpauseEvent, INFINITE);
        if (phcctx->status == CANCEL_REQUESTED)
			return(FALSE);

		pszPrev = pszCurrent;
	}
    return(TRUE);
}

VOID WINAPI HashCalcWalkDirectory( PHASHCALCCONTEXT phcctx, PTSTR pszPath, UINT cchPath )
{
	HANDLE hFind;
	WIN32_FIND_DATA finddata;

	PTSTR pszPathAppend = pszPath + cchPath;
	*pszPathAppend = TEXT('\\');
	SSCpy2Ch(++pszPathAppend, TEXT('*'), 0);

	if ((hFind = FindFirstFile(pszPath, &finddata)) == INVALID_HANDLE_VALUE)
		return;

	do
	{
		// Add 1 to the length since we are also going to count the slash that
		// was added at the end of the directory
		UINT cchLeaf = (UINT)SSLen(finddata.cFileName) + 1;
		UINT cchNew = cchPath + cchLeaf;

        if (phcctx->status == PAUSED)
            WaitForSingleObject(phcctx->hUnpauseEvent, INFINITE);
		if (phcctx->status == CANCEL_REQUESTED)
			break;

		if ( (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)) &&
		     (cchNew < MAX_PATH_BUFFER - 2) )
		{
			SSChainNCpy(pszPathAppend, finddata.cFileName, cchLeaf);

			if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Directory: Recurse
				if (!IsSpecialDirectoryName(finddata.cFileName))
					HashCalcWalkDirectory(phcctx, pszPath, cchNew);
			}
			else
			{
                if (phcctx->bSeparateFiles && phcctx->wIfExists != IFEXISTS_TREATNORMAL &&
                    HasChecksumExt(pszPath, cchNew))
                {
                    continue;
                }

				// File: Add to the list
				UINT cbPathBuffer = (cchNew + 1) * sizeof(TCHAR);
				PHASHCALCITEM pItem = SLAddItem(phcctx->hList, NULL, sizeof(HASHCALCITEM) + cbPathBuffer +
                    (phcctx->bSeparateFiles ? MAX_FILE_EXT_LEN * sizeof(TCHAR) : 0));

				if (pItem)
				{
                    pItem->results.dwFlags = 0;
					pItem->cchPath = cchNew;
					memcpy(pItem->szPath, pszPath, cbPathBuffer);

					if (phcctx->cchMax < cchNew)
						phcctx->cchMax = cchNew;

					++phcctx->cTotal;
				}
			}
		}

	} while (FindNextFile(hFind, &finddata));

	FindClose(hFind);
}

BOOL WINAPI IsSpecialDirectoryName( PCTSTR pszPath )
{
	// TRUE if name is "." or ".."

	#ifdef UNICODE
	return(
		(*((UPDWORD)pszPath) == WCHARS2DWORD(L'.', 0)) ||
		(*((UPDWORD)pszPath) == WCHARS2DWORD(L'.', L'.') && pszPath[2] == 0)
	);
	#else
	return(
		(*((UPWORD)pszPath) == CHARS2WORD('.', 0)) ||
		(*((UPWORD)pszPath) == CHARS2WORD('.', '.') && pszPath[2] == 0)
	);
	#endif
}

BOOL WINAPI IsDoubleSlashPath( PCTSTR pszPath )
{
	// TRUE if string starts with "\\"

	#ifdef UNICODE
	return(*((UPDWORD)pszPath) == WCHARS2DWORD(L'\\', L'\\'));
	#else
	return(*((UPWORD)pszPath) == CHARS2WORD('\\', '\\'));
	#endif
}



/*============================================================================*\
	Save dialogs
\*============================================================================*/

VOID WINAPI HashCalcInitSave( PHASHCALCCONTEXT phcctx )
{
	HWND hWnd = phcctx->hWnd;

	// We can use the extended portion of the scratch buffer for the file name
	PTSTR pszFile = (PTSTR)phcctx->scratch.ext;

	// Default result value
	phcctx->hFileOut = INVALID_HANDLE_VALUE;

	// Load settings
	phcctx->opt.dwFlags = HCOF_FILTERINDEX | HCOF_SAVEENCODING;
	OptionsLoad(&phcctx->opt);

	// Initialize the struct for the first time, if needed
	if (phcctx->ofn.lStructSize == 0)
	{
		phcctx->ofn.lStructSize = sizeof(phcctx->ofn);
		phcctx->ofn.hwndOwner = hWnd;
		phcctx->ofn.lpstrFilter = HASH_FILE_FILTERS;
		phcctx->ofn.nFilterIndex = phcctx->opt.dwFilterIndex;
		phcctx->ofn.lpstrFile = pszFile;
		phcctx->ofn.nMaxFile = MAX_PATH_BUFFER + MAX_FILE_EXT_LEN;
		phcctx->ofn.Flags = OFN_DONTADDTORECENT | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
		phcctx->ofn.lpstrDefExt = TEXT("");

		// Set the initial file name
		{
			PTSTR pszOrigPath;

			SLReset(phcctx->hListRaw);
			pszOrigPath = SLGetDataAndStep(phcctx->hListRaw);

			if (SLCheck(phcctx->hListRaw))
			{
				// Multiple items were selected in Explorer
				SSChainNCpy2(
					pszFile,
					pszOrigPath, phcctx->cchPrefix,
					SAVE_DEFAULT_NAME, countof(SAVE_DEFAULT_NAME)
				);
			}
			else
			{
				// Only one item was selected in Explorer (may be a single
				// file or a directory containing multiple files)
				SSChainCpyCat(pszFile, pszOrigPath, g_szHashExtsTab[phcctx->ofn.nFilterIndex - 1]);
			}
		}
	}

	// We should also do a sanity check to make sure that the filter index
	// is set to a valid value since we depend on that to determine the format
	if ( GetSaveFileName(&phcctx->ofn) &&
	     phcctx->ofn.nFilterIndex &&
		 phcctx->ofn.nFilterIndex <= NUM_HASHES)
	{
		// Save the filter in the user's preferences
		if (phcctx->opt.dwFilterIndex != phcctx->ofn.nFilterIndex)
		{
			phcctx->opt.dwFilterIndex = phcctx->ofn.nFilterIndex;
			phcctx->opt.dwFlags = HCOF_FILTERINDEX;
			OptionsSave(&phcctx->opt);
		}

		// Extension fixup: Correct the extension to match the selected
		// type, but only if the extension was one of those in the list
		if (phcctx->ofn.nFileExtension)
		{
			PTSTR pszExt = pszFile + phcctx->ofn.nFileExtension - 1;

#define HASH_EXT_CMP_OR_op(alg) StrCmpI(pszExt, HASH_EXT_##alg) == 0 ||
			if (FOR_EACH_HASH(HASH_EXT_CMP_OR_op) FALSE)  // the FALSE is to ignore the last trailing ||
			{
				if (StrCmpI(pszExt, g_szHashExtsTab[phcctx->ofn.nFilterIndex - 1]))
					SSCpy(pszExt, g_szHashExtsTab[phcctx->ofn.nFilterIndex - 1]);
			}
		}

		// Adjust the file paths for the output path, if necessary
		HashCalcSetSavePrefix(phcctx, pszFile);

		// Open the file for output
		phcctx->hFileOut = CreateFile(
			pszFile,
			FILE_APPEND_DATA,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (phcctx->hFileOut != INVALID_HANDLE_VALUE)
		{
			// The actual format will be set when HashCalcWriteResult is called
			phcctx->szFormat[0] = 0;

			if (phcctx->opt.dwSaveEncoding == 1)
			{
				// Write the BOM for UTF-16LE
				WCHAR BOM = 0xFEFF;
				DWORD cbWritten;
				WriteFile(phcctx->hFileOut, &BOM, sizeof(WCHAR), &cbWritten, NULL);
			}
		}
		else
		{
			TCHAR szFormat[MAX_STRINGMSG], szMessageBuffer[MAX_STRINGMSG], *pszMessage;
			LoadString(g_hModThisDll, IDS_HC_SAVE_ERROR, szFormat, countof(szFormat));
            if (StrStr(szFormat, TEXT("%d")) != NULL)
            {
                StringCchPrintf(szMessageBuffer, countof(szMessageBuffer), szFormat, 1);
                pszMessage = szMessageBuffer;
            }
            else
            {
                pszMessage = szFormat;
            }
			MessageBox(hWnd, pszMessage, NULL, MB_OK | MB_ICONERROR);
		}
	}
}

// input:   dwInitParam is the (1-based) hash id to pre-select (from the hash_algorithm enum in WinHash.h)
// returns: (from DialogBoxParam()) 0 if canceled, otherwise:
//          in the LOWORD: the (1-based) hash id chosen by the user
//          in the HIWORD: IFEXISTS_KEEP (0), IFEXISTS_OVERWRITE (1), or IFEXISTS_TREATNORMAL (2)
INT_PTR CALLBACK HashCalcDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetAppIDForWindow(hWnd, TRUE);

            // Set the window's title w/o any of the characters added for the menu access keys
            TCHAR szTitle[MAX_STRINGMSG];
            LoadString(g_hModThisDll, IDS_HS_MENUTEXT_SEP, szTitle, countof(szTitle));
            LPTSTR lpszSrc = szTitle, lpszDest = szTitle;
            while (*lpszSrc && *lpszSrc != '(' && *lpszSrc != '.')
            {
                if (*lpszSrc != '&')
                {
                    *lpszDest = *lpszSrc;
                    ++lpszDest;
                }
                ++lpszSrc;
            }
            *lpszDest = 0;
            SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szTitle);

            // Set the window's icon and localized text
            SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(g_hModThisDll, MAKEINTRESOURCE(IDI_FILETYPE)));
            SetControlText(hWnd, IDC_SEP_CHK,           IDS_SEP_CHK);
            SetControlText(hWnd, IDC_SEP_EX,            IDS_SEP_EX);
            SetControlText(hWnd, IDC_SEP_EX_KEEP,       IDS_SEP_EX_KEEP);
            SetControlText(hWnd, IDC_SEP_EX_OVERWRITE,  IDS_SEP_EX_OVERWRITE);
            SetControlText(hWnd, IDC_SEP_EX_NOTSPECIAL, IDS_SEP_EX_TREATNORMAL);
            SetControlText(hWnd, IDC_OK,                IDS_SEP_OK);
            SetControlText(hWnd, IDC_CANCEL,            IDS_SEP_CANCEL);

            // Tick the pre-selected hash and default if-exists options
            if (lParam >= 1 && lParam <= NUM_HASHES)
                SendDlgItemMessage(hWnd, IDC_SEP_CHK_FIRSTID + (int)lParam - 1, BM_SETCHECK, BST_CHECKED, 0);
            SendDlgItemMessage(hWnd, IDC_SEP_EX_KEEP, BM_SETCHECK, BST_CHECKED, 0);

            return(TRUE);

        case WM_DESTROY:
            SetAppIDForWindow(hWnd, FALSE);
            break;

        case WM_ENDSESSION:
            if (wParam == FALSE)  // if TRUE, fall through to WM_CLOSE
                break;
        case WM_CLOSE:
            goto end_dialog;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                WORD i, wHashSelected, wExistsSelected;
                case IDC_OK:
                    
                    // Retrieve the selected hash
                    for (wHashSelected = i = 0; i < NUM_HASHES; i++)
                        if (SendDlgItemMessage(hWnd, IDC_SEP_CHK_FIRSTID + i, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            wHashSelected = i + 1;
                            break;
                        }
                    if (!wHashSelected)  // shouldn't happen
                    {
                        assert(FALSE);
                        goto end_dialog;
                    }

                    // Retrieve the selected if-exists option
                    for (wExistsSelected = i = 0; i < IDC_SEP_EX_COUNT; i++)
                        if (SendDlgItemMessage(hWnd, IDC_SEP_EX_FIRSTID + i, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            wExistsSelected = i;
                            break;
                        }

                    EndDialog(hWnd, MAKELONG(wHashSelected, wExistsSelected));
                    return(TRUE);

                case IDC_CANCEL:
                    end_dialog: EndDialog(hWnd, 0);
                    return(TRUE);
            }
            break;
    }
    return(FALSE);
}

VOID WINAPI HashCalcInitSaveSeparate(PHASHCALCCONTEXT phcctx)
{
    // Load settings
    phcctx->opt.dwFlags = HCOF_FILTERINDEX | HCOF_SAVEENCODING;
    OptionsLoad(&phcctx->opt);

    DWORD dwOrigFilterIndex = phcctx->opt.dwFilterIndex;
    INT_PTR nDialogRet = DialogBoxParam(
        g_hModThisDll,
        MAKEINTRESOURCE(IDD_HASHSAVE_SEP),
        NULL,
        HashCalcDlgProc,
        dwOrigFilterIndex
    );
    phcctx->ofn.nFilterIndex = LOWORD(nDialogRet);
    phcctx->wIfExists        = HIWORD(nDialogRet);

    if (phcctx->ofn.nFilterIndex && phcctx->ofn.nFilterIndex != dwOrigFilterIndex)
    {
        // Save modified setting
        phcctx->opt.dwFilterIndex = phcctx->ofn.nFilterIndex;
        phcctx->opt.dwFlags = HCOF_FILTERINDEX;
        OptionsSave(&phcctx->opt);
    }

    // The actual format will be set when HashCalcWriteResult is called
    phcctx->szFormat[0] = 0;
}

VOID WINAPI HashCalcSetSaveFormat( PHASHCALCCONTEXT phcctx )
{
	// Set szFormat if necessary
	if (phcctx->szFormat[0] == 0)
	{
		// Did I ever mention that I hate SFV?
		// The reason we tracked cchMax was because of this idiotic format
		if (phcctx->ofn.nFilterIndex == 1)
		{
            if (! phcctx->bSeparateFiles)  // if there's a single output file
                StringCchPrintf(
                    phcctx->szFormat,
                    countof(phcctx->szFormat),
                    TEXT("%%-%ds %%s\r\n"),
                    phcctx->cchMax - phcctx->cchAdjusted
                );
            else  // else each hash is in its own file with no SFV padding
                SSStaticCpy(phcctx->szFormat, TEXT("%s %s\r\n"));
		}
		else
		{
			SSStaticCpy(phcctx->szFormat, TEXT("%s *%s\r\n"));
		}
	}
}

BOOL WINAPI HashCalcWriteResult( PHASHCALCCONTEXT phcctx, HANDLE hFileOut, PHASHCALCITEM pItem )
{
	PCTSTR pszHash;                     // will be pointed to the hash name
    WCHAR szWbuffer[MAX_PATH_BUFFER];   // wide-char buffer
    CHAR  szAbuffer[MAX_PATH_BUFFER];   // narrow-char buffer
#ifdef UNICODE
#   define szTbuffer szWbuffer
#else
#   define szTbuffer szAbuffer
#endif
    PTSTR szTbufferAppend = szTbuffer;  // current end of the buffer used to build output
    size_t cchLine = MAX_PATH_BUFFER;   // starts off as count of remaining TCHARS in the buffer
    PVOID pvLine;                       // will be pointed to the buffer to write out
    size_t cbLine;                      // will be line length in bytes, EXCLUDING nul terminator

	// If the checksum to save isn't present in the results
    if (! ((1 << (phcctx->ofn.nFilterIndex - 1)) & pItem->results.dwFlags))
    {
        // Start with a commented-out error message - "; UNREADABLE:"
        WCHAR szUnreadable[MAX_STRINGRES];
        LoadString(g_hModThisDll, IDS_HV_STATUS_UNREADABLE, szUnreadable, MAX_STRINGRES);
        StringCchPrintfEx(szTbufferAppend, cchLine, &szTbufferAppend, &cchLine, 0, TEXT("; %s:\r\n"), szUnreadable);

        // We'll still output a hash, but it will be all 0's, that way Verify will indicate an mismatch
        HashCalcClearInvalid(&pItem->results, TEXT('0'));
    }

	// Translate the filter index to a hash
	switch (phcctx->ofn.nFilterIndex)
	{
#define HASH_INDEX_TO_RESULTS_op(alg) \
        case alg:  pszHash = pItem->results.szHex##alg;  break;
        FOR_EACH_HASH(HASH_INDEX_TO_RESULTS_op)

		default:
            assert(FALSE);
            return(FALSE);
	}

	// Format the line
    LPCTSTR pszPathAdjusted;  // will point to the path about to be written out
    //
    // If there's a single output file
    if (! phcctx->bSeparateFiles)
        // The path common to all files (including the output file) is removed
        pszPathAdjusted = pItem->szPath + phcctx->cchAdjusted;
    //
    // Otherwise if it's one output file per file hashed, remove everything but the base filename
    else
    {
        for (pszPathAdjusted = pItem->szPath + pItem->cchPath - 1; pszPathAdjusted > pItem->szPath; pszPathAdjusted--)
            if (*pszPathAdjusted == TEXT('\\') || *pszPathAdjusted == TEXT('/'))
            {
                pszPathAdjusted++;
                break;
            }
    }
    //
	#define HashCalcFormat(a, b) StringCchPrintfEx(szTbufferAppend, cchLine, &szTbufferAppend, &cchLine, 0, phcctx->szFormat, a, b)
	(phcctx->ofn.nFilterIndex == 1) ?
		HashCalcFormat(pszPathAdjusted, pszHash) : // SFV
		HashCalcFormat(pszHash, pszPathAdjusted);  // everything else
	#undef HashCalcFormat

#ifdef _TIMED
    StringCchPrintfEx(szTbufferAppend, cchLine, NULL, &cchLine, 0,
                      _T("; Elapsed: %d ms\r\n"), pItem->dwElapsed);
#endif

	cchLine = MAX_PATH_BUFFER - cchLine;  // from now on cchLine is the line length in bytes, EXCLUDING nul terminator
	if (cchLine > 0)
	{
		// Convert to the correct encoding
		switch (phcctx->opt.dwSaveEncoding)
		{
			case 0:
			{
				// UTF-8
				#ifdef UNICODE
				cbLine = WStrToUTF8(szWbuffer, szAbuffer, MAX_PATH_BUFFER) - 1;
				#else
				         AStrToWStr(szAbuffer, szWbuffer, MAX_PATH_BUFFER));
				cbLine = WStrToUTF8(szWbuffer, szAbuffer, MAX_PATH_BUFFER)) - 1;
				#endif

				pvLine = szAbuffer;
				break;
			}

			case 1:
			{
				// UTF-16
				#ifndef UNICODE
				cchLine = AStrToWStr(szAbuffer, szWbuffer, MAX_PATH_BUFFER) - 1;
				#endif

				cbLine = cchLine * sizeof(WCHAR);
				pvLine = szWbuffer;
				break;
			}

			case 2:
			{
				// ANSI
				#ifdef UNICODE
				cbLine = WStrToAStr(szWbuffer, szAbuffer, MAX_PATH_BUFFER) - 1;
				#else
				cbLine = cchLine;
				#endif

				pvLine = szAbuffer;
				break;
			}

			default:
                assert(FALSE);
                return(FALSE);
		}

		if (cbLine > 0)
		{
			DWORD cbWritten;
			if (! WriteFile(hFileOut, pvLine, (DWORD)cbLine, &cbWritten, NULL) || cbLine != cbWritten)
                return(FALSE);
		}
		else 
        {
            assert(FALSE);
            return(FALSE);
        }
	}
	else
    {
        assert(FALSE);
        return(FALSE);
    }

	return(TRUE);
}

VOID WINAPI HashCalcClearInvalid( PWHRESULTEX pwhres, WCHAR cInvalid )
{
#ifdef UNICODE
#   define _tmemset wmemset
#else
#   define _tmemset memset
#endif

#define HASH_CLEAR_INVALID_op(alg)                                                \
    if (! (pwhres->dwFlags & WHEX_CHECK##alg))                                    \
    {                                                                             \
        _tmemset(pwhres->szHex##alg, cInvalid, countof(pwhres->szHex##alg) - 1);  \
        pwhres->szHex##alg[countof(pwhres->szHex##alg) - 1] = L'\0';              \
    }
    FOR_EACH_HASH(HASH_CLEAR_INVALID_op)
}

// This can only succeed on Windows Vista and later;
// returns FALSE on failure
BOOL WINAPI HashCalcDeleteFileByHandle(HANDLE hFile)
{
    if (hFile == INVALID_HANDLE_VALUE)
        return(FALSE);

    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
    if (hKernel32 == NULL)
        return(FALSE);

    typedef BOOL(WINAPI* PFN_SFIBH)(_In_ HANDLE, _In_ FILE_INFO_BY_HANDLE_CLASS, _In_ LPVOID, _In_ DWORD);
    PFN_SFIBH pfnSetFileInformationByHandle = (PFN_SFIBH)GetProcAddress(hKernel32, "SetFileInformationByHandle");
    if (pfnSetFileInformationByHandle == NULL)
        return(FALSE);

    FILE_DISPOSITION_INFO fdi;
    fdi.DeleteFile = TRUE;
    return(pfnSetFileInformationByHandle(hFile, FileDispositionInfo, &fdi, sizeof(fdi)));
}

VOID WINAPI HashCalcSetSavePrefix( PHASHCALCCONTEXT phcctx, PTSTR pszSave )
{
	// We have to be careful here about case sensitivity since we are now
	// working with a user-provided path instead of a system-provided path...

	// We want to build new paths without resorting to using "..", as that is
	// ugly, fragile (often more so than absolute paths), and not to mention,
	// complicated to calculate.  This means that relative paths will be used
	// only for paths within the same line of ancestry.

	BOOL bMultiSel;
	PTSTR pszOrig;
	PTSTR pszTail;

	// First, grab one of the original paths to work with
	SLReset(phcctx->hListRaw);
	pszOrig = SLGetDataAndStep(phcctx->hListRaw);
	bMultiSel = SLCheck(phcctx->hListRaw);

	// Unfortunately, we also have to contend with the possibility that one of
	// these paths may be in short name format (e.g., if the user navigates to
	// %TEMP% on a NT 5.x system)
	{
		// The scratch buffer's sz members are large enough for us
		PTSTR pszOrigLong = (PTSTR)phcctx->scratch.szW;
		PTSTR pszSaveLong = (PTSTR)phcctx->scratch.szA;

		// Copy original path to scratch and terminate
		pszTail = SSChainNCpy(pszOrigLong, pszOrig, phcctx->cchPrefix);
		pszTail[0] = 0;

		// Copy output path to scratch and terminate
		pszTail = SSChainNCpy(pszSaveLong, pszSave, phcctx->ofn.nFileOffset);
		pszTail[0] = 0;

		// Normalize both paths to LFN
		GetLongPathName(pszOrigLong, pszOrigLong, MAX_PATH_BUFFER);
		GetLongPathName(pszSaveLong, pszSaveLong, MAX_PATH_BUFFER);

		// We will only handle the case where they are the same, to prevent our
		// re-prefixing from messing up the base behavior; it is not worth the
		// trouble to account for LFN for all cases--just let it fall through
		// to an absolute path.
		if (StrCmpNI(pszOrigLong, pszSaveLong, MAX_PATH_BUFFER) == 0)
		{
			phcctx->cchAdjusted = phcctx->cchPrefix;
			return;
		}
	}

	if (pszTail = StrRChr(pszSave, NULL, TEXT('\\')))
	{
		phcctx->cchAdjusted = (UINT)(pszTail - pszSave) + 1;

		if (phcctx->cchAdjusted <= phcctx->cchPrefix)
		{
			if (StrCmpNI(pszOrig, pszSave, phcctx->cchAdjusted) == 0)
			{
				// If the ouput prefix is the same as or a parent of the input
				// prefix...

				if (!(IsDoubleSlashPath(pszSave) && phcctx->cchAdjusted < 3))
					return;
			}
		}
		else if (!bMultiSel)
		{
			// We will make an exception for the case where the user selects
			// a single directory from the Shell and then saves the output in
			// that directory...

			BOOL bEqual;

			*pszTail = 0;
			bEqual = StrCmpNI(pszOrig, pszSave, phcctx->cchAdjusted) == 0;
			*pszTail = TEXT('\\');

			if (bEqual) return;
		}
	}

	// If we have reached this point, we need to use an absolute path

	if ( pszSave[1] == TEXT(':') && phcctx->cchPrefix > 2 &&
	     StrCmpNI(pszOrig, pszSave, 2) == 0 )
	{
		// Omit drive letter
		phcctx->cchAdjusted = 2;
	}
	else
	{
		// Full absolute path
		phcctx->cchAdjusted = 0;
	}
}



/*============================================================================*\
	Progress bar
\*============================================================================*/

VOID WINAPI HashCalcTogglePrep( PHASHCALCCONTEXT phcctx, BOOL bState )
{
	DWORD dwStyle = (DWORD)GetWindowLongPtr(phcctx->hWndPBTotal, GWL_STYLE);

	if (bState)
	{
		dwStyle &= ~PBS_SMOOTH;
		dwStyle |= PBS_MARQUEE;
		phcctx->dwFlags |= HCF_MARQUEE;
	}
	else
	{
		dwStyle |= PBS_SMOOTH;
		dwStyle &= ~PBS_MARQUEE;
		phcctx->dwFlags &= ~HCF_MARQUEE;
	}

	SetWindowLongPtr(phcctx->hWndPBTotal, GWL_STYLE, dwStyle);
	SendMessage(phcctx->hWndPBTotal, PBM_SETMARQUEE, bState, MARQUEE_INTERVAL);

	if (!bState)
		SendMessage(phcctx->hWndPBTotal, PBM_SETRANGE32, 0, phcctx->cTotal);
}
