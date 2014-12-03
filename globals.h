/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libs\WinIntrinsics.h"

#include <windows.h>
#include <olectl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <process.h>

#include "libs\SimpleString.h"
#include "libs\SimpleList.h"
#include "libs\SwapIntrinsics.h"
#include "HashCheckResources.h"
#include "HashCheckTranslations.h"
#include "version.h"

// Define globals for bookkeeping this DLL instance
extern HMODULE g_hModThisDll;
extern CREF g_cRefThisDll;

// Activation context cache, to reduce the number of CreateActCtx calls
extern volatile BOOL g_bActCtxCreated;
extern HANDLE g_hActCtx;

// Major and minor Windows version
extern UINT16 g_uWinVer;

// Define the data and strings used for COM registration
static const GUID CLSID_HashCheck = { 0x705977c7, 0x86cb, 0x4743, { 0xbf, 0xaf, 0x69, 0x08, 0xbd, 0x19, 0xb7, 0xb0 } };
#define CLSID_STR_HashCheck         TEXT("{705977C7-86CB-4743-BFAF-6908BD19B7B0}")
#define CLSNAME_STR_HashCheck       TEXT("HashCheck Shell Extension")
#define PROGID_STR_HashCheck        TEXT("HashCheck")

// Application ID for the NT6.1+ taskbar
#define APP_USER_MODEL_ID           L"KL.HashCheck"

// Define helper macros
#define countof(x)                  (sizeof(x)/sizeof(x[0]))
#define BYTEDIFF(a, b)              ((PBYTE)(a) - (PBYTE)(b))
#define BYTEADD(a, cb)              ((PVOID)((PBYTE)(a) + (cb)))

// Max translation string lengths; increase this as necessary
#define MAX_STRINGRES               0x20
#define MAX_STRINGMSG               0x40

#ifdef __cplusplus
}
#endif

#endif
