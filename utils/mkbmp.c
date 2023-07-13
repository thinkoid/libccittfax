/* -*- mode: c; -*- */

/**
 *  Load 1bpp image file with a prefix of two integers -- the width and the
 *  height of the image -- and spit out an 8bpp BMP file.
 */

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include <stb/stb_image_write.h>

#define UNUSED(x) ((void)(x))

static inline unsigned char
getbit(unsigned char *p, int off)
{
        return *(p + off / 8) & (0x80 >> (off % 8)) ? 0xFF : 0x00;
}

static unsigned char *
to_8bpp(unsigned char *psrc, int w, int h)
{
        int i, j, len;
        unsigned char *pdst, *pcur;

        len = w * h;

        pdst = malloc(len);
        if (0 == pdst) {
                perror("malloc");
                return 0;
        }

        memset(pdst, 0, len);
        pcur = pdst;

        for (i = 0; i < h; ++i, psrc += (w + 7) / 8) {
                for (j = 0; j < w; ++j, ++pcur) {
                        *pcur = getbit(psrc, j);
                }
        }

        return pdst;
}

static unsigned char *
load_file(const char *filename, int *w, int *h)
{
        unsigned char *pbuf = 0;
        size_t n, nread;

        FILE *pf;

        pf = filename && filename[0]
                ? fopen(filename, "rb") : freopen(0, "rb", stdin);

        if (0 == pf) {
                fprintf(stderr, "input file : %s\n", strerror(errno));
                goto err;
        }

        if (1 != fread(w, sizeof *w, 1, pf) ||
            1 != fread(h, sizeof *h, 1, pf)) {
                fprintf(stderr, "missing header\n");
                goto err;
        }

        fprintf(stderr, "read width %d, height %d\n", *w, *h);
        n = (*w + 7) / 8 * (*h);

        pbuf = malloc(n);
        if (0 == pbuf) {
                fprintf(stderr, "malloc : %s\n", strerror(errno));
                goto end;
        }

        nread = fread(pbuf, 1, n, pf);
        if (n != nread) {
                fprintf(stderr, "input read fail (read %lu, expected %lu)\n",
                        nread, n);
                goto err;
        }

        goto end;

err:
        free(pbuf);
        pbuf = 0;

end:
        if (pf != stdin)
                fclose(pf);

        return pbuf;
}

static void
write_func(void *context, void *data, int size)
{
        UNUSED(context);
        fwrite(data, size, 1, stdout);
}

int main(int argc, char **argv)
{
        UNUSED(argc);
        UNUSED(argv);

        int w, h, ret = 1;
        unsigned char *pbuf = 0, *psrc = 0;

        pbuf = load_file(argv[1], &w, &h);
        if (0 == pbuf)
                goto end;

        psrc = to_8bpp(pbuf, w, h);
        if (0 == psrc)
                goto end;

        stbi_write_bmp_to_func(write_func, 0, w, h, 1, psrc);
        ret = 0;

end:
        free(pbuf);
        free(psrc);

        return ret;
}
