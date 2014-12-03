/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "globals.h"
#include "HashCheckCommon.h"
#include "SetAppID.h"
#include "UnicodeHelpers.h"
#include <uxtheme.h>

#define HV_COL_FILENAME 0
#define HV_COL_SIZE     1
#define HV_COL_STATUS   2
#define HV_COL_EXPECTED 3
#define HV_COL_ACTUAL   4
#define HV_COL_FIRST    HV_COL_FILENAME
#define HV_COL_LAST     HV_COL_ACTUAL

#define HV_STATUS_NULL       0
#define HV_STATUS_MATCH      1
#define HV_STATUS_MISMATCH   2
#define HV_STATUS_UNREADABLE 3

#define LISTVIEW_EXSTYLES ( LVS_EX_HEADERDRAGDROP | \
                            LVS_EX_FULLROWSELECT  | \
                            LVS_EX_LABELTIP       | \
                            LVS_EX_DOUBLEBUFFER )

// Fix up missing A/W aliases
#ifdef UNICODE
#define LPNMLVDISPINFO LPNMLVDISPINFOW
#define StrCmpLogical StrCmpLogicalW
#else
#define LPNMLVDISPINFO LPNMLVDISPINFOA
#define StrCmpLogical StrCmpIA
#endif

// VC6/7 do not support qsort_s, so we have to provide our own (qsort_s_uptr);
// if the environment does support qsort_s, we will use that instead
#if __STDC_WANT_SECURE_LIB__
#define qsort_s_uptr qsort_s
#else
void __cdecl qsort_s_uptr(
	void *base,
	size_t num,
	size_t width,
	int (__cdecl *compare)( void *, const void *, const void * ),
	void *context
);
#endif

// Due to the stupidity of the x64 compiler, the code emitted for the non-inline
// function is not as efficient as it is on x86
#ifdef _M_IX86
#undef SSChainNCpy2
#define SSChainNCpy2 SSChainNCpy2F
#endif

typedef struct {
	UINT               idStartOfDirtyRange;
	UINT               cMatch;       // number of matches
	UINT               cMismatch;    // number of mismatches
	UINT               cUnreadable;  // number of unreadable files
} HASHVERIFYPREV, *PHASHVERIFYPREV;

typedef struct {
	INT                iColumn;      // column to sort
	BOOL               bReverse;     // reverse sort?
} HASHVERIFYSORT, *PHASHVERIFYSORT;

typedef struct {
	FILESIZE           filesize;
	PTSTR              pszDisplayName;
	PTSTR              pszExpected;
	INT16              cchDisplayName;
	UINT8              uState;
	UINT8              uStatusID;
	TCHAR              szActual[41];
} HASHVERIFYITEM, *PHASHVERIFYITEM, *PHVITEM, **PPHVITEM;

typedef CONST HASHVERIFYITEM **PPCHVITEM;

typedef struct {
	// Common block (see COMMONCONTEXT)
	WORKERTHREADSTATUS status;       // thread status
	DWORD              dwFlags;      // misc. status flags
	MSGCOUNT           cSentMsgs;    // number update messages sent by the worker
	MSGCOUNT           cHandledMsgs; // number update messages processed by the UI
	HWND               hWnd;         // handle of the dialog window
	HWND               hWndPBTotal;  // cache of the IDC_PROG_TOTAL progress bar handle
	HWND               hWndPBFile;   // cache of the IDC_PROG_FILE progress bar handle
	HANDLE             hThread;      // handle of the worker thread
	WORKERTHREADEXTRA  ex;           // extra parameter with varying uses
	// Members specific to HashVerify
	HWND               hWndList;     // handle of the list
	HSIMPLELIST        hList;        // where we store all the data
	PPHVITEM           index;        // index of the items in the list
	PTSTR              pszPath;      // raw path, set by initial input
	PTSTR              pszFileData;  // raw file data, set by initial input
	HASHVERIFYSORT     sort;         // sort information
	BOOL               bFreshStates; // is our copy of the item states fresh?
	UINT               cTotal;       // total number of files
	UINT               cMatch;       // number of matches
	UINT               cMismatch;    // number of mismatches
	UINT               cUnreadable;  // number of unreadable files
	HASHVERIFYPREV     prev;         // previous update data, used for update coalescing
	UINT               uMaxBatch;    // maximum number of updates to coalesce
	WHCTXEX            whctx;        // context for the WinHash library
	TCHAR              szStatus[4][MAX_STRINGRES];
} HASHVERIFYCONTEXT, *PHASHVERIFYCONTEXT;



/*============================================================================*\
	Function declarations
\*============================================================================*/

// Data parsing functions
__forceinline PBYTE WINAPI HashVerifyLoadData( PHASHVERIFYCONTEXT phvctx );
VOID WINAPI HashVerifyParseData( PHASHVERIFYCONTEXT phvctx );
BOOL WINAPI ValidateHexSequence( PTSTR psz, UINT cch );

// Worker thread
VOID __fastcall HashVerifyWorkerMain( PHASHVERIFYCONTEXT phvctx );

// Dialog general
INT_PTR CALLBACK HashVerifyDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
VOID WINAPI HashVerifyDlgInit( PHASHVERIFYCONTEXT phvctx );

// Dialog status
VOID WINAPI HashVerifyUpdateSummary( PHASHVERIFYCONTEXT phvctx, PHASHVERIFYITEM pItem );

