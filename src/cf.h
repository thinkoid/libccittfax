/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CF_H
#define CCITTFAX_SRC_CF_H

#include <ccittfax/defs.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define CF_DO_CAT(a, b) a##b
#define CF_CAT(a, b) CF_DO_CAT(a, b)

#define ROTL_DEF(type, width)                                       \
        static inline type CF_CAT(cf_rotl, width)(type x, size_t n) \
        {                                                           \
                assert(n <= (sizeof(type) << 3));                   \
                return (x << n) | (x >> ((sizeof x << 3) - n));     \
        }

#define ROTR_DEF(type, width)                                       \
        static inline type CF_CAT(cf_rotr, width)(type x, size_t n) \
        {                                                           \
                assert(n <= (sizeof(type) << 3));                   \
                return (x >> n) | (x << ((sizeof x << 3) - n));     \
        }

ROTR_DEF(uint8_t, 8)
ROTR_DEF(uint16_t, 16)
ROTR_DEF(uint32_t, 32)
ROTR_DEF(uint64_t, 64)

ROTL_DEF(uint8_t, 8)
ROTL_DEF(uint16_t, 16)
ROTL_DEF(uint32_t, 32)
ROTL_DEF(uint64_t, 64)

int cf_getbit(const char *buf, size_t pos);

void cf_setbit(char *buf, size_t pos, int value);
void cf_setbits(char *buf, size_t beg, size_t end, int color);

struct cf_buffer_t {
        char *buf;
        size_t cap, pos;
};

struct cf_buffer_t *
cf_resize_buffer_least(struct cf_buffer_t *cf_buf, size_t add);

struct cf_buffer_t *
cf_resize_buffer(struct cf_buffer_t *cf_buf);

struct cf_buffer_t *
cf_make_buffer();

#endif /* CCITTFAX_SRC_CF_H */
