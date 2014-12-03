/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "globals.h"
#include "HashCheckCommon.h"
#include "HashCalc.h"
#include "SetAppID.h"

// Control structures, from HashCalc.h
#define  HASHSAVESCRATCH  HASHCALCSCRATCH
#define PHASHSAVESCRATCH PHASHCALCSCRATCH
#define  HASHSAVECONTEXT  HASHCALCCONTEXT
#define PHASHSAVECONTEXT PHASHCALCCONTEXT
#define  HASHSAVEITEM     HASHCALCITEM
#define PHASHSAVEITEM    PHASHCALCITEM



/*============================================================================*\
	Function declarations
\*============================================================================*/

// Entry points / main functions
DWORD WINAPI HashSaveThread( PHASHSAVECONTEXT phsctx );

// Worker thread
VOID __fastcall HashSaveWorkerMain( PHASHSAVECONTEXT phsctx );

// Dialog general
INT_PTR CALLBACK HashSaveDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
VOID WINAPI HashSaveDlgInit( PHASHSAVECONTEXT phsctx );



/*============================================================================*\
	Entry points / main functions
\*============================================================================*/

VOID WINAPI HashSaveStart( HWND hWndOwner, HSIMPLELIST hListRaw )
{
	// Explorer will be blocking as long as this function is running, so we
	// want to return as quickly as possible and leave the work up to the
	// thread that we are going to spawn

	PHASHSAVECONTEXT phsctx = SLSetContextSize(hListRaw, sizeof(HASHSAVECONTEXT));

	if (phsctx)
	{
		HANDLE hThread;

		phsctx->hWnd = hWndOwner;
		phsctx->hListRaw = hListRaw;

		InterlockedIncrement(&g_cRefThisDll);
		SLAddRef(hListRaw);

		if (hThread = CreateThreadCRT(HashSaveThread, phsctx))
		{
			CloseHandle(hThread);
			return;
		}

		// If the thread creation was successful, the thread will be
		// responsible for decrementing the ref count
		SLRelease(hListRaw);
		InterlockedDecrement(&g_cRefThisDll);
	}
}

DWORD WINAPI HashSaveThread( PHASHSAVECONTEXT phsctx )
{
	// First, activate our manifest and AddRef our host
	ULONG_PTR uActCtxCookie = ActivateManifest(TRUE);
	ULONG_PTR uHostCookie = HostAddRef();

	// Calling HashCalcPrepare with a NULL hList will cause it to calculate
	// and set cchPrefix, but it will not copy the data or walk the directories
	// (we will leave that for the worker thread to do); the reason we do a
	// limited scan now is so that we can show the file dialog (which requires
	// cchPrefix for the automatic name generation) as soon as possible
	phsctx->status = INACTIVE;
	phsctx->hList = NULL;
	HashCalcPrepare(phsctx);

	// Get a file name from the user
	ZeroMemory(&phsctx->ofn, sizeof(phsctx->ofn));
	HashCalcInitSave(phsctx);

	if (phsctx->hFileOut != INVALID_HANDLE_VALUE)
	{
		if (phsctx->hList = SLCreateEx(TRUE))
		{
			DialogBoxParam(
				g_hModThisDll,
				MAKEINTRESOURCE(IDD_HASHSAVE),
				NULL,
				HashSaveDlgProc,
				(LPARAM)phsctx
			);

			SLRelease(phsctx->hList);
		}

		CloseHandle(phsctx->hFileOut);
	}

	// This must be the last thing that we free, since this is what supports
	// our context!
	SLRelease(phsctx->hListRaw);

	// Clean up the manifest activation and release our host
	DeactivateManifest(uActCtxCookie);
	HostRelease(uHostCookie);

	InterlockedDecrement(&g_cRefThisDll);
	return(0);
}



/*============================================================================*\
	Worker thread
\*============================================================================*/

VOID __fastcall HashSaveWorkerMain( PHASHSAVECONTEXT phsctx )
{
	// Note that ALL message communication to and from the main window MUST
	// be asynchronous, or else there may be a deadlock.

	PHASHSAVEITEM pItem;

	// Prep: expand directories, max path, etc. (prefix was set by earlier call)
	PostMessage(phsctx->hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM)phsctx, TRUE);
	HashCalcPrepare(phsctx);
	PostMessage(phsctx->hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM)phsctx, FALSE);

	// Indicate which hash type we are after
	// WHEX_CHECKCRC32 = 0x01 = 1 << 0
	// WHEX_CHECKMD4   = 0x02 = 1 << 1
	// WHEX_CHECKMD5   = 0x04 = 1 << 2
	// WHEX_CHECKSHA1  = 0x08 = 1 << 3
	phsctx->whctx.flags = 1 << (phsctx->ofn.nFilterIndex - 1);

	while (pItem = SLGetDataAndStep(phsctx->hList))
	{
		// Get the hash
		WorkerThreadHashFile(
			(PCOMMONCONTEXT)phsctx,
			pItem->szPath,
			&pItem->bValid,
			&phsctx->whctx,
			&pItem->results,
			NULL
		);

		if (phsctx->status == CANCEL_REQUESTED)
			return;

		// Write the data
		HashCalcWriteResult(phsctx, pItem);

		// Update the UI
		++phsctx->cSentMsgs;
		PostMessage(phsctx->hWnd, HM_WORKERTHREAD_UPDATE, (WPARAM)phsctx, (LPARAM)pItem);
	}
}



