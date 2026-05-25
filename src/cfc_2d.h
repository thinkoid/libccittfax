/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CFC_2D_H
#define CCITTFAX_SRC_CFC_2D_H

#include <ccittfax/ccittfax.h>

#include "cf.h"

/* Encode one 2D line relative to ref. Returns 0 on success, 1 on error. */
int
cfc_2d_line(struct cf_buffer_t *dst, const char *coding, const char *ref,
            const struct cf_params_t *params);

#endif /* CCITTFAX_SRC_CFC_2D_H */
