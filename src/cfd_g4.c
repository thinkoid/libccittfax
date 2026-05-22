/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfd_trie.h"

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

#define MODE_PASS   0
#define MODE_H      1
#define MODE_V0     2
#define MODE_VR1    3
#define MODE_VL1    4
#define MODE_VR2    5
#define MODE_VL2    6
#define MODE_VR3    7
#define MODE_VL3    8
#define MODE_EOFB   9
#define MODE_ERROR  -1

static inline int
get_bit(struct cf_buffer_t *src)
{
        if (src->pos >= (src->cap << 3))
                return -1;
        return cf_getbit(src->buf, src->pos++);
}

/*
 * Decode a G4 mode codeword from src.
 * Consumes exactly the bits belonging to the recognised code.
 * Returns MODE_* or MODE_ERROR.
 */
static int
get_mode(struct cf_buffer_t *src)
{
        int b;

        b = get_bit(src); if (b < 0) return MODE_ERROR;
        if (b) return MODE_V0;                          /* 1        */

        b = get_bit(src); if (b < 0) return MODE_ERROR;
        if (b) {
                /* 01x — VR(1)=011 or VL(1)=010 */
                b = get_bit(src); if (b < 0) return MODE_ERROR;
                return b ? MODE_VR1 : MODE_VL1;         /* 011/010  */
        }

        b = get_bit(src); if (b < 0) return MODE_ERROR;
        if (b) return MODE_H;                           /* 001      */

        b = get_bit(src); if (b < 0) return MODE_ERROR;
        if (b) return MODE_PASS;                        /* 0001     */

        b = get_bit(src); if (b < 0) return MODE_ERROR;
        if (b) {
                /* 00001x — VR(2) or VL(2) */
                b = get_bit(src); if (b < 0) return MODE_ERROR;
                return b ? MODE_VR2 : MODE_VL2;         /* 000011 / 000010 */
        }

        b = get_bit(src); if (b < 0) return MODE_ERROR;
        if (b) {
                /* 000001x — VR(3) or VL(3) */
                b = get_bit(src); if (b < 0) return MODE_ERROR;
                return b ? MODE_VR3 : MODE_VL3;         /* 0000011 / 0000010 */
        }

        /*
         * 0000 000... — only valid sequence remaining is EOFB:
         * twelve zeros followed by one, twice.
         * We have consumed 7 bits (all zero); need 5 more zeros then a 1
         * for the first EOL, then 12 more bits (000000000001) for the second.
         */
        {
                int i;
                for (i = 7; i < 12; ++i) {
                        b = get_bit(src); if (b != 0) return MODE_ERROR;
                }
                b = get_bit(src); if (b != 1) return MODE_ERROR;
                /* first EOL consumed; now second */
                for (i = 0; i < 11; ++i) {
                        b = get_bit(src); if (b != 0) return MODE_ERROR;
                }
                b = get_bit(src); if (b != 1) return MODE_ERROR;
                return MODE_EOFB;
        }
}

/*
 * Decode one 1D run-length for color `color' using the MH trie.
 * Returns run length >= 0, or -1 on error.
 * Reuses the trie infrastructure from cfd_g3_1d.
 */
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

/*
 * Emit `rle' pixels of `color' (pre-polarity) to dst.
 * Returns 0 on success, 1 on allocation failure.
 */
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

/*
 * find_b1: first changing element on ref strictly to the right of a0
 * whose color is opposite to `color'.
 *
 * The reference line has an implied white pixel at position -1 (before
 * the line). cf_find_changing(ref, pos, columns) returns the position of
 * the next color transition starting at pos, or columns if none found.
 */
static int
find_b1(const char *ref, int a0, int color, int columns)
{
        int pos, ref_color;

        pos = (a0 < 0) ? 0 : a0 + 1;
        if (pos >= columns)
                return columns;

        ref_color = cf_getbit(ref, pos);

        /* If the reference line at pos is already the opposite color, b1=pos */
        if (ref_color != color)
                return pos;

        /* Otherwise skip the run of `color' pixels on the reference line */
        return cf_find_changing(ref, pos, columns);
}

