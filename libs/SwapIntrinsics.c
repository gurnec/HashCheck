/**
 * Endianness Conversion (Swap) Library
 * Last modified: 2008/12/13
 *
 * Provides an efficient way to reverse the endianness of words or dwords, if
 * the compiler does not provide an intrinsic for it, and also provides macros
 * for use with constants that can be computed at compile-time.
 *
 * SwapIntrinsics.c is needed only if SwapA16/32 is used.
 *
 * Since both __fastcall and the Windows x86-64 calling conventions pass the
 * first parameter as ECX/RCX and since ECX/RCX is more suitable for use as a
 * counter than other registers, we reverse the parameter order.
 **/

#include "SwapIntrinsics.h"

// These functions are explicitly not inline
#pragma auto_inline(off)

#ifdef _M_IX86

void __fastcall SwapA16R( size_t words, unsigned short *data )
{
	__asm
	{
		// ecx: words
		// edx: data
		jecxz       loopend

		loopstart:
		mov         ax,[edx]
		mov         [edx],ah
		inc         edx
		mov         [edx],al
		inc         edx
		dec         ecx
		jnz         loopstart

		loopend:
	}
}

void __fastcall SwapA32R( size_t dwords, unsigned long *data )
{
	__asm
	{
		// ecx: dwords
		// edx: data
		jecxz       loopend

		loopstart:
		mov         eax,[edx]
		bswap       eax
		mov         [edx],eax
		add         edx,4
		dec         ecx
		jnz         loopstart

		loopend:
	}
}

#else

void __fastcall SwapA16R( size_t words, unsigned short *data ) { SwapA16I(data,  words); }
void __fastcall SwapA32R( size_t dwords, unsigned long *data ) { SwapA32I(data, dwords); }

#endif
