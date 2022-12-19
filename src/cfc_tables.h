/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CFC_TABLES_H
#define CCITTFAX_SRC_CFC_TABLES_H

struct cfc_code_t {
        unsigned value, len;
};

extern const struct cfc_code_t cfc_white_term_rle[64];
extern const struct cfc_code_t cfc_white_makeup_rle[40];

extern const struct cfc_code_t cfc_black_term_rle[64];
extern const struct cfc_code_t cfc_black_makeup_rle[40];

extern const struct cfc_code_t cfc_eol;

#endif /* CCITTFAX_SRC_CFC_TABLES_H */
