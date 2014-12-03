/**
 * Endianness Conversion (Swap) Library
 * Last modified: 2008/12/13
 *
 * Provides an efficient way to reverse the endianness of words or dwords, if
 * the compiler does not provide an intrinsic for it, and also provides macros
 * for use with constants that can be computed at compile-time.
 **/

#ifndef __SWAPINTRINSICS_H__
#define __SWAPINTRINSICS_H__

#ifdef __cplusplus
extern "C" {
#endif


// -----------------------------------------------------------------------------
// SwapV16/SwapV32
// These are functions for use with variable data (for constants, the Swap__C
// macros are more efficient, as they can be evaluated at compile-time); for
// VC7.1+, we can just use the compiler's built-in intrinsic, but for older
// versions, we will have to define our own function.
#if _MSC_VER >= 1310
#include <stdlib.h>
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#elif defined(_M_IX86)
#pragma warning(push)
#pragma warning(disable: 4035) // returns for inline asm functions

__forceinline unsigned short _byteswap_ushort( unsigned short val )
{
	__asm
	{
		mov         ax,val
		xchg        al,ah
	}
}

__forceinline unsigned long _byteswap_ulong( unsigned long val )
{
	__asm
	{
		mov         eax,val
		bswap       eax
	}
}

#pragma warning(pop)
#endif

#define SwapV16 _byteswap_ushort
#define SwapV32 _byteswap_ulong
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SwapC16/SwapC32
// These are macros for use with constants--these are values that can be
// evaluated at compile-time and "baked" into the code; do not use these for
// run-time swapping, as they are very inefficient.
#include <windows.h>
#define SwapC16(x) ((UINT16)MAKEWORD(HIBYTE(x), LOBYTE(x)))
#define SwapC32(x) ((UINT32)MAKELONG(SwapC16(HIWORD(x)), SwapC16(LOWORD(x))))
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SwapA16(I)/SwapA32(I)
// These are functions for use with arrays of words or dwords; the "I" variants
// are inline while the non-"I" versions require SwapIntrinsics.c.
#define SwapA16(data, count) SwapA16R(count, data)
#define SwapA32(data, count) SwapA32R(count, data)

__forceinline void SwapA16I( unsigned short *data, size_t words )
{
	while (words)
	{
		*data = SwapV16(*data);
		++data;
		--words;
	}
}

__forceinline void SwapA32I( unsigned long *data, size_t dwords )
{
	while (dwords)
	{
		*data = SwapV32(*data);
		++data;
		--dwords;
	}
}

void __fastcall SwapA16R( size_t words, unsigned short *data );
void __fastcall SwapA32R( size_t dwords, unsigned long *data );

// -----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif
