/**
* SHA-256
* Copyright (C) 2014 Christopher Gurnee.  All rights reserved.
*
* Please refer to license.txt for details about distribution and modification.
*
* Algorithm and naming conventions are straight from FIPS PUB 180-4.
**/


#include "sha256.h"

#include <stdlib.h>
#include <string.h>
#pragma intrinsic(_rotr, memcpy, memset)


// section 3.2
#define SHR(n, x)     (  (x) >> (n)  )
//
#if _MSC_VER >= 1300
#  define ROTR(n, x)  _rotr(x, n)
#else
#  define ROTR(n, x)  (  (x) >> (n)  |  (x) << (32-(n))  )
#endif

// secion 4.1.2
#define  Ch(x, y, z)  (  (x) & (y)  |  ~(x) & (z)  )
#define Maj(x, y, z)  (  (x) & (y)  |   (x) & (z)  |  (y) & (z)  )
//
#define SIGMA_0(x)    ( ROTR(2,  x) ^ ROTR(13, x) ^ ROTR(22, x) )
#define SIGMA_1(x)    ( ROTR(6,  x) ^ ROTR(11, x) ^ ROTR(25, x) )
#define sigma_0(x)    ( ROTR(7,  x) ^ ROTR(18, x) ^ SHR (3,  x) )
#define sigma_1(x)    ( ROTR(17, x) ^ ROTR(19, x) ^ SHR (10, x) )

// section 4.2.2
const uint32_t K[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


// swap endianness of a uint32
#define SWAP_32(x)  (  (x) << 24  |  ((x) & 0xff00) << 8  |  ((x) & 0xff0000) >> 8  |  (x) >> 24  )

// array length (WARNING: arg must be of array type, not pointer)
#define ARRAYLEN(a)  ( sizeof(a) / sizeof(a[0]) )


// initialize the Sha256Context struct
void sha256_init(Sha256Context* context)
{
    // section 5.3.3
    context->H[0] = 0x6a09e667;
    context->H[1] = 0xbb67ae85;
    context->H[2] = 0x3c6ef372;
    context->H[3] = 0xa54ff53a;
    context->H[4] = 0x510e527f;
    context->H[5] = 0x9b05688c;
    context->H[6] = 0x1f83d9ab;
    context->H[7] = 0x5be0cd19;

    context->cur_buffer_size = 0;
    context->total_size      = 0;
}


// update the hash H given a full block M already in uint32 form
void sha256_block(uint32_t H[8], const uint32_t M[16])
{
    // section 6.2.2 step 1
    uint32_t W[64];
    for (int t = 0; t < 16; t++) {
        W[t] = M[t];
    }
    for (int t = 16; t < 64; t++) {
        W[t] = sigma_1(W[t-2]) + W[t-7] + sigma_0(W[t-15]) + W[t-16];
    }

    // section 6.2.2 step 2
    uint32_t a, b, c, d, e, f, g, h;
    a = H[0];
    b = H[1];
    c = H[2];
    d = H[3];
    e = H[4];
    f = H[5];
    g = H[6];
    h = H[7];

    // section 6.2.2 step 3
    uint32_t T1, T2;
    for (int t = 0; t < 64; t++) {
        T1 = h + SIGMA_1(e) + Ch(e, f, g) + K[t] + W[t];
        T2 = SIGMA_0(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // section 6.2.2 step 4
    H[0] = a + H[0];
    H[1] = b + H[1];
    H[2] = c + H[2];
    H[3] = d + H[3];
    H[4] = e + H[4];
    H[5] = f + H[5];
    H[6] = g + H[6];
    H[7] = h + H[7];
}


#if _MSC_VER
#  undef min
#  define inline __inline
#endif
inline size_t min(size_t a, size_t b) { return a < b ? a : b; }

// add data to the running hash
void sha256_update(Sha256Context* context, const uint8_t* input, size_t input_size)
{
    while (input_size) {

        // copy bytes from the input to the working buffer
        int num_bytes_to_copy = (int)min(input_size, sizeof(context->buffer) - context->cur_buffer_size);
        memcpy(&context->buffer[context->cur_buffer_size], input, num_bytes_to_copy);
 
        // update our data structures
        context->cur_buffer_size += num_bytes_to_copy;
        input                    += num_bytes_to_copy;
        input_size               -= num_bytes_to_copy;

        // if the buffer is full
        if (context->cur_buffer_size == sizeof(context->buffer)) {

            // convert the sequence of bytes into uint32's (assuming we're a little-endian machine)
            for (int i = 0; i < ARRAYLEN(context->buffer_as_uint32); i++) {
                context->buffer_as_uint32[i] = SWAP_32(context->buffer_as_uint32[i]);
            }

            // process the full block
            sha256_block(context->H, context->buffer_as_uint32);
            context->cur_buffer_size = 0;
            context->total_size += sizeof(context->buffer);
        }
    }
}


// append the padding and process the last block(s)
void sha256_finalize(Sha256Context* context)
{
    // count the remaining valid bytes in the buffer
    context->total_size += context->cur_buffer_size;

    // section 5.1.1

    // append the "1" bit (safe because the buffer is never left completely full)
    context->buffer[context->cur_buffer_size] = 0x80;
    context->cur_buffer_size++;

    int empty_bytes = sizeof(context->buffer) - context->cur_buffer_size;

    // if there's not enough space left in this block for the padding, we'll need two blocks
    if (empty_bytes < 8) {

        // append the "0" padding bits for the rest of this second-to-last block
        memset(&context->buffer[context->cur_buffer_size], 0, empty_bytes);

        // convert the sequence of bytes into uint32's (assuming we're a little-endian machine)
        for (int i = 0; i < ARRAYLEN(context->buffer_as_uint32); i++) {
            context->buffer_as_uint32[i] = SWAP_32(context->buffer_as_uint32[i]);
        }

        // process the full block
        sha256_block(context->H, context->buffer_as_uint32);

        // update our data structures
        context->cur_buffer_size = 0;
        empty_bytes = sizeof(context->buffer);
    }
    
    // append the "0" padding bits (leaving 64 bits at the end untouched)
    memset(&context->buffer[context->cur_buffer_size], 0, empty_bytes - 8);

    // convert the sequence of bytes into uint32's (assuming we're a little-endian machine)
    // note that the last 64 bits haven't been set yet and so aren't converted
    for (int i = 0; i < ARRAYLEN(context->padding_as_uint32); i++) {
        context->padding_as_uint32[i] = SWAP_32(context->padding_as_uint32[i]);
    }

    // append the bit count
    uint64_t total_bit_count = context->total_size * 8;
    context->bit_count_high  = total_bit_count >> 32;
    context->bit_count_low   = total_bit_count & 0xffffffff;

    // process the full block
    sha256_block(context->H, context->buffer_as_uint32);

    // convert the "H" uint32's into the hash_result (assuming we're a little-endian machine)
    for (int i = 0; i < ARRAYLEN(context->H); i++) {
        context->H[i] = SWAP_32(context->H[i]);
    }
}