// List management
__forceinline VOID WINAPI HashVerifyListInfo( PHASHVERIFYCONTEXT phvctx, LPNMLVDISPINFO pdi );
__forceinline LONG_PTR WINAPI HashVerifySetColor( PHASHVERIFYCONTEXT phvctx, LPNMLVCUSTOMDRAW pcd );
__forceinline LONG_PTR WINAPI HashVerifyFindItem( PHASHVERIFYCONTEXT phvctx, LPNMLVFINDITEM pfi );
__forceinline VOID WINAPI HashVerifySortColumn( PHASHVERIFYCONTEXT phvctx, LPNMLISTVIEW plv );
__forceinline VOID WINAPI HashVerifyReadStates( PHASHVERIFYCONTEXT phvctx );
__forceinline VOID WINAPI HashVerifySetStates( PHASHVERIFYCONTEXT phvctx );
__forceinline INT WINAPI Compare64( PLONGLONG p64A, PLONGLONG p64B );
INT __cdecl HashVerifySortCompare( PHASHVERIFYCONTEXT phvctx, PPCHVITEM ppItemA, PPCHVITEM ppItemB );



/*============================================================================*\
	Entry points / main functions
\*============================================================================*/

VOID CALLBACK HashVerify_RunDLLW( HWND hWnd, HINSTANCE hInstance,
                                  PWSTR pszCmdLine, INT nCmdShow )
{
	SIZE_T cchPath = SSLenW(pszCmdLine) + 1;
	PTSTR pszPath;

	// HashVerifyThread will try to free the path passed to it, as it expects
	// it to be allocated by malloc; it also expects g_cRefThisDll to be
	// incremented by the caller.

	if (pszPath = malloc(cchPath * sizeof(TCHAR)))
	{
		if (WStrToTStr(pszCmdLine, pszPath, (UINT)cchPath))
		{
			++g_cRefThisDll;
			HashVerifyThread(pszPath);
		}
		else
		{
			free(pszPath);
		}
	}
}

DWORD WINAPI HashVerifyThread( PTSTR pszPath )
{
	// We will need to free the memory allocated for the data when done
	PBYTE pbRawData;

	// First, activate our manifest and AddRef our host
	ULONG_PTR uActCtxCookie = ActivateManifest(TRUE);
	ULONG_PTR uHostCookie = HostAddRef();

	// Allocate the context data that will persist across this session
	HASHVERIFYCONTEXT hvctx;

	// It's important that we zero the memory since an initial value of zero is
	// assumed for many of the elements
	ZeroMemory(&hvctx, sizeof(hvctx));

	// Prep the path
	NormalizeString(pszPath);
	StrTrim(pszPath, TEXT(" "));
	hvctx.pszPath = pszPath;

	// Load the raw data
	pbRawData = HashVerifyLoadData(&hvctx);

	if (hvctx.pszFileData && (hvctx.hList = SLCreateEx(TRUE)))
	{
		HashVerifyParseData(&hvctx);

		DialogBoxParam(
			g_hModThisDll,
			MAKEINTRESOURCE(IDD_HASHVERF),
			NULL,
			HashVerifyDlgProc,
			(LPARAM)&hvctx
		);

		SLRelease(hvctx.hList);
	}
	else if (*pszPath)
	{
		// Technically, we could reach this point by either having a file read
		// error or a memory allocation error, but I really don't feel like
		// doing separate messages for what are supposed to be rare edge cases.
		TCHAR szFormat[MAX_STRINGRES], szMessage[0x100];
		LoadString(g_hModThisDll, IDS_HV_LOADERROR_FMT, szFormat, countof(szFormat));
		wnsprintf(szMessage, countof(szMessage), szFormat, pszPath);
		MessageBox(NULL, szMessage, NULL, MB_OK | MB_ICONERROR);
	}

	free(pbRawData);
	free(pszPath);

	// Clean up the manifest activation and release our host
	DeactivateManifest(uActCtxCookie);
	HostRelease(uHostCookie);

	InterlockedDecrement(&g_cRefThisDll);
	return(0);
}



/*============================================================================*\
	Data parsing functions
\*============================================================================*/

PBYTE WINAPI HashVerifyLoadData( PHASHVERIFYCONTEXT phvctx )
{
	PBYTE pbRawData = NULL;
	HANDLE hFile;

	if ((hFile = OpenFileForReading(phvctx->pszPath)) != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER cbRawData;
		DWORD cbBytesRead;

		if ( (GetFileSizeEx(hFile, &cbRawData)) &&
		     (pbRawData = malloc(cbRawData.LowPart + sizeof(DWORD))) &&
		     (ReadFile(hFile, pbRawData, cbRawData.LowPart, &cbBytesRead, NULL)) &&
		     (cbRawData.LowPart == cbBytesRead) )
		{
			// When we allocated a block of memory for the file data, we
			// reserved a DWORD at the end for NULL termination and to serve as
			// the extra buffer needed by IsTextUTF8...
			*((UPDWORD)(pbRawData + cbRawData.LowPart)) = 0;

			// Prepare the data for the parser...
			phvctx->pszFileData = BufferToWStr(&pbRawData, cbRawData.LowPart);
			NormalizeString(phvctx->pszFileData);
		}

		CloseHandle(hFile);
	}

	return(pbRawData);
}

VOID WINAPI HashVerifyParseData( PHASHVERIFYCONTEXT phvctx )
{
	PTSTR pszData = phvctx->pszFileData;  // Points to the next line to process

	UINT cchChecksum;             // Expected length of the checksum in TCHARs
	BOOL bReverseFormat = FALSE;  // TRUE if using SFV's format of putting the checksum last
	BOOL bLinesRemaining = TRUE;  // TRUE if we have not reached the end of the data

	// Try to determine the file type from the extension
	{
		PTSTR pszExt = StrRChr(phvctx->pszPath, NULL, TEXT('.'));

		if (pszExt)
		{
			if (StrCmpI(pszExt, TEXT(".sfv")) == 0)
			{
				phvctx->whctx.flags = WHEX_CHECKCRC32;
				cchChecksum = 8;
				bReverseFormat = TRUE;
			}
			else if (StrCmpI(pszExt, TEXT(".md4")) == 0)
			{
				phvctx->whctx.flags = WHEX_CHECKMD4;
				cchChecksum = 32;
			}
			else if (StrCmpI(pszExt, TEXT(".md5")) == 0)
			{
				phvctx->whctx.flags = WHEX_CHECKMD5;
				cchChecksum = 32;
			}
			else if (StrCmpI(pszExt - 1, TEXT(".sha1")) == 0)
			{
				phvctx->whctx.flags = WHEX_CHECKSHA1;
				cchChecksum = 40;
			}
		}
	}

	while (bLinesRemaining)
	{
		PTSTR pszStartOfLine;  // First non-whitespace character of the line
		PTSTR pszEndOfLine;    // Last non-whitespace character of the line
		PTSTR pszChecksum, pszFileName = NULL;
		INT16 cchPath;         // This INCLUDES the NULL terminator!

		// Step 1: Isolate the current line as a NULL-terminated string
		{
			pszStartOfLine = pszData;

			// Find the end of the line
			while (*pszData && *pszData != TEXT('\n'))
				++pszData;

			// Terminate it if necessary, otherwise flag the end of the data
			if (*pszData)
				*pszData = 0;
			else
				bLinesRemaining = FALSE;

			pszEndOfLine = pszData;

			// Strip spaces from the end of the line...
			while (--pszEndOfLine >= pszStartOfLine && *pszEndOfLine == TEXT(' '))
				*pszEndOfLine = 0;

			// ...and from the start of the line
			while (*pszStartOfLine == TEXT(' '))
				++pszStartOfLine;

			// Skip past this line's terminator; point at the remaining data
			++pszData;
		}

		// Step 2a: Parse the line as SFV
		if (bReverseFormat)
		{
			pszEndOfLine -= 7;

			if (pszEndOfLine > pszStartOfLine && ValidateHexSequence(pszEndOfLine, 8))
			{
				pszChecksum = pszEndOfLine;

				// Trim spaces between the checksum and the file name
				while (--pszEndOfLine >= pszStartOfLine && *pszEndOfLine == TEXT(' '))
					*pszEndOfLine = 0;

				// Lines that begin with ';' are comments in SFV
				if (*pszStartOfLine && *pszStartOfLine != TEXT(';'))
					pszFileName = pszStartOfLine;
			}
		}

		// Step 2b: All other file formats
		else
		{
			// If we do not know the type yet, make a stab at detecting it
			if (phvctx->whctx.flags == 0)
			{
				if (ValidateHexSequence(pszStartOfLine, 8))
				{
					cchChecksum = 8;
					phvctx->whctx.flags = WHEX_ALL32;  // WHEX_CHECKCRC32
				}
				else if (ValidateHexSequence(pszStartOfLine, 32))
				{
					cchChecksum = 32;
					phvctx->whctx.flags = WHEX_ALL128;  // WHEX_CHECKMD4 | WHEX_CHECKMD5

					// Disambiguate from the filename, if possible
					if (StrStrI(phvctx->pszPath, TEXT("MD5")))
						phvctx->whctx.flags = WHEX_CHECKMD5;
					else if (StrStrI(phvctx->pszPath, TEXT("MD4")))
						phvctx->whctx.flags = WHEX_CHECKMD4;
				}
				else if (ValidateHexSequence(pszStartOfLine, 40))
				{
					cchChecksum = 40;
					phvctx->whctx.flags = WHEX_ALL160;  // WHEX_CHECKSHA1
				}
			}

			// Parse the line
			if ( phvctx->whctx.flags && pszEndOfLine > pszStartOfLine + cchChecksum &&
			     ValidateHexSequence(pszStartOfLine, cchChecksum) )
			{
				pszChecksum = pszStartOfLine;
				pszStartOfLine += cchChecksum + 1;

				// Skip over spaces between the checksum and filename
				while (*pszStartOfLine == TEXT(' '))
					++pszStartOfLine;

				if (*pszStartOfLine)
					pszFileName = pszStartOfLine;
			}
		}

		// Step 3: Do something useful with the results
		if (pszFileName && (cchPath = (INT16)(pszEndOfLine + 2 - pszFileName)) > 1)
		{
			// Since pszEndOfLine points to the character BEFORE the terminator,
			// cchLine == 1 + pszEnd - pszStart, and then +1 for the NULL
			// terminator means that we need to add 2 TCHARs to the length

			// By treating cchPath as INT16 and checking the sign, we ensure
			// that the path does not exceed 32K.

			// Create the new data block
			PHASHVERIFYITEM pItem = SLAddItem(phvctx->hList, NULL, sizeof(HASHVERIFYITEM));

			// Abort if we are out of memory
			if (!pItem) break;

			pItem->filesize.ui64 = -1;
			pItem->filesize.sz[0] = 0;
			pItem->pszDisplayName = pszFileName;
			pItem->pszExpected = pszChecksum;
			pItem->cchDisplayName = cchPath;
			pItem->uStatusID = HV_STATUS_NULL;
			pItem->szActual[0] = 0;

			++phvctx->cTotal;

		} // If the current line was found to be valid

	} // Loop until there are no lines left

	// Build the index
	if ( phvctx->cTotal && (phvctx->index =
	     SLSetContextSize(phvctx->hList, phvctx->cTotal * sizeof(PHVITEM))) )
	{
		SLBuildIndex(phvctx->hList, phvctx->index);
	}
	else
	{
		phvctx->cTotal = 0;
	}
}

BOOL WINAPI ValidateHexSequence( PTSTR psz, UINT cch )
{
	// Check that the given hex string matches /[0-9A-Fa-f]{cch}\b/, and if it
	// does, convert to lower-case and NULL-terminate it.

	while (cch)
	{
		TCHAR ch = *psz;

		if (ch < TEXT('0'))
		{
			return(FALSE);
		}
		else if (ch > TEXT('9'))
		{
			ch |= 0x20; // Convert to lower-case

			if (ch < TEXT('a') || ch > TEXT('f'))
				return(FALSE);

			*psz = ch;
		}

		++psz;
		--cch;
	}

	if (*psz == 0 || *psz == TEXT('\n') || *psz == TEXT(' '))
	{
		*psz = 0;
		return(TRUE);
	}

	return(FALSE);
}



