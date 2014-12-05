/**
* SHA-256
* Copyright (C) 2014 Christopher Gurnee.  All rights reserved.
*
* Please refer to license.txt for details about distribution and modification.
**/


#ifndef __SHA256_H__
#define __SHA256_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef struct {
    union {
        uint32_t H[8];                  // the running hash
        uint8_t  result[32];            // the hash result (only valid after sha256_finalize())
    };
    union {
        uint8_t  buffer[64];            // (potentially incomplete) block to be hashed
        uint32_t buffer_as_uint32[16];  // interpreted as uint32's
        struct {
            uint32_t padding_as_uint32[16 - 2];  // the buffer excluding the last 64 bits
            uint32_t bit_count_high;    // high 32 bits of the 64-bit total bit count
            uint32_t bit_count_low;     // low 32 bits of the 64-bit total bit count
        };
    };
    int cur_buffer_size;                // count of valid bytes currently in the buffer
    uint64_t total_size;                // total count of bytes hashed so far (excluding padding)
} Sha256Context;


// initialize the Sha256Context struct
void sha256_init(Sha256Context* context);

// add data to the running hash
void sha256_update(Sha256Context* context, const uint8_t* input, size_t input_size);

// append the padding and process the last block(s)
void sha256_finalize(Sha256Context* context);


#ifdef __cplusplus
}
#endif

#endif
