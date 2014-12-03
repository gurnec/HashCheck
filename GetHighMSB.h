/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __GETHIGHMSB_H__
#define __GETHIGHMSB_H__

#ifdef __cplusplus
extern "C" {
#endif


// -----------------------------------------------------------------------------
// LODWORD/HIDWORD helper macros
#define LODWORD(ull) ((DWORD)((ull) & 0xFFFFFFFF))
#define HIDWORD(ull) ((DWORD)((ull) >> 32))
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GetHighMSB
// Gets the 1-based index of the most significant bit of the upper 32-bits of
// a 64-bit integer; this is the smallest number by which the integer could be
// shifted so that the upper 32-bits are unused.
#if _MSC_VER >= 1310
#ifndef __INTRIN_H_
unsigned char _BitScanReverse( unsigned long *, unsigned long );
#endif

#pragma intrinsic(_BitScanReverse)

__forceinline UINT GetHighMSB( PULARGE_INTEGER puli )
{
	UINT uIndex;

	if (_BitScanReverse(&uIndex, puli->HighPart))
		return(uIndex + 1);
	else
		return(0);
}

#elif defined(_M_IX86)
#pragma warning(push)
#pragma warning(disable: 4035) // returns for inline asm functions

__forceinline UINT GetHighMSB( PULARGE_INTEGER puli )
{
	DWORD dwHigh = puli->HighPart;

	__asm
	{
		bsr         eax,dwHigh
		jnz         done
		or          eax,-1
		done:
		inc         eax
	}
}

#pragma warning(pop)
#endif
// -----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif
