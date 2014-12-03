/**
 * SimpleString Library
 * Last modified: 2009/01/06
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This is a custom C string library that provides wide-character inline
 * intrinsics for older compilers as well as some helpful chained functions.
 **/

#ifndef __SIMPLESTRING_H__
#define __SIMPLESTRING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

// Unaligned pointer types
typedef UNALIGNED PWORD UPWORD;
typedef UNALIGNED PDWORD UPDWORD;

#if _MSC_VER >= 1400
#pragma warning(disable: 4996) // do not mother me about so-called "insecure" funcs
#endif

#if _MSC_VER >= 1500 && _MSC_VER < 1600
#pragma warning(disable: 4985) // appears to be a bug in VC9
#endif

#pragma warning(push)
#pragma warning(disable: 4035) // returns for inline asm functions


// INTRINSIC: movs, stos -------------------------------------------------------
#if _MSC_VER >= 1400 && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))
#ifndef __INTRIN_H_
void __movsb( unsigned  char *, unsigned  char const *, size_t );
void __movsw( unsigned short *, unsigned short const *, size_t );
void __movsd( unsigned  long *, unsigned  long const *, size_t );
void __stosb( unsigned  char *, unsigned  char, size_t );
void __stosw( unsigned short *, unsigned short, size_t );
void __stosd( unsigned  long *, unsigned  long, size_t );
#endif
#pragma intrinsic(__movsb, __movsw, __movsd, __stosb, __stosw, __stosd)
#endif
// END: movs, stos -------------------------------------------------------------


// INTRINSIC: memcmp, memcpy, memset -------------------------------------------
#ifndef SS_NOINTRIN_MEM
#pragma intrinsic(memcmp, memcpy, memset)
#if _MSC_VER >= 1400 && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))

__forceinline void * intrin_memcpy( void *dest, const void *src, size_t count )
{
	__movsb((unsigned char *)dest, (unsigned char const *)src, count);
	return(dest);
}

__forceinline void * intrin_memset( void *dest, int c, size_t count )
{
	__stosb((unsigned char *)dest, (unsigned char)c, count);
	return(dest);
}

#define memcpy intrin_memcpy
#define memset intrin_memset
#endif
#endif
// END: memcmp, memcpy, memset -------------------------------------------------


// INTRINSIC: strlen, wcslen ---------------------------------------------------
#ifndef SS_NOINTRIN_STRLEN
#pragma intrinsic(strlen)
#if _MSC_VER >= 1400
#pragma intrinsic(wcslen)
#elif defined(_M_IX86)

__forceinline size_t intrin_strlen_w( const wchar_t *string )
{
	__asm
	{
		xor         eax,eax
		mov         edi,string
		or          ecx,-1
		repnz scasw
		not         ecx
		dec         ecx
		xchg        eax,ecx
	}
}

#define wcslen intrin_strlen_w
#endif
#endif
// END: strlen, wcslen ---------------------------------------------------------


// INTRINSIC: strcpy, wcscpy ---------------------------------------------------
#ifndef SS_NOINTRIN_STRCPY
#pragma intrinsic(strcpy)
#if _MSC_VER >= 1400
#pragma intrinsic(wcscpy)
#elif defined(_M_IX86)

__forceinline wchar_t * intrin_strcpy_w( wchar_t *dest, const wchar_t *src )
{
	__asm
	{
		xor         eax,eax
		mov         esi,src
		mov         edi,esi
		or          ecx,-1
		repnz scasw
		not         ecx
		mov         edi,dest
		rep movsw
	}

	return(dest);
}

#define wcscpy intrin_strcpy_w
#endif
#endif
// END: strcpy, wcscpy ---------------------------------------------------------


// INTRINSIC: strcat, wcscat ---------------------------------------------------
#ifndef SS_NOINTRIN_STRCAT
#pragma intrinsic(strcat)
#if _MSC_VER >= 1400
#pragma intrinsic(wcscat)
#elif defined(_M_IX86)

__forceinline wchar_t * intrin_strcat_w( wchar_t *dest, const wchar_t *src )
{
	__asm
	{
		xor         eax,eax
		mov         esi,src
		mov         edi,esi
		or          ecx,-1
		repnz scasw
		not         ecx
		mov         edi,dest
		push        ecx
		or          ecx,-1
		repnz scasw
		pop         ecx
		dec         edi
		dec         edi
		rep movsw
	}

	return(dest);
}

#define wcscat intrin_strcat_w
#endif
#endif
// END: strcat, wcscat ---------------------------------------------------------


// BEGIN: SSLen, SSCpy, SSCat --------------------------------------------------
#define SSLenA strlen
#define SSLenW wcslen

#define SSCpyA strcpy
#define SSCpyW wcscpy

#define SSCatA strcat
#define SSCatW wcscat

#ifdef UNICODE
#define SSLen SSLenW
#define SSCpy SSCpyW
#define SSCat SSCatW
#else
#define SSLen SSLenA
#define SSCpy SSCpyA
#define SSCat SSCatA
#endif
// END: SSLen, SSCpy, SSCat ----------------------------------------------------


#define SSINLINE __forceinline
#define SSCALL __stdcall


// BEGIN: SSCpyNCh -------------------------------------------------------------
SSINLINE VOID SSInternal_Cpy16( PVOID pvDest,  WORD  wSource ) { *( (UPWORD)pvDest) =  wSource; }
SSINLINE VOID SSInternal_Cpy32( PVOID pvDest, DWORD dwSource ) { *((UPDWORD)pvDest) = dwSource; }

#define CAST2BYTE(a)                ((BYTE)((DWORD_PTR)(a) & 0xFF))
#define CAST2WORD(a)                ((WORD)((DWORD_PTR)(a) & 0xFFFF))
#define CHARS2WORD(a, b)            ((WORD)(CAST2BYTE(a) | ((WORD)CAST2BYTE(b)) << 8))
#define WCHARS2DWORD(a, b)          ((DWORD)(CAST2WORD(a) | ((DWORD)CAST2WORD(b)) << 16))
#define CHARS2DWORD(a, b, c, d)     WCHARS2DWORD(CHARS2WORD(a, b), CHARS2WORD(c, d))
#define SSCpy2ChA(s, a, b)          SSInternal_Cpy16(s, CHARS2WORD(a, b))
#define SSCpy2ChW(s, a, b)          SSInternal_Cpy32(s, WCHARS2DWORD(a, b))
#define SSCpy4ChA(s, a, b, c, d)    SSInternal_Cpy32(s, CHARS2DWORD(a, b, c, d))

#ifdef UNICODE
#define SSCpy2Ch SSCpy2ChW
#else
#define SSCpy2Ch SSCpy2ChA
#endif
// END: SSCpyNCh ---------------------------------------------------------------


// BEGIN: SSChainNCpy ----------------------------------------------------------
#if _MSC_VER >= 1400 && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))

SSINLINE PSTR SSChainNCpyA( PSTR pszDest, PCSTR pszSrc, SIZE_T cch )
{
	__movsb((unsigned char *)pszDest, (unsigned char const *)pszSrc, cch);
	return(pszDest + cch);
}

SSINLINE PWSTR SSChainNCpyW( PWSTR pszDest, PCWSTR pszSrc, SIZE_T cch )
{
	__movsw((unsigned short *)pszDest, (unsigned short const *)pszSrc, cch);
	return(pszDest + cch);
}

#else

SSINLINE PSTR SSChainNCpyA( PSTR pszDest, PCSTR pszSrc, SIZE_T cch )
{
	memcpy(pszDest, pszSrc, cch);
	return(pszDest + cch);
}

SSINLINE PWSTR SSChainNCpyW( PWSTR pszDest, PCWSTR pszSrc, SIZE_T cch )
{
	memcpy(pszDest, pszSrc, cch * sizeof(WCHAR));
	return(pszDest + cch);
}

#endif

#ifdef UNICODE
#define SSChainNCpy SSChainNCpyW
#else
#define SSChainNCpy SSChainNCpyA
#endif
// END: SSChainNCpy ------------------------------------------------------------


// BEGIN: SSChainNCpy2/3 -------------------------------------------------------
#if defined(_M_IX86)

SSINLINE PSTR SSChainNCpy2A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2 )
{
	__asm
	{
		mov         edi,pszDest
		mov         esi,pszSrc1
		mov         ecx,cch1
		rep movsb
		mov         esi,pszSrc2
		mov         ecx,cch2
		rep movsb
		xchg        eax,edi
	}
}

SSINLINE PSTR SSChainNCpy3A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2,
                                           PCSTR pszSrc3, SIZE_T cch3 )
{
	__asm
	{
		mov         edi,pszDest
		mov         esi,pszSrc1
		mov         ecx,cch1
		rep movsb
		mov         esi,pszSrc2
		mov         ecx,cch2
		rep movsb
		mov         esi,pszSrc3
		mov         ecx,cch3
		rep movsb
		xchg        eax,edi
	}
}

SSINLINE PWSTR SSChainNCpy2W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2 )
{
	__asm
	{
		mov         edi,pszDest
		mov         esi,pszSrc1
		mov         ecx,cch1
		rep movsw
		mov         esi,pszSrc2
		mov         ecx,cch2
		rep movsw
		xchg        eax,edi
	}
}

