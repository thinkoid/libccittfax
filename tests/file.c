/* -*- mode: c; -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ccittfax/ccittfax.h>

#include "file.h"
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

unsigned char *
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
