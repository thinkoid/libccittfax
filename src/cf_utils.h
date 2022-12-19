/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_UTILS_H
#define CCITTFAX_SRC_UTILS_H

#include <ccittfax/defs.h>

#include "cf.h"

size_t
new_capacity(size_t cap, size_t size, size_t add);

struct cf_buffer_t *
reserve_buffer(struct cf_buffer_t *dst);

#endif /* CCITTFAX_SRC_UTILS_H */
