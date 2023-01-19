/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#include "cf.h"

struct cf_buffer_t *
cfd_g3_1d(const char *src, size_t srclen, struct cf_params_t *params);

struct cf_buffer_t *
cfd_g3_2d(const char *src, size_t srclen, struct cf_params_t *params);

struct cf_buffer_t *
cfd_g4(const char *src, size_t srclen, struct cf_params_t *params);

struct cf_buffer_t *
cfd(const char *src, size_t srclen, struct cf_params_t *params)
{
        if (0 == params)
                return 0;

        /*
         * CCITT encoding is bit oriented, not byte oriented.
         *
         * Unencoded data is stored as complete scan lines that end at byte
         * boundary. If data does not end at byte boundary, zero bits will be
         * inserted at the end. After the end of data has been reached, the
         * reader advances to the next byte boundary.
         *
         * Encoded data is an uninterrupted bit stream where the end of the
         * line is signaled by a special, distinct code, in order to
         * facilitate error recovery.
         *
         * See REAMDE.
         */

        return params->k < 0
                ? cfd_g4(src, srclen, params) : 0 == params->k
                ? cfd_g3_1d(src, srclen, params) : cfd_g3_2d(src, srclen, params);
}
