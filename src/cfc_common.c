/* -*- mode: c; -*- */

#include <stdio.h>

#include <ccittfax/ccittfax.h>

#include "cf.h"
#include "cfc_tables.h"

int
cfc_put_rle_explicit(struct cf_buffer_t *buf, unsigned value, unsigned len)
{
        const char *p;
        size_t i, n, written;

        if (0 == len)
                return 0;

        /* fprintf(stderr, "[0x%08x %4d]\n", value, len); */

        if (0 == cf_resize_buffer(buf))
                return 1;

        written = buf->pos >> 3;

        value <<= (sizeof(value) << 3) - len;
        value >>= buf->pos & 7;

        value = CF_TOBE32(value);
        p = (char *)&value;

        for (i = 0, n = ((buf->pos & 7) + len + 7) >> 3; i < n; ++i)
                buf->buf[written + i] |= p[i];
        buf->pos += len;

        return 0;
}

static inline int
cfc_put_code(struct cf_buffer_t *buf, const struct cfc_code_t *code)
{
        return cfc_put_rle_explicit(buf, code->value, code->len);
}

int
cfc_put_eol(struct cf_buffer_t *buf)
{
        return cfc_put_code(buf, &cfc_eol);
}

static inline int
cfc_put_eol_x(struct cf_buffer_t *buf, int x)
{
        return cfc_put_eol(buf) ? cfc_put_rle_explicit(buf, x, 1) : 1;
}

int
cfc_put_eol_0(struct cf_buffer_t *buf)
{
        return cfc_put_eol_x(buf, 0);
}

int
cfc_put_eol_1(struct cf_buffer_t *buf)
{
        return cfc_put_eol_x(buf, 1);
}

int
cfc_put_eol_n(struct cf_buffer_t *buf, size_t n)
{
        for (size_t i = 0; i < n; ++i) {
                if (cfc_put_eol(buf))
                        return 1;
        }

        return 0;
}

int
cfc_put_rle(struct cf_buffer_t *buf, int rle, int color)
{
        int m, n;
        const struct cfc_code_t *term_table, *mkup_table;

        term_table = color ? cfc_white_term_rle : cfc_black_term_rle;
        mkup_table = color ? cfc_white_makeup_rle : cfc_black_makeup_rle;

        m = rle >> 6;
        n = rle & 0x3F;

        for (; m; m -= (m < 40 ? m : 40)) {
                if (cfc_put_code(buf, mkup_table + (m < 40 ? m : 40) - 1)) {
                        fprintf(stderr, "put_rle : rle %d, color %d\n", rle,
                                color);
                        return 1;
                }
        }

        if (cfc_put_code(buf, term_table + n)) {
                fprintf(stderr, "put_rle : rle %d, color %d\n", rle, color);
                return 1;
        }

        return 0;
}
