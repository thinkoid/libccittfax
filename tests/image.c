/* -*- mode: c; -*- */

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <ccittfax/ccittfax.h>

#include "../src/cf.h"
#include "image.h"

#define UNUSED(x) ((void)x)

static char *to_1bpp(char *src, int w, int h)
{
        int i, j, len;
        char *dst, *pcur;

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

char *load_image(FILE *pf, int *w, int *h)
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
