/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfc_common.h"
#include "cfc_2d.h"

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

struct cf_buffer_t *
cfc_g4(const char *src, const struct cf_params_t *params)
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
                cf_free_buffer(dst);
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

                if (cfc_2d_line(dst, coding, ref, params)) {
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
        cf_free_buffer(dst);
        return 0;
}
