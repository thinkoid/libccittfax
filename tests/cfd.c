/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

#include "../src/cf.h"
#include "file.h"

int
main(int argc, char **argv)
{
        unsigned char src[256], *psrc = 0;
        size_t n, srclen;

        struct cf_buffer_t *pdst;
        struct cf_params_t params = { 0 };

        psrc = load_file(argv[1], &srclen, &params);
        if (0 == psrc) {
                goto end;
        }

        if (0 == params.columns || 0 == params.rows) {
                fprintf(stderr, "missing data dimensions\n");
                goto end;
        }

        fprintf(stderr, "Input:\n");

        fprintf(stderr, "  k                  : %d\n", params.k);
        fprintf(stderr, "  end_of_line        : %d\n", params.end_of_line);
        fprintf(stderr, "  encoded_byte_align : %d\n", params.encoded_byte_align);
        fprintf(stderr, "  columns            : %d\n", params.columns);
        fprintf(stderr, "  rows               : %d\n", params.rows);
        fprintf(stderr, "  end_of_block       : %d\n", params.end_of_block);
        fprintf(stderr, "  black_is_1         : %d\n", params.black_is_1);
        fprintf(stderr, "  damage_limit       : %d\n", params.damage_limit);

        pdst = cfd(psrc, srclen, &params);
        if (0 == pdst)
                goto err;

        fwrite(&params.columns, sizeof params.columns, 1, stdout);
        fwrite(&params.rows,    sizeof params.rows,    1, stdout);

        fwrite(pdst->buf, (pdst->pos + 7) >> 3, 1, stdout);

        free(pdst->buf);
        free(pdst);
err:
        free(psrc);
end:
        return 0;
}
