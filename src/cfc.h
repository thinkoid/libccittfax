/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CFC_H
#define CCITTFAX_SRC_CFC_H

#include <ccittfax/defs.h>

#define UNUSED(x) ((void)(x))

#define SIZE_T_MAX ((size_t)-1)
#define SIZE_T_HALF_MAX ((SIZE_T_MAX >> 1) + 1)

struct cfc_buffer_t
{
        char *buf;
        size_t cap, pos;
};

#endif /* CCITTFAX_SRC_CFC_H */
