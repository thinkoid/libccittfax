/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfc_common.h"
#include "cfc_tables.h"

/*
 * G4 (T.6 MMR) encoder.
 *
 * Encodes raw 1bpp pixel data using pure 2D MMR encoding.  Every line is
 * encoded relative to the previous (reference) line.  No EOL codes are
 * emitted between rows.  An optional EOFB (two 12-bit EOL codewords) is
 * appended when params->end_of_block is set.
 *
 * Mode code bit patterns (MSB-first):
 *
 *   V(0)   1
 *   VR(1)  011
 *   VL(1)  010
 *   H      001
 *   Pass   0001
 *   VR(2)  000011
 *   VL(2)  000010
 *   VR(3)  0000011
 *   VL(3)  0000010
 *
 * Color convention: 1 = white, 0 = black (black_is_1 XOR applied on read).
 * Coding line starts white (color = 1) at a0 = -1.
 */

/* Mode code table: { value, len } matching cfc_code_t layout. */
static const struct cfc_code_t mode_codes[] = {
        { 0x1,  1 },    /* V(0)  */
        { 0x3,  3 },    /* VR(1) */
        { 0x2,  3 },    /* VL(1) */
        { 0x1,  3 },    /* H     */
        { 0x1,  4 },    /* Pass  */
        { 0x3,  6 },    /* VR(2) */
        { 0x2,  6 },    /* VL(2) */
        { 0x3,  7 },    /* VR(3) */
        { 0x2,  7 },    /* VL(3) */
};

#define CODE_V0     0
#define CODE_VR1    1
#define CODE_VL1    2
#define CODE_H      3
#define CODE_PASS   4
#define CODE_VR2    5
#define CODE_VL2    6
#define CODE_VR3    7
#define CODE_VL3    8

static inline int
put_mode(struct cf_buffer_t *dst, int code)
{
        return cfc_put_rle_explicit(dst, mode_codes[code].value,
                                    mode_codes[code].len);
}

/*
 * Read the pixel color at position `pos' in a bit-packed row,
 * applying black_is_1 polarity.
 */
static inline int
pixel(const char *buf, int pos, int black_is_1)
{
        int raw = cf_getbit(buf, pos);
        return raw ^ black_is_1;
}

/*
 * find_a1: next changing element on the coding line strictly to the
 * right of a0, with the current coding color.
 *
 * Returns the position of the first pixel whose color differs from
 * the pixel at a0 (or `color' if a0 == -1), or `columns' if none found.
 */
static int
find_a1(const char *coding, int a0, int color, int columns, int black_is_1)
{
        int pos = (a0 < 0) ? 0 : a0 + 1;

        for (; pos < columns; ++pos) {
                if (pixel(coding, pos, black_is_1) != color)
                        break;
        }

        return pos;
}

/*
 * find_a2: next changing element after a1.
 */
static int
find_a2(const char *coding, int a1, int columns, int black_is_1)
{
        int color;

        if (a1 >= columns)
                return columns;

        color = pixel(coding, a1, black_is_1);

        return find_a1(coding, a1, color, columns, black_is_1);
}

/*
 * find_b1: first changing element on the reference line strictly to the
 * right of a0 whose color is opposite to `color'.
 */
static int
find_b1(const char *ref, int a0, int color, int columns, int black_is_1)
{
        int pos = (a0 < 0) ? 0 : a0 + 1;

        if (pos >= columns)
                return columns;

        /* If ref[pos] is already opposite color, b1 = pos */
        if (pixel(ref, pos, black_is_1) != color)
                return pos;

        /* Otherwise find the next transition on the reference line */
        for (++pos; pos < columns; ++pos) {
                if (pixel(ref, pos, black_is_1) != color)
                        break;
        }

        return pos;
}

/*
 * find_b2: next changing element on ref after b1.
 */
static int
find_b2(const char *ref, int b1, int columns, int black_is_1)
{
        int color;

        if (b1 >= columns)
                return columns;

        color = pixel(ref, b1, black_is_1);

        for (++b1; b1 < columns; ++b1) {
                if (pixel(ref, b1, black_is_1) != color)
                        break;
        }

        return b1;
}

/*
 * Encode one G4 line.
 *
 * coding   : current line to encode (bit-packed, byte-aligned)
 * ref      : previous decoded line (bit-packed, byte-aligned), or
 *            all-white imaginary line for row 0
 * dst      : output bitstream
 * params   : encode parameters
 *
 * Returns 0 on success, non-zero on error.
 */
