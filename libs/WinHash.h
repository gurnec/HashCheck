/**
 * Windows Hashing/Checksumming Library
 * Last modified: 2016/02/21
 * Original work copyright (C) Kai Liu.  All rights reserved.
 * Modified work copyright (C) 2014, 2016 Christopher Gurnee.  All rights reserved.
 * Modified work copyright (C) 2016 Tim Schlueter.  All rights reserved.
 *
 * This is a wrapper for the CRC32, MD5, SHA1, SHA2-256, and SHA2-512
 * algorithms.
 **/

#ifndef __WINHASH_H__
#define __WINHASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <tchar.h>
#include "sha3/KeccakHash.h"
#include "BitwiseIntrinsics.h"

#if _MSC_VER >= 1600 && !defined(NO_PPL)
#define USE_PPL
#endif

typedef CONST BYTE *PCBYTE;

#define CRLF _T("\r\n")
#define CCH_CRLF 2

/**
 * Returns the offset of a member in a struct such that:
 * type * t; &t->member == ((BYTE *) t) + FINDOFFSET(type, member)
 */
#define FINDOFFSET(type,member) (&(((type *) 0)->member))

/**
 * Apply a macro for every hash algorithm
 * @param   op      A macro to perform on every hash algorithm
 */
#define FOR_EACH_HASH(op)   op(CRC32)   \
                            op(MD5)     \
                            op(SHA1)    \
                            op(SHA256)  \
                            op(SHA512)  \
                            op(SHA3_256)\
                            op(SHA3_512)
// In approximate order from longest to shortest compute time
#define FOR_EACH_HASH_R(op) op(SHA512)  \
                            op(SHA256)  \
                            op(SHA3_512)\
                            op(SHA3_256)\
                            op(SHA1)    \
                            op(CRC32)   \
                            op(MD5)

/**
 * Some constants related to the hash algorithms
 */

// Hash algorithms
enum hash_algorithm {
    CRC32 = 1,
    MD5,
    SHA1,
    SHA256,
    SHA512,
    SHA3_256,
    SHA3_512
};
#define NUM_HASHES SHA3_512

// The default hash algorithm to use when creating a checksum file
#define DEFAULT_HASH_ALGORITHM SHA256
// and when viewing checksums in the explorer file propery sheet
// (if this is changed, also update the test in HashProp.cs)
#define DEFAULT_HASH_ALGORITHMS (WHEX_CHECKCRC32 | WHEX_CHECKSHA1 | WHEX_CHECKSHA256 | WHEX_CHECKSHA512)

// Bitwise representation of the hash algorithms
#define WHEX_CHECKCRC32     (1UL << (CRC32  - 1))
#define WHEX_CHECKMD5       (1UL << (MD5    - 1))
#define WHEX_CHECKSHA1      (1UL << (SHA1   - 1))
#define WHEX_CHECKSHA256    (1UL << (SHA256 - 1))
#define WHEX_CHECKSHA512    (1UL << (SHA512 - 1))
#define WHEX_CHECKSHA3_256  (1UL << (SHA3_256 - 1))
#define WHEX_CHECKSHA3_512  (1UL << (SHA3_512 - 1))
#define WHEX_CHECKLAST      WHEX_CHECKSHA3_512

// Bitwise representation of the hash algorithms, by digest length (in bits)
#define WHEX_ALL            ((1UL << NUM_HASHES) - 1)
#define WHEX_ALL32          WHEX_CHECKCRC32
#define WHEX_ALL128         WHEX_CHECKMD5
#define WHEX_ALL160         WHEX_CHECKSHA1
#define WHEX_ALL256         (WHEX_CHECKSHA256 | WHEX_CHECKSHA3_256)
#define WHEX_ALL512         (WHEX_CHECKSHA512 | WHEX_CHECKSHA3_512)

