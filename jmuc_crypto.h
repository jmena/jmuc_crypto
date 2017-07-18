/*
 *
 * JMUC - Javier Mena Unsecure Crypto library. This library is NOT intended to
 * be secure. It's intended to be easy to use. In particular this library is
 * NOT protected against any kind of attacks, like timing attacks or bad input.
 *
 */
#ifndef JMUC_CRYPTO_INCLUDE_H
#define JMUC_CRYPTO_INCLUDE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  block[64];
    uint32_t digest[5];
    uint32_t chunk_idx;
    uint32_t size;
} jmuc_sha1_t;


uint8_t* jmuc_sha1_compute(const void *buffer, uint32_t len, uint8_t digest[20]);

void jmuc_sha1_initialize(jmuc_sha1_t *context);
void jmuc_sha1_feed_byte(jmuc_sha1_t *context, uint8_t octet);
void jmuc_sha1_feed_bytes(jmuc_sha1_t *context, const void *buffer, uint32_t len);
void jmuc_sha1_finish(jmuc_sha1_t *context);
void jmuc_sha1_get_digest_bytes(jmuc_sha1_t *context, uint8_t digest[20]);

#ifdef __cplusplus
}
#endif


#endif // JMUC_CRYPTO_INCLUDE_H

// END OF INCLUDE

#ifdef JMUC_CRYPTO_IMPLEMENTATION


#ifndef _MSC_VER
   #ifdef __cplusplus
   #define jmuc_inline inline
   #else
   #define jmuc_inline
   #endif
#else
   #define jmuc_inline __forceinline
#endif


jmuc_inline static uint32_t jmuc_sha1_left_rotate(uint32_t value, uint32_t count) {
	return (value << count) ^ (value >> (32-count));
}

jmuc_inline static void uint32t_to_bytes(uint32_t in, uint8_t *out) {
    out[0] = (in >> 24);
    out[1] = (in >> 16) & 0xFF;
    out[2] = (in >>  8) & 0xFF;
    out[3] = (in)       & 0xFF;
}


jmuc_inline static void jmuc_sha1_process_chunk(uint8_t block[64], uint32_t digest[5]) {
	uint32_t w[80];
	for (uint32_t i = 0; i < 16; i++) {
		w[i]  = (block[i*4 + 0] << 24);
		w[i] |= (block[i*4 + 1] << 16);
		w[i] |= (block[i*4 + 2] << 8);
		w[i] |= (block[i*4 + 3]);
	}
	for (uint32_t i = 16; i < 80; i++) {
		w[i] = jmuc_sha1_left_rotate((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);
	}

	uint32_t a = digest[0];
	uint32_t b = digest[1];
	uint32_t c = digest[2];
	uint32_t d = digest[3];
	uint32_t e = digest[4];

	for (uint32_t i=0; i < 80; ++i) {
		uint32_t f = 0;
		uint32_t k = 0;

		if (i < 20) {
			f = (b & c) | (~b & d);
			k = 0x5A827999;
		} else if (i < 40) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		} else if (i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDC;
		} else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}
		uint32_t temp = jmuc_sha1_left_rotate(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = jmuc_sha1_left_rotate(b, 30);
		b = a;
		a = temp;
	}

	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
}


void jmuc_sha1_initialize(jmuc_sha1_t *context) {
	context->digest[0] = 0x67452301;
	context->digest[1] = 0xEFCDAB89;
	context->digest[2] = 0x98BADCFE;
	context->digest[3] = 0x10325476;
	context->digest[4] = 0xC3D2E1F0;
	context->chunk_idx = 0;
	context->size = 0;
}

void jmuc_sha1_feed_byte(jmuc_sha1_t *context, uint8_t octet) {
	context->block[context->chunk_idx++] = octet;
	++context->size;
	if (context->chunk_idx == 64) {
		context->chunk_idx = 0;
		jmuc_sha1_process_chunk(context->block, context->digest);
	}
}

void jmuc_sha1_feed_bytes(jmuc_sha1_t *context, const void *buffer, uint32_t len) {
	const uint8_t *it = buffer;
	while (len--) {
		jmuc_sha1_feed_byte(context, *it);
		it++;
	}
}

void jmuc_sha1_finish(jmuc_sha1_t *context) {
    uint32_t size = context->size;
	jmuc_sha1_feed_byte(context, 0x80);

    // complete the chunk to 448 bits. We need space for 64 bits.
	if (context->chunk_idx > 56) {
		while (context->chunk_idx != 0) {
			jmuc_sha1_feed_byte(context, 0);
		}
    }
	while (context->chunk_idx < 56) {
		jmuc_sha1_feed_byte(context, 0);
	}

    // only support 32 bits
	jmuc_sha1_feed_byte(context, 0);
	jmuc_sha1_feed_byte(context, 0);
	jmuc_sha1_feed_byte(context, 0);
	jmuc_sha1_feed_byte(context, 0);
	jmuc_sha1_feed_byte(context, (size >> 21) & 0xFF);
	jmuc_sha1_feed_byte(context, (size >> 13) & 0xFF);
	jmuc_sha1_feed_byte(context, (size >>  5)  & 0xFF);
	jmuc_sha1_feed_byte(context, (size <<  3)  & 0xFF);
}

void jmuc_sha1_get_digest_bytes(jmuc_sha1_t *context, uint8_t digest[20]) {
    uint32t_to_bytes(context->digest[0], digest +  0);
    uint32t_to_bytes(context->digest[1], digest +  4);
    uint32t_to_bytes(context->digest[2], digest +  8);
    uint32t_to_bytes(context->digest[3], digest + 12);
    uint32t_to_bytes(context->digest[4], digest + 16);
}


uint8_t* jmuc_sha1_compute(const void *buffer, uint32_t len, uint8_t digest[20]) {
    jmuc_sha1_t context;
    jmuc_sha1_initialize(&context);
    jmuc_sha1_feed_bytes(&context, buffer, len);
    jmuc_sha1_finish(&context);
    jmuc_sha1_get_digest_bytes(&context, digest);
    return digest;
}


#endif // JMUC_CRYPTO_IMPLEMENTATION

/*

revision history:
  0.01 initial release with support for sha1

*/

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Javier Mena
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
