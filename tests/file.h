/* -*- mode: c; -*- */

#ifndef CCITTFAX_TESTS_FILE_H
#define CCITTFAX_TESTS_FILE_H

#include <ccittfax/defs.h>

unsigned char *
load_file(const char *filename, size_t *srclen, struct cf_params_t *params);

#endif /* CCITTFAX_TESTS_FILE_H */
