/* -*- mode: c; -*- */

/*
 * mkraw: read an image file (BMP, PNG, etc.) and write raw 1bpp pixel
 * data to stdout, prefixed by width and height as two ints.
 *
 * Output format:
 *   int      width
 *   int      height
 *   char[]   1bpp packed pixels, top-down, rows byte-aligned
 *
 * Usage:
 *   mkraw [file]
 *
 * If no file is given, reads from stdin.
 */

#include <stdio.h>
#include <stdlib.h>

#include "image.h"

int
main(int argc, char **argv)
{
        const char *filename = argc > 1 ? argv[1] : 0;
        int w, h;

        char *buf = load_image(filename, &w, &h);
        if (0 == buf) {
                fprintf(stderr, "mkraw: failed to load image\n");
                return 1;
        }

        fwrite(&w, sizeof w, 1, stdout);
        fwrite(&h, sizeof h, 1, stdout);

        fwrite(buf, h * ((w + 7) / 8), 1, stdout);
        free(buf);

        return 0;
}