// The block lengths of the hash algorithms, if required below
#define MD5_BLOCK_LENGTH            64
#define SHA1_BLOCK_LENGTH           64
#define SHA224_BLOCK_LENGTH         64
#define SHA256_BLOCK_LENGTH         64
#define SHA384_BLOCK_LENGTH         128
#define SHA512_BLOCK_LENGTH         128

// The digest lengths of the hash algorithms
#define CRC32_DIGEST_LENGTH         4
#define MD5_DIGEST_LENGTH           16
#define SHA1_DIGEST_LENGTH          20
#define SHA224_DIGEST_LENGTH        28
#define SHA256_DIGEST_LENGTH        32
#define SHA384_DIGEST_LENGTH        48
#define SHA512_DIGEST_LENGTH        64
#define SHA3_256_DIGEST_LENGTH      32
#define SHA3_512_DIGEST_LENGTH      64
#define MAX_DIGEST_LENGTH           SHA512_DIGEST_LENGTH

// The minimum string length required to hold the hex digest strings
#define CRC32_DIGEST_STRING_LENGTH  (CRC32_DIGEST_LENGTH  * 2 + 1)
#define MD5_DIGEST_STRING_LENGTH    (MD5_DIGEST_LENGTH    * 2 + 1)
#define SHA1_DIGEST_STRING_LENGTH   (SHA1_DIGEST_LENGTH   * 2 + 1)
#define SHA224_DIGEST_STRING_LENGTH (SHA224_DIGEST_LENGTH * 2 + 1)
#define SHA256_DIGEST_STRING_LENGTH (SHA256_DIGEST_LENGTH * 2 + 1)
#define SHA384_DIGEST_STRING_LENGTH (SHA384_DIGEST_LENGTH * 2 + 1)
#define SHA512_DIGEST_STRING_LENGTH (SHA512_DIGEST_LENGTH * 2 + 1)
#define SHA3_256_DIGEST_STRING_LENGTH (SHA3_256_DIGEST_LENGTH * 2 + 1)
#define SHA3_512_DIGEST_STRING_LENGTH (SHA3_512_DIGEST_LENGTH * 2 + 1)
#define MAX_DIGEST_STRING_LENGTH    SHA512_DIGEST_STRING_LENGTH

// Hash file extensions
#define HASH_EXT_CRC32          _T(".sfv")
#define HASH_EXT_MD5            _T(".md5")
#define HASH_EXT_SHA1           _T(".sha1")
#define HASH_EXT_SHA256         _T(".sha256")
#define HASH_EXT_SHA512         _T(".sha512")
#define HASH_EXT_SHA3_256       _T(".sha3-256")
#define HASH_EXT_SHA3_512       _T(".sha3-512")

// Table of supported Hash file extensions
extern LPCTSTR g_szHashExtsTab[NUM_HASHES];

// Hash names
#define HASH_NAME_CRC32         _T("CRC-32")
#define HASH_NAME_MD5           _T("MD5")
#define HASH_NAME_SHA1          _T("SHA-1")
#define HASH_NAME_SHA256        _T("SHA-256")
#define HASH_NAME_SHA512        _T("SHA-512")
#define HASH_NAME_SHA3_256      _T("SHA3-256")
#define HASH_NAME_SHA3_512      _T("SHA3-512")

// Right-justified Hash names
#define HASH_RNAME_CRC32        _T("  CRC-32")
#define HASH_RNAME_MD5          _T("     MD5")
#define HASH_RNAME_SHA1         _T("   SHA-1")
#define HASH_RNAME_SHA256       _T(" SHA-256")
#define HASH_RNAME_SHA512       _T(" SHA-512")
#define HASH_RNAME_SHA3_256     _T("SHA3-256")
#define HASH_RNAME_SHA3_512     _T("SHA3-512")

// Hash OPENFILENAME filters, E.G. "MD5 (*.md5)\0*.md5\0"
#define HASH_FILTER_op(alg)     HASH_NAME_##alg _T(" (*")   \
                                HASH_EXT_##alg  _T(")\0*")  \
                                HASH_EXT_##alg  _T("\0")

