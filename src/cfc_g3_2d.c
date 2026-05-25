/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cf.h"
#include "cfc_2d.h"
#include "cfc_common.h"
#include "cfc_g3_1d.h"

static inline int
cfc_encoded_byte_align(struct cf_buffer_t *buf)
{
        return cfc_put_rle_explicit(buf, 0, (8 - (buf->pos & 7)) & 7);
}

static int
put_tag(struct cf_buffer_t *dst, int is_1d)
{
        return cfc_put_rle_explicit(dst, is_1d ? 1 : 0, 1);
}

static int
put_line_prefix(struct cf_buffer_t *dst, const struct cf_params_t *params,
                int is_1d)
{
        if (params->end_of_line && cfc_put_eol(dst))
                return 1;

        if (params->encoded_byte_align && cfc_encoded_byte_align(dst))
                return 1;

        return put_tag(dst, is_1d);
}

struct cf_buffer_t *
cfc_g3_2d(const char *buf, struct cf_params_t *params)
{
        struct cf_buffer_t src, *dst;
        char *ref;
        int line, row_bytes;

        if (!params || params->k <= 0 || params->columns <= 0 ||
            params->rows <= 0)
                return 0;

        src.buf = (char *)buf;
        src.cap = params->rows * ((params->columns + 7) >> 3);
        src.pos = 0;

        dst = cf_make_buffer();
        if (!dst) {
                fprintf(stderr, "malloc compression buffer : %s\n",
                        strerror(errno));
                return 0;
        }

        row_bytes = (params->columns + 7) >> 3;

        ref = malloc(row_bytes);
        if (!ref) {
                fprintf(stderr, "cfc_g3_2d: malloc ref: %s\n",
                        strerror(errno));
                free(dst->buf);
                free(dst);
                return 0;
        }
        memset(ref, params->black_is_1 ? 0x00 : 0xff, row_bytes);

        for (line = 0; line < params->rows; ++line) {
                const char *coding = src.buf + (line * row_bytes);
                int is_1d = (line % params->k) == 0;

                if (put_line_prefix(dst, params, is_1d)) {
                        fprintf(stderr, "cfc_g3_2d: failed line prefix %d\n",
                                line);
                        goto err;
                }

                if (is_1d) {
                        if (cfc_g3_1d_line(dst, &src, params)) {
                                fprintf(stderr,
                                        "cfc_g3_2d: 1D error on line %d\n",
                                        line);
                                goto err;
                        }
                } else {
                        if (cfc_2d_line(dst, coding, ref, params)) {
                                fprintf(stderr,
                                        "cfc_g3_2d: 2D error on line %d\n",
                                        line);
                                goto err;
                        }
                        src.pos += params->columns;
                }

                cf_byte_align(&src);
                memcpy(ref, coding, row_bytes);
        }

        if (params->end_of_block && cfc_put_eol_n(dst, 6))
                goto err;

        free(ref);
        return dst;

err:
        free(ref);
        free(dst->buf);
        free(dst);
        return 0;
}
