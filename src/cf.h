/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CF_H
#define CCITTFAX_SRC_CF_H

#include <ccittfax/defs.h>

#include <stddef.h>

struct cf_buffer_t {
        char *buf;
        size_t cap, pos;
};

struct cf_buffer_t *
cf_resize_buffer_least(struct cf_buffer_t *cf_buf, size_t add);

struct cf_buffer_t *
cf_resize_buffer(struct cf_buffer_t *cf_buf);

struct cf_buffer_t *
cf_make_buffer();

#endif /* CCITTFAX_SRC_CF_H */
