/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __HASHCALC_H__
#define __HASHCALC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "HashCheckOptions.h"
#include "libs\WinHash.h"

/**
 * Much of what is in the HashCalc module used to reside within HashProp; with
 * the addition of HashSave, which shares a lot of code with HashProp (but not
 * with HashVerify), the shared portions were spun out of HashProp to form
 * HashCalc.
 *
 * This sharing was tailored to reduce code duplication (both non-compiled and
 * compiled duplication), so there will be a little bit of waste on the part
 * of HashSave (e.g., scratch.ext), but it is only a few dozen KB, and this
 * really does simplify things a lot.
 **/

// Constants
#define RESULTS_LEN 0xC0
#define SCRATCH_BUFFER_SIZE (MAX_PATH_BUFFER + RESULTS_LEN)

// Scratch buffer
typedef struct {
	union
	{
		#ifdef UNICODE
		WCHAR sz[SCRATCH_BUFFER_SIZE];
		#endif
		WCHAR szW[SCRATCH_BUFFER_SIZE];
	};
	union
	{
		#ifndef UNICODE
		CHAR sz[SCRATCH_BUFFER_SIZE * 3];
		#endif
		CHAR szA[SCRATCH_BUFFER_SIZE * 3];
	};
	BYTE ext[0x8000];  // extra padding for batching large sets of small files
} HASHCALCSCRATCH, *PHASHCALCSCRATCH;

// Hash creation context
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
	// Members specific to HashCalc
	HSIMPLELIST        hListRaw;     // data from IShellExtInit
	HSIMPLELIST        hList;        // our expanded/processed data
	HANDLE             hFileOut;     // handle of the output file
	HFONT              hFont;        // fixed-width font for the results box: handle
	WNDPROC            wpSearchBox;  // original WNDPROC for the HashProp search box
	UINT               cchMax;       // max length, in characters of the longest path (stupid SFV formatting)
	UINT               cchPrefix;    // number of characters to omit from the start of the path
	UINT               cchAdjusted;  // cchPrefix, adjusted for the path of the output file
	UINT               cTotal;       // total number of files
	UINT               cSuccess;     // total number of successfully hashed
	HASHCHECKOPTIONS   opt;          // HashCheck settings
	OPENFILENAME       ofn;          // struct used for the save dialog; this needs to persist
	TCHAR              szFormat[20]; // output format for wnsprintf
	WHCTXEX            whctx;        // context for the WinHash library
	UINT               obScratch;    // offset, in bytes, to the scratch, for update coalescing
	HASHCALCSCRATCH    scratch;      // scratch buffers
} HASHCALCCONTEXT, *PHASHCALCCONTEXT;

// Per-file data
typedef struct {
	BOOL bValid;                     // FALSE if the file could not be opened
	UINT cchPath;                    // length of path in characters, not including NULL
	WHRESULTEX results;              // hash results
	TCHAR szPath[];                  // unaltered path
} HASHCALCITEM, *PHASHCALCITEM;

// Public functions
VOID WINAPI HashCalcPrepare( PHASHCALCCONTEXT phcctx );
VOID WINAPI HashCalcInitSave( PHASHCALCCONTEXT phcctx );
BOOL WINAPI HashCalcWriteResult( PHASHCALCCONTEXT phcctx, PHASHCALCITEM pItem );
VOID WINAPI HashCalcTogglePrep( PHASHCALCCONTEXT phcctx, BOOL bState );

#ifdef __cplusplus
}
#endif

#endif
