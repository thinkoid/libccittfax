/* -*- mode: c; -*- */

#include <stdlib.h>
#include <unistd.h>

#include <ccittfax/ccittfax.h>

#include "image.h"

static void usage()
{
        fprintf(stderr, "usage: program [options]\n");
        fprintf(stderr, "\nOptions\n");
        fprintf(stderr, "    -k number : encoding (negative is Group 4, "
                "0 is Group 3 1D, positive is Group 3 2D)\n");
        fprintf(stderr, "    -l        : EOL codes (default false)\n");
        fprintf(stderr, "    -a        : byte-aligned lines (default false)\n");
        fprintf(stderr, "    -e        : EOB present (default true)\n");
        fprintf(stderr, "    -b        : black value is 1 (default false, black is 0)\n");
        fprintf(stderr, "    -d number : DamagedRowsBeforeError (default k - 1)\n");
        exit(2);
}

static struct cf_params_t *
parse_args(int argc, char **argv, struct cf_params_t *params)
{
        int opt;

        if (0 == params) {
                params = malloc(sizeof *params);
                if (0 == params) {
                        perror("malloc");
                        return 0;
                }
                *params = (struct cf_params_t){ 0, 0, 0, 0, 0, 1, 0, 0 };
        }

        while ((opt = getopt(argc, argv, "k:alebd:")) != -1) {
                switch (opt) {
                case 'k': /* compression type */
                        params->k = atol(optarg);
                        break;

                case 'a': /* EncodedByteAlign indicator */
                        params->encoded_byte_align = 1;
                        break;

                case 'l': /* EOL indicator */
                        params->end_of_line = 1;
                        break;

                case 'e': /* EndOfBlock indicator */
                        params->end_of_block = 1;
                        break;

                case 'b': /* BlackIsOne indicator, default false */
                        params->black_is_1 = 1;
                        break;

                case 'd': /* damage rows before error */
                        params->damage_limit = atol(optarg);
                        break;

                default: /* '?' */
                        usage();
                }
        }

        if (params->end_of_line && params->k >= 0) {
                if (params->damage_limit > params->k - 1) {
                        params->damage_limit = params->k - 1;
                }
        } else {
                params->damage_limit = 0;
        }

        return params;
}

int main(int argc, char **argv)
{
        char *src;
        struct cf_buffer_t *dst;

        struct cf_params_t params = { 0, 0, 0, 1728, 0, 1, 0, 0 };
        parse_args(argc, argv, &params);

        src = load_image(stdin, &params.columns, &params.rows);
        if (0 == src) {
                fprintf(stderr, "stbi_load failed\n");
                return 1;
        }

        dst = cfc(src, &params);
        if (0 == dst) {
                fprintf(stderr, "CCITT compression failed\n");
                free(src);
                return 1;
        }

        fprintf(stderr, " --> encoded : %p : %lu bits (unused: %lu)\n",
                dst->buf, dst->pos, (dst->cap << 3) - dst->pos);

        fwrite(&params, sizeof params, 1, stdout);
        fwrite((unsigned char *)dst->buf, (dst->pos + 7) >> 3, 1, stdout);

        free(dst->buf);
        free(dst);

        free(src);

        return 0;
}
