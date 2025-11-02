/* -*- mode: c; -*- */

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <ccittfax/ccittfax.h>

#include "../src/cf.h"
#include "image.h"

static char *to_1bpp(char *src, int w, int h)
{
        int i, j, len;
        char *dst, *pcur;

        /* 1bpp image lines are padded to next byte boundary */
        len = h * ((w + 7) / 8);

        dst = malloc(len);
        if (0 == dst) {
                fprintf(stderr, "to_1bpp malloc failed\n");
                return 0;
        }

        memset(dst, 0, len);
        pcur = dst;

        for (i = 0; i < h; ++i) {
                for (j = 0; j < w; ++j)
                        cf_setbit(pcur, j, *src++);
                pcur += ((w + 7) / 8);
        }

        return dst;
}

#include <stdio.h>
#include <stdlib.h>

static char *
load_file_to_memory(FILE *pf, size_t *srclen)
{
        char *srcbuf = 0;
        size_t len = 0, cap = 0, n;

        if(0 == pf)
                return 0;

        for (;;) {
                if (len >= cap) {
                        cap = cap ? cap * 2 : 0x4000;

                        char *tmp = realloc(srcbuf, cap);
                        if (0 == tmp) {
                                fprintf(stderr, "image buffer realloc failed\n");
                                goto fail;
                        }

                        srcbuf = tmp;
                }

                n = fread(srcbuf + len, 1, cap - len, pf);
                if (0 == n)
                        break;

                len += n;
        }

        if (ferror(pf)) {
                fprintf(stderr, "image buffer read failed\n");
                goto fail;
        }

        if (srclen)
                *srclen = len;

        return srcbuf;

fail:
        if(srcbuf)
                free(srcbuf);

        return 0;
}

static char *
do_load_image(FILE *pf, int *w, int *h)
{
        char *psrc = 0, *pbuf;
        size_t srclen = 0;

        int ncomp;

        psrc = load_file_to_memory(pf, &srclen);
        if (0 == psrc)
                return 0;

        pbuf = (char *)stbi_load_from_memory(
                (unsigned char *)psrc, srclen, w, h, &ncomp, 1);
        if (0 == pbuf) {
                fprintf(stderr, "stbi_load_from_memory failed\n");
                goto fail;
        }

        if (srclen < (size_t)(w[0] * h[0]) >> 3) {
                fprintf(stderr, "incomplete image loaded\n");
                goto fail;
        }

        free(psrc);

        psrc = to_1bpp(pbuf, *w, *h);
        if (0 == psrc)
                goto fail;

        free(pbuf);

        return psrc;

fail:
        free(psrc);
        free(pbuf);

        return 0;
}

char *
load_image(const char *filename, int *w, int *h)
{
        char *pbuf;
        FILE *pf;

        pf = filename && filename[0] ? fopen(filename, "rb") : stdin;
        if (0 == pf) {
                perror("fopen");
                return 0;
        }

        pbuf = do_load_image(pf, w, h);

        if (pf != stdin)
                fclose(pf);

        return pbuf;
}
