/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CF_H
#define CCITTFAX_SRC_CF_H

#include <ccittfax/defs.h>

#include <stddef.h>

struct cf_buffer_t
{
        char *buf;
        size_t cap, pos;
};

struct cf_buffer_t *
resize_cf_buffer(struct cf_buffer_t *dst);

struct cf_buffer_t *
make_cf_buffer();

#endif /* CCITTFAX_SRC_CF_H */
