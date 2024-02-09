/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CFC_COMMON_H
#define CCITTFAX_SRC_CFC_COMMON_H

#include <ccittfax/defs.h>
#include <ccittfax/ccittfax.h>

int
cfc_put_rle_explicit(struct cf_buffer_t *dst, unsigned value, unsigned len);

int
cfc_put_eol(struct cf_buffer_t *dst);

int
cfc_put_eol_n(struct cf_buffer_t *buf, size_t n);

int
cfc_put_eol_0(struct cf_buffer_t *buf);

int
cfc_put_eol_1(struct cf_buffer_t *buf);

int
cfc_put_rle(struct cf_buffer_t *dst, int rle, int color);

#endif /* CCITTFAX_SRC_CFC_COMMON_H */
