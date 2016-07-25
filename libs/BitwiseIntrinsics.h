/**
 * Bitwise Operations Library
 * Last modified: 2016/07/25
 *
 * Provides an efficient way to reverse the endianness of words, dwords, and qwords,
 * and to perform bitwise rotations.
 **/

#ifndef __SWAPINTRINSICS_H__
#define __SWAPINTRINSICS_H__

#ifdef __cplusplus
extern "C" {
#endif


// -----------------------------------------------------------------------------
// SwapV16/SwapV32/SwapV64
// Endian swap functions for 16, 32, and 64 bit words.
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


// -----------------------------------------------------------------------------
// RotLV32/RotRV32/RotLV64/RotRV64
// Rotation functions for 32 and 64 bit words.
#pragma intrinsic(_rotl)
#pragma intrinsic(_rotr)
#pragma intrinsic(_rotl64)
#pragma intrinsic(_rotr64)

#define RotLV32 _rotl
#define RotRV32 _rotr
#define RotLV64 _rotl64
#define RotRV64 _rotr64
// -----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif
