/**
 * Windows Hashing/Checksumming Library
 * Last modified: 2009/01/13
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This is a wrapper for the MD4, MD5, SHA1, and CRC32 algorithms supported
 * natively by the Windows API.
 **/

#ifndef __WINHASH_H__
#define __WINHASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "SwapIntrinsics.h"

typedef CONST BYTE *PCBYTE;

/**
 * Structures used by the system libraries
 **/

typedef struct {
	UINT32 state[4];
	UINT32 count[2];
	BYTE buffer[64];
	BYTE result[16];
} MD4_CTX, *PMD4_CTX, MD5_CTX, *PMD5_CTX;

typedef struct {
	UINT32 reserved[6];
	UINT32 state[5];
	UINT32 count[2];
	BYTE buffer[64];
} SHA1_CTX, *PSHA1_CTX;

/**
 * Functions imported from system libraries
 **/

#ifndef WH_NO_DLL_IMPORT
#define WHDLLIMP __declspec( dllimport )
#else
#define WHDLLIMP
#endif

WHDLLIMP UINT32 WINAPI RtlComputeCrc32( UINT32 uInitial, PCBYTE pbIn, UINT cbIn );

WHDLLIMP VOID WINAPI MD4Init( PMD4_CTX pContext );
WHDLLIMP VOID WINAPI MD4Update( PMD4_CTX pContext, PCBYTE pbIn, UINT cbIn );
WHDLLIMP VOID WINAPI MD4Final( PMD4_CTX pContext );

WHDLLIMP VOID WINAPI MD5Init( PMD5_CTX pContext );
WHDLLIMP VOID WINAPI MD5Update( PMD5_CTX pContext, PCBYTE pbIn, UINT cbIn );
WHDLLIMP VOID WINAPI MD5Final( PMD5_CTX pContext );

WHDLLIMP VOID WINAPI A_SHAInit( PSHA1_CTX pContext );
WHDLLIMP VOID WINAPI A_SHAUpdate( PSHA1_CTX pContext, PCBYTE pbIn, UINT cbIn );
WHDLLIMP VOID WINAPI A_SHAFinal( PSHA1_CTX pContext, PBYTE pbResult );

/**
 * Structures used by our consistency wrapper layer
 **/

typedef union {
	UINT32 state;
	BYTE result[4];
} WHCTXCRC32, *PWHCTXCRC32;

#define  WHCTXMD4  MD4_CTX
#define PWHCTXMD4 PMD4_CTX
#define  WHCTXMD5  MD5_CTX
#define PWHCTXMD5 PMD5_CTX

typedef struct {
	SHA1_CTX ctx;
	BYTE result[20];
} WHCTXSHA1, *PWHCTXSHA1;

typedef struct {
	MD4_CTX ctxList;
	MD4_CTX ctxChunk;
	PBYTE result;
	UINT cbChunkRemaining;
} WHCTXED2K, *PWHCTXED2K;

/**
 * Wrapper layer functions to ensure a more consistent interface
 **/

__forceinline VOID WHInitCRC32( PWHCTXCRC32 pContext )
{
	pContext->state = 0;
}

__forceinline VOID WHUpdateCRC32( PWHCTXCRC32 pContext, PCBYTE pbIn, UINT cbIn )
{
	pContext->state = RtlComputeCrc32(pContext->state, pbIn, cbIn);
}

__forceinline VOID WHFinishCRC32( PWHCTXCRC32 pContext )
{
	pContext->state = SwapV32(pContext->state);
}

#define WHInitMD4 MD4Init
#define WHUpdateMD4 MD4Update
#define WHFinishMD4 MD4Final

#define WHInitMD5 MD5Init
#define WHUpdateMD5 MD5Update
#define WHFinishMD5 MD5Final

__forceinline VOID WHInitSHA1( PWHCTXSHA1 pContext )
{
	A_SHAInit(&pContext->ctx);
}

__forceinline VOID WHUpdateSHA1( PWHCTXSHA1 pContext, PCBYTE pbIn, UINT cbIn )
{
	A_SHAUpdate(&pContext->ctx, pbIn, cbIn);
}

__forceinline VOID WHFinishSHA1( PWHCTXSHA1 pContext )
{
	A_SHAFinal(&pContext->ctx, pContext->result);
}

__forceinline VOID WHInitED2K( PWHCTXED2K pContext )
{
	MD4Init(&pContext->ctxList);
	MD4Init(&pContext->ctxChunk);
	pContext->cbChunkRemaining = 9500 << 10;
	pContext->result = pContext->ctxChunk.result;
}

__forceinline VOID WHUpdateED2K( PWHCTXED2K pContext, PCBYTE pbIn, UINT cbIn )
{
	if (cbIn >= pContext->cbChunkRemaining)
	{
		// Finish off the current chunk and add it to the list hash
		MD4Update(&pContext->ctxChunk, pbIn, pContext->cbChunkRemaining);
		MD4Final(&pContext->ctxChunk);
		MD4Update(&pContext->ctxList, pContext->ctxChunk.result, sizeof(pContext->ctxChunk.result));
		pbIn += pContext->cbChunkRemaining;
		cbIn -= pContext->cbChunkRemaining;

		// Reset the chunk context
		MD4Init(&pContext->ctxChunk);
		pContext->cbChunkRemaining = 9500 << 10;

		// The final result will now be the list hash, not the chunk hash
		pContext->result = pContext->ctxList.result;
	}

	MD4Update(&pContext->ctxChunk, pbIn, cbIn);
	pContext->cbChunkRemaining -= cbIn;
}

__forceinline VOID WHFinishED2K( PWHCTXED2K pContext )
{
	MD4Final(&pContext->ctxChunk);
	MD4Update(&pContext->ctxList, pContext->ctxChunk.result, sizeof(pContext->ctxChunk.result));
	MD4Final(&pContext->ctxList);
}

/**
 * WH*To* hex string conversion functions: These require WinHash.c
 **/

#define WHAPI __fastcall

#define WHFMT_UPPERCASE 0x00
#define WHFMT_LOWERCASE 0x20

BOOL WHAPI WHHexToByte( PTSTR pszSrc, PBYTE pbDest, UINT cchHex );
PTSTR WHAPI WHByteToHex( PBYTE pbSrc, PTSTR pszDest, UINT cchHex, UINT8 uCaseMode );

/**
 * WH*Ex functions: These require WinHash.c
 **/

typedef struct {
	TCHAR szHexCRC32[9];
	TCHAR szHexMD4[33];
	TCHAR szHexMD5[33];
	TCHAR szHexSHA1[41];
} WHRESULTEX, *PWHRESULTEX;

typedef struct {
	UINT8      flags;
	UINT8      uCaseMode;
	WHCTXCRC32 ctxCRC32;
	WHCTXMD4   ctxMD4;
	WHCTXMD5   ctxMD5;
	WHCTXSHA1  ctxSHA1;
	WHRESULTEX results;
} WHCTXEX, *PWHCTXEX;

#define WHEX_CHECKCRC32 0x01
#define WHEX_CHECKMD4   0x02
#define WHEX_CHECKMD5   0x04
#define WHEX_CHECKSHA1  0x08
#define WHEX_ALL        0x0F
#define WHEX_ALL32      0x01
#define WHEX_ALL128     0x06
#define WHEX_ALL160     0x08

VOID WHAPI WHInitEx( PWHCTXEX pContext );
VOID WHAPI WHUpdateEx( PWHCTXEX pContext, PCBYTE pbIn, UINT cbIn );
VOID WHAPI WHFinishEx( PWHCTXEX pContext, PWHRESULTEX pResults );

#ifdef __cplusplus
}
#endif

#endif
