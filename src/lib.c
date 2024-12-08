/* -*- mode: c; -*- */

#include <ccittfax/ccittfax.h>

#define STR_(x) #x

const char *version()
{
        return STR_(VERSION);
}
