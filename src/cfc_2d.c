/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include "cf.h"
#include "cfc_common.h"
#include "cfc_tables.h"
#include "cfc_2d.h"

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
int
cfc_2d_line(struct cf_buffer_t *dst,
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

