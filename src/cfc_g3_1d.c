/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfc_common.h"
#include "cfc_tables.h"

static inline void
cf_byte_align(struct cf_buffer_t *buf)
{
        buf->pos = (buf->pos + 7) & ~7;
}

static inline int
is_same_color(const char *arr, size_t pos, int color)
{
        return color == !!((unsigned char)arr[pos >> 3] & (0x80 >> (pos & 7)));
}

static int
get_rle(const char *arr, size_t pos, size_t end, int color)
{
        size_t cur = pos;
        for (; cur < end && is_same_color(arr, cur, color); ++cur) ;
        return cur - pos;
}

static int
cfc_g3_1d_line(struct cf_buffer_t *dst, struct cf_buffer_t *src,
               struct cf_params_t *params)
{
        int color;

        size_t endpos = src->pos + params->columns;
        assert(endpos <= (src->cap << 3));

        for (color = 1; src->pos < endpos; color = !color) {
                int rle = get_rle(src->buf, src->pos, endpos, color);

                if (cfc_put_rle(dst, rle, color))
                        return 1;

                src->pos += rle;
        }

        return 0;
}

static struct cf_buffer_t *
cfc_do_g3_1d(struct cf_buffer_t *dst, struct cf_buffer_t *src,
             struct cf_params_t *params)
{
        int i;

        cfc_put_eol(dst);

        for (i = 0; i < params->rows; ++i) {
                if (cfc_g3_1d_line(dst, src, params))
                        goto err;

                /* source row always starts at a byte boundary */
                cf_byte_align(src);

                if (params->end_of_line && cfc_put_eol(dst))
                        goto err;

                if (params->encoded_byte_align &&
                    cfc_put_rle_explicit(dst, 0, (8 - (dst->pos & 7)) & 7))
                        goto err;
        }

        if (params->end_of_block && cfc_put_eol_n(dst, 6))
                goto err;

        return dst;

err:
        free(dst->buf);
        free(dst);

        return 0;
}

struct cf_buffer_t *
cfc_g3_1d(const char *buf, struct cf_params_t *params)
{
        struct cf_buffer_t src, *dst;

        src.buf = (char *)buf;
        src.cap = params->rows * ((params->columns + 7) & ~7);
        src.pos = 0;

        dst = cf_make_buffer();
        if (0 == dst) {
                fprintf(stderr, "malloc compression buffer : %s\n",
                        strerror(errno));
                return 0;
        }

        return cfc_do_g3_1d(dst, &src, params);
}
