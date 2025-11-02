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
                perror("malloc");
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

static char *
do_load_image(FILE *pf, int *w, int *h)
{
        int ncomp;
        char *src, *pbuf;

        src = (char *)stbi_load_from_file(pf, w, h, &ncomp, 1);
        if (0 == src) {
                fprintf(stderr, "stbi_load_from_file failed\n");
                return 0;
        }

        fprintf(stderr, " --> loaded : %dx%d : %d\n", *w, *h, ncomp);

        pbuf = to_1bpp(src, *w, *h);
        free(src);

        return pbuf;
}

char *
load_image(const char *filename, int *w, int *h)
{
        char *pbuf;
        FILE *pf;

        pf = filename && filename[0]
                ? fopen(filename, "rb") : freopen(0, "rb", stdin);

        if (0 == pf) {
                perror("fopen");
                return 0;
        }

        pbuf = do_load_image(pf, w, h);

        if (pf != stdin)
                fclose(pf);

        return pbuf;
}