static int
cfc_g4_line(struct cf_buffer_t *dst,
            const char *coding, const char *ref,
            const struct cf_params_t *params)
{
        const int columns    = params->columns;
        const int black_is_1 = params->black_is_1;
        int a0    = -1;
        int color = 1;          /* coding line starts white */

        while (a0 < columns) {
                int a1, a2, b1, b2;

                a1 = find_a1(coding, a0, color, columns, black_is_1);
                b1 = find_b1(ref,    a0, color, columns, black_is_1);
                b2 = find_b2(ref,    b1, columns, black_is_1);

                /*
                 * Pass mode: b2 lies to the left of a1.
                 * Emit Pass; a0 advances to b2, color unchanged.
                 */
                if (b2 < a1) {
                        if (put_mode(dst, CODE_PASS))
                                return 1;
                        a0 = b2;
                        continue;
                }

                /*
                 * Vertical mode: |a1 - b1| <= 3.
                 * Emit V(a1-b1); a0 advances to a1, color toggles.
                 */
                {
                        int delta = a1 - b1;

                        if (delta >= -3 && delta <= 3) {
                                int code;

                                switch (delta) {
                                case  0: code = CODE_V0;  break;
                                case  1: code = CODE_VR1; break;
                                case -1: code = CODE_VL1; break;
                                case  2: code = CODE_VR2; break;
                                case -2: code = CODE_VL2; break;
                                case  3: code = CODE_VR3; break;
                                default: code = CODE_VL3; break;
                                }

                                if (put_mode(dst, code))
                                        return 1;

                                a0    = a1;
                                color = !color;
                                continue;
                        }
                }

                /*
                 * Horizontal mode: |a1 - b1| > 3 and Pass doesn't apply.
                 * Emit H + run(color, a0..a1) + run(!color, a1..a2).
                 */
                a2 = find_a2(coding, a1, columns, black_is_1);

                if (put_mode(dst, CODE_H))
                        return 1;
                if (cfc_put_rle(dst, a1 - (a0 < 0 ? 0 : a0), color))
                        return 1;
                if (cfc_put_rle(dst, a2 - a1, !color))
                        return 1;

                a0 = a2;
                /* color unchanged after H */
        }

        return 0;
}

struct cf_buffer_t *
cfc_g4(const char *src, struct cf_params_t *params)
{
        struct cf_buffer_t *dst;
        const char *coding;
        char *ref;
        int line, row_bytes;

        if (!params || params->columns <= 0 || params->rows <= 0)
                return 0;

        dst = cf_make_buffer();
        if (!dst) {
                fprintf(stderr, "cfc_g4: malloc: %s\n", strerror(errno));
                return 0;
        }

        row_bytes = (params->columns + 7) >> 3;

        /*
         * Reference line for row 0: imaginary all-white line.
         * With black_is_1=0: white = bit 1, so 0xff bytes.
         * With black_is_1=1: white = bit 0, so 0x00 bytes.
         */
        ref = malloc(row_bytes);
        if (!ref) {
                fprintf(stderr, "cfc_g4: malloc ref: %s\n", strerror(errno));
                free(dst->buf);
                free(dst);
                return 0;
        }
        memset(ref, params->black_is_1 ? 0x00 : 0xff, row_bytes);

        for (line = 0; line < params->rows; ++line) {
                coding = src + line * row_bytes;

                if (params->encoded_byte_align) {
                        size_t pad = (8 - (dst->pos & 7)) & 7;
                        if (pad && cfc_put_rle_explicit(dst, 0, pad))
                                goto err;
                }

                if (cfc_g4_line(dst, coding, ref, params)) {
                        fprintf(stderr, "cfc_g4: error on line %d\n", line);
                        goto err;
                }

                /* Current coding line becomes next reference line */
                memcpy(ref, coding, row_bytes);
        }

        /* EOFB: two 12-bit EOL codewords (000000000001 twice) */
        if (params->end_of_block) {
                if (cfc_put_rle_explicit(dst, 0x1, 12))
                        goto err;
                if (cfc_put_rle_explicit(dst, 0x1, 12))
                        goto err;
        }

        free(ref);
        return dst;

err:
        free(ref);
        free(dst->buf);
        free(dst);
        return 0;
}
