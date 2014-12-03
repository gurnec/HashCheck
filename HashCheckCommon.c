/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "globals.h"
#include "HashCheckCommon.h"
#include "GetHighMSB.h"

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

VOID __fastcall NormalizeString( PTSTR psz )
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
		wnsprintf(pszBuffer, MAX_STRINGMSG, pszFormat, uPart, uTotal);
	else
		wnsprintf(pszBuffer, MAX_STRINGMSG, pszFormat + 1, uTotal, uPart);
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
		if (SuspendThread(pcmnctx->hThread) != THREAD_SUSPEND_ERROR)
		{
			pcmnctx->status = PAUSED;

			if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
			{
				SetControlText(pcmnctx->hWnd, IDC_PAUSE, IDS_HC_RESUME);
				SetProgressBarPause(pcmnctx, PBST_PAUSED);
			}
		}
	}
	else if (pcmnctx->status == PAUSED)
	{
		DWORD dwResult;

		do
		{
			dwResult = ResumeThread(pcmnctx->hThread);

		} while (dwResult != THREAD_SUSPEND_ERROR && dwResult > 1);

		pcmnctx->status = ACTIVE;

		if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
		{
			SetControlText(pcmnctx->hWnd, IDC_PAUSE, IDS_HC_PAUSE);
			SetProgressBarPause(pcmnctx, PBST_NORMAL);
		}
	}
}

VOID WINAPI WorkerThreadStop( PCOMMONCONTEXT pcmnctx )
{
	if (pcmnctx->status == INACTIVE || pcmnctx->status == CLEANUP_COMPLETED)
		return;

	// If the thread is paused, unpause it
	if (pcmnctx->status == PAUSED)
		WorkerThreadTogglePause(pcmnctx);

	// Signal cancellation
	pcmnctx->status = CANCEL_REQUESTED;

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
			// stopped: wait for the thread, but nuke it if it appears to be hanged.
			if (WaitForSingleObject(pcmnctx->hThread, 10000) == WAIT_TIMEOUT)
				TerminateThread(pcmnctx->hThread, 0);
		}

		CloseHandle(pcmnctx->hThread);
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
	// Exchange the WORKERTHREADEXTRA member
	PFNWORKERMAIN pfnWorkerMain = pcmnctx->ex.pfnWorkerMain;
	pcmnctx->ex.pvBuffer = VirtualAlloc(NULL, READ_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);

	if (pcmnctx->ex.pvBuffer)
	{
		pfnWorkerMain(pcmnctx);
		VirtualFree(pcmnctx->ex.pvBuffer, 0, MEM_RELEASE);
	}

	pcmnctx->status = INACTIVE;

	if (!(pcmnctx->dwFlags & HCF_EXIT_PENDING))
		PostMessage(pcmnctx->hWnd, HM_WORKERTHREAD_DONE, (WPARAM)pcmnctx, 0);

	return(0);
}

VOID WINAPI WorkerThreadHashFile( PCOMMONCONTEXT pcmnctx, PCTSTR pszPath, PBOOL pbSuccess,
                                  PWHCTXEX pwhctx, PWHRESULTEX pwhres, PFILESIZE pFileSize )
{
	#define GETMSB(ui64) (LODWORD(ui64 >> uShiftBits))

	HANDLE hFile;

	*pbSuccess = FALSE;

	// If the worker thread is working so fast that the UI cannot catch up,
	// pause for a bit to let things settle down
	while (pcmnctx->cSentMsgs > pcmnctx->cHandledMsgs + 50)
	{
		Sleep(50);
		if (pcmnctx->status == CANCEL_REQUESTED)
			return;
	}

	// Indicate that we want lower-case results (TODO: make this an option)
	pwhctx->uCaseMode = WHFMT_LOWERCASE;

	if ((hFile = OpenFileForReading(pszPath)) != INVALID_HANDLE_VALUE)
	{
		ULONGLONG cbFileSize, cbFileRead = 0;
		DWORD cbBufferRead;
		UINT uShiftBits;
		UINT8 cInner = 0;

		if (GetFileSizeEx(hFile, (PLARGE_INTEGER)&cbFileSize))
		{
			// The progress bar is updates only once every 4 buffer reads; if
			// the file is small enough that it requires only one such cycle,
			// then do not bother with updating the progress bar; this improves
			// performance when working with large numbers of small files
			BOOL bUpdateProgress = cbFileSize >= READ_BUFFER_SIZE * 4;

			// Get the shift factor for the file size; unless the file is 2GiB
			// or larger in size, this will always be zero
			if ( (uShiftBits = GetHighMSB((PULARGE_INTEGER)&cbFileSize)) ||
			     ((INT)LODWORD(cbFileSize) < 0) )
			{
				// The progress bar expects a signed integer, so we need to
				// ensure that the highest bit is never set
				++uShiftBits;
			}

			// Initialize the file progress bar
			if (bUpdateProgress)
				PostMessage(pcmnctx->hWndPBFile, PBM_SETRANGE32, 0, GETMSB(cbFileSize));

			// If the caller provides a way to return the file size, then set
			// the file size and send a SETSIZE notification
			if (pFileSize)
			{
				pFileSize->ui64 = cbFileSize;
				StrFormatKBSize(cbFileSize, pFileSize->sz, countof(pFileSize->sz));
				PostMessage(pcmnctx->hWnd, HM_WORKERTHREAD_SETSIZE, (WPARAM)pcmnctx, (LPARAM)pFileSize);
			}

			// Finally, read the file and calculate the checksum; the
			// progress bar is updated only once every 4 buffer reads (512K)
			WHInitEx(pwhctx);

			do // Outer loop: keep going until the end
			{
				do // Inner loop: break every 4 cycles or if the end is reached
				{
					if (pcmnctx->status == CANCEL_REQUESTED)
					{
						CloseHandle(hFile);
						return;
					}

					ReadFile(hFile, pcmnctx->ex.pvBuffer, READ_BUFFER_SIZE, &cbBufferRead, NULL);
					WHUpdateEx(pwhctx, pcmnctx->ex.pvBuffer, cbBufferRead);
					cbFileRead += cbBufferRead;

				} while (cbBufferRead == READ_BUFFER_SIZE && (++cInner & 0x03));

				if (bUpdateProgress)
					PostMessage(pcmnctx->hWndPBFile, PBM_SETPOS, GETMSB(cbFileRead), 0);

			} while (cbBufferRead == READ_BUFFER_SIZE);

			WHFinishEx(pwhctx, pwhres);

			if (cbFileRead == cbFileSize)
				*pbSuccess = TRUE;

			if (bUpdateProgress)
				PostMessage(pcmnctx->hWndPBFile, PBM_SETPOS, 0, 0);
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
