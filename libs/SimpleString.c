/**
 * SimpleString Library
 * Last modified: 2009/01/06
 * Copyright (C) Kai Liu.  All rights reserved.
 **/

#include "SimpleString.h"

// These functions are explicitly not inline
#pragma auto_inline(off)

#pragma warning(push)
#pragma warning(disable: 4035) // returns for inline asm functions

#ifdef _M_IX86

PSTR SSCALL SSChainNCpy2FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2 )
{
	__asm
	{
		// The auto-generated pushing of edi and esi onto the stack means that
		// our esp is offset by 8 bytes

		mov         edi,[esp+0x0C] // pszDest
		mov         esi,[esp+0x10] // pszSrc1
		mov         ecx,[esp+0x14] // cch1
		rep movsb
		mov         esi,[esp+0x18] // pszSrc2
		mov         ecx,[esp+0x1C] // cch2
		rep movsb
		xchg        eax,edi
	}
}

PSTR SSCALL SSChainNCpy3FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2,
                                          PCSTR pszSrc3, SIZE_T cch3 )
{
	__asm
	{
		// The auto-generated pushing of edi and esi onto the stack means that
		// our esp is offset by 8 bytes

		mov         edi,[esp+0x0C] // pszDest
		mov         esi,[esp+0x10] // pszSrc1
		mov         ecx,[esp+0x14] // cch1
		rep movsb
		mov         esi,[esp+0x18] // pszSrc2
		mov         ecx,[esp+0x1C] // cch2
		rep movsb
		mov         esi,[esp+0x20] // pszSrc3
		mov         ecx,[esp+0x24] // cch3
		rep movsb
		xchg        eax,edi
	}
}

PWSTR SSCALL SSChainNCpy2FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2 )
{
	__asm
	{
		// The auto-generated pushing of edi and esi onto the stack means that
		// our esp is offset by 8 bytes

		mov         edi,[esp+0x0C] // pszDest
		mov         esi,[esp+0x10] // pszSrc1
		mov         ecx,[esp+0x14] // cch1
		rep movsw
		mov         esi,[esp+0x18] // pszSrc2
		mov         ecx,[esp+0x1C] // cch2
		rep movsw
		xchg        eax,edi
	}
}

PWSTR SSCALL SSChainNCpy3FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2,
                                            PCWSTR pszSrc3, SIZE_T cch3 )
{
	__asm
	{
		// The auto-generated pushing of edi and esi onto the stack means that
		// our esp is offset by 8 bytes

		mov         edi,[esp+0x0C] // pszDest
		mov         esi,[esp+0x10] // pszSrc1
		mov         ecx,[esp+0x14] // cch1
		rep movsw
		mov         esi,[esp+0x18] // pszSrc2
		mov         ecx,[esp+0x1C] // cch2
		rep movsw
		mov         esi,[esp+0x20] // pszSrc3
		mov         ecx,[esp+0x24] // cch3
		rep movsw
		xchg        eax,edi
	}
}

PSTR SSCALL SSChainCpyCatA( PSTR pszDest, PCSTR pszSrc1, PCSTR pszSrc2 )
{
	__asm
	{
		// The auto-generated pushing of edi and esi onto the stack means that
		// our esp is offset by 8 bytes

		xor         eax,eax

		mov         esi,[esp+0x10] // pszSrc1
		mov         edi,esi
		or          ecx,-1
		repnz scasb
		not         ecx
		dec         ecx
		mov         edi,[esp+0x0C] // pszDest
		rep movsb

		mov         esi,[esp+0x14] // pszSrc2
		push        edi
		mov         edi,esi
		or          ecx,-1
		repnz scasb
		not         ecx
		pop         edi
		rep movsb
		dec         edi
		xchg        eax,edi
	}
}

PWSTR SSCALL SSChainCpyCatW( PWSTR pszDest, PCWSTR pszSrc1, PCWSTR pszSrc2 )
{
	__asm
	{
		// The auto-generated pushing of edi and esi onto the stack means that
		// our esp is offset by 8 bytes

		xor         eax,eax

		mov         esi,[esp+0x10] // pszSrc1
		mov         edi,esi
		or          ecx,-1
		repnz scasw
		not         ecx
		dec         ecx
		mov         edi,[esp+0x0C] // pszDest
		rep movsw

		mov         esi,[esp+0x14] // pszSrc2
		push        edi
		mov         edi,esi
		or          ecx,-1
		repnz scasw
		not         ecx
		pop         edi
		rep movsw
		dec         edi
		dec         edi
		xchg        eax,edi
	}
}

#else

PSTR SSCALL SSChainNCpy2FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2 )
{
	return(SSChainNCpy2A(pszDest, pszSrc1, cch1, pszSrc2, cch2));
}

PSTR SSCALL SSChainNCpy3FA( PSTR pszDest, PCSTR pszSrc1, SIZE_T cch1,
                                          PCSTR pszSrc2, SIZE_T cch2,
                                          PCSTR pszSrc3, SIZE_T cch3 )
{
	return(SSChainNCpy3A(pszDest, pszSrc1, cch1, pszSrc2, cch2, pszSrc3, cch3));
}

PWSTR SSCALL SSChainNCpy2FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2 )
{
	return(SSChainNCpy2W(pszDest, pszSrc1, cch1, pszSrc2, cch2));
}

PWSTR SSCALL SSChainNCpy3FW( PWSTR pszDest, PCWSTR pszSrc1, SIZE_T cch1,
                                            PCWSTR pszSrc2, SIZE_T cch2,
                                            PCWSTR pszSrc3, SIZE_T cch3 )
{
	return(SSChainNCpy3W(pszDest, pszSrc1, cch1, pszSrc2, cch2, pszSrc3, cch3));
}

PSTR SSCALL SSChainCpyCatA( PSTR pszDest, PCSTR pszSrc1, PCSTR pszSrc2 )
{
	return(SSChainNCpy2A(pszDest, pszSrc1, SSLenA(pszSrc1), pszSrc2, SSLenA(pszSrc2) + 1));
}

PWSTR SSCALL SSChainCpyCatW( PWSTR pszDest, PCWSTR pszSrc1, PCWSTR pszSrc2 )
{
	return(SSChainNCpy2W(pszDest, pszSrc1, SSLenW(pszSrc1), pszSrc2, SSLenW(pszSrc2) + 1));
}

#endif

#pragma warning(pop)
