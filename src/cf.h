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

int cf_getbit(const char *buf, size_t pos);

inline int cf_get_color(const char *buf, size_t pos) {
        /* 1 is white, 0 is black (black_is_1 inverts the raster) */
        return cf_getbit(buf, pos);
}

void cf_setbit(char *buf, size_t pos, int value);
void cf_setbits(char *buf, size_t beg, size_t end, int color);

struct cf_buffer_t *
cf_resize_buffer_explicit(struct cf_buffer_t *cf_buf, size_t cap);

struct cf_buffer_t *
cf_resize_buffer_least(struct cf_buffer_t *cf_buf, size_t add);

struct cf_buffer_t *
cf_resize_buffer(struct cf_buffer_t *cf_buf);

struct cf_buffer_t *
cf_make_buffer();

void cf_byte_align(struct cf_buffer_t *buf);
int cf_find_changing(const char *buf, int pos, int endpos);

#endif /* CCITTFAX_SRC_CF_H */
