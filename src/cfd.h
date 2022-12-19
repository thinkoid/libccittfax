/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CFD_H
#define CCITTFAX_SRC_CFD_H

#include <ccittfax/defs.h>

struct cfd_buffer_t
{
        char *buf;
        size_t cap, pos;
};

#endif /* CCITTFAX_SRC_CFD_H */
