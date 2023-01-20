/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <stdio.h>
#include <stdlib.h>

#include "cf.h"
#include "cfd_trie.h"

struct cf_state_t {
        struct cf_buffer_t *src, *dst;
        struct cf_params_t *params;
        int a0, a1, a2, b1, b2;
        int color;
};

static inline int
eob(struct cf_buffer_t *src)
{
        return src->pos >= (src->cap << 3);
}

static int
do_get_rle(struct cf_buffer_t *cf_buf, int color)
{
        int c, rle;
        size_t endpos;

        struct cfd_trie_state_t state;
        cfd_trie_init(&state, color);

        for (endpos = cf_buf->cap << 3; cf_buf->pos < endpos; ) {
                c = cf_getbit(cf_buf->buf, cf_buf->pos);
                if (!cfd_trie_walk(&state, c))
                        break;

                ++cf_buf->pos;

                if (cfd_trie_is_terminal(&state)) {
                        cfd_trie_get_value(&state, &rle);
                        return rle;
                }
        }

        return -2;
}

/* Decodes the incoming stream. When successful it will have decoded a full
 * runlength of one or zero bits. When it fails it will leave the position
 * indicator at the place of failure.
 */
static int
get_rle(struct cf_buffer_t *cf_buf, int color)
{
        int rle, tail;

        rle = do_get_rle(cf_buf, color);
        if (rle >= 64) {
                tail = do_get_rle(cf_buf, color);
                return 0 <= tail ? rle + tail : -2;
        }

        return rle;
}

static inline int
get_eol(struct cf_buffer_t *cf_buf)
{
        return -1 == get_rle(cf_buf, 0);
}

static inline int
try_get_eol(struct cf_buffer_t *cf_buf)
{
        size_t pos = cf_buf->pos;

        if (-1 != get_rle(cf_buf, 0))
                cf_buf->pos = pos;

        return cf_buf->pos != pos;
}

static inline int
get_eol_tail(struct cf_buffer_t *cf_buf)
{
        return  get_eol(cf_buf) &&
                get_eol(cf_buf) &&
                get_eol(cf_buf) &&
                get_eol(cf_buf) &&
                get_eol(cf_buf)
                ;
}

static inline int
try_get_eol_tail(struct cf_buffer_t *cf_buf)
{
        size_t pos = cf_buf->pos;

        if (!get_eol_tail(cf_buf))
                cf_buf->pos = pos;

        return cf_buf->pos != pos;
}

static size_t
skip_bits(struct cf_buffer_t *cf_buf, int x)
{
        size_t start = cf_buf->pos, endpos = cf_buf->cap << 3;

        for (; cf_buf->pos < endpos && x == cf_getbit(cf_buf->buf, cf_buf->pos);
             ++cf_buf->pos) ;

        return cf_buf->pos - start;
}

static size_t
skip_zeroes(struct cf_buffer_t *cf_buf)
{
        return skip_bits(cf_buf, 0);
}

static size_t
skip_ones(struct cf_buffer_t *cf_buf)
{
        return skip_bits(cf_buf, 1);
}

static void
skip_to_newline(struct cf_buffer_t *cf_buf)
{
        size_t endpos = (cf_buf->cap << 3);

        while (cf_buf->pos < endpos) {
                skip_ones(cf_buf);

                if (11 <= skip_zeroes(cf_buf)) {
                        if (cf_buf->pos < endpos)
                                ++cf_buf->pos;

                        return;
                }
        }
}

static int
fill(struct cf_buffer_t *cf_buf, int n, int color)
{
        if (0 >= n)
                return 0;

        if (0 == cf_resize_buffer_least(cf_buf, (n + 7) >> 3))
                return 1;

        cf_setbits(cf_buf->buf, cf_buf->pos, cf_buf->pos + n, color);
        cf_buf->pos += n;

        return 0;
}

static void
byte_align(struct cf_buffer_t *buf)
{
        buf->pos = (buf->pos + 7) & ~7;
}

static int
cfd_g3_1d_decode_other(struct cf_state_t *state, int rle)
{
        if (fill(state->dst, rle, state->color ^ state->params->black_is_1))
                return 1;

        state->a0 += rle;
        state->color = !state->color;

        return 0;
}

static int
cfd_g3_1d_decode_newline(struct cf_state_t *state)
{
        struct cf_params_t *params = state->params;

        struct cf_buffer_t *src = state->src;
        struct cf_buffer_t *dst = state->dst;

        if (fill(dst, params->columns - state->a0, params->black_is_1))
                return 1;

        byte_align(dst);

        if (try_get_eol_tail(src)) {
                return 0;
        }

        if (state->a0 < state->params->columns)
                fprintf(stderr, "underflow: expected %d columns, got %d\n",
                        state->params->columns, state->a0);

        if (state->params->encoded_byte_align)
                byte_align(state->src);

        state->a0 = 0;
        state->color = 1;

        return 0;
}

static int
cfd_g3_1d_decode(struct cf_state_t *state, int rle)
{
        struct cf_params_t *params = state->params;
        struct cf_buffer_t *src = state->src;

        switch (rle) {
        case -1:
                return cfd_g3_1d_decode_newline(state);

        case -2:
                skip_to_newline(src);
                return cfd_g3_1d_decode(state, -1);

        default:
                if (state->a0 + rle > params->columns) {
                        if (cfd_g3_1d_decode(state, params->columns - state->a0))
                                return 1;

                        return cfd_g3_1d_decode(state, -2);
                }

                return cfd_g3_1d_decode_other(state, rle);
        }
}

static struct cf_buffer_t *
cfd_do_g3_1d(struct cf_state_t *state)
{
        struct cf_buffer_t *src = state->src;
        try_get_eol(src);

        for (; !eob(src); ) {
                if (cfd_g3_1d_decode(state, get_rle(src, state->color))) {
                        free(state->dst->buf);
                        free(state->dst);
                        state->dst = 0;
                        break;
                }
        }

        return state->dst;
}

struct cf_buffer_t *
cfd_g3_1d(const char *buf, size_t len, struct cf_params_t *params)
{
        struct cf_state_t state;
        struct cf_buffer_t src = { (char *)buf, len, 0 }, *dst;

        dst = cf_make_buffer();
        if (0 == dst)
                return 0;

        state = (struct cf_state_t){ &src, dst, params, 0, 0, 0, 0, 0, 0 };
        return cfd_do_g3_1d(&state);
}