/*============================================================================*\
	Worker thread
\*============================================================================*/

VOID __fastcall HashVerifyWorkerMain( PHASHVERIFYCONTEXT phvctx )
{
	// Note that ALL message communication to and from the main window MUST
	// be asynchronous, or else there may be a deadlock

	PHASHVERIFYITEM pItem;

	// We need to keep track of the thread's execution time so that we can do a
	// sound notification of completion when appropriate
	DWORD dwTickStart = GetTickCount();

	// Initialize the path prefix length; used for building the full path
	PTSTR pszPathTail = StrRChr(phvctx->pszPath, NULL, TEXT('\\'));
	SIZE_T cchPathPrefix = (pszPathTail) ? pszPathTail + 1 - phvctx->pszPath : 0;

	while (pItem = SLGetDataAndStep(phvctx->hList))
	{
		BOOL bSuccess;

		// Part 1: Build the path
		{
			SIZE_T cchPrefix = cchPathPrefix;

			// Do not use the prefix if pszDisplayName is an absolute path
			if ( pItem->pszDisplayName[0] == TEXT('\\') ||
			     pItem->pszDisplayName[1] == TEXT(':') )
			{
				cchPrefix = 0;
			}

			SSChainNCpy2(
				phvctx->ex.pszPath,
				phvctx->pszPath, cchPrefix,
				pItem->pszDisplayName, pItem->cchDisplayName
			);
		}

		// Part 2: Calculate the checksum
		WorkerThreadHashFile(
			(PCOMMONCONTEXT)phvctx,
			phvctx->ex.pszPath,
			&bSuccess,
			&phvctx->whctx,
			NULL,
			&pItem->filesize
		);

		if (phvctx->status == CANCEL_REQUESTED)
			return;

		// Part 3: Do something with the results
		if (bSuccess)
		{
			if (phvctx->whctx.flags == WHEX_ALL128)
			{
				// If the MD4/MD5 STILL has not been settled by this point, then
				// settle it by a simple heuristic: if the checksum matches MD4,
				// go with that, otherwise default to MD5.

				if (StrCmpI(pItem->pszExpected, phvctx->whctx.results.szHexMD4) == 0)
					phvctx->whctx.flags = WHEX_CHECKMD4;
				else
					phvctx->whctx.flags = WHEX_CHECKMD5;
			}

			switch (phvctx->whctx.flags)
			{
				case WHEX_CHECKCRC32:
					SSStaticCpy(pItem->szActual, phvctx->whctx.results.szHexCRC32);
					break;
				case WHEX_CHECKMD4:
					SSStaticCpy(pItem->szActual, phvctx->whctx.results.szHexMD4);
					break;
				case WHEX_CHECKMD5:
					SSStaticCpy(pItem->szActual, phvctx->whctx.results.szHexMD5);
					break;
				case WHEX_CHECKSHA1:
					SSStaticCpy(pItem->szActual, phvctx->whctx.results.szHexSHA1);
					break;
			}

			if (StrCmpI(pItem->pszExpected, pItem->szActual) == 0)
				pItem->uStatusID = HV_STATUS_MATCH;
			else
				pItem->uStatusID = HV_STATUS_MISMATCH;
		}
		else
		{
			pItem->uStatusID = HV_STATUS_UNREADABLE;
		}

		// Part 4: Update the UI
		++phvctx->cSentMsgs;
		PostMessage(phvctx->hWnd, HM_WORKERTHREAD_UPDATE, (WPARAM)phvctx, (LPARAM)pItem);
	}

	// Play a sound to signal the normal, successful termination of operations,
	// but exempt operations that were nearly instantaneous
	if (phvctx->cTotal && GetTickCount() - dwTickStart >= 2000)
		MessageBeep(MB_ICONASTERISK);
}



/*============================================================================*\
	Dialog general
\*============================================================================*/

