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
#include "UnicodeHelpers.h"
#include "libs/WinHash.h"
#include <Strsafe.h>

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

VOID WINAPI HashCalcPrepare( PHASHCALCCONTEXT phcctx )
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

			if (GetFileAttributes(pszCurrent) & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (cchCurrent < MAX_PATH_BUFFER - 2)
				{
					memcpy(phcctx->scratch.sz, pszCurrent, cbCurrent);
					HashCalcWalkDirectory(phcctx, phcctx->scratch.sz, cchCurrent);
				}
			}
			else
			{
				PHASHCALCITEM pItem = SLAddItem(phcctx->hList, NULL, sizeof(HASHCALCITEM) + cbCurrent);

				if (pItem)
				{
					pItem->bValid = FALSE;
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
			return;

		pszPrev = pszCurrent;
	}
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
				// File: Add to the list
				UINT cbPathBuffer = (cchNew + 1) * sizeof(TCHAR);
				PHASHCALCITEM pItem = SLAddItem(phcctx->hList, NULL, sizeof(HASHCALCITEM) + cbPathBuffer);

				if (pItem)
				{
					pItem->bValid = FALSE;
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
	Save dialog
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
		phcctx->ofn.nMaxFile = MAX_PATH_BUFFER + 10;
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
		BOOL bSuccess = FALSE;

		// Save the filter in the user's preferences
		if (phcctx->opt.dwFilterIndex != phcctx->ofn.nFilterIndex)
		{
			phcctx->opt.dwFilterIndex = phcctx->ofn.nFilterIndex;
			phcctx->opt.dwFlags = HCOF_FILTERINDEX;
			OptionsSave(&phcctx->opt);
		}

		// Extension fixup: Correct the extension to match the selected
		// type, but only if the extension was one of the 5 in the list
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
			TCHAR szMessage[MAX_STRINGMSG];
			LoadString(g_hModThisDll, IDS_HC_SAVE_ERROR, szMessage, countof(szMessage));
			MessageBox(hWnd, szMessage, NULL, MB_OK | MB_ICONERROR);
		}
	}
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
			StringCchPrintf(
				phcctx->szFormat,
				countof(phcctx->szFormat),
				TEXT("%%-%ds %%s\r\n"),
				phcctx->cchMax - phcctx->cchAdjusted
			);
		}
		else
		{
			SSStaticCpy(phcctx->szFormat, TEXT("%s *%s\r\n"));
		}
	}
}

BOOL WINAPI HashCalcWriteResult( PHASHCALCCONTEXT phcctx, PHASHCALCITEM pItem )
{
	PCTSTR pszHash;
    WCHAR szWbuffer[MAX_PATH_BUFFER];
    CHAR  szAbuffer[MAX_PATH_BUFFER];
#ifdef UNICODE
#   define szTbuffer szWbuffer
#else
#   define szTbuffer szAbuffer
#endif
	PVOID pvLine;            // Will be pointed to the buffer to write out
	size_t cchLine, cbLine;  // Length of line, in TCHARs or BYTEs, EXCLUDING the terminator

	if (!pItem->bValid)
		return(FALSE);

	// Translate the filter index to a hash
	switch (phcctx->ofn.nFilterIndex)
	{
#define HASH_INDEX_TO_RESULTS_op(alg) \
        case alg:  pszHash = pItem->results.szHex##alg;  break;
        FOR_EACH_HASH(HASH_INDEX_TO_RESULTS_op)
		default: return(FALSE);
	}

	// Format the line
	#define HashCalcFormat(a, b) StringCchPrintfEx(szTbuffer, MAX_PATH_BUFFER, NULL, &cchLine, 0, phcctx->szFormat, a, b)
	(phcctx->ofn.nFilterIndex == 1) ?
		HashCalcFormat(pItem->szPath + phcctx->cchAdjusted, pszHash) : // SFV
		HashCalcFormat(pszHash, pItem->szPath + phcctx->cchAdjusted);  // everything else
	#undef HashCalcFormat
	// cchLine is temporarily the count of characters left in the buffer instead of the line length

#ifdef _TIMED
    StringCchPrintfEx(szTbuffer + (MAX_PATH_BUFFER-cchLine), cchLine, NULL, &cchLine, 0,
                      _T("; Elapsed: %d ms\r\n"), pItem->dwElapsed);
#endif

	cchLine = MAX_PATH_BUFFER - cchLine;  // now it's back to being the line length
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

			default: return(FALSE);
		}

		if (cbLine > 0)
		{
			INT cbWritten;
			WriteFile(phcctx->hFileOut, pvLine, (DWORD)cbLine, &cbWritten, NULL);
			if (cbLine != cbWritten) return(FALSE);
		}
		else return(FALSE);
	}
	else return(FALSE);

	return(TRUE);
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
