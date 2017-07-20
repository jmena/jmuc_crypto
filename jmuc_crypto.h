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
    uint8_t block[64];
    uint32_t digest[5];
    uint32_t chunk_idx;
    uint32_t size;
} jmuc_sha1_t;


uint8_t *jmuc_sha1_compute(const void *buffer, uint32_t len, uint8_t digest[20]);

void jmuc_sha1_initialize(jmuc_sha1_t *context);
void jmuc_sha1_feed_byte(jmuc_sha1_t *context, uint8_t octet);
void jmuc_sha1_feed_bytes(jmuc_sha1_t *context, const void *buffer, uint32_t len);
void jmuc_sha1_finish(jmuc_sha1_t *context);
void jmuc_sha1_get_digest_bytes(jmuc_sha1_t *context, uint8_t digest[20]);


typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t reserved;
} jmuc_bigint;

void jmuc_bigint_set_zero(jmuc_bigint *n);
void jmuc_bigint_set_zeros(jmuc_bigint *n);
jmuc_bigint jmuc_bigint_new();
void jmuc_bigint_free(jmuc_bigint *n);
void jmuc_bigint_reserve_size(jmuc_bigint *num, uint32_t reserve);
void jmuc_bigint_push_byte(jmuc_bigint *num, uint8_t v);
uint32_t jmuc_bigint_to_hex(jmuc_bigint *n, char *buffer, uint32_t buffer_size);
void jmuc_bigint_from_hex(jmuc_bigint *n, char *buffer, uint32_t buffer_size);
void jmuc_bigint_from_uint64(jmuc_bigint *n, uint64_t v);
uint64_t jmuc_bigint_to_uint64(jmuc_bigint *n);
void jmuc_bigint_add(jmuc_bigint *n1, jmuc_bigint *n2, jmuc_bigint *num);
void jmuc_bigint_mult(jmuc_bigint *n1, jmuc_bigint *n2, jmuc_bigint *num);
int jmuc_bigint_compare(jmuc_bigint *n1, jmuc_bigint *n2);
int jmuc_bigint_is_zero(jmuc_bigint *n);
void jmuc_bigint_copy(jmuc_bigint *dst, jmuc_bigint *src);
void jmuc_bigint_div(jmuc_bigint *n, jmuc_bigint *d, jmuc_bigint *q, jmuc_bigint *r);
int jmuc_bigint_is_odd(jmuc_bigint *n);
void jmuc_bigint_pow_mod(jmuc_bigint *base_, jmuc_bigint *exp_, jmuc_bigint *mod, jmuc_bigint *r);

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

#include <stdlib.h>
#include <string.h>

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


static char to_hex(uint8_t v) {
    if (v > 0xF) {
        return '*';
    }
    return "0123456789ABCDEF"[v];
}

static char from_hex(char ch) {
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    } else if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 10;
    } else if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return 0;
}

static void reduce_size(jmuc_bigint *n) {
    while (n->size != 0 && n->data[n->size - 1] == 0) {
        n->size--;
    }
}

void jmuc_bigint_set_zero(jmuc_bigint *n) {
    n->size = 0;
}

void jmuc_bigint_set_zeros(jmuc_bigint *n) {
    n->size = 0;
    memset(n->data, 0, n->reserved);
}

jmuc_bigint jmuc_bigint_new() {
    jmuc_bigint num;
    num.data = 0;
    num.size = 0;
    num.reserved = 0;
    return num;
}

void jmuc_bigint_free(jmuc_bigint *n) {
    free(n->data);
    n->size = 0;
    n->data = 0;
    n->reserved = 0;
}

void jmuc_bigint_reserve_size(jmuc_bigint *num, uint32_t reserve) {
    if (num->reserved >= reserve) {
        return;
    }

    num->data = realloc(num->data, reserve);
    if (num->reserved < reserve) {
        for (uint32_t i = num->reserved; i < reserve; i++) {
            num->data[i] = 0;
        }
    }
    num->reserved = reserve;
}

void jmuc_bigint_push_byte(jmuc_bigint *num, uint8_t v) {
    jmuc_bigint_reserve_size(num, num->size + 1);
    num->data[num->size] = v;
    num->size++;
}

uint32_t jmuc_bigint_to_hex(jmuc_bigint *n, char *buffer, uint32_t buffer_size) {
    reduce_size(n);
    if (buffer == 0 || buffer_size <= n->size) {
        return n->size + 1;
    }
    for (uint32_t i = 0; i < n->size; i++) {
        buffer[2 * i] = to_hex(n->data[n->size - i - 1] >> 4);
        buffer[2 * i + 1] = to_hex(n->data[n->size - i - 1] & 0x0F);
    }

    buffer[n->size * 2] = 0;
    return 0;
}

