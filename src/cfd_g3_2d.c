/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfd_2d.h"
#include "cfd_trie.h"

static inline int
get_bit(struct cf_buffer_t *src)
{
        if (src->pos >= (src->cap << 3))
                return -1;
        return cf_getbit(src->buf, src->pos++);
}

static int
try_consume_eol(struct cf_buffer_t *src)
{
        size_t saved = src->pos;
        int i, b;

        for (i = 0; i < 11; ++i) {
                b = get_bit(src);
                if (b != 0) {
                        src->pos = saved;
                        return (b < 0) ? -1 : 0;
                }
        }

        b = get_bit(src);
        if (b != 1) {
                src->pos = saved;
                return (b < 0) ? -1 : 0;
        }

        return 1;
}

static int
do_get_rle(struct cf_buffer_t *src, int color)
{
        struct cfd_trie_state_t state;
        size_t endpos = src->cap << 3;
        int c;

        cfd_trie_init(&state, color);

        for (; src->pos < endpos; ) {
                c = cf_getbit(src->buf, src->pos);
                if (!cfd_trie_walk(&state, c))
                        break;
                ++src->pos;
                if (cfd_trie_is_terminal(&state))
                        return cfd_trie_get_value(&state);
        }

        return -1;
}

static int
get_rle(struct cf_buffer_t *src, int color)
{
        int rle, total = 0;

        do {
                rle = do_get_rle(src, color);
                if (rle < 0)
                        return -1;
                total += rle;
        } while (rle >= 64);

        return total;
}

static int
emit(struct cf_buffer_t *dst, int rle, int color, int black_is_1)
{
        int out_color;
        size_t start;

        if (rle <= 0)
                return 0;

        if (!cf_resize_buffer_least(dst, (rle + 7) >> 3))
                return 1;

        out_color = color ^ black_is_1;
        start = dst->pos;
        cf_setbits(dst->buf, start, start + rle, out_color);
        dst->pos += rle;

        return 0;
}

static int
cfd_1d_line(struct cf_buffer_t *dst, struct cf_buffer_t *src,
            const struct cf_params_t *params)
{
        int a0 = 0;
        int color = 1;

        while (a0 < params->columns) {
                int rle = get_rle(src, color);
                if (rle < 0)
                        return 1;

                if (a0 + rle > params->columns)
                        rle = params->columns - a0;

                if (emit(dst, rle, color, params->black_is_1))
                        return 1;

                a0 += rle;
                color = !color;
        }

        return 0;
}

static int
get_eob_tail(struct cf_buffer_t *src)
{
        int i;

        for (i = 0; i < 6; ++i) {
                int rc = try_consume_eol(src);
                if (rc != 1)
                        return 0;
        }

        return 1;
}

struct cf_buffer_t *
cfd_g3_2d(const char *src, size_t srclen, struct cf_params_t *params)
{
        struct cf_buffer_t *dst;
        struct cf_buffer_t sbuf;
        char *ref;
        int line, row_bytes;
        const unsigned char white_byte = params->black_is_1 ? 0x00 : 0xff;

        if (!params || params->k <= 0 || params->columns <= 0 ||
            params->rows <= 0)
                return 0;

        dst = cf_make_buffer();
        if (!dst)
                return 0;

        sbuf.buf = (char *)src;
        sbuf.cap = srclen;
        sbuf.pos = 0;

        row_bytes = (params->columns + 7) >> 3;

        ref = malloc(row_bytes);
        if (!ref) {
                free(dst->buf);
                free(dst);
                return 0;
        }
        memset(ref, white_byte, row_bytes);

        for (line = 0; line < params->rows; ++line) {
                size_t row_start = dst->pos;
                int tag, rc;

                if (params->end_of_line)
                        try_consume_eol(&sbuf);

                if (params->encoded_byte_align)
                        sbuf.pos = (sbuf.pos + 7) & ~7;

                tag = get_bit(&sbuf);
                if (tag < 0) {
                        fprintf(stderr, "cfd_g3_2d: missing tag on line %d\n", line);
                        break;
                }

                if (tag) {
                        rc = cfd_1d_line(dst, &sbuf, params);
                } else {
                        rc = cfd_2d_line(ref, dst, &sbuf, params);
                        if (rc == 2)
                                rc = 1;
                }

                if (rc != 0) {
                        fprintf(stderr, "cfd_g3_2d: error on line %d\n", line);
                        break;
                }

                cf_byte_align(dst);

                {
                        size_t written = (dst->pos - row_start) >> 3;
                        size_t copy = written < (size_t)row_bytes
                                        ? written : (size_t)row_bytes;
                        memcpy(ref, dst->buf + (row_start >> 3), copy);
                        if (copy < (size_t)row_bytes)
                                memset(ref + copy, white_byte, row_bytes - copy);
                }
        }

        if (line >= params->rows && params->end_of_block &&
            !get_eob_tail(&sbuf)) {
                fprintf(stderr, "cfd_g3_2d: missing EOB/RTC\n");
        }

        free(ref);
        return dst;
}
