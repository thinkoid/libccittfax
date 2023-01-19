/* -*- mode: c; -*- */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "cf.h"

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
