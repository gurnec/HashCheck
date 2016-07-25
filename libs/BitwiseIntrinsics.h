/**
 * Bitwise Operations Library
 * Last modified: 2016/07/25
 *
 * Provides an efficient way to reverse the endianness of words, dwords, and qwords.
 **/

#ifndef __SWAPINTRINSICS_H__
#define __SWAPINTRINSICS_H__

#ifdef __cplusplus
extern "C" {
#endif


// -----------------------------------------------------------------------------
// SwapV16/SwapV32/SwapV64
// These are functions for use with variable data.
#if _MSC_VER < 1310
#   error bitwise intrinsics require MSVC 7.1+
#endif
#include <stdlib.h>
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)

#define SwapV16 _byteswap_ushort
#define SwapV32 _byteswap_ulong
#define SwapV64 _byteswap_uint64
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SwapA16I
// This is for use with arrays of words.
__forceinline void SwapA16I( unsigned short *data, size_t words )
{
	while (words)
	{
		*data = SwapV16(*data);
		++data;
		--words;
	}
}
// -----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif
