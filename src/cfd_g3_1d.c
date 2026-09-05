/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <stdio.h>
#include <stdlib.h>

#include "cf.h"
#include "cfd_trie.h"

struct cf_state_t {
        struct cf_buffer_t *src, *dst;
        const struct cf_params_t *params;
        int a0, a1, a2, b1, b2;
        int color;
};

static int
do_get_rle(struct cf_buffer_t *cf_buf, int color)
{
        int c;
        size_t endpos;

        struct cfd_trie_state_t state;
        cfd_trie_init(&state, color);

        for (endpos = cf_buf->cap << 3; cf_buf->pos < endpos; ) {
                c = cf_getbit(cf_buf->buf, cf_buf->pos);
                if (!cfd_trie_walk(&state, c))
                        break;

                ++cf_buf->pos;

                if (cfd_trie_is_terminal(&state))
                        return cfd_trie_get_value(&state);
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
        int rle, total = 0;

        do {
                if(0 > (rle = do_get_rle(cf_buf, color)))
                        return rle;
                total += rle;
        } while(rle >= 64);

        return total;
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
putback_eol(struct cf_buffer_t *cf_buf)
{
        if (cf_buf->pos > 12) {
                cf_buf->pos -= 12;
                return 0;
        }

        return 1;
}

static inline int
get_eob_tail(struct cf_buffer_t *cf_buf)
{
        return  get_eol(cf_buf) &&
                get_eol(cf_buf) &&
                get_eol(cf_buf) &&
                get_eol(cf_buf) &&
                get_eol(cf_buf)
                ;
}

static inline int
try_get_eob_tail(struct cf_buffer_t *cf_buf)
{
        size_t pos = cf_buf->pos;

        if (!get_eob_tail(cf_buf))
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
                        if (cf_buf->pos < endpos) {
                                cf_buf->pos -= 11;
                                break;
                        }
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

static int
emit_run(struct cf_state_t *state, int rle)
{
        const struct cf_params_t *params = state->params;

        if (state->a0 + rle > params->columns)
                rle = params->columns - state->a0;

        if (fill(state->dst, rle, state->color ^ params->black_is_1))
                return 1;

        state->a0 += rle;
        return 0;
}

static int
cfd_g3_1d_line(struct cf_state_t *state)
{
        const struct cf_params_t *params = state->params;

        state->a0 = 0;
        state->color = 1;

        for (; state->a0 < params->columns; state->color = !state->color) {
                int rle = get_rle(state->src, state->color);
                if (rle < 0) {
                        if (rle == -2 /* && params->end_of_line */)
                                /* always skip on error */
                                skip_to_newline(state->src);

                        if (emit_run(state, params->columns - state->a0))
                                return 1;

                        if (rle == -1)
                                /* unexpected, early EOL */
                                putback_eol(state->src);

                        break;
                }

                if (emit_run(state, rle))
                        return 1;
        }

        return 0;
}

struct cf_buffer_t*
cfd_g3_1d(const char *buf, size_t len, const struct cf_params_t *params)
{
        int line;

        struct cf_state_t state = {0};
        struct cf_buffer_t src = { (char*)buf, len, 0 }, *dst;

        dst = cf_make_buffer();
        if (0 == dst)
                return 0;

        state = (struct cf_state_t){ &src, dst, params, 0, 0, 0, 0, 0, 1 };

        for (line = 0; line < params->rows; line++) {
                if (params->end_of_line && !get_eol(&src)) {
                        fprintf(stderr, "missing EOL before line %d\n", line);
                        goto err;
                }

                if (params->encoded_byte_align)
                        cf_byte_align(&src);

                if (cfd_g3_1d_line(&state)) {
                        fprintf(stderr, "error decoding line %d\n", line);
                        goto err;
                }

                cf_byte_align(dst);
        }

        if (line >= params->rows &&
            params->end_of_block && !get_eob_tail(&src)) {
                fprintf(stderr, "missing EOB/RTC\n");
                goto err;
        }

err:
        return dst;
}
