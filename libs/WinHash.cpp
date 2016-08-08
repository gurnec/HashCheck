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
#ifdef USE_PPL
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
#define WIN_HASH_INIT_op(alg)               \
    if (pContext->flags & WHEX_CHECK##alg)  \
        WHInit##alg(&pContext->ctx##alg);
    FOR_EACH_HASH(WIN_HASH_INIT_op)
}

VOID WHAPI WHUpdateEx( PWHCTXEX pContext, PCBYTE pbIn, UINT cbIn )
{
#ifdef USE_PPL
    if (cbIn > 384u) {  // determined experimentally--smaller than this and multithreading doesn't help, but ymmv

        int cTasks = 0;
#define WIN_HASH_UPDATE_COUNT_op(alg)           \
        if (pContext->flags & WHEX_CHECK##alg)  \
            cTasks++;
        FOR_EACH_HASH(WIN_HASH_UPDATE_COUNT_op)

        if (cTasks > 1)
        {

#define WIN_HASH_UPDATE_TASK_op(alg)  \
            auto task_WHUpdate##alg = concurrency::make_task([&] { WHUpdate##alg(&pContext->ctx##alg, pbIn, cbIn); } );
            FOR_EACH_HASH_R(WIN_HASH_UPDATE_TASK_op)

            concurrency::structured_task_group hashing_task_group;

#define WIN_HASH_UPDATE_RUN_TASK_op(alg)            \
            if (pContext->flags & WHEX_CHECK##alg)  \
                hashing_task_group.run(task_WHUpdate##alg);
            FOR_EACH_HASH_R(WIN_HASH_UPDATE_RUN_TASK_op)

            hashing_task_group.wait();
            return;
        }
    }
#endif

#define WIN_HASH_UPDATE_RUN_op(alg)         \
    if (pContext->flags & WHEX_CHECK##alg)  \
        WHUpdate##alg(&pContext->ctx##alg, pbIn, cbIn);
    FOR_EACH_HASH(WIN_HASH_UPDATE_RUN_op)
}

VOID WHAPI WHFinishEx( PWHCTXEX pContext, PWHRESULTEX pResults )
{
	if (pResults == NULL)
		pResults = &pContext->results;

#define WIN_HASH_FINISH_op(alg)              \
    if (pContext->flags & WHEX_CHECK##alg)   \
    {                                        \
        WHFinish##alg(&pContext->ctx##alg);  \
        WHByteToHex(pContext->ctx##alg.result, pResults->szHex##alg, alg##_DIGEST_LENGTH * 2, pContext->uCaseMode);  \
    }
    FOR_EACH_HASH(WIN_HASH_FINISH_op)
}
