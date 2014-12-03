/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __HASHCHECKCOMMON_H__
#define __HASHCHECKCOMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "HashCheckUI.h"
#include "libs\WinHash.h"

// Tuning constants
#define MAX_PATH_BUFFER       0x800
#define READ_BUFFER_SIZE      0x20000
#define BASE_STACK_SIZE       0x1000
#define MARQUEE_INTERVAL      100  // marquee progress bar animation interval

// Progress bar states (Vista-only)
#ifndef PBM_SETSTATE
#define PBM_SETSTATE          (WM_USER + 16)
#define PBST_NORMAL           0x0001
#define PBST_PAUSED           0x0003
#endif

// Codes
#define THREAD_SUSPEND_ERROR  ((DWORD)-1)
#define TIMER_ID_PAUSE        1

// Flags
#define HCF_EXIT_PENDING      0x0001
#define HCF_MARQUEE           0x0002
#define HVF_HAS_SET_TYPE      0x0004
#define HVF_ITEM_HILITE       0x0008
#define HPF_HAS_RESIZED       0x0004

// Messages
#define HM_WORKERTHREAD_DONE        (WM_APP + 0)  // wParam = ctx, lParam = 0
#define HM_WORKERTHREAD_UPDATE      (WM_APP + 1)  // wParam = ctx, lParam = data
#define HM_WORKERTHREAD_SETSIZE     (WM_APP + 2)  // wParam = ctx, lParam = filesize
#define HM_WORKERTHREAD_TOGGLEPREP  (WM_APP + 3)  // wParam = ctx, lParam = state

// Some convenient typedefs for worker thread control
typedef volatile UINT MSGCOUNT, *PMSGCOUNT;
typedef VOID (__fastcall *PFNWORKERMAIN)( PVOID );

// Worker thread status
typedef volatile enum {
	INACTIVE,
	ACTIVE,
	PAUSED,
	CANCEL_REQUESTED,
	CLEANUP_COMPLETED
} WORKERTHREADSTATUS, *PWORKERTHREADSTATUS;

// Extra worker thread parameters: start function address and read buffer
typedef union {
	PFNWORKERMAIN pfnWorkerMain;
	PVOID pvBuffer;
	PTSTR pszPath;
} WORKERTHREADEXTRA, *PWORKERTHREADEXTRA;

// Worker thread context; all other contexts must start with this
typedef struct {
	WORKERTHREADSTATUS status;       // thread status
	DWORD              dwFlags;      // misc. status flags
	MSGCOUNT           cSentMsgs;    // number update messages sent by the worker
	MSGCOUNT           cHandledMsgs; // number update messages processed by the UI
	HWND               hWnd;         // handle of the dialog window
	HWND               hWndPBTotal;  // cache of the IDC_PROG_TOTAL progress bar handle
	HWND               hWndPBFile;   // cache of the IDC_PROG_FILE progress bar handle
	HANDLE             hThread;      // handle of the worker thread
	WORKERTHREADEXTRA  ex;           // extra parameter with varying uses
} COMMONCONTEXT, *PCOMMONCONTEXT;

// File size
typedef struct {
	ULONGLONG ui64;  // uint64 representation
	TCHAR sz[32];    // string representation
} FILESIZE, *PFILESIZE;

// Convenience wrappers
HANDLE __fastcall OpenFileForReading( PCTSTR pszPath );

// Parsing helpers
VOID __fastcall NormalizeString( PTSTR psz );

// UI-related functions
VOID WINAPI SetControlText( HWND hWnd, UINT uCtrlID, UINT uStringID );
VOID WINAPI EnableControl( HWND hWnd, UINT uCtrlID, BOOL bEnable );
VOID WINAPI FormatFractionalResults( PTSTR pszFormat, PTSTR pszBuffer, UINT uPart, UINT uTotal );
VOID WINAPI SetProgressBarPause( PCOMMONCONTEXT pcmnctx, WPARAM iState );

// Functions used by the main thread to control the worker thread
VOID WINAPI WorkerThreadTogglePause( PCOMMONCONTEXT pcmnctx );
VOID WINAPI WorkerThreadStop( PCOMMONCONTEXT pcmnctx );
VOID WINAPI WorkerThreadCleanup( PCOMMONCONTEXT pcmnctx );

// Worker thread functions
DWORD WINAPI WorkerThreadStartup( PCOMMONCONTEXT pcmnctx );
VOID WINAPI WorkerThreadHashFile( PCOMMONCONTEXT pcmnctx, PCTSTR pszPath, PBOOL pbSuccess,
                                  PWHCTXEX pwhctx, PWHRESULTEX pwhres, PFILESIZE pFileSize );

// Wrappers for SHGetInstanceExplorer
ULONG_PTR __fastcall HostAddRef( );
VOID __fastcall HostRelease( ULONG_PTR uCookie );

#ifdef __cplusplus
}
#endif

#endif
