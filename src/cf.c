/* -*- mode: c; -*- */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "cf.h"

static size_t
new_capacity(size_t cap, size_t size, size_t add)
{
        static const size_t x = (size_t)-1;
        static const size_t h = (x >> 1) + 1;

        if (0 == cap)
                return add;

        assert(size <= cap);

        if (add > x - size)
                return 0;

        for (; cap < h && cap < size + add; cap <<= 1) ;
        for (; cap < x - 1 && cap < size + add; cap += (x - cap) >> 1) ;

        if (cap < size + add)
                cap = x;

        return cap;
}

struct cf_buffer_t *
resize_cf_buffer(struct cf_buffer_t *dst)
{
        char *buf;
        size_t cap, written;

        written = (dst->pos + 7) >> 3;

        if (dst->cap - written < sizeof(unsigned)) {
                cap = new_capacity(dst->cap, written, sizeof(unsigned));
                assert(cap && cap > dst->cap);

                buf = malloc(cap);
                if (0 == buf) {
                        /* fprintf(stderr, "reserve buffer : %s\n", strerror(errno)); */
                        return 0;
                }

                memcpy(buf, dst->buf, written);
                memset(buf + written, 0, cap - written);

                free(dst->buf);

                dst->buf = buf;
                dst->cap = cap;
        }

        return dst;
}

struct cf_buffer_t *
make_cf_buffer()
{
        struct cf_buffer_t *dst;

        dst = calloc(1, sizeof *dst);
        if (0 == dst)
                return 0;

        if (0 == resize_cf_buffer(dst)) {
                free(dst->buf);
                free(dst);
                dst = 0;
        }

        return dst;
}
