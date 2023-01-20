/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfc_tables.h"

static int
put_explicit(struct cf_buffer_t *dst, unsigned value, unsigned len)
{
        const char *p;
        size_t i, n, written;

        if (0 == len)
                return 0;

        /* fprintf(stderr, "[0x%08x %4d]\n", value, len); */

        if (0 == cf_resize_buffer(dst))
                return 1;

        written = dst->pos >> 3;

        value <<= (sizeof(value) << 3) - len;
        value >>= dst->pos & 7;

        value = CF_TOBE32(value);
        p = (char *)&value;

        for (i = 0, n = ((dst->pos & 7) + len + 7) >> 3; i < n; ++i)
                dst->buf[written + i] |= p[i];
        dst->pos += len;

        return 0;
}

static inline int
put_code(struct cf_buffer_t *dst, const struct cfc_code_t *code)
{
        return put_explicit(dst, code->value, code->len);
}

static inline int
put_eol(struct cf_buffer_t *dst)
{
        return put_code(dst, &cfc_eol);
}

static int
put_rle(struct cf_buffer_t *dst, int rle, int color)
{
        int m, n;
        const struct cfc_code_t *term_table, *mkup_table;

        term_table = color ? cfc_white_term_rle : cfc_black_term_rle;
        mkup_table = color ? cfc_white_makeup_rle : cfc_black_makeup_rle;

        m = rle >> 6;
        n = rle & 0x3F;

        for (; m; m -= (m < 40 ? m : 40)) {
                if (put_code(dst, mkup_table + (m < 40 ? m : 40) - 1)) {
                        fprintf(stderr, "put_rle : rle %d, color %d\n", rle,
                                color);
                        return 1;
                }
        }

        if (put_code(dst, term_table + n)) {
                fprintf(stderr, "put_rle : rle %d, color %d\n", rle, color);
                return 1;
        }

        return 0;
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

struct cf_buffer_t *
cfc_g3_1d(const char *src, struct cf_params_t *params)
{
        int i, line, a0, stride, rle, color;
        struct cf_buffer_t *dst;

        assert(params->columns);

        dst = cf_make_buffer();
        if (0 == dst) {
                fprintf(stderr, "malloc compression buffer : %s\n",
                        strerror(errno));
                goto err;
        }

        put_eol(dst);

        stride = (params->columns + 7) >> 3;
        for (line = 0; line < params->rows; ++line, src += stride) {
                color = 1;

                for (a0 = 0; a0 < params->columns;) {
                        rle = get_rle(src, a0, params->columns, color);
                        if (put_rle(dst, rle, color))
                                goto err;

                        a0 += rle;
                        color = !color;
                }

                if (params->end_of_line)
                        put_eol(dst);

                if (params->encoded_byte_align)
                        put_explicit(dst, 0, (8 - (dst->pos & 7)) & 7);
        }

        if (params->end_of_block) {
                for (i = 0; i < 6; ++i)
                        put_eol(dst);
        }

        return dst;

err:
        if (dst) {
                free(dst->buf);
                free(dst);
        }

        return 0;
}