INT_PTR CALLBACK HashVerifyDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	PHASHVERIFYCONTEXT phvctx;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			phvctx = (PHASHVERIFYCONTEXT)lParam;

			// Associate the window with the context and vice-versa
			phvctx->hWnd = hWnd;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)phvctx);

			SetAppIDForWindow(hWnd, TRUE);

			HashVerifyDlgInit(phvctx);

			phvctx->ex.pfnWorkerMain = HashVerifyWorkerMain;
			phvctx->hThread = CreateThreadCRT(NULL, phvctx);

			if (!phvctx->hThread)
				WorkerThreadCleanup((PCOMMONCONTEXT)phvctx);

			// Initialize the summary
			SendMessage(phvctx->hWndPBTotal, PBM_SETRANGE32, 0, phvctx->cTotal);
			HashVerifyUpdateSummary(phvctx, NULL);

			return(TRUE);
		}

		case WM_DESTROY:
		{
			SetAppIDForWindow(hWnd, FALSE);
			break;
		}

		case WM_ENDSESSION:
		case WM_CLOSE:
		{
			phvctx = (PHASHVERIFYCONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);
			goto cleanup_and_exit;
		}

		case WM_COMMAND:
		{
			phvctx = (PHASHVERIFYCONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);

			switch (LOWORD(wParam))
			{
				case IDC_PAUSE:
				{
					WorkerThreadTogglePause((PCOMMONCONTEXT)phvctx);
					return(TRUE);
				}

				case IDC_STOP:
				{
					WorkerThreadStop((PCOMMONCONTEXT)phvctx);
					return(TRUE);
				}

				case IDC_EXIT:
				{
					cleanup_and_exit:
					phvctx->dwFlags |= HCF_EXIT_PENDING;
					WorkerThreadStop((PCOMMONCONTEXT)phvctx);
					WorkerThreadCleanup((PCOMMONCONTEXT)phvctx);
					EndDialog(hWnd, 0);
					break;
				}
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR pnm = (LPNMHDR)lParam;

			if (pnm && pnm->idFrom == IDC_LIST)
			{
				phvctx = (PHASHVERIFYCONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);

				switch (pnm->code)
				{
					case LVN_GETDISPINFO:
					{
						HashVerifyListInfo(phvctx, (LPNMLVDISPINFO)lParam);
						return(TRUE);
					}
					case NM_CUSTOMDRAW:
					{
						SetWindowLongPtr(hWnd, DWLP_MSGRESULT, HashVerifySetColor(phvctx, (LPNMLVCUSTOMDRAW)lParam));
						return(TRUE);
					}
					case LVN_ODFINDITEM:
					{
						SetWindowLongPtr(hWnd, DWLP_MSGRESULT, HashVerifyFindItem(phvctx, (LPNMLVFINDITEM)lParam));
						return(TRUE);
					}
					case LVN_COLUMNCLICK:
					{
						HashVerifySortColumn(phvctx, (LPNMLISTVIEW)lParam);
						return(TRUE);
					}
					case LVN_ITEMCHANGED:
					{
						if (((LPNMLISTVIEW)lParam)->uChanged & LVIF_STATE)
							phvctx->bFreshStates = FALSE;
						break;
					}
					case LVN_ODSTATECHANGED:
					{
						phvctx->bFreshStates = FALSE;
						break;
					}
				}
			}

			break;
		}

		case WM_TIMER:
		{
			// Vista: Workaround to fix their buggy progress bar
			KillTimer(hWnd, TIMER_ID_PAUSE);
			phvctx = (PHASHVERIFYCONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);
			if (phvctx->status == PAUSED)
				SetProgressBarPause((PCOMMONCONTEXT)phvctx, PBST_PAUSED);
			return(TRUE);
		}

		case HM_WORKERTHREAD_DONE:
		{
			phvctx = (PHASHVERIFYCONTEXT)wParam;
			WorkerThreadCleanup((PCOMMONCONTEXT)phvctx);
			return(TRUE);
		}

		case HM_WORKERTHREAD_UPDATE:
		{
			phvctx = (PHASHVERIFYCONTEXT)wParam;
			++phvctx->cHandledMsgs;
			HashVerifyUpdateSummary(phvctx, (PHASHVERIFYITEM)lParam);
			return(TRUE);
		}

		case HM_WORKERTHREAD_SETSIZE:
		{
			phvctx = (PHASHVERIFYCONTEXT)wParam;

			// At the time we receive this message, cSentMsgs will be the ID of
			// the item the worker thread is currently working on, and
			// cHandledMsgs will be the ID of the item for which the SETSIZE
			// message was intended for; we need to update the UI only if
			// the worker thread is still working on this item when we process
			// this message; otherwise, we can just wait for the UPDATE message.

			if (phvctx->cSentMsgs == phvctx->cHandledMsgs)
				ListView_RedrawItems(phvctx->hWndList, phvctx->cHandledMsgs, phvctx->cHandledMsgs);

			return(TRUE);
		}
	}

	return(FALSE);
}

VOID WINAPI HashVerifyDlgInit( PHASHVERIFYCONTEXT phvctx )
{
	HWND hWnd = phvctx->hWnd;
	UINT i;

	// Load strings
	{
		static const UINT16 arStrMap[][2] =
		{
			{ IDC_SUMMARY,          IDS_HV_SUMMARY    },
			{ IDC_MATCH_LABEL,      IDS_HV_MATCH      },
			{ IDC_MISMATCH_LABEL,   IDS_HV_MISMATCH   },
			{ IDC_UNREADABLE_LABEL, IDS_HV_UNREADABLE },
			{ IDC_PENDING_LABEL,    IDS_HV_PENDING    },
			{ IDC_PAUSE,            IDS_HV_PAUSE      },
			{ IDC_STOP,             IDS_HV_STOP       },
			{ IDC_EXIT,             IDS_HV_EXIT       }
		};

		for (i = 0; i < countof(arStrMap); ++i)
			SetControlText(hWnd, arStrMap[i][0], arStrMap[i][1]);
	}

	// Set the window icon and title
	{
		PTSTR pszFileName = StrRChr(phvctx->pszPath, NULL, TEXT('\\'));

		if (!(pszFileName && *++pszFileName))
			pszFileName = phvctx->pszPath;

		SendMessage(
			hWnd,
			WM_SETTEXT,
			0,
			(LPARAM)pszFileName
		);

		SendMessage(
			hWnd,
			WM_SETICON,
			ICON_BIG, // No need to explicitly set the small icon
			(LPARAM)LoadIcon(g_hModThisDll, MAKEINTRESOURCE(IDI_FILETYPE))
		);
	}

	// Initialize the list box
	{
		typedef struct {
			UINT16 iStringID;
			UINT16 iAlign;
			UINT16 iWidth;
		} COLINFO, *PCOLINFO;

		static const COLINFO arCols[] =
		{
			{ IDS_HV_COL_FILENAME, LVCFMT_LEFT,  245 },
			{ IDS_HV_COL_SIZE,     LVCFMT_RIGHT,  64 },
			{ IDS_HV_COL_STATUS,   LVCFMT_CENTER, 64 },
			{ IDS_HV_COL_EXPECTED, LVCFMT_CENTER,  0 },
			{ IDS_HV_COL_ACTUAL,   LVCFMT_CENTER,  0 },
		};

		// We will be using the list window handle a lot throughout HashVerify,
		// so we should cache it to reduce the number of lookups
		phvctx->hWndList = GetDlgItem(hWnd, IDC_LIST);

		for (i = 0; i < countof(arCols); ++i)
		{
			TCHAR szBuffer[MAX_STRINGRES];
			LVCOLUMN lvc;
			RECT rc;

			LoadString(g_hModThisDll, arCols[i].iStringID, szBuffer, countof(szBuffer));

			rc.left = arCols[i].iWidth;

			if (rc.left == 0)
			{
				if (phvctx->whctx.flags == WHEX_CHECKCRC32)
					rc.left =  8 * 4 + 20 + 40;  // extra size to accommodate the header labels
				else if (phvctx->whctx.flags == WHEX_CHECKSHA1)
					rc.left = 40 * 4 + 20;
				else if (phvctx->whctx.flags & WHEX_ALL128)
					rc.left = 32 * 4 + 20;
			}

			MapDialogRect(hWnd, &rc);

			lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
			lvc.fmt = arCols[i].iAlign;
			lvc.cx = rc.left;
			lvc.pszText = szBuffer;

			ListView_InsertColumn(phvctx->hWndList, i, &lvc);
		}

		ListView_SetExtendedListViewStyle(phvctx->hWndList, LISTVIEW_EXSTYLES);
		ListView_SetItemCount(phvctx->hWndList, phvctx->cTotal);

		// Use the new-fangled list view style for Vista
		if (g_uWinVer >= 0x0600)
			SetWindowTheme(phvctx->hWndList, L"Explorer", NULL);

		phvctx->sort.iColumn = -1;
	}

	// Initialize the status strings
	{
		UINT i;

		for (i = 1; i <= 3; ++i)
		{
			LoadString(
				g_hModThisDll,
				i + (IDS_HV_STATUS_MATCH - 1),
				phvctx->szStatus[i],
				countof(phvctx->szStatus[i])
			);
		}
	}

	// Initialize miscellaneous stuff
	{
		phvctx->uMaxBatch = (phvctx->cTotal < (0x20 << 8)) ? 0x20 : phvctx->cTotal >> 8;
	}
}



