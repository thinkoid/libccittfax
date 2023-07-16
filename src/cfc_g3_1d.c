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

struct cf_buffer_t *
cfc_g3_1d(const char *src, struct cf_params_t *params)
{
        int line, a0, stride, rle, color;
        struct cf_buffer_t *dst;

        assert(params->columns);

        dst = cf_make_buffer();
        if (0 == dst) {
                fprintf(stderr, "malloc compression buffer : %s\n",
                        strerror(errno));
                goto err;
        }

        cfc_put_eol(dst);

        stride = (params->columns + 7) >> 3;
        for (line = 0; line < params->rows; ++line, src += stride) {
                color = 1;

                for (a0 = 0; a0 < params->columns;) {
                        rle = get_rle(src, a0, params->columns, color);
                        if (cfc_put_rle(dst, rle, color))
                                goto err;

                        a0 += rle;
                        color = !color;
                }

                if (params->end_of_line)
                        cfc_put_eol(dst);

                if (params->encoded_byte_align)
                        cfc_put_rle_explicit(dst, 0, (8 - (dst->pos & 7)) & 7);
        }

        if (params->end_of_block)
                cfc_put_eol_n(dst, 6);

        return dst;

err:
        if (dst) {
                free(dst->buf);
                free(dst);
        }

        return 0;
}
