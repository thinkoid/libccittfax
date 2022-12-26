/* -*- mode: c; -*- */

#ifndef CCITTFAX_CCITTFAX_H
#define CCITTFAX_CCITTFAX_H

#include <ccittfax/defs.h>

#include <stddef.h>

struct cf_params_t
{
        int k;
        int end_of_line;
        int encoded_byte_align;
        int columns;
        int rows;
        int end_of_block;
        int black_is_1;
        int damage_limit;
};

const char *version();

struct cf_buffer_t *
cfc(const char *src, struct cf_params_t *params);

struct cf_buffer_t *
cfc_g3_1d(const char *src, struct cf_params_t *params);

struct cf_buffer_t *
cfc_g3_2d(const char *src, struct cf_params_t *params);

struct cf_buffer_t *
cfc_g4(const char *src, struct cf_params_t *params);


struct cf_buffer_t *
cfd(const char *src, size_t srclen, struct cf_params_t *params);

struct cf_buffer_t *
cfd_g3_1d(const char *src, size_t srclen, struct cf_params_t *params);

struct cf_buffer_t *
cfd_g3_2d(const char *src, size_t srclen, struct cf_params_t *params);

struct cf_buffer_t *
cfd_g4(const char *src, size_t srclen, struct cf_params_t *params);

#endif /* CCITTFAX_CCITTFAX_H */
