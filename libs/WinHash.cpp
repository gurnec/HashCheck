/**
 * Windows Hashing/Checksumming Library
 * Last modified: 2016/02/21
 * Original work copyright (C) Kai Liu.  All rights reserved.
 * Modified work copyright (C) 2014, 2016 Christopher Gurnee.  All rights reserved.
 * Modified work copyright (C) 2016 Tim Schlueter.  All rights reserved.
 *
 * This is a wrapper for the CRC32, MD5, SHA1, SHA2-256, and SHA2-512
 * algorithms.
 *
 * WinHash.cpp is needed only if the WH*To* or WH*Ex functions are used.
 **/

#include "WinHash.h"
#if _MSC_VER >= 1600
#include <ppl.h>
#endif

// Macro to populate the extensions table. E.g. HASH_EXT_MD5,
#define HASH_EXT_op(alg) HASH_EXT_##alg,

// Table of supported Hash file extensions
LPCTSTR g_szHashExtsTab[NUM_HASHES] = {
	FOR_EACH_HASH(HASH_EXT_op)
};

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

	if (pContext->flags & WHEX_CHECKMD5)
		WHInitMD5(&pContext->ctxMD5);

	if (pContext->flags & WHEX_CHECKSHA1)
		WHInitSHA1(&pContext->ctxSHA1);

	if (pContext->flags & WHEX_CHECKSHA256)
		WHInitSHA256(&pContext->ctxSHA256);

	if (pContext->flags & WHEX_CHECKSHA512)
		WHInitSHA512(&pContext->ctxSHA512);
}

VOID WHAPI WHUpdateEx( PWHCTXEX pContext, PCBYTE pbIn, UINT cbIn )
{
#if _MSC_VER >= 1600
    auto task_WHUpdateSHA512 = concurrency::make_task([&] { WHUpdateSHA512(&pContext->ctxSHA512, pbIn, cbIn); } );
    auto task_WHUpdateSHA256 = concurrency::make_task([&] { WHUpdateSHA256(&pContext->ctxSHA256, pbIn, cbIn); } );
    auto task_WHUpdateSHA1   = concurrency::make_task([&] { WHUpdateSHA1  (&pContext->ctxSHA1,   pbIn, cbIn); } );
    auto task_WHUpdateMD5    = concurrency::make_task([&] { WHUpdateMD5   (&pContext->ctxMD5,    pbIn, cbIn); } );
    auto task_WHUpdateCRC32  = concurrency::make_task([&] { WHUpdateCRC32 (&pContext->ctxCRC32,  pbIn, cbIn); } );

    concurrency::structured_task_group hashing_task_group;

    if (pContext->flags & WHEX_CHECKSHA512)
        hashing_task_group.run(task_WHUpdateSHA512);

    if (pContext->flags & WHEX_CHECKSHA256)
        hashing_task_group.run(task_WHUpdateSHA256);

    if (pContext->flags & WHEX_CHECKSHA1)
        hashing_task_group.run(task_WHUpdateSHA1);

    if (pContext->flags & WHEX_CHECKMD5)
        hashing_task_group.run(task_WHUpdateMD5);

    if (pContext->flags & WHEX_CHECKCRC32)
        hashing_task_group.run(task_WHUpdateCRC32);

    hashing_task_group.wait();

#else
	if (pContext->flags & WHEX_CHECKCRC32)
		WHUpdateCRC32(&pContext->ctxCRC32, pbIn, cbIn);

	if (pContext->flags & WHEX_CHECKMD5)
		WHUpdateMD5(&pContext->ctxMD5, pbIn, cbIn);

	if (pContext->flags & WHEX_CHECKSHA1)
		WHUpdateSHA1(&pContext->ctxSHA1, pbIn, cbIn);

	if (pContext->flags & WHEX_CHECKSHA256)
		WHUpdateSHA256(&pContext->ctxSHA256, pbIn, cbIn);

	if (pContext->flags & WHEX_CHECKSHA512)
		WHUpdateSHA512(&pContext->ctxSHA512, pbIn, cbIn);
#endif
}

VOID WHAPI WHFinishEx( PWHCTXEX pContext, PWHRESULTEX pResults )
{
	if (pResults == NULL)
		pResults = &pContext->results;

	if (pContext->flags & WHEX_CHECKCRC32)
	{
		WHFinishCRC32(&pContext->ctxCRC32);
		WHByteToHex(pContext->ctxCRC32.result, pResults->szHexCRC32, CRC32_DIGEST_LENGTH * 2, pContext->uCaseMode);
	}

	if (pContext->flags & WHEX_CHECKMD5)
	{
		WHFinishMD5(&pContext->ctxMD5);
		WHByteToHex(pContext->ctxMD5.result, pResults->szHexMD5, MD5_DIGEST_LENGTH * 2, pContext->uCaseMode);
	}

	if (pContext->flags & WHEX_CHECKSHA1)
	{
		WHFinishSHA1(&pContext->ctxSHA1);
		WHByteToHex(pContext->ctxSHA1.result, pResults->szHexSHA1, SHA1_DIGEST_LENGTH * 2, pContext->uCaseMode);
	}

	if (pContext->flags & WHEX_CHECKSHA256)
	{
		WHFinishSHA256(&pContext->ctxSHA256);
		WHByteToHex(pContext->ctxSHA256.result, pResults->szHexSHA256, SHA256_DIGEST_LENGTH * 2, pContext->uCaseMode);
	}

	if (pContext->flags & WHEX_CHECKSHA512)
	{
		WHFinishSHA512(&pContext->ctxSHA512);
		WHByteToHex(pContext->ctxSHA512.result, pResults->szHexSHA512, SHA512_DIGEST_LENGTH * 2, pContext->uCaseMode);
	}
}