// All OPENFILENAME filters together as one big string
#define HASH_FILE_FILTERS       FOR_EACH_HASH(HASH_FILTER_op)

// Hash results strings (colon aligned).
// E.g. "    MD5: "
#define HASH_RESULT_op(alg)     HASH_RNAME_##alg _T(": ")

/**
 * Structures used by the system libraries
 **/

typedef struct {
	UINT32 state[4];
	UINT64 count;
	BYTE buffer[MD5_BLOCK_LENGTH];
	BYTE result[MD5_DIGEST_LENGTH];
} MD5_CTX, *PMD5_CTX;

typedef struct {
	UINT32 state[5];
	UINT64 count;
	BYTE buffer[SHA1_BLOCK_LENGTH];
	BYTE result[SHA1_DIGEST_LENGTH];
} SHA1_CTX, *PSHA1_CTX;

typedef struct _SHA2_CTX {
	union {
		UINT32	st32[8];
		UINT64	st64[8];
	} state;
	UINT64 bitcount[2];
	BYTE buffer[SHA512_BLOCK_LENGTH];
	BYTE result[SHA512_DIGEST_LENGTH];
} SHA2_CTX, *PSHA2_CTX;


UINT32 crc32( UINT32 uInitial, PCBYTE pbIn, UINT cbIn );

void MD5Init( PMD5_CTX pContext );
void MD5Update( PMD5_CTX pContext, PCBYTE pbIn, UINT cbIn );
void MD5Final( PMD5_CTX pContext );

void SHA1Init( PSHA1_CTX pContext );
void SHA1Update( PSHA1_CTX pContext, PCBYTE pbIn, UINT cbIn );
void SHA1Final( PSHA1_CTX pContext );

void SHA256Init( PSHA2_CTX pContext );
void SHA256Update( PSHA2_CTX pContext, PCBYTE pbIn, UINT cbIn );
void SHA256Final( PSHA2_CTX pContext );

void SHA512Init( PSHA2_CTX pContext );
void SHA512Update( PSHA2_CTX pContext, PCBYTE pbIn, UINT cbIn );
void SHA512Final( PSHA2_CTX pContext );

/**
 * Structures used by our consistency wrapper layer
 **/

typedef union {
	UINT32 state;
	BYTE result[CRC32_DIGEST_LENGTH];
} WHCTXCRC32, *PWHCTXCRC32;

#define  WHCTXMD5  MD5_CTX
#define PWHCTXMD5 PMD5_CTX

#define  WHCTXSHA1  SHA1_CTX
#define PWHCTXSHA1 PSHA1_CTX

#define  WHCTXSHA256  SHA2_CTX
#define PWHCTXSHA256 PSHA2_CTX

#define  WHCTXSHA512  SHA2_CTX
#define PWHCTXSHA512 PSHA2_CTX

typedef struct {
    Keccak_HashInstance state;
    BYTE result[SHA3_256_DIGEST_LENGTH];
} WHCTXSHA3_256, *PWHCTXSHA3_256;

typedef struct {
    Keccak_HashInstance state;
    BYTE result[SHA3_512_DIGEST_LENGTH];
} WHCTXSHA3_512, *PWHCTXSHA3_512;

/**
 * Wrapper layer functions to ensure a more consistent interface
 **/

#define  WHAPI  __fastcall

__inline void WHAPI WHInitCRC32( PWHCTXCRC32 pContext )
{
	pContext->state = 0;
}

__inline void WHAPI WHUpdateCRC32( PWHCTXCRC32 pContext, PCBYTE pbIn, UINT cbIn )
{
	pContext->state = crc32(pContext->state, pbIn, cbIn);
}

__inline void WHAPI WHFinishCRC32( PWHCTXCRC32 pContext )
{
	pContext->state = SwapV32(pContext->state);
}

#define WHInitMD5 MD5Init
#define WHUpdateMD5 MD5Update
#define WHFinishMD5 MD5Final

#define WHInitSHA1 SHA1Init
#define WHUpdateSHA1 SHA1Update
#define WHFinishSHA1 SHA1Final

