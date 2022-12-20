/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <assert.h>
#include <stdlib.h>

#include "cf.h"
#include "cfc_tables.h"

#include "cf_utils.h"

#define CF_LITTLE_ENDIAN (*(unsigned char *)(&(int){ 1 }))

#define CF_DO_TO_BE(x)                          \
        ((x << 24) |                            \
         ((x << 8) & 0x00FF0000) |              \
         ((x >> 8) & 0x0000FF00) |              \
         ((x >> 24) & 0x000000FF))

#define CF_TO_BE(x)  CF_LITTLE_ENDIAN ? CF_DO_TO_BE(x) : (x)

static int
put_rle_explicit(struct cf_buffer_t *dst, unsigned value, unsigned len)
{
        const char *p;
        size_t i, n, written;

        /* fprintf(stderr, "[%d %d]\n", value, len); */

        if (0 == reserve_buffer(dst))
                return 1;

        written = dst->pos >> 3;

        value <<= (sizeof(value) << 3) - len;
        value >>= dst->pos & 7;

        value = CF_TO_BE(value);
        p = (char *)&value;

        for (i = 0, n = ((dst->pos & 7) + len + 7) >> 3; i < n; ++i)
                dst->buf[written + i] |= p[i];
        dst->pos += len;

        return 0;
}

static inline int
put_code(struct cf_buffer_t *dst, const struct cfc_code_t *code)
{
        return put_rle_explicit(dst, code->value, code->len);
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

        term_table = color ? cfc_white_term_rle   : cfc_black_term_rle;
        mkup_table = color ? cfc_white_makeup_rle : cfc_black_makeup_rle;

        m = rle >> 6;
        n = rle & 0x3F;

        for (; m; m -= (m < 40 ? m : 40)) {
                if (put_code(dst, mkup_table + (m < 40 ? m : 40) - 1)) {
                        /* fprintf(stderr, "put_rle : rle %d, color %d\n", */
                        /*         rle, color); */
                        return 1;
                }
        }

        if (put_code(dst, term_table + n)) {
                /* fprintf(stderr, "put_rle : rle %d, color %d\n", */
                /*         rle, color); */
                return 1;
        }

        return 0;
}

static inline int
is_same_color(const char *arr, size_t pos, int color)
{
        return color == !!((unsigned char)arr[pos >> 3] & (0x80 >> (pos % 8)));
}

static int
get_rle(const char *arr, size_t pos, size_t end, int color)
{
        size_t i;
        for (i = pos; i < end && is_same_color(arr, i, color); ++i) ;
        return i - pos;
}

static struct cf_buffer_t *
make_cfc_buffer()
{
        struct cf_buffer_t *dst;

        dst = calloc(1, sizeof *dst);
        if (0 == dst)
                return 0;

        if (0 == reserve_buffer(dst)) {
                free(dst->buf);
                free(dst);
                dst = 0;
        }

        return dst;
}

struct cf_buffer_t *
cfc_g3_1d(const char *src, struct cf_params_t *params)
{
        int i, pos, stride, rle, color;

        struct cf_buffer_t *dst;

        assert(params->rows);
        assert(params->columns);

        stride = (params->columns + 7) >> 3;

        dst = make_cfc_buffer();
        if (0 == dst) {
                /* fprintf(stderr, "malloc compression buffer : %s\n", */
                /*         strerror(errno)); */
                goto err;
        }

        put_eol(dst);

        for (i = 0; i < params->rows; ++i, src += stride) {
                color = 1;

                for (pos = 0; pos < params->columns; ) {
                        rle = get_rle(src, pos, params->columns, color);
                        put_rle(dst, rle, color);

                        pos += rle;
                        color = !color;
                }

                if (params->end_of_line)
                        put_eol(dst);

                if ((pos % 8) && params->encoded_byte_align)
                        put_rle_explicit(dst, 0, 8 - pos % 8);
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
