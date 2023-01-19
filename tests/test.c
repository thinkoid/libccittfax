/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

#include "../src/cf.h"

static unsigned char *
do_load_file(FILE *pf, size_t *srclen, struct cf_params_t *params)
{
        unsigned char *pbuf = 0;
        size_t n, cap = 0;

        if (1 != fread(params, sizeof *params, 1, pf)) {
                fprintf(stderr, "failed to read CCITT parameters\n");
                return 0;
        }

        *srclen = 0;

        pbuf = malloc(cap);
        if (0 == pbuf) {
                fprintf(stderr, "malloc : %s\n", strerror(errno));
                return 0;
        }

        while (!feof(pf)) {
                if (*srclen >= cap) {
                        if (0 == cap)
                                cap = 2048;

                        pbuf = realloc(pbuf, cap *= 2);
                        if (0 == pbuf) {
                                perror("realloc");
                                goto err;
                        }
                }

                n = fread(pbuf + *srclen, 1, cap - *srclen, pf);
                *srclen += n;

                if (0 == n && ferror(pf)) {
                        perror("fread");
                        goto err;
                }
        }

        return pbuf;

err:
        free(pbuf);
        return 0;
}

static unsigned char *
load_file(const char *filename, size_t *srclen, struct cf_params_t *params)
{
        unsigned char *pbuf;
        FILE *pf;

        pf = filename && filename[0]
                ? fopen(filename, "rb") : freopen(0, "rb", stdin);

        if (0 == pf) {
                perror("fopen");
                return 0;
        }

        pbuf = do_load_file(pf, srclen, params);

        if (pf != stdin)
                fclose(pf);

        return pbuf;
}

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
        if (0 == pdst) {
                fprintf(stderr, " --> lookie ma\n");
        }

        if (pdst) {
                free(pdst->buf);
                free(pdst);
        }

        free(psrc);

end:
        return 0;
}
