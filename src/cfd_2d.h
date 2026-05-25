/* -*- mode: c; -*- */

#ifndef CFD_2D_H
#define CFD_2D_H

#include <ccittfax/ccittfax.h>

#include "cf.h"

/* Decode one 2D line. Returns 0 on success, 1 on error, 2 on EOFB. */
int cfd_2d_line(const char *ref, struct cf_buffer_t *dst,
                struct cf_buffer_t *src, const struct cf_params_t *params);

#endif