#define WHInitSHA256 SHA256Init
#define WHUpdateSHA256 SHA256Update
#define WHFinishSHA256 SHA256Final

#define WHInitSHA512 SHA512Init
#define WHUpdateSHA512 SHA512Update
#define WHFinishSHA512 SHA512Final

__inline void WHAPI WHInitSHA3_256( PWHCTXSHA3_256 pContext )
{
    Keccak_HashInitialize_SHA3_256(&pContext->state);
}

__inline void WHAPI WHUpdateSHA3_256( PWHCTXSHA3_256 pContext, PCBYTE pbIn, UINT cbIn)
{
    Keccak_HashUpdate(&pContext->state, pbIn, cbIn * 8);
}

__inline void WHAPI WHFinishSHA3_256( PWHCTXSHA3_256 pContext )
{
    Keccak_HashFinal(&pContext->state, pContext->result);
}

__inline void WHAPI WHInitSHA3_512(PWHCTXSHA3_512 pContext)
{
    Keccak_HashInitialize_SHA3_512(&pContext->state);
}

__inline void WHAPI WHUpdateSHA3_512(PWHCTXSHA3_512 pContext, PCBYTE pbIn, UINT cbIn)
{
    Keccak_HashUpdate(&pContext->state, pbIn, cbIn * 8);
}

__inline void WHAPI WHFinishSHA3_512(PWHCTXSHA3_512 pContext)
{
    Keccak_HashFinal(&pContext->state, pContext->result);
}

/**
 * WH*To* hex string conversion functions: These require WinHash.cpp
 **/

#define WHAPI __fastcall

#define WHFMT_UPPERCASE 0x00
#define WHFMT_LOWERCASE 0x20

BOOL WHAPI WHHexToByte( PTSTR pszSrc, PBYTE pbDest, UINT cchHex );
PTSTR WHAPI WHByteToHex( PBYTE pbSrc, PTSTR pszDest, UINT cchHex, UINT8 uCaseMode );

/**
 * WH*Ex functions: These require WinHash.cpp
 **/

typedef struct {
    TCHAR szHexCRC32[CRC32_DIGEST_STRING_LENGTH];
    TCHAR szHexMD5[MD5_DIGEST_STRING_LENGTH];
    TCHAR szHexSHA1[SHA1_DIGEST_STRING_LENGTH];
    TCHAR szHexSHA256[SHA256_DIGEST_STRING_LENGTH];
    TCHAR szHexSHA512[SHA512_DIGEST_STRING_LENGTH];
    TCHAR szHexSHA3_256[SHA3_256_DIGEST_STRING_LENGTH];
    TCHAR szHexSHA3_512[SHA3_512_DIGEST_STRING_LENGTH];
    DWORD dwFlags;
} WHRESULTEX, *PWHRESULTEX;

// Align all the hash contexts to avoid false sharing (of L1/2 cache lines in multi-core systems)
typedef struct {
	__declspec(align(64)) WHCTXCRC32  ctxCRC32;
	__declspec(align(64)) WHCTXMD5    ctxMD5;
	__declspec(align(64)) WHCTXSHA1   ctxSHA1;
	__declspec(align(64)) WHCTXSHA256 ctxSHA256;
	__declspec(align(64)) WHCTXSHA512 ctxSHA512;
	__declspec(align(64)) WHCTXSHA3_256 ctxSHA3_256;
	__declspec(align(64)) WHCTXSHA3_512 ctxSHA3_512;
	DWORD dwFlags;
	UINT8 uCaseMode;
} WHCTXEX, *PWHCTXEX;


VOID WHAPI WHInitEx( PWHCTXEX pContext );
VOID WHAPI WHUpdateEx( PWHCTXEX pContext, PCBYTE pbIn, UINT cbIn );
VOID WHAPI WHFinishEx( PWHCTXEX pContext, PWHRESULTEX pResults );

#ifdef __cplusplus
}
#endif

#endif