void jmuc_bigint_from_hex(jmuc_bigint *n, char *buffer, uint32_t buffer_size) {
    jmuc_bigint_set_zero(n);
    for (uint32_t i = 0; i < buffer_size; i += 2) {
        uint32_t hi = from_hex(buffer[buffer_size - i - 2]);
        uint32_t lo = from_hex(buffer[buffer_size - i - 1]);
        jmuc_bigint_push_byte(n, (hi << 4) | lo);
    }
}


void jmuc_bigint_from_uint64(jmuc_bigint *n, uint64_t v) {
    jmuc_bigint_reserve_size(n, 8);
    jmuc_bigint_set_zero(n);

    for (int i = 0; i < 8 && v != 0; i++) {
        jmuc_bigint_push_byte(n, v & 0xFF);
        v = v >> 8;
    }
}

uint64_t jmuc_bigint_to_uint64(jmuc_bigint *n) {
    uint64_t v = 0;
    for (int i = 0; i < n->size; i++) {
        v = v << 8;
        v = v | n->data[n->size - i - 1];
    }
    return v;
}

void jmuc_bigint_add(jmuc_bigint *n1, jmuc_bigint *n2, jmuc_bigint *num) {
    // TODO: something faster is possible
    uint32_t max_size = (n1->size > n2->size) ? n1->size : n2->size;
    uint32_t reserve = max_size + 2;
    jmuc_bigint_reserve_size(num, reserve);

    // copy n1
    for (uint32_t i = 0; i < n1->size; i++) {
        num->data[i] = n1->size;
        num->size++;
    }

    // copy n2
    for (uint32_t i = 0; i < n2->size; i++) {
        uint32_t sum = num->data[i] + n2->data[i];
        num->data[i] = sum & 0xFF;
        num->data[i + 1] = sum >> 1;
    }

    // determine size
    for (uint32_t i = 0; i < reserve; i++) {
        if (num->data[i] != 0) {
            num->size = i + 1;
        }
    }
}

void jmuc_bigint_mult(jmuc_bigint *n1, jmuc_bigint *n2, jmuc_bigint *num) {
    // TODO: something faster is possible
    uint32_t reserved = n1->size + n2->size + 2;
    jmuc_bigint_reserve_size(num, reserved);
    jmuc_bigint_set_zeros(num);

    for (uint32_t i = 0; i < n1->size; i++) {
        uint32_t d1 = n1->data[i];
        if (d1 == 0) {
            continue;
        }

        for (uint32_t j = 0; j < n2->size; j++) {
            uint32_t d2 = n2->data[j];
            uint32_t mult = d1 * d2;

            if (d2 == 0) {
                continue;
            }

            uint32_t sum1 = num->data[i + j] + ((mult) & 0xFF);
            uint32_t sum2 = num->data[i + j + 1] + ((mult >> 8) & 0xFF) + ((sum1 >> 8) & 0xFF);
            num->data[i + j] = sum1;
            num->data[i + j + 1] = sum2;

            uint32_t carry = sum2 >> 8;
            for (int k = i + j + 2; carry != 0; k++) {
                uint32_t new_sum = num->data[k] + carry;
                num->data[k] = new_sum & 0xFF;
                carry = new_sum >> 8;
            }
        }
    }
    // determine size
    for (uint32_t i = 0; i < reserved; i++) {
        if (num->data[i] != 0) {
            num->size = i + 1;
        }
    }
}

int jmuc_bigint_compare(jmuc_bigint *n1, jmuc_bigint *n2) {
    reduce_size(n1);
    reduce_size(n2);
    uint32_t n1_size = n1->size;
    uint32_t n2_size = n2->size;
    while (n1_size > 0 && n1->data[n1_size - 1] == 0) {
        n1_size--;
    }
    while (n2_size > 0 && n2->data[n2_size - 1] == 0) {
        n2_size--;
    }

    if (n1_size < n2_size) {
        return -1;
    } else if (n1_size > n2_size) {
        return 1;
    }
    // same size
    for (int i = n2_size - 1; i >= 0; i--) {
        int diff = (int) n1->data[i] - (int) n2->data[i];
        if (diff < 0) {
            return -1;
        } else if (diff > 0) {
            return 1;
        }
    }
    return 0;
}

static uint32_t jmuc_bigint_top_digit(jmuc_bigint *src, uint32_t offset) {
    return src->data[src->size - offset - 1];
}

jmuc_inline static void jmuc_bigint_decrease(uint8_t *data) {
    while (*data == 0) {
        *data = 0xFF;
        ++data;
    }
    --(*data);
}

jmuc_inline static void jmuc_bigint_sub_inplace(jmuc_bigint *n1, jmuc_bigint *n2) {
    for (int i = 0; i < n2->size; i++) {
        uint32_t base = n1->data[i] - n2->data[i];
        if (base > 0xFF * 0xFF) {
            base += 0x100;
            jmuc_bigint_decrease(n1->data + i + 1);
        }
        n1->data[i] = base & 0xFF;
    }
}


