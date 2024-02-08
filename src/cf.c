/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "cf.h"
#include "cfc_tables.h"

extern inline uint16_t cf_bswap16(uint16_t x);
extern inline uint32_t cf_bswap32(uint32_t x);
extern inline uint64_t cf_bswap64(uint64_t x);

static size_t
cf_new_capacity(size_t cap, size_t size, size_t add)
{
        static const size_t x = (size_t)-1;
        static const size_t h = (x >> 1) + 1;

        if (0 == cap)
                return add;

        assert(size <= cap);

        if (add > x - size)
                return 0;

        for (; cap < h && cap < size + add; cap <<= 1)
                ;
        for (; cap < x - 1 && cap < size + add; cap += (x - cap) >> 1)
                ;

        if (cap < size + add)
                cap = x;

        return cap;
}

static struct cf_buffer_t *
cf_resize_buffer_explicit(struct cf_buffer_t *cf_buf, size_t cap)
{
        char *buf;
        size_t written;

        buf = malloc(cap);
        if (0 == buf) {
                fprintf(stderr, "reserve buffer : %s\n", strerror(errno));
                return 0;
        }

        written = (cf_buf->pos + 7) >> 3;

        memcpy(buf, cf_buf->buf, written);
        memset(buf + written, 0, cap - written);

        free(cf_buf->buf);

        cf_buf->buf = buf;
        cf_buf->cap = cap;

        return cf_buf;
}

struct cf_buffer_t *
cf_resize_buffer_least(struct cf_buffer_t *cf_buf, size_t add)
{
        size_t cap, written;

        written = (cf_buf->pos + 7) >> 3;
        if (add > cf_buf->cap - written) {
                cap = cf_new_capacity(cf_buf->cap, written, add);
                if (0 == cap)
                        return 0;

                return cf_resize_buffer_explicit(cf_buf, cap);
        }

        return cf_buf;
}

struct cf_buffer_t *
cf_resize_buffer(struct cf_buffer_t *cf_buf)
{
        return cf_resize_buffer_least(cf_buf, sizeof(unsigned));
}

struct cf_buffer_t *
cf_make_buffer()
{
        struct cf_buffer_t *cf_buf;

        cf_buf = calloc(1, sizeof *cf_buf);
        if (0 == cf_buf)
                return 0;

        if (0 == cf_resize_buffer(cf_buf)) {
                free(cf_buf->buf);
                free(cf_buf);
                cf_buf = 0;
        }

        return cf_buf;
}

static void cf_setbits_white(char *buf, size_t beg, size_t end)
{
        if (end - beg < 8 - (beg & 7)) {
                buf[beg >> 3] |= (0xff >> (beg & 7)) &
                                 (0xff << (8 - (end & 7)));
        } else {
                buf[beg >> 3] |= (0xff >> (beg & 7));
                beg = (beg + 7) & ~7;

                if ((end - beg) >> 3)
                        memset(buf + (beg >> 3), 0xff, (end - beg) >> 3);

                if (end & 7)
                        buf[end >> 3] |= (0xff << (8 - (end & 7)));
        }
}

static void cf_setbits_black(char *buf, size_t beg, size_t end)
{
        if (end - beg < 8 - (beg & 7)) {
                buf[beg >> 3] &= (0xff << (8 - (beg & 7))) |
                                 (0xff >> (end & 7));
        } else {
                buf[beg >> 3] &= 0xff << (8 - (beg & 7));
                beg = (beg + 7) & ~7;

                if ((end - beg) >> 3)
                        memset(buf + (beg >> 3), 0, (end - beg) >> 3);

                if (end & 7)
                        buf[end >> 3] &= 0xff >> (end & 7);
        }
}

int cf_getbit(const char *buf, size_t pos)
{
        return !!(*((uint8_t *)buf + (pos >> 3)) & (0x80 >> (pos & 7)));
}

void cf_setbit(char *buf, size_t pos, int value)
{
        *((uint8_t *)buf + (pos / 8)) |= (uint8_t)(!!value) << (7 - (pos & 7));
}

void cf_setbits(char *buf, size_t beg, size_t end, int color)
{
        color ? cf_setbits_white(buf, beg, end) :
                cf_setbits_black(buf, beg, end);
}

