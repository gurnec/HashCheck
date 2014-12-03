/**
 * UTF-16/UTF-8 Helper Functions
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __UNICODEHELPERS_H__
#define __UNICODEHELPERS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "libs\SimpleString.h"

/**
 * Helper Macros
 * Argument order: source, destination, count (follow the order in the name)
 **/

#define WStrToStrEx(w, a, c, cp) WideCharToMultiByte(cp, 0, w, -1, a, c, NULL, NULL)
#define StrToWStrEx(a, w, c, cp) MultiByteToWideChar(cp, 0, a, -1, w, c)

#define WStrToAStr(w, a, c)      WideCharToMultiByte(CP_ACP,  0, w, -1, a, c, NULL, NULL)
#define AStrToWStr(a, w, c)      MultiByteToWideChar(CP_ACP,  0, a, -1, w, c)
#define WStrToUTF8(w, a, c)      WideCharToMultiByte(CP_UTF8, 0, w, -1, a, c, NULL, NULL)
#define UTF8ToWStr(a, w, c)      MultiByteToWideChar(CP_UTF8, 0, a, -1, w, c)

#ifdef UNICODE
#define TStrToAStr               WStrToAStr
#define AStrToTStr               AStrToWStr
#define TStrToWStr(t, w, c)      SSCpyW(w, t)
#define WStrToTStr(w, t, c)      SSCpyW(t, w)
#else
#define TStrToAStr(t, a, c)      SSCpyA(a, t)
#define AStrToTStr(a, t, c)      SSCpyA(t, a)
#define TStrToWStr               AStrToWStr
#define WStrToTStr               WStrToAStr
#endif

/**
 * IsTextUnicode definitions
 **/

#define IS_TEXT_UNICODE (IS_TEXT_UNICODE_UNICODE_MASK | IS_TEXT_UNICODE_REVERSE_MASK)
#define IS_TEXT_BOM (IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE)

/**
 * IsTextUTF8 - Checks if a string is UTF-8, and if it is, returns a pointer to
 * the start of the string (skipping the BOM if there is one); NULL is returned
 * if the string is not UTF-8.
 *
 * NOTE: To simplify bounds checking, the buffer passed to IsTextUTF8 needs to
 * be NULL-terminated and must extend at least 2 bytes past the NULL terminator!
 **/

PBYTE __fastcall IsTextUTF8( PBYTE pbData );

/**
 * BufferToWStr - Converts a malloc-allocated buffer to Unicode and returns
 * a pointer to the start of the new Unicode string, with any BOMs skipped;
 * NULL is returned if the operation failed.
 *
 * NOTE: BufferToWStr may re-allocate a larger buffer, in which case the
 * original buffer is freed and *ppbData will be a pointer to the new buffer
 * (otherwise, *ppbData will be unchanged); cbData should be the size, in bytes
 * of the data in the buffer, without any NULL terminations.
 *
 * NOTE: To simplify bounds checking, the buffer passed to BufferToWStr should
 * be at least 3 bytes larger than cbData, and the first 2 of these bytes
 * should be NULL (to serve as the termination of the string); the easiest way
 * to do this is to append a zero DWORD to the end of the string.
 **/

PWSTR __fastcall BufferToWStr( PBYTE *ppbData, DWORD cbData );

#ifdef __cplusplus
}
#endif

#endif
