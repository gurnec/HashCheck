/**
 * UTF-16/UTF-8 Helper Functions
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "UnicodeHelpers.h"
#include "libs\SwapIntrinsics.h"

#define GETWORD(p)  (*((UPWORD) (p)))
#define GETDWORD(p) (*((UPDWORD)(p)))

PBYTE __fastcall IsTextUTF8( PBYTE pbData )
{
	PBYTE pbOriginal = pbData;

	// Step 1: Check for the BOM (signature)

	if (*pbData == 0xEF && GETWORD(pbData + 1) == 0xBFBB)
		return(pbData + 3);

	// Step 2: If there is no BOM, then manually walk the string and sniff

	// We check for validity by masking out the data bits and testing if the
	// remaining bits match a valid control sequence... this is not a perfect
	// check, as it will pass overlong encodings (depending on your goal, you
	// might actually want to pass overlongs) since the cost of being pedantic
	// with overlongs is not worth it given the lack of substantial benefit.

	while (*pbData)
	{
		if (*pbData < 0x80)
			pbData += 1; // Plain ASCII
		else if ((GETWORD(pbData) & 0xC0E0) == 0x80C0)
			pbData += 2; // Valid 2-byte UTF-8 sequence
		else if ((GETDWORD(pbData) & 0x00C0C0F0) == 0x008080E0)
			pbData += 3; // Valid 3-byte UTF-8 sequence
		else if ((GETDWORD(pbData) & 0xC0C0C0F8) == 0x808080F0)
			pbData += 4; // Valid 4-byte UTF-8 sequence (outside of UTF-16 range)
		else
			return(NULL);
	}

	return(pbOriginal);
}

PWSTR __fastcall BufferToWStr( PBYTE *ppbData, DWORD cbData )
{
	PBYTE pbData = *ppbData;
	PWSTR pszResult;

	// Step 1: Find out if the buffer is a UTF-16 string

	INT iUnicodeTests = IS_TEXT_UNICODE;
	IsTextUnicode(pbData, cbData, &iUnicodeTests);

	if (iUnicodeTests & IS_TEXT_UNICODE)
	{
		// Reverse if necessary
		if (iUnicodeTests & IS_TEXT_UNICODE_REVERSE_MASK)
			SwapA16I((PWSTR)pbData, cbData / sizeof(WCHAR));

		// Skip the BOM if it exists
		if (iUnicodeTests & IS_TEXT_BOM)
			pbData += sizeof(WCHAR);

		return((PWSTR)pbData);
	}

	// Step 2: If this is not UTF-16, then check if this is UTF-8 or ANSI, and
	// then convert as appropriate

	if (pszResult = malloc((cbData + 1) * sizeof(WCHAR)))
	{
		PBYTE pbScratch;
		UINT uCodePage;

		// Check for UTF-8 and select the appropriate conversion method
		if (pbScratch = IsTextUTF8(pbData))
		{
			uCodePage = CP_UTF8;
		}
		else
		{
			pbScratch = pbData;
			uCodePage = CP_ACP;
		}

		// Convert, and if successful, replace the old buffer with the new
		if (StrToWStrEx(pbScratch, pszResult, cbData + 1, uCodePage))
		{
			free(pbData);
			*ppbData = (PBYTE)pszResult;
			return(pszResult);
		}
		else
		{
			free(pszResult);
		}
	}

	return(NULL);
}
