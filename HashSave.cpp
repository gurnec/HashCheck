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
#include "IsSSD.h"
#include <Strsafe.h>
#include <vector>
#include <cassert>
#include <algorithm>
#ifdef USE_PPL
#include <ppl.h>
#include <concurrent_vector.h>
#endif

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

VOID WINAPI HashSaveStart( HWND hWndOwner, HSIMPLELIST hListRaw, BOOL bSeparateFiles )
{
	// Explorer will be blocking as long as this function is running, so we
	// want to return as quickly as possible and leave the work up to the
	// thread that we are going to spawn

	PHASHSAVECONTEXT phsctx = (PHASHSAVECONTEXT)SLSetContextSize(hListRaw, sizeof(HASHSAVECONTEXT));

	if (phsctx)
	{
		HANDLE hThread;

		phsctx->hWnd = hWndOwner;
		phsctx->hListRaw = hListRaw;
        phsctx->bSeparateFiles = bSeparateFiles;

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

	phsctx->status = INACTIVE;

    if (! phsctx->bSeparateFiles)
    {
        // Calling HashCalcPrepare with a NULL hList will cause it to calculate
        // and set cchPrefix, but it will not copy the data or walk the directories
        // (we will leave that for the worker thread to do); the reason we do a
        // limited scan now is so that we can show the file dialog (which requires
        // cchPrefix for the automatic name generation) as soon as possible
        phsctx->hList = NULL;
        HashCalcPrepare(phsctx);

        // Get a file name from the user
        ZeroMemory(&phsctx->ofn, sizeof(phsctx->ofn));
        HashCalcInitSave(phsctx);
    }
    else
    {
        // Get the choice of hash from the user
        HashCalcInitSaveSeparate(phsctx);
    }

	if (phsctx->bSeparateFiles ? phsctx->ofn.nFilterIndex : (phsctx->hFileOut != INVALID_HANDLE_VALUE))
	{
        BOOL bDeletionFailed = TRUE;
        phsctx->cFileOutErrors = 0;

		if (phsctx->hList = SLCreateEx(TRUE))
		{
            bDeletionFailed = ! DialogBoxParam(
				g_hModThisDll,
				MAKEINTRESOURCE(IDD_HASHSAVE),
				NULL,
				HashSaveDlgProc,
				(LPARAM)phsctx
			);

			SLRelease(phsctx->hList);
		}

        if (! phsctx->bSeparateFiles)
        {
            CloseHandle(phsctx->hFileOut);

            // Should only happen on Windows XP
            if (bDeletionFailed)
                DeleteFile(phsctx->ofn.lpstrFile);
        }

        if (phsctx->cFileOutErrors)
        {
            TCHAR szFormat[MAX_STRINGMSG], szMessageBuffer[MAX_STRINGMSG], *pszMessage;
            LoadString(g_hModThisDll, IDS_HC_SAVE_ERROR, szFormat, countof(szFormat));
            if (StrStr(szFormat, TEXT("%d")) != NULL)
            {
                StringCchPrintf(szMessageBuffer, countof(szMessageBuffer), szFormat, phsctx->cFileOutErrors);
                pszMessage = szMessageBuffer;
            }
            else
            {
                pszMessage = szFormat;
            }
            MessageBox(NULL, pszMessage, NULL, MB_OK | MB_ICONERROR);
        }
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

	// Prep: expand directories, max path, etc. (prefix was set by earlier call)
	PostMessage(phsctx->hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM)phsctx, TRUE);
	if (! HashCalcPrepare(phsctx))
        return;
    HashCalcSetSaveFormat(phsctx);
	PostMessage(phsctx->hWnd, HM_WORKERTHREAD_TOGGLEPREP, (WPARAM)phsctx, FALSE);

    // Extract the slist into a vector for parallel_for_each
    std::vector<PHASHSAVEITEM> vecpItems;
    vecpItems.resize(phsctx->cTotal + 1);
    SLBuildIndex(phsctx->hList, (PVOID*)vecpItems.data());
    assert(vecpItems.back() == nullptr);
    vecpItems.pop_back();
    assert(vecpItems.back() != nullptr);

#ifdef USE_PPL
    const bool bMultithreaded = vecpItems.size() > 1 && IsSSD(vecpItems[0]->szPath);
    concurrency::concurrent_vector<void*> vecBuffers;  // a vector of all allocated read buffers (one per thread)
    DWORD dwBufferTlsIndex = TlsAlloc();               // TLS index of the current thread's read buffer
    if (dwBufferTlsIndex == TLS_OUT_OF_INDEXES)
        return;
#else
    constexpr bool bMultithreaded = false;
#endif

    PBYTE pbTheBuffer;  // file read buffer, used iff not multithreaded
    if (! bMultithreaded)
    {
        pbTheBuffer = (PBYTE)VirtualAlloc(NULL, READ_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);
        if (pbTheBuffer == NULL)
            return;
    }

    // Initialize the progress bar update synchronization vars
    CRITICAL_SECTION updateCritSec;
    volatile ULONGLONG cbCurrentMaxSize = 0;
    if (bMultithreaded)
        InitializeCriticalSection(&updateCritSec);

#ifdef _TIMED
    DWORD dwStarted;
    if (! phsctx->bSeparateFiles)
        dwStarted = GetTickCount();
#endif

    class CanceledException {};

    // concurrency::parallel_for_each(vecpItems.cbegin(), vecpItems.cend(), ...
    auto per_file_worker = [&](PHASHSAVEITEM pItem)
	{
        WHCTXEX whctx;

        // Indicate which hash type we are after, see WHEX... values in WinHash.h
        whctx.dwFlags = 1 << (phsctx->ofn.nFilterIndex - 1);

        PBYTE pbBuffer;
#ifdef USE_PPL
        if (bMultithreaded)
        {
            // Allocate or retrieve the already-allocated read buffer for the current thread
            pbBuffer = (PBYTE)TlsGetValue(dwBufferTlsIndex);
            if (pbBuffer == NULL)
            {
                pbBuffer = (PBYTE)VirtualAlloc(NULL, READ_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);
                if (pbBuffer == NULL)
                    throw CanceledException();
                // Cache the read buffer for the current thread
                vecBuffers.push_back(pbBuffer);
                TlsSetValue(dwBufferTlsIndex, pbBuffer);
            }
        }
#endif

        if (! WorkerThreadThrottleForUI((PCOMMONCONTEXT)phsctx))
            throw CanceledException();

        // If separate output files are being created for each input
        HANDLE hFileOut;
        if (phsctx->bSeparateFiles)
        {
            // Temporarily append the hash's file extension
            // (HashCalcPrepare/WalkDirectory allocates MAX_FILE_EXT_LEN extra chars to the end of each pItem->szPath)
            StringCchCat(pItem->szPath + pItem->cchPath, MAX_FILE_EXT_LEN, g_szHashExtsTab[phsctx->ofn.nFilterIndex - 1]);

            // Open the file for output
            hFileOut = CreateFile(
                pItem->szPath,
                FILE_APPEND_DATA,
                FILE_SHARE_READ,
                NULL,
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

            pItem->szPath[pItem->cchPath] = '\0';  // remove the temporary file extension
            if (hFileOut == INVALID_HANDLE_VALUE)
            {
                InterlockedIncrement(&phsctx->cFileOutErrors);
                goto update_ui;
            }

            if (phsctx->opt.dwSaveEncoding == 1)
            {
                // Write the BOM for UTF-16LE
                WCHAR BOM = 0xFEFF;
                DWORD cbWritten;
                WriteFile(hFileOut, &BOM, sizeof(WCHAR), &cbWritten, NULL);
            }
        }

#pragma warning(push)
#pragma warning(disable: 4700 4703)  // potentially uninitialized local pointer variable 'pbBuffer' used
		// Get the hash
		WorkerThreadHashFile(
			(PCOMMONCONTEXT)phsctx,
			pItem->szPath,
			&whctx,
			&pItem->results,
            bMultithreaded ? pbBuffer : pbTheBuffer,
			NULL, 0,
            bMultithreaded ? &updateCritSec : NULL, &cbCurrentMaxSize
#ifdef _TIMED
          , &pItem->dwElapsed
#endif
        );

        if (phsctx->status == PAUSED)
            WaitForSingleObject(phsctx->hUnpauseEvent, INFINITE);
		if (phsctx->status == CANCEL_REQUESTED)
        {
            if (phsctx->bSeparateFiles)
            {
                BOOL bDeletionFailed = ! HashCalcDeleteFileByHandle(hFileOut);
                CloseHandle(hFileOut);
                if (bDeletionFailed)  // should only happen on Windows XP
                {
                    // Append the hash's file extension
                    StringCchCat(pItem->szPath + pItem->cchPath, MAX_FILE_EXT_LEN, g_szHashExtsTab[phsctx->ofn.nFilterIndex - 1]);
                    DeleteFile(pItem->szPath);
                }
            }
            throw CanceledException();
        }

		// Write the data
		if (! HashCalcWriteResult(phsctx, phsctx->bSeparateFiles ? hFileOut : phsctx->hFileOut, pItem))
        {
            if (phsctx->bSeparateFiles)
                InterlockedIncrement(&phsctx->cFileOutErrors);
            else
            {
                // If there's only one checksum file, give up after the first write error
                phsctx->cFileOutErrors = 1;
                throw CanceledException();
            }
        }

        if (phsctx->bSeparateFiles)
            CloseHandle(hFileOut);

		// Update the UI
        update_ui:
		InterlockedIncrement(&phsctx->cSentMsgs);
		PostMessage(phsctx->hWnd, HM_WORKERTHREAD_UPDATE, (WPARAM)phsctx, (LPARAM)pItem);
    };
#pragma warning(pop)

    try
    {
#ifdef USE_PPL
        if (bMultithreaded)
            concurrency::parallel_for_each(vecpItems.cbegin(), vecpItems.cend(), per_file_worker);
        else
#endif
            std::for_each(vecpItems.cbegin(), vecpItems.cend(), per_file_worker);
    }
    catch (CanceledException) {}  // ignore cancellation requests

#ifdef _TIMED
    if (phsctx->cTotal > 1 && phsctx->status != CANCEL_REQUESTED && ! phsctx->bSeparateFiles)
    {
        union {
            CHAR  szA[MAX_STRINGMSG];
            WCHAR szW[MAX_STRINGMSG];
        } buffer;
        size_t cbBufferLeft;
        if (phsctx->opt.dwSaveEncoding == 1)  // UTF-16
        {
            StringCbPrintfExW(buffer.szW, sizeof(buffer), NULL, &cbBufferLeft, 0, L"; Total elapsed: %d ms\r\n", GetTickCount() - dwStarted);
        }
        else                                  // UTF-8 or ANSI
        {
            StringCbPrintfExA(buffer.szA, sizeof(buffer), NULL, &cbBufferLeft, 0,  "; Total elapsed: %d ms\r\n", GetTickCount() - dwStarted);
        }
        DWORD dwUnused;
        WriteFile(phsctx->hFileOut, buffer.szA, (DWORD) (sizeof(buffer) - cbBufferLeft), &dwUnused, NULL);
    }
#endif

#ifdef USE_PPL
    if (bMultithreaded)
    {
        for (void* pBuffer : vecBuffers)
            VirtualFree(pBuffer, 0, MEM_RELEASE);
        DeleteCriticalSection(&updateCritSec);
    }
    else
#endif
        VirtualFree(pbTheBuffer, 0, MEM_RELEASE);
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

			phsctx->pfnWorkerMain = (PFNWORKERMAIN)HashSaveWorkerMain;
			phsctx->hThread = CreateThreadCRT(NULL, phsctx);

			if (!phsctx->hThread)
			{
				WorkerThreadCleanup((PCOMMONCONTEXT)phsctx);
                BOOL bDeleted = phsctx->bSeparateFiles ? TRUE : HashCalcDeleteFileByHandle(phsctx->hFileOut);
				EndDialog(hWnd, bDeleted);
			}
            if (WaitForSingleObject(phsctx->hThread, 1000) != WAIT_TIMEOUT)
            {
                WorkerThreadCleanup((PCOMMONCONTEXT)phsctx);
                EndDialog(hWnd, TRUE);
            }

			return(TRUE);
		}

		case WM_DESTROY:
		{
			SetAppIDForWindow(hWnd, FALSE);
			break;
		}

		case WM_ENDSESSION:
        {
            if (wParam == FALSE)  // if TRUE, fall through to WM_CLOSE
                break;
        }
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

                    // Don't keep partially generated checksum files (separate files have already been deleted)
                    BOOL bDeleted = phsctx->bSeparateFiles ? TRUE : HashCalcDeleteFileByHandle(phsctx->hFileOut);

					phsctx->dwFlags |= HCF_EXIT_PENDING;
					WorkerThreadStop((PCOMMONCONTEXT)phsctx);
					WorkerThreadCleanup((PCOMMONCONTEXT)phsctx);

                    // If the user is canceling, never display the write(s) failed message box
                    phsctx->cFileOutErrors = 0;

					EndDialog(hWnd, bDeleted);
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
			EndDialog(hWnd, TRUE);
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
        if (! phsctx->bSeparateFiles)
        {
            PTSTR pszFileName = phsctx->ofn.lpstrFile + phsctx->ofn.nFileOffset;
            TCHAR szFormat[MAX_STRINGRES];
            LoadString(g_hModThisDll, IDS_HS_TITLE_FMT, szFormat, countof(szFormat));
            StringCchPrintf(phsctx->scratch.sz, countof(phsctx->scratch.sz), szFormat, pszFileName);
        }
        else
        {
            LoadString(g_hModThisDll, IDS_HS_MENUTEXT_SEP, phsctx->scratch.sz, countof(phsctx->scratch.sz));
            LPTSTR lpszSrc  = phsctx->scratch.sz;
            LPTSTR lpszDest = phsctx->scratch.sz;
            while (*lpszSrc && *lpszSrc != '(')
            {
                if (*lpszSrc != '&')
                {
                    *lpszDest = *lpszSrc;
                    ++lpszDest;
                }
                ++lpszSrc;
            }
            *lpszDest = 0;
        }

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
        phsctx->hThread = NULL;
        phsctx->hUnpauseEvent = NULL;
    }
}
