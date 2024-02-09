/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CF_H
#define CCITTFAX_SRC_CF_H

#include <ccittfax/defs.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

inline uint16_t cf_bswap16(uint16_t x)
{
        return (x << 8) | (x >> 8);
}

inline uint32_t cf_bswap32(uint32_t x)
{
        return (x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) |
               (x >> 24);
}

inline uint64_t cf_bswap64(uint64_t x)
{
        return (x << 56) | ((x << 40) & 0x00FF000000000000) |
               ((x << 24) & 0x0000FF0000000000) |
               ((x << 8) & 0x000000FF00000000) |
               ((x >> 8) & 0x00000000FF000000) |
               ((x >> 24) & 0x0000000000FF0000) |
               ((x >> 40) & 0x000000000000FF00) | (x >> 56);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define CF_LITTLE_ENDIAN 1
#else
#  define CF_LITTLE_ENDIAN 0
#endif /* __BYTE_ORDER__  == __ORDER_LITTLE_ENDIAN__ */

#if CF_LITTLE_ENDIAN
#  define CF_TOBE16(x) cf_bswap16(x)
#  define CF_TOBE32(x) cf_bswap32(x)
#  define CF_TOBE64(x) cf_bswap64(x)
#else
#  define CF_TOBE16(x) (x)
#  define CF_TOBE32(x) (x)
#  define CF_TOBE64(x) (x)
#endif /* CF_LITTLE_ENDIAN */

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

inline int cf_get_color(const char *buf, size_t pos) {
        /* 0 is white, 1 is black; allow for inverse colors, pass params */
        return cf_getbit(buf, pos);
}

inline int cf_is_white(const char *buf, size_t pos) {
        return 0 == cf_get_color(buf, pos);
}

inline int cf_is_black(const char *buf, size_t pos) {
        return 1 == cf_get_color(buf, pos);
}


void cf_setbit(char *buf, size_t pos, int value);
void cf_setbits(char *buf, size_t beg, size_t end, int color);

struct cf_buffer_t *
cf_resize_buffer_least(struct cf_buffer_t *cf_buf, size_t add);

struct cf_buffer_t *
cf_resize_buffer(struct cf_buffer_t *cf_buf);

struct cf_buffer_t *
cf_make_buffer();

void cf_byte_align(struct cf_buffer_t *buf);
int cf_find_changing(const char *buf, int pos, int endpos);

#endif /* CCITTFAX_SRC_CF_H */