/*
 * find_b2: next changing element on ref after b1.
 */
static int
find_b2(const char *ref, int b1, int columns)
{
        if (b1 >= columns)
                return columns;
        return cf_find_changing(ref, b1, columns);
}

/*
 * Try to consume an EOL marker (twelve bits: 000000000001) from src.
 * Returns  1 if an EOL was consumed,
 *          0 if no EOL was present (src position unchanged),
 *         -1 on a partial / malformed sequence (src position unchanged).
 *
 * Implementation: peek bits one at a time; on mismatch roll back.
 */
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

/*
 * Decode one G4 line into dst.
 * ref      : previous decoded scanline (bit-packed, columns wide), or an
 *            all-white imaginary line for the first row
 * Returns 0 on success, 1 on error, 2 on EOFB.
 */
static int
cfd_g4_line(const char *ref, struct cf_buffer_t *dst,
            struct cf_buffer_t *src, const struct cf_params_t *params)
{
        const int columns = params->columns;
        const int black_is_1 = params->black_is_1;
        int a0 = -1;
        int color = 1; /* coding line starts white */

        if (try_consume_eol(src) != 1 && params->end_of_line)
                return 1;

        while (a0 < columns) {
                int b1, b2, a1, rle;
                int mode = get_mode(src);

                switch (mode) {

                case MODE_EOFB:
                        return 2;

                case MODE_ERROR:
                        return 1;

                case MODE_PASS:
                        b1 = find_b1(ref, a0, color, columns);
                        b2 = find_b2(ref, b1, columns);
                        rle = b2 - (a0 < 0 ? 0 : a0);
                        if (emit(dst, rle, color, black_is_1))
                                return 1;
                        a0 = b2;
                        /* color unchanged */
                        break;

                case MODE_H: {
                        int rle2, a2;

                        rle = get_rle(src, color);
                        if (rle < 0) return 1;
                        a1 = (a0 < 0 ? 0 : a0) + rle;
                        if (a1 > columns) a1 = columns;

                        rle2 = get_rle(src, !color);
                        if (rle2 < 0) return 1;
                        a2 = a1 + rle2;
                        if (a2 > columns) a2 = columns;

                        if (emit(dst, a1 - (a0 < 0 ? 0 : a0), color, black_is_1))
                                return 1;
                        if (emit(dst, a2 - a1, !color, black_is_1))
                                return 1;

                        a0 = a2;
                        /* color unchanged */
                        break;
                }

                case MODE_V0:
                case MODE_VR1: case MODE_VL1:
                case MODE_VR2: case MODE_VL2:
                case MODE_VR3: case MODE_VL3: {
                        static const int voffset[7] = { 0, 1, -1, 2, -2, 3, -3 };
                        int offset = voffset[mode - MODE_V0];

                        b1 = find_b1(ref, a0, color, columns);
                        a1 = b1 + offset;
                        if (a1 < 0)       a1 = 0;
                        if (a1 > columns) a1 = columns;

                        rle = a1 - (a0 < 0 ? 0 : a0);
                        if (emit(dst, rle, color, black_is_1))
                                return 1;

                        a0 = a1;
                        color = !color;
                        break;
                }

                default:
                        return 1;
                }
        }

        return 0;
}

struct cf_buffer_t *
cfd_g4(const char *src, size_t srclen, struct cf_params_t *params)
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
                free(dst->buf);
                free(dst);
                return 0;
        }
        memset(ref, white_byte, row_bytes);

        for (line = 0; line < params->rows; ++line) {
                size_t row_start = dst->pos;

                if (params->encoded_byte_align)
                        sbuf.pos = (sbuf.pos + 7) & ~7;

                rc = cfd_g4_line(ref, dst, &sbuf, params);
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
