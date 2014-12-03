/**
 * Windows API Compiler Intrinsics Library
 * Last modified: 2009/01/04
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * These are intrinsic definitions that need to be made prior to the inclusion
 * of windows.h.
 **/

#ifndef __WININTRINSICS_H__
#define __WININTRINSICS_H__

#ifdef __cplusplus
extern "C" {
#endif

// Certain parts of Windows API headers make use of the mem* functions, so
// should be declared as intrinsic beforehand.
#include <memory.h>
#pragma intrinsic(memcmp, memcpy, memset)

// winbase.h defines the Interlocked* functions as intrinsic, but only for the
// IA-64 and x86-64 architectures; such a definition was not made for x86-32 to
// maintain consistency and compatibility with older compilers, we will make
// that definition for VC6+ on x86-32
#if _MSC_VER >= 1200 && defined(_M_IX86)
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#include <windows.h>
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#endif

// The Interlocked* functions use volatile LONG, so let's define a helper type
typedef volatile long VLONG, *PVLONG, CREF, *PCREF;

#ifdef __cplusplus
}
#endif

#endif