SSINLINE PWSTR SSChainNCpy3W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2,
                                             PCWSTR pszSrc3, SIZE_T cch3 )
{
	__asm
	{
		mov         edi,pszDest
		mov         esi,pszSrc1
		mov         ecx,cch1
		rep movsw
		mov         esi,pszSrc2
		mov         ecx,cch2
		rep movsw
		mov         esi,pszSrc3
		mov         ecx,cch3
		rep movsw
		xchg        eax,edi
	}
}

#else

SSINLINE PSTR SSChainNCpy2A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2 )
{
	pszDest = SSChainNCpyA(pszDest, pszSrc1, cch1);
	return(SSChainNCpyA(pszDest, pszSrc2, cch2));
}

SSINLINE PSTR SSChainNCpy3A( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                           PCSTR pszSrc2, SIZE_T cch2,
                                           PCSTR pszSrc3, SIZE_T cch3 )
{
	pszDest = SSChainNCpyA(pszDest, pszSrc1, cch1);
	pszDest = SSChainNCpyA(pszDest, pszSrc2, cch2);
	return(SSChainNCpyA(pszDest, pszSrc3, cch3));
}

SSINLINE PWSTR SSChainNCpy2W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2 )
{
	pszDest = SSChainNCpyW(pszDest, pszSrc1, cch1);
	return(SSChainNCpyW(pszDest, pszSrc2, cch2));
}

SSINLINE PWSTR SSChainNCpy3W( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                             PCWSTR pszSrc2, SIZE_T cch2,
                                             PCWSTR pszSrc3, SIZE_T cch3 )
{
	pszDest = SSChainNCpyW(pszDest, pszSrc1, cch1);
	pszDest = SSChainNCpyW(pszDest, pszSrc2, cch2);
	return(SSChainNCpyW(pszDest, pszSrc3, cch3));
}

#endif

#ifdef UNICODE
#define SSChainNCpy2 SSChainNCpy2W
#define SSChainNCpy3 SSChainNCpy3W
#else
#define SSChainNCpy2 SSChainNCpy2A
#define SSChainNCpy3 SSChainNCpy3A
#endif
// END: SSChainNCpy2/3 ---------------------------------------------------------


// BEGIN: SSChainNCpy2F/3F -----------------------------------------------------
PSTR  SSCALL SSChainNCpy2FA( PSTR  pszDest, PCSTR  pszSrc1, SIZE_T cch1,
                                            PCSTR  pszSrc2, SIZE_T cch2 );
PSTR  SSCALL SSChainNCpy3FA( PSTR  pszDest, PCSTR  pszSrc1, SIZE_T cch1,
                                            PCSTR  pszSrc2, SIZE_T cch2,
                                            PCSTR  pszSrc3, SIZE_T cch3 );
PWSTR SSCALL SSChainNCpy2FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2 );
PWSTR SSCALL SSChainNCpy3FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2,
                                            PCWSTR pszSrc3, SIZE_T cch3 );

#ifdef UNICODE
#define SSChainNCpy2F SSChainNCpy2FW
#define SSChainNCpy3F SSChainNCpy3FW
#else
#define SSChainNCpy2F SSChainNCpy2FA
#define SSChainNCpy3F SSChainNCpy3FA
#endif
// END: SSChainNCpy2F/3F -------------------------------------------------------


// BEGIN: SSChainCpyCat --------------------------------------------------------
PSTR SSCALL SSChainCpyCatA( PSTR pszDest, PCSTR pszSrc1, PCSTR pszSrc2 );
PWSTR SSCALL SSChainCpyCatW( PWSTR pszDest, PCWSTR pszSrc1, PCWSTR pszSrc2 );

#ifdef UNICODE
#define SSChainCpyCat SSChainCpyCatW
#else
#define SSChainCpyCat SSChainCpyCatA
#endif
// END: SSChainCpyCat ----------------------------------------------------------


// BEGIN: SSStaticCpy ----------------------------------------------------------
#if _MSC_VER < 1400
#define SSStaticCpyA(dest, src) memcpy(dest, src, sizeof(src))
#define SSStaticCpyW(dest, src) memcpy(dest, src, sizeof(src))
#else
#define SSStaticCpyA(dest, src) SSChainNCpyA(dest, src, sizeof(src))
#define SSStaticCpyW(dest, src) SSChainNCpyW(dest, src, sizeof(src)/sizeof(WCHAR))
#endif

#ifdef UNICODE
#define SSStaticCpy SSStaticCpyW
#else
#define SSStaticCpy SSStaticCpyA
#endif
// END: SSStaticCpy ------------------------------------------------------------


#pragma warning(pop)

#ifdef __cplusplus
}
#endif

#endif