/*============================================================================*\
	Dialog status
\*============================================================================*/

VOID WINAPI HashVerifyUpdateSummary( PHASHVERIFYCONTEXT phvctx, PHASHVERIFYITEM pItem )
{
	HWND hWnd = phvctx->hWnd;
	TCHAR szFormat[MAX_STRINGRES], szBuffer[MAX_STRINGMSG];

	// If this is not the initial update and we are lagging, and our update
	// drought is not TOO long, then we should skip the update...
	BOOL bUpdateUI = !(pItem && phvctx->cSentMsgs > phvctx->cHandledMsgs &&
	                   phvctx->cHandledMsgs < phvctx->prev.idStartOfDirtyRange + phvctx->uMaxBatch);

	// Update the list
	if (pItem)
	{
		switch (pItem->uStatusID)
		{
			case HV_STATUS_MATCH:
				++phvctx->cMatch;
				break;
			case HV_STATUS_MISMATCH:
				++phvctx->cMismatch;
				break;
			default:
				++phvctx->cUnreadable;
		}

		if (bUpdateUI)
		{
			ListView_RedrawItems(
				phvctx->hWndList,
				phvctx->prev.idStartOfDirtyRange,
				phvctx->cHandledMsgs - 1
			);
		}
	}

	// Update the counts and progress bar
	if (bUpdateUI)
	{
		// FormatFractionalResults expects an empty format buffer on the first call
		szFormat[0] = 0;

		if (!pItem || phvctx->prev.cMatch != phvctx->cMatch)
		{
			FormatFractionalResults(szFormat, szBuffer, phvctx->cMatch, phvctx->cTotal);
			SetDlgItemText(hWnd, IDC_MATCH_RESULTS, szBuffer);
		}

		if (!pItem || phvctx->prev.cMismatch != phvctx->cMismatch)
		{
			FormatFractionalResults(szFormat, szBuffer, phvctx->cMismatch, phvctx->cTotal);
			SetDlgItemText(hWnd, IDC_MISMATCH_RESULTS, szBuffer);
		}

		if (!pItem || phvctx->prev.cUnreadable != phvctx->cUnreadable)
		{
			FormatFractionalResults(szFormat, szBuffer, phvctx->cUnreadable, phvctx->cTotal);
			SetDlgItemText(hWnd, IDC_UNREADABLE_RESULTS, szBuffer);
		}

		FormatFractionalResults(szFormat, szBuffer, phvctx->cTotal - phvctx->cHandledMsgs, phvctx->cTotal);
		SetDlgItemText(hWnd, IDC_PENDING_RESULTS, szBuffer);

		SendMessage(phvctx->hWndPBTotal, PBM_SETPOS, phvctx->cHandledMsgs, 0);

		// Now that we've updated the UI, update the prev structure
		phvctx->prev.idStartOfDirtyRange = phvctx->cHandledMsgs;
		phvctx->prev.cMatch = phvctx->cMatch;
		phvctx->prev.cMismatch = phvctx->cMismatch;
		phvctx->prev.cUnreadable = phvctx->cUnreadable;
	}

	// Update the header
	if (!(phvctx->dwFlags & HVF_HAS_SET_TYPE))
	{
		PCTSTR pszSubtitle = NULL;

		switch (phvctx->whctx.flags)
		{
			case WHEX_CHECKCRC32: pszSubtitle = TEXT("CRC-32"); break;
			case WHEX_CHECKMD4:   pszSubtitle = TEXT("MD4");    break;
			case WHEX_CHECKMD5:   pszSubtitle = TEXT("MD5");    break;
			case WHEX_CHECKSHA1:  pszSubtitle = TEXT("SHA-1");  break;
		}

		if (pszSubtitle)
		{
			LoadString(g_hModThisDll, IDS_HV_SUMMARY, szFormat, countof(szFormat));
			wnsprintf(szBuffer, countof(szBuffer), TEXT("%s (%s)"), szFormat, pszSubtitle);
			SetDlgItemText(hWnd, IDC_SUMMARY, szBuffer);
			phvctx->dwFlags |= HVF_HAS_SET_TYPE;
		}
	}
}



/*============================================================================*\
	List management
\*============================================================================*/

