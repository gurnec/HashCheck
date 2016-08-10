/**
 * HashCheck Shell Extension
 * Original work copyright (C) Kai Liu.  All rights reserved.
 * Modified work copyright (C) 2016 Christopher Gurnee.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include <assert.h>
#include "globals.h"
#include "HashCheckCommon.h"
#include "GetHighMSB.h"
#include <Strsafe.h>

#define PROGRESS_BAR_STEPS 300

HANDLE __fastcall CreateThreadCRT( PVOID pThreadProc, PVOID pvParam )
{
	if (!pThreadProc)
	{
		// If we have a NULL pThreadProc, then we are starting a worker thread,
		// and there is some initialization that we need to take care of...

		PCOMMONCONTEXT pcmnctx = pvParam;
		pcmnctx->status = ACTIVE;
		pcmnctx->cSentMsgs = 0;
		pcmnctx->cHandledMsgs = 0;
		pcmnctx->hWndPBTotal = GetDlgItem(pcmnctx->hWnd, IDC_PROG_TOTAL);
		pcmnctx->hWndPBFile = GetDlgItem(pcmnctx->hWnd, IDC_PROG_FILE);
		SendMessage(pcmnctx->hWndPBFile, PBM_SETRANGE, 0, MAKELPARAM(0, PROGRESS_BAR_STEPS));

		pThreadProc = WorkerThreadStartup;
	}

	return((HANDLE)_beginthreadex(
		NULL,
		BASE_STACK_SIZE,
		pThreadProc,
		pvParam,
		0,
		NULL
	));
}

HANDLE __fastcall OpenFileForReading( PCTSTR pszPath )
{
	return(CreateFile(
		pszPath,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
	));
}

VOID __fastcall HCNormalizeString( PTSTR psz )
{
	if (!psz) return;

	while (*psz)
	{
		switch (*psz)
		{
			case TEXT('\r'):
				*psz = TEXT('\n');
				break;

			case TEXT('\t'):
			case TEXT('"'):
			case TEXT('*'):
				*psz = TEXT(' ');
				break;

			case TEXT('/'):
				*psz = TEXT('\\');
				break;
		}

		++psz;
	}
}

VOID WINAPI SetControlText( HWND hWnd, UINT uCtrlID, UINT uStringID )
{
	TCHAR szBuffer[MAX_STRINGMSG];
	LoadString(g_hModThisDll, uStringID, szBuffer, countof(szBuffer));
	SetDlgItemText(hWnd, uCtrlID, szBuffer);
}

VOID WINAPI EnableControl( HWND hWnd, UINT uCtrlID, BOOL bEnable )
{
	HWND hWndControl = GetDlgItem(hWnd, uCtrlID);
	ShowWindow(hWndControl, (bEnable) ? SW_SHOW : SW_HIDE);
	EnableWindow(hWndControl, bEnable);
}

VOID WINAPI FormatFractionalResults( PTSTR pszFormat, PTSTR pszBuffer, UINT uPart, UINT uTotal )
{
	// pszFormat must be at least MAX_STRINGRES TCHARs long
	// pszBuffer must be at least MAX_STRINGMSG TCHARs long

	if (*pszFormat == 0)
	{
		LoadString(
			g_hModThisDll,
			(uTotal == 1) ? IDS_HP_RESULT_FMT : IDS_HP_RESULTS_FMT,
			pszFormat,
			MAX_STRINGRES
		);
	}

	if (*pszFormat != TEXT('!'))
		StringCchPrintf(pszBuffer, MAX_STRINGMSG, pszFormat, uPart, uTotal);
	else
		StringCchPrintf(pszBuffer, MAX_STRINGMSG, pszFormat + 1, uTotal, uPart);
}

VOID WINAPI SetProgressBarPause( PCOMMONCONTEXT pcmnctx, WPARAM iState )
{
	// For Windows Classic, we can change the color to indicate a pause
	COLORREF clrProgress = (iState == PBST_NORMAL) ? CLR_DEFAULT : RGB(0xFF, 0x80, 0x00);
	SendMessage(pcmnctx->hWndPBTotal, PBM_SETBARCOLOR, 0, clrProgress);
	SendMessage(pcmnctx->hWndPBFile, PBM_SETBARCOLOR, 0, clrProgress);

	// Toggle the marquee animation if applicable
	if (pcmnctx->dwFlags & HCF_MARQUEE)
		SendMessage(pcmnctx->hWndPBTotal, PBM_SETMARQUEE, iState == PBST_NORMAL, MARQUEE_INTERVAL);

	if (g_uWinVer >= 0x0600)
	{
		// If this is Vista, we can also set the state
		SendMessage(pcmnctx->hWndPBTotal, PBM_SETSTATE, iState, 0);
		SendMessage(pcmnctx->hWndPBFile, PBM_SETSTATE, iState, 0);

		// Vista's progress bar is buggy--if you pause it while it is animating,
		// the color will not change (but it will stop animating), so it may
		// be necessary to send another PBM_SETSTATE to get it right
		SetTimer(pcmnctx->hWnd, TIMER_ID_PAUSE, 750, NULL);
	}
}

VOID WINAPI WorkerThreadTogglePause( PCOMMONCONTEXT pcmnctx )
{
	if (pcmnctx->status == ACTIVE)
	{
        ResetEvent(pcmnctx->hUnpauseEvent);
		pcmnctx->status = PAUSED;

		if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
		{
			SetControlText(pcmnctx->hWnd, IDC_PAUSE, IDS_HC_RESUME);
			SetProgressBarPause(pcmnctx, PBST_PAUSED);
		}
	}
	else if (pcmnctx->status == PAUSED)
	{
		pcmnctx->status = ACTIVE;

		if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
		{
			SetControlText(pcmnctx->hWnd, IDC_PAUSE, IDS_HC_PAUSE);
			SetProgressBarPause(pcmnctx, PBST_NORMAL);
		}

        SetEvent(pcmnctx->hUnpauseEvent);
	}
}

VOID WINAPI WorkerThreadStop( PCOMMONCONTEXT pcmnctx )
{
	if (pcmnctx->status == INACTIVE || pcmnctx->status == CLEANUP_COMPLETED)
		return;

	// If the thread is paused, unpause it
    if (pcmnctx->status == PAUSED)
    {
        // Signal cancellation first, so that unpaused threads immediately exit
        pcmnctx->status = CANCEL_REQUESTED;
        SetEvent(pcmnctx->hUnpauseEvent);
    }
    else
    {
        // Signal cancellation
        pcmnctx->status = CANCEL_REQUESTED;
    }

	// Disable the control buttons
	if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
	{
		EnableWindow(GetDlgItem(pcmnctx->hWnd, IDC_PAUSE), FALSE);
		EnableWindow(GetDlgItem(pcmnctx->hWnd, IDC_STOP), FALSE);
	}
}

VOID WINAPI WorkerThreadCleanup( PCOMMONCONTEXT pcmnctx )
{
	if (pcmnctx->status == CLEANUP_COMPLETED)
		return;

	// There are only two times this function gets called:
	// Case 1: The worker thread has exited on its own, and this function
	// was invoked in response to the thread's exit notification.
	// Case 2: A forced abort was requested (app exit, system shutdown, etc.),
	// where this is called right after calling WorkerThreadStop to signal the
	// thread to exit.

	if (pcmnctx->hThread)
	{
		if (pcmnctx->status != INACTIVE)
		{
			// Forced abort, where the thread has been told to stop but has not yet
			// stopped. With the MS Concurrency runtime, there's no simple way to
			// terminate errant threads; it's better to abort the process than to
			// allow them to continue (maybe maxing out the CPU) in the background.
			if (WaitForSingleObject(pcmnctx->hThread, 10000) == WAIT_TIMEOUT)
				abort();
		}

		CloseHandle(pcmnctx->hThread);
        CloseHandle(pcmnctx->hUnpauseEvent);
	}

	pcmnctx->status = CLEANUP_COMPLETED;

	if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
	{
		static const UINT16 arCtrls[] =
		{
			IDC_PROG_TOTAL,
			IDC_PROG_FILE,
			IDC_PAUSE,
			IDC_STOP
		};

		UINT i;

		for (i = 0; i < countof(arCtrls); ++i)
			EnableControl(pcmnctx->hWnd, arCtrls[i], FALSE);
	}
}

DWORD WINAPI WorkerThreadStartup( PCOMMONCONTEXT pcmnctx )
{
    pcmnctx->hUnpauseEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    pcmnctx->pfnWorkerMain(pcmnctx);

	pcmnctx->status = INACTIVE;

	if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
		PostMessage(pcmnctx->hWnd, HM_WORKERTHREAD_DONE, (WPARAM)pcmnctx, 0);

	return(0);
}

// Post messages to update the progress bar. If there are multiple file-hashing threads,
// then only the thread currently operating on the largest file updates the progress bar.
__inline VOID UpdateProgressBar( HWND hWndPBFile, PCRITICAL_SECTION pCritSec,
                                 PBOOL pbCurrentlyUpdating, volatile ULONGLONG* pcbCurrentMaxSize,
                                 ULONGLONG cbFileSize, ULONGLONG cbFileRead, PUINT pLastProgress )
{
    if (pCritSec)  // if we're one among many file-hashing threads
    {
        // All the checks below outside of critical sections are innacurate; they're meant
        // to quickly check if entering the critical section can be avoided, but they must
        // be repeated inside the critical sections to avoid potential race conditions

        if (*pbCurrentlyUpdating)
        {
            if (cbFileSize != *pcbCurrentMaxSize)
            {
                // Some other thread is now updating the progress bar
                *pbCurrentlyUpdating = FALSE;
                return;
            }
            UINT newProgress = (UINT) (PROGRESS_BAR_STEPS * cbFileRead / cbFileSize);
            if (newProgress == *pLastProgress)
                return;

            EnterCriticalSection(pCritSec);
            if (cbFileSize != *pcbCurrentMaxSize)
            {
                LeaveCriticalSection(pCritSec);
                // Some other thread is now updating the progress bar
                *pbCurrentlyUpdating = FALSE;
                return;
            }
            // Special case for when this file has been completed:
            if (cbFileRead == 0)
                *pcbCurrentMaxSize = 0;  // relinquish progress bar control to the next largest file
            PostMessage(hWndPBFile, PBM_SETPOS, newProgress, 0);
            LeaveCriticalSection(pCritSec);
            *pLastProgress = newProgress;
        }
        else  // if not *pbCurrentlyUpdating
        {
            if (cbFileSize > *pcbCurrentMaxSize)  // if we should take over updating the progress bar
            {
                UINT newProgress = (UINT)(PROGRESS_BAR_STEPS * cbFileRead / cbFileSize);
                EnterCriticalSection(pCritSec);
                if (cbFileSize > *pcbCurrentMaxSize)  // if we should definitely take over updating the progress bar
                {
                    *pcbCurrentMaxSize = cbFileSize;
                    PostMessage(hWndPBFile, PBM_SETPOS, newProgress, 0);
                }
                LeaveCriticalSection(pCritSec);
                *pbCurrentlyUpdating = TRUE;
                *pLastProgress = newProgress;
            }
        }
    }
    else  // if we're the only file-hashing thread
    {
        UINT newProgress = (UINT)(PROGRESS_BAR_STEPS * cbFileRead / cbFileSize);
        if (newProgress != *pLastProgress)
        {
            PostMessage(hWndPBFile, PBM_SETPOS, newProgress, 0);
            *pLastProgress = newProgress;
        }
    }
}

VOID WINAPI WorkerThreadHashFile( PCOMMONCONTEXT pcmnctx, PCTSTR pszPath, PBOOL pbSuccess,
                                  PWHCTXEX pwhctx, PWHRESULTEX pwhres, PBYTE pbuffer,
                                  PFILESIZE pFileSize, LPARAM lParam,
                                  PCRITICAL_SECTION pUpdateCritSec, volatile ULONGLONG* pcbCurrentMaxSize
#ifdef _TIMED
                                , PDWORD pdwElapsed
#endif
                                )
{
	HANDLE hFile;

	*pbSuccess = FALSE;

	// If the worker thread is working so fast that the UI cannot catch up,
	// pause for a bit to let things settle down
	while (pcmnctx->cSentMsgs > pcmnctx->cHandledMsgs + 50)
	{
		Sleep(50);
        if (pcmnctx->status == PAUSED)
            WaitForSingleObject(pcmnctx->hUnpauseEvent, INFINITE);
		if (pcmnctx->status == CANCEL_REQUESTED)
			return;
	}

	// Indicate that we want lower-case results (TODO: make this an option)
	pwhctx->uCaseMode = WHFMT_LOWERCASE;

	if ((hFile = OpenFileForReading(pszPath)) != INVALID_HANDLE_VALUE)
	{
		ULONGLONG cbFileSize, cbFileRead = 0;
		DWORD cbBufferRead;
		UINT lastProgress = 0;
		UINT8 cInner = 0;

		if (GetFileSizeEx(hFile, (PLARGE_INTEGER)&cbFileSize))
		{
			// The progress bar is updates only once every 4 buffer reads; if
			// the file is small enough that it requires only one such cycle,
			// then do not bother with updating the progress bar; this improves
			// performance when working with large numbers of small files
			BOOL bUpdateProgress = cbFileSize >= READ_BUFFER_SIZE * 4,
			     bCurrentlyUpdating = FALSE;

			// If the caller provides a way to return the file size, then set
			// the file size; send a SETSIZE notification only if it was "big"
			if (pFileSize)
			{
				pFileSize->ui64 = cbFileSize;
				StrFormatKBSize(cbFileSize, pFileSize->sz, countof(pFileSize->sz));
				if (cbFileSize > READ_BUFFER_SIZE)
				    PostMessage(pcmnctx->hWnd, HM_WORKERTHREAD_SETSIZE, (WPARAM)pcmnctx, lParam != -1 ? lParam : (LPARAM)pFileSize);
			}
#ifdef _TIMED
            DWORD dwStarted;
            if (pdwElapsed)
                dwStarted = GetTickCount();
#endif
			// Finally, read the file and calculate the checksum; the
			// progress bar is updated only once every 4 buffer reads (512K)
			WHInitEx(pwhctx);

			do // Outer loop: keep going until the end
			{
				do // Inner loop: break every 4 cycles or if the end is reached
				{
                    if (pcmnctx->status == PAUSED)
                        WaitForSingleObject(pcmnctx->hUnpauseEvent, INFINITE);
					if (pcmnctx->status == CANCEL_REQUESTED)
					{
						CloseHandle(hFile);
						return;
					}

					ReadFile(hFile, pbuffer, READ_BUFFER_SIZE, &cbBufferRead, NULL);
					WHUpdateEx(pwhctx, pbuffer, cbBufferRead);
					cbFileRead += cbBufferRead;

				} while (cbBufferRead == READ_BUFFER_SIZE && (++cInner & 0x03));

				if (bUpdateProgress)
					UpdateProgressBar(pcmnctx->hWndPBFile, pUpdateCritSec, &bCurrentlyUpdating,
					                  pcbCurrentMaxSize, cbFileSize, cbFileRead, &lastProgress);

			} while (cbBufferRead == READ_BUFFER_SIZE);

			WHFinishEx(pwhctx, pwhres);
#ifdef _TIMED
            if (pdwElapsed)
                *pdwElapsed = GetTickCount() - dwStarted;
#endif
			if (cbFileRead == cbFileSize)
				*pbSuccess = TRUE;

			if (bUpdateProgress)
				UpdateProgressBar(pcmnctx->hWndPBFile, pUpdateCritSec, &bCurrentlyUpdating,
				                  pcbCurrentMaxSize, cbFileSize, 0, &lastProgress);
		}

		CloseHandle(hFile);
	}
}

__forceinline HANDLE WINAPI GetActCtx( HMODULE hModule, PCTSTR pszResourceName )
{
	// Wraps away the silliness of CreateActCtx, including the fact that
	// it will fail if you do not provide a valid path string even if you
	// supply a valid module handle, which is quite silly since the path is
	// just going to get translated into a module handle anyway by the API.

	ACTCTX ctx;
	TCHAR szModule[MAX_PATH << 1];

	GetModuleFileName(hModule, szModule, countof(szModule));

	ctx.cbSize = sizeof(ctx);
	ctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
	ctx.lpSource = szModule;
	ctx.lpResourceName = pszResourceName;
	ctx.hModule = hModule;

	return(CreateActCtx(&ctx));
}

ULONG_PTR WINAPI ActivateManifest( BOOL bActivate )
{
	if (!g_bActCtxCreated)
	{
		g_bActCtxCreated = TRUE;
		g_hActCtx = GetActCtx(g_hModThisDll, MAKEINTRESOURCE(IDR_RT_MANIFEST));
	}

	if (g_hActCtx != INVALID_HANDLE_VALUE)
	{
		ULONG_PTR uCookie;

		if (!bActivate)
			return(1);  // Just indicate that we have a good g_hActCtx
		else if (ActivateActCtx(g_hActCtx, &uCookie))
			return(uCookie);
	}

	// We can assume that zero is never a valid cookie value...
	// * http://support.microsoft.com/kb/830033
	// * http://blogs.msdn.com/jonwis/archive/2006/01/12/512405.aspx
	return(0);
}

ULONG_PTR __fastcall HostAddRef( )
{
	LPUNKNOWN pUnknown;

	if (SHGetInstanceExplorer(&pUnknown) == S_OK)
		return((ULONG_PTR)pUnknown);
	else
		return(0);
}

VOID __fastcall HostRelease( ULONG_PTR uCookie )
{
	if (uCookie)
	{
		LPUNKNOWN pUnknown = (LPUNKNOWN)uCookie;
		pUnknown->lpVtbl->Release(pUnknown);
	}
}
