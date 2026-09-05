/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfd_2d.h"


static inline int
get_bit(struct cf_buffer_t *src)
{
        if (src->pos >= (src->cap << 3))
                return -1;
        return cf_getbit(src->buf, src->pos++);
}

static void
try_consume_eol(struct cf_buffer_t *src)
{
        size_t saved = src->pos;
        int i, b;

        for (i = 0; i < 11; ++i) {
                b = get_bit(src);
                if (b != 0) {
                        src->pos = saved;
                        return;
                }
        }

        b = get_bit(src);
        if (b != 1)
                src->pos = saved;
}

/*
 * G4 (T.6 MMR) decoder.
 *
 * Pure 2D encoding: every line is encoded relative to the previous
 * (reference) line. No EOL codes, no RTC. Termination is by row count
 * (params->rows). An optional EOFB (two consecutive 12-bit EOL codewords:
 * 000000000001 000000000001) may appear at the end and is consumed if
 * present.
 *
 * Mode codes (MSB-first):
 *
 *   Pass       0001
 *   H          001
 *   V(0)       1
 *   VR(1)      011
 *   VL(1)      010
 *   VR(2)      0000 11
 *   VL(2)      0000 10
 *   VR(3)      0000 011
 *   VL(3)      0000 010
 *
 * Changing elements nomenclature:
 *
 *   a0  : current reference position on coding line (starts at -1 each row)
 *   a1  : next changing element on coding line to the right of a0,
 *         same color as current coding color
 *   a2  : next changing element on coding line to the right of a1
 *   b1  : first changing element on reference line to the right of a0,
 *         opposite color to current coding color
 *   b2  : next changing element on reference line to the right of b1
 *
 * Color convention matches rest of library: 1 = white, 0 = black.
 * coding line starts white (color = 1) at a0 = -1.
 */

struct cf_buffer_t *
cfd_g4(const char *src, size_t srclen, const struct cf_params_t *params)
{
        struct cf_buffer_t *dst;
        struct cf_buffer_t sbuf;
        char *ref;
        int line, row_bytes, rc;

        const unsigned char white_byte = params->black_is_1 ? 0x00 : 0xff;

        if (!params || params->columns <= 0 || params->rows <= 0)
                return 0;

        dst = cf_make_buffer();
        if (!dst)
                return 0;

        sbuf.buf = (char *)src;
        sbuf.cap = srclen;
        sbuf.pos = 0;

        row_bytes = (params->columns + 7) >> 3;

        /* The reference line for row 0 is an imaginary all-white line */
        ref = calloc(row_bytes, 1);
        if (!ref) {
                cf_free_buffer(dst);
                return 0;
        }
        memset(ref, white_byte, row_bytes);

        for (line = 0; line < params->rows; ++line) {
                size_t row_start = dst->pos;

                if (params->encoded_byte_align)
                        sbuf.pos = (sbuf.pos + 7) & ~7;

                if (params->end_of_line)
                        try_consume_eol(&sbuf);

                rc = cfd_2d_line(ref, dst, &sbuf, params);
                if (rc == 2) {
                        /* EOFB: normal end before params->rows exhausted */
                        break;
                }
                if (rc != 0) {
                        fprintf(stderr, "cfd_g4: error on line %d\n", line);
                        break;
                }

                /* Pad output row to byte boundary */
                cf_byte_align(dst);

                /* Current decoded row becomes the next reference line */
                {
                        size_t written = (dst->pos - row_start) >> 3;
                        size_t copy = written < (size_t)row_bytes
                                        ? written : (size_t)row_bytes;
                        memcpy(ref, dst->buf + (row_start >> 3), copy);
                        if (copy < (size_t)row_bytes)
                                memset(ref + copy, white_byte, row_bytes - copy);
                }
        }

        free(ref);
        return dst;
}