VOID WINAPI HashVerifyListInfo( PHASHVERIFYCONTEXT phvctx, LPNMLVDISPINFO pdi )
{
	if ((UINT)pdi->item.iItem >= phvctx->cTotal)
		return;  // Invalid index; by casting to unsigned, we also catch negatives

	if (pdi->item.mask & LVIF_TEXT)
	{
		PHASHVERIFYITEM pItem = phvctx->index[pdi->item.iItem];

		switch (pdi->item.iSubItem)
		{
			case HV_COL_FILENAME: pdi->item.pszText = pItem->pszDisplayName;              break;
			case HV_COL_SIZE:     pdi->item.pszText = pItem->filesize.sz;                 break;
			case HV_COL_STATUS:   pdi->item.pszText = phvctx->szStatus[pItem->uStatusID]; break;
			case HV_COL_EXPECTED: pdi->item.pszText = pItem->pszExpected;                 break;
			case HV_COL_ACTUAL:   pdi->item.pszText = pItem->szActual;                    break;
			default:              pdi->item.pszText = TEXT("");                           break;
		}
	}

	if (pdi->item.mask & LVIF_IMAGE)
		pdi->item.iImage = I_IMAGENONE;

	// We can (and should) ignore LVIF_STATE
}

LONG_PTR WINAPI HashVerifySetColor( PHASHVERIFYCONTEXT phvctx, LPNMLVCUSTOMDRAW pcd )
{
	switch (pcd->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
			return(CDRF_NOTIFYITEMDRAW);

		case CDDS_ITEMPREPAINT:
		{
			// We need to determine the highlight state during the item stage
			// because this information becomes subitem-specific if we try to
			// retrieve it when we actually need it in the subitem stage

			if (g_uWinVer >= 0x0600 && IsAppThemed())
			{
				// Clear the highlight bit...
				phvctx->dwFlags &= ~HVF_ITEM_HILITE;

				// uItemState is buggy; if LVS_SHOWSELALWAYS is set, uItemState
				// will ALWAYS have the CDIS_SELECTED bit set, regardless of
				// whether the item is actually selected, so a more expensive
				// test for the LVIS_SELECTED bit is needed...
				if ( pcd->nmcd.uItemState & CDIS_HOT ||
				     ListView_GetItemState(pcd->nmcd.hdr.hwndFrom, pcd->nmcd.dwItemSpec, LVIS_SELECTED) )
				{
					phvctx->dwFlags |= HVF_ITEM_HILITE;
				}
			}

			return(CDRF_NOTIFYSUBITEMDRAW);
		}

		case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		{
			PHASHVERIFYITEM pItem;

			if (pcd->nmcd.dwItemSpec >= phvctx->cTotal)
				break;  // Invalid index

			pItem = phvctx->index[pcd->nmcd.dwItemSpec];

			// By default, we use the default foreground and background colors
			// except when the item is a mismatch or is unreadable, in which
			// case, we change the foreground color
			switch (pItem->uStatusID)
			{
				case HV_STATUS_MISMATCH:
					pcd->clrText = RGB(0xC0, 0x00, 0x00);
					break;

				case HV_STATUS_UNREADABLE:
					pcd->clrText = RGB(0x80, 0x80, 0x80);
					break;

				default:
					pcd->clrText = CLR_DEFAULT;
			}

			pcd->clrTextBk = CLR_DEFAULT;

			// The status column, however, deserves special treatment
			if (pcd->iSubItem == HV_COL_STATUS)
			{
				if (phvctx->dwFlags & HVF_ITEM_HILITE)
				{
					// Vista-style highlighting means that the foreground
					// color can show through, but not the background color
					if (pItem->uStatusID == HV_STATUS_MATCH)
						pcd->clrText = RGB(0x00, 0x80, 0x00);
				}
				else
				{
					switch (pItem->uStatusID)
					{
						case HV_STATUS_MATCH:
							pcd->clrText = RGB(0x00, 0x00, 0x00);
							pcd->clrTextBk = RGB(0x00, 0xE0, 0x00);
							break;

						case HV_STATUS_MISMATCH:
							pcd->clrText = RGB(0xFF, 0xFF, 0xFF);
							pcd->clrTextBk = RGB(0xC0, 0x00, 0x00);
							break;

						case HV_STATUS_UNREADABLE:
							pcd->clrText = RGB(0x00, 0x00, 0x00);
							pcd->clrTextBk = RGB(0xFF, 0xE0, 0x00);
							break;
					}
				}
			}

			break;
		}
	}

	return(CDRF_DODEFAULT);
}

LONG_PTR WINAPI HashVerifyFindItem( PHASHVERIFYCONTEXT phvctx, LPNMLVFINDITEM pfi )
{
	PHASHVERIFYITEM pItem;
	INT cchCompare, iStart = pfi->iStart;
	LONG_PTR i;

	if (pfi->lvfi.flags & (LVFI_PARAM | LVFI_NEARESTXY))
		goto not_found;  // Unsupported search types

	if (!(pfi->lvfi.flags & (LVFI_PARTIAL | LVFI_STRING)))
		goto not_found;  // No valid search type specified

	// According to the documentation, LVFI_STRING without a corresponding
	// LVFI_PARTIAL should match the FULL string, but when the user sends
	// keyboard input (which uses a partial match), the notification does not
	// have the LVFI_PARTIAL flag, so we should just always assume LVFI_PARTIAL
	// INT cchCompare = (pfi->lvfi.flags & LVFI_PARTIAL) ? 0 : 1;
	// cchCompare += SSLen(pfi->lvfi.psz);
	// The above code should have been correct, but it is not...
	cchCompare = (INT)SSLen(pfi->lvfi.psz);

	// Fix out-of-range indices; by casting to unsigned, we also catch negatives
	if ((UINT)iStart > phvctx->cTotal)
		iStart = phvctx->cTotal;

	for (i = iStart; i < (INT)phvctx->cTotal; ++i)
	{
		pItem = phvctx->index[i];
		if (StrCmpNI(pItem->pszDisplayName, pfi->lvfi.psz, cchCompare) == 0)
			return(i);
	}

	if (pfi->lvfi.flags & LVFI_WRAP)
	{
		for (i = 0; i < iStart; ++i)
		{
			pItem = phvctx->index[i];
			if (StrCmpNI(pItem->pszDisplayName, pfi->lvfi.psz, cchCompare) == 0)
				return(i);
		}
	}

	not_found: return(-1);
}

