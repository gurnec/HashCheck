/**
 * Windows Hashing/Checksumming Library
 * Last modified: 2009/01/13
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This is a wrapper for the MD4, MD5, SHA1, and CRC32 algorithms supported
 * natively by the Windows API.
 *
 * WinHash.c is needed only if the WH*To* or WH*Ex functions are used.
 **/

#include "WinHash.h"

/**
 * WH*To* hex string conversion functions
 **/

BOOL WHAPI WHHexToByte( PTSTR pszSrc, PBYTE pbDest, UINT cchHex )
{
	while (cchHex)
	{
		BYTE hex = LOBYTE(*pszSrc);

		// The high byte of UTF-16 text should always be zero
		if (sizeof(TCHAR) == 2 && HIBYTE(*pszSrc))
			return(FALSE);

		// Now convert the hex character into a 4-bit nybble
		if (hex < '0')
		{
			return(FALSE);
		}
		else if (hex > TEXT('9'))
		{
			// Ensure that this is lower-case
			hex |= 0x20;

			// The x86-64 compiler will allocate registers inefficiently if
			// these two comparisons are joined with an logical OR
			if (hex < 'a') return(FALSE);
			if (hex > 'f') return(FALSE);

			hex -= 'a' - 10;
		}
		else
		{
			hex -= '0';
		}

		// Save to the resulting byte array
		if (!(cchHex & 1))
		{
			*pbDest = hex << 4;
		}
		else
		{
			*pbDest |= hex;
			++pbDest;
		}

		++pszSrc;
		--cchHex;
	}

	return(TRUE);
}

PTSTR WHAPI WHByteToHex( PBYTE pbSrc, PTSTR pszDest, UINT cchHex, UINT8 uCaseMode )
{
	while (cchHex)
	{
		BYTE hex = *pbSrc;

		if (!(cchHex & 1))
			hex >>= 4;
		else
			++pbSrc;

		hex &= 0x0F;
		hex |= 0x30;

		if (hex > '9')
		{
			hex += 'A' - ('0' + 10);
			hex |= uCaseMode;
		}

		*pszDest = hex;

		++pszDest;
		--cchHex;
	}

	*pszDest = 0;

	return(pszDest);
}

/**
 * WH*Ex functions
 **/

VOID WHAPI WHInitEx( PWHCTXEX pContext )
{
	if (pContext->flags & WHEX_CHECKCRC32)
		WHInitCRC32(&pContext->ctxCRC32);

	if (pContext->flags & WHEX_CHECKMD4)
		WHInitMD4(&pContext->ctxMD4);

	if (pContext->flags & WHEX_CHECKMD5)
		WHInitMD5(&pContext->ctxMD5);

	if (pContext->flags & WHEX_CHECKSHA1)
		WHInitSHA1(&pContext->ctxSHA1);
}

VOID WHAPI WHUpdateEx( PWHCTXEX pContext, PCBYTE pbIn, UINT cbIn )
{
	if (pContext->flags & WHEX_CHECKCRC32)
		WHUpdateCRC32(&pContext->ctxCRC32, pbIn, cbIn);

	if (pContext->flags & WHEX_CHECKMD4)
		WHUpdateMD4(&pContext->ctxMD4, pbIn, cbIn);

	if (pContext->flags & WHEX_CHECKMD5)
		WHUpdateMD5(&pContext->ctxMD5, pbIn, cbIn);

	if (pContext->flags & WHEX_CHECKSHA1)
		WHUpdateSHA1(&pContext->ctxSHA1, pbIn, cbIn);
}

VOID WHAPI WHFinishEx( PWHCTXEX pContext, PWHRESULTEX pResults )
{
	if (pResults == NULL)
		pResults = &pContext->results;

	if (pContext->flags & WHEX_CHECKCRC32)
	{
		WHFinishCRC32(&pContext->ctxCRC32);
		WHByteToHex(pContext->ctxCRC32.result, pResults->szHexCRC32, 8, pContext->uCaseMode);
	}

	if (pContext->flags & WHEX_CHECKMD4)
	{
		WHFinishMD4(&pContext->ctxMD4);
		WHByteToHex(pContext->ctxMD4.result, pResults->szHexMD4, 32, pContext->uCaseMode);
	}

	if (pContext->flags & WHEX_CHECKMD5)
	{
		WHFinishMD5(&pContext->ctxMD5);
		WHByteToHex(pContext->ctxMD5.result, pResults->szHexMD5, 32, pContext->uCaseMode);
	}

	if (pContext->flags & WHEX_CHECKSHA1)
	{
		WHFinishSHA1(&pContext->ctxSHA1);
		WHByteToHex(pContext->ctxSHA1.result, pResults->szHexSHA1, 40, pContext->uCaseMode);
	}
}