/*============================================================================*\
	Dialog general
\*============================================================================*/

INT_PTR CALLBACK HashSaveDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	PHASHSAVECONTEXT phsctx;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			phsctx = (PHASHSAVECONTEXT)lParam;

			// Associate the window with the context and vice-versa
			phsctx->hWnd = hWnd;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)phsctx);

			SetAppIDForWindow(hWnd, TRUE);

			HashSaveDlgInit(phsctx);

			phsctx->ex.pfnWorkerMain = HashSaveWorkerMain;
			phsctx->hThread = CreateThreadCRT(NULL, phsctx);

			if (!phsctx->hThread || WaitForSingleObject(phsctx->hThread, 1000) != WAIT_TIMEOUT)
			{
				WorkerThreadCleanup((PCOMMONCONTEXT)phsctx);
				EndDialog(hWnd, 0);
			}

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
			phsctx = (PHASHSAVECONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);
			goto cleanup_and_exit;
		}

		case WM_COMMAND:
		{
			phsctx = (PHASHSAVECONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);

			switch (LOWORD(wParam))
			{
				case IDC_PAUSE:
				{
					WorkerThreadTogglePause((PCOMMONCONTEXT)phsctx);
					return(TRUE);
				}

				case IDC_CANCEL:
				{
					cleanup_and_exit:
					phsctx->dwFlags |= HCF_EXIT_PENDING;
					WorkerThreadStop((PCOMMONCONTEXT)phsctx);
					WorkerThreadCleanup((PCOMMONCONTEXT)phsctx);
					EndDialog(hWnd, 0);
					break;
				}
			}

			break;
		}

		case WM_TIMER:
		{
			// Vista: Workaround to fix their buggy progress bar
			KillTimer(hWnd, TIMER_ID_PAUSE);
			phsctx = (PHASHSAVECONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);
			if (phsctx->status == PAUSED)
				SetProgressBarPause((PCOMMONCONTEXT)phsctx, PBST_PAUSED);
			return(TRUE);
		}

		case HM_WORKERTHREAD_DONE:
		{
			phsctx = (PHASHSAVECONTEXT)wParam;
			WorkerThreadCleanup((PCOMMONCONTEXT)phsctx);
			EndDialog(hWnd, 0);
			return(TRUE);
		}

		case HM_WORKERTHREAD_UPDATE:
		{
			phsctx = (PHASHSAVECONTEXT)wParam;
			++phsctx->cHandledMsgs;
			SendMessage(phsctx->hWndPBTotal, PBM_SETPOS, phsctx->cHandledMsgs, 0);
			return(TRUE);
		}

		case HM_WORKERTHREAD_TOGGLEPREP:
		{
			HashCalcTogglePrep((PHASHSAVECONTEXT)wParam, (BOOL)lParam);
			return(TRUE);
		}
	}

	return(FALSE);
}

VOID WINAPI HashSaveDlgInit( PHASHSAVECONTEXT phsctx )
{
	HWND hWnd = phsctx->hWnd;

	// Load strings
	{
		SetControlText(hWnd, IDC_PAUSE, IDS_HS_PAUSE);
		SetControlText(hWnd, IDC_CANCEL, IDS_HS_CANCEL);
	}

	// Set the window icon and title
	{
		PTSTR pszFileName = phsctx->ofn.lpstrFile + phsctx->ofn.nFileOffset;
		TCHAR szFormat[MAX_STRINGRES];
		LoadString(g_hModThisDll, IDS_HS_TITLE_FMT, szFormat, countof(szFormat));
		wnsprintf(phsctx->scratch.sz, countof(phsctx->scratch.sz), szFormat, pszFileName);

		SendMessage(
			hWnd,
			WM_SETTEXT,
			0,
			(LPARAM)phsctx->scratch.sz
		);

		SendMessage(
			hWnd,
			WM_SETICON,
			ICON_BIG, // No need to explicitly set the small icon
			(LPARAM)LoadIcon(g_hModThisDll, MAKEINTRESOURCE(IDI_FILETYPE))
		);
	}

	// Initialize miscellaneous stuff
	{
		phsctx->dwFlags = 0;
		phsctx->cTotal = 0;
	}
}