VOID WINAPI HashVerifySortColumn( PHASHVERIFYCONTEXT phvctx, LPNMLISTVIEW plv )
{
	if (phvctx->status != CLEANUP_COMPLETED)
		return;  // Sorting is available only after the worker is done

	// Capture the current selection/focus state
	HashVerifyReadStates(phvctx);

	if (phvctx->sort.iColumn != plv->iSubItem)
	{
		// Change to a new column
		phvctx->sort.iColumn = plv->iSubItem;
		phvctx->sort.bReverse = FALSE;
		qsort_s_uptr(phvctx->index, phvctx->cTotal, sizeof(PHVITEM), HashVerifySortCompare, phvctx);
	}
	else if (phvctx->sort.bReverse)
	{
		// Clicking a column thrice in a row reverts to the original file order
		phvctx->sort.iColumn = -1;
		phvctx->sort.bReverse = FALSE;

		// We do need to validate phvctx->index to handle the edge case where
		// the list is really non-empty, but we are treating it as empty because
		// we could not allocate an index (qsort_s uses the given length while
		// SLBuildIndex uses the actual length); this is, admittedly, a very
		// extreme edge case, as it crops up only in an OOM situation where the
		// user tries to click-sort an empty list view!
		if (phvctx->index)
			SLBuildIndex(phvctx->hList, phvctx->index);
	}
	else
	{
		// Clicking a column twice in a row reverses the order; since we are
		// just reversing the order of an already-sorted column, we can just
		// naively flip the index

		if (phvctx->index)
		{
			PHVITEM pItemTemp;
			PPHVITEM ppItemLow = phvctx->index;
			PPHVITEM ppItemHigh = phvctx->index + phvctx->cTotal - 1;

			while (ppItemHigh > ppItemLow)
			{
				pItemTemp = *ppItemLow;
				*ppItemLow = *ppItemHigh;
				*ppItemHigh = pItemTemp;
				++ppItemLow;
				--ppItemHigh;
			}
		}

		phvctx->sort.bReverse = TRUE;
	}

	// Restore the selection/focus state
	HashVerifySetStates(phvctx);

	// Update the UI
	{
		HWND hWndHeader = ListView_GetHeader(phvctx->hWndList);
		INT i;

		HDITEM hdi;
		hdi.mask = HDI_FORMAT;

		for (i = HV_COL_FIRST; i <= HV_COL_LAST; ++i)
		{
			Header_GetItem(hWndHeader, i, &hdi);
			hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
			if (phvctx->sort.iColumn == i)
				hdi.fmt |= (phvctx->sort.bReverse) ? HDF_SORTDOWN : HDF_SORTUP;
			Header_SetItem(hWndHeader, i, &hdi);
		}

		// Invalidate all items
		ListView_RedrawItems(phvctx->hWndList, 0, phvctx->cTotal);

		// Set a light gray background on the sorted column
		ListView_SetSelectedColumn(
			phvctx->hWndList,
			(phvctx->sort.iColumn != HV_COL_STATUS) ? phvctx->sort.iColumn : -1
		);

		// Unfortunately, the list does not automatically repaint all of the
		// areas affected by SetSelectedColumn, so it is necessary to force a
		// repaint of the list view's visible areas in order to avoid artifacts
		InvalidateRect(phvctx->hWndList, NULL, FALSE);
	}
}

VOID WINAPI HashVerifyReadStates( PHASHVERIFYCONTEXT phvctx )
{
	if (!phvctx->bFreshStates)
	{
		UINT i;

		for (i = 0; i < phvctx->cTotal; ++i)
		{
			phvctx->index[i]->uState = ListView_GetItemState(
				phvctx->hWndList,
				i,
				LVIS_FOCUSED | LVIS_SELECTED
			);
		}
	}
}

VOID WINAPI HashVerifySetStates( PHASHVERIFYCONTEXT phvctx )
{
	UINT i;

	// Optimize for the case where most items are unselected
	ListView_SetItemState(phvctx->hWndList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);

	for (i = 0; i < phvctx->cTotal; ++i)
	{
		if (phvctx->index[i]->uState)
		{
			ListView_SetItemState(
				phvctx->hWndList,
				i,
				phvctx->index[i]->uState,
				LVIS_FOCUSED | LVIS_SELECTED
			);
		}
	}

	phvctx->bFreshStates = TRUE;
}

INT WINAPI Compare64( PLONGLONG p64A, PLONGLONG p64B )
{
	LARGE_INTEGER diff;
	diff.QuadPart = *p64A - *p64B;
	if (diff.HighPart)
		return(diff.HighPart);
	else
		return(diff.LowPart > 0);
}

INT __cdecl HashVerifySortCompare( PHASHVERIFYCONTEXT phvctx, PPCHVITEM ppItemA, PPCHVITEM ppItemB )
{
	PHASHVERIFYITEM pItemA = *(PPHVITEM)ppItemA;
	PHASHVERIFYITEM pItemB = *(PPHVITEM)ppItemB;

	switch (phvctx->sort.iColumn)
	{
		case HV_COL_FILENAME:
			return(StrCmpLogical(pItemA->pszDisplayName, pItemB->pszDisplayName));

		case HV_COL_SIZE:
			return(Compare64(&pItemA->filesize.ui64, &pItemB->filesize.ui64));

		case HV_COL_STATUS:
			return((INT8)pItemA->uStatusID - (INT8)pItemB->uStatusID);

		case HV_COL_EXPECTED:
			return(StrCmpI(pItemA->pszExpected, pItemB->pszExpected));

		case HV_COL_ACTUAL:
			return(StrCmpI(pItemA->szActual, pItemB->szActual));
	}

	return(0);
}