int jmuc_bigint_is_zero(jmuc_bigint *n) {
    for (int i = n->size; i--;) {
        if (n->data[i] != 0) {
            return 0;
        }
    }
    return 1;
}


void jmuc_bigint_copy(jmuc_bigint *dst, jmuc_bigint *src) {
    jmuc_bigint_reserve_size(dst, src->size);
    dst->size = src->size;
    memcpy(dst->data, src->data, src->size);
}

void jmuc_bigint_div(jmuc_bigint *n, jmuc_bigint *d, jmuc_bigint *q, jmuc_bigint *r) {

    reduce_size(n);
    reduce_size(d);
    jmuc_bigint_set_zero(q);
    jmuc_bigint_set_zero(r);

    jmuc_bigint_copy(r, n);

    uint8_t raw_num;
    jmuc_bigint a_num;
    a_num.size = 1;
    a_num.data = &raw_num;

    jmuc_bigint mult_res = jmuc_bigint_new();

    jmuc_bigint a_num_window;
    a_num_window.size = d->size;
    a_num_window.data = r->data + r->size - d->size;
    a_num_window.reserved = r->reserved;
    if (a_num_window.data < r->data) {
        return;
    }

    for (;;) {
        uint32_t candidate = 0;
        if (jmuc_bigint_compare(&a_num_window, d) >= 0) {
            if (d->size == a_num_window.size) {
                candidate = jmuc_bigint_top_digit(&a_num_window, 0) / jmuc_bigint_top_digit(d, 0);
            } else if (a_num_window.size == d->size + 1) {
                candidate = (
                    (jmuc_bigint_top_digit(&a_num_window, 0) << 8)
                    + jmuc_bigint_top_digit(&a_num_window, 1)
                ) / jmuc_bigint_top_digit(d, 0);
            } else {
                candidate = 0xFF;
            }
            if (candidate > 0xFF) {
                candidate = 0xFF;
            }

            for (;;) {
                raw_num = candidate;
                jmuc_bigint_mult(&a_num, d, &mult_res);
                if (jmuc_bigint_compare(&mult_res, &a_num_window) <= 0) {
                    jmuc_bigint_sub_inplace(&a_num_window, &mult_res);
                    break;
                }
                candidate--;
            }
        }

        jmuc_bigint_push_byte(q, candidate);

        if (a_num_window.data == r->data) {
            break;
        }
        // move the window
        a_num_window.data--;
        a_num_window.reserved++;
        a_num_window.size++;

        reduce_size(&a_num_window);
    }

    // we need to reverse the data in the result
    for (uint32_t i = q->size / 2; i--;) {
        uint8_t t = q->data[i];
        q->data[i] = q->data[q->size - i - 1];
        q->data[q->size - i - 1] = t;
    }

    jmuc_bigint_free(&mult_res);
}


int jmuc_bigint_is_odd(jmuc_bigint *n) {
    if (n->size == 0) {
        return 0;
    }
    return n->data[0] % 2 == 1;
}

void jmuc_bigint_pow_mod(jmuc_bigint *base_, jmuc_bigint *exp_, jmuc_bigint *mod, jmuc_bigint *r) {
    // calculate c = m^e (mod n)
    jmuc_bigint_set_zero(r);
    jmuc_bigint_push_byte(r, 1);

    jmuc_bigint base = jmuc_bigint_new();
    jmuc_bigint_copy(&base, base_);

    jmuc_bigint expo = jmuc_bigint_new();
    jmuc_bigint_copy(&expo, exp_);

    jmuc_bigint tmp = jmuc_bigint_new();
    jmuc_bigint tmp2 = jmuc_bigint_new();

    jmuc_bigint two;
    uint8_t raw_data = 2;
    two.data = &raw_data;
    two.size = 1;

    // base := base % modulus
    jmuc_bigint_div(&base, mod, &tmp, &tmp2);
    jmuc_bigint_copy(&base, &tmp2);

    while (!jmuc_bigint_is_zero(&expo)) {
        if (jmuc_bigint_is_odd(&expo)) {
            // result := (result * base) % modulus
            jmuc_bigint_mult(r, &base, &tmp); // tmp = result * base
            jmuc_bigint_div(&tmp, mod, &tmp2, r); // result = (tmp % mod)
        }

        // exponent := exponent >> 1
        jmuc_bigint_div(&expo, &two, &tmp, &tmp2);
        jmuc_bigint_copy(&expo, &tmp);

        // base := (base * base) mod modulus
        jmuc_bigint_mult(&base, &base, &tmp); // tmp = base * base
        jmuc_bigint_div(&tmp, mod, &tmp2, &base); // base = (tmp % mod)
    }
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
