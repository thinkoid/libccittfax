/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CFC_H
#define CCITTFAX_SRC_CFC_H

#include <ccittfax/defs.h>

#include <stddef.h>

struct cfc_buffer_t
{
        char *buf;
        size_t cap, pos;
};

#endif /* CCITTFAX_SRC_CFC_H */
