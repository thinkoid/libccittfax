/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

#include "../src/cf.h"
#include "../src/cfd_trie.h"
#include "../src/cfc_tables.c"

#define SIZEOF(x) (sizeof(x) / sizeof(*x))

static inline unsigned to_big_endian(unsigned x) {
#if __BIG_ENDIAN__
        return x;
#else
        return  ((x & 0x000000FF) << 24) |
                ((x & 0x0000FF00) <<  8) |
                ((x & 0xFF000000) >> 24) |
                ((x & 0x00FF0000) >>  8);
#endif /* __BIG_ENDIAN__ */
}

static int
get_rle(const char *buf, size_t len, int color)
{
        int c, rle;
        size_t pos;

        struct cfd_trie_state_t state;
        cfd_trie_init(&state, color);

        for (pos = 0; pos < len; ++pos) {
                c = cf_getbit(buf, pos);
                if (!cfd_trie_walk(&state, c))
                        break;

                ++pos;

                if (cfd_trie_is_terminal(&state)) {
                        cfd_trie_get_value(&state, &rle);
                        return rle;
                }
        }

        return -2;
}

static void
test_white_rle(const struct cfc_code_t *code, int i)
{
        union {
                char c[4];
                unsigned int u;
        } buf = { 0 };

        buf.c[0] = code->value;

        if (code->len > 8)
                buf.u = to_big_endian(buf.u) >> (code->len - 8);
        else
                buf.u = to_big_endian(buf.u) << (8 - code->len);
        buf.u = to_big_endian(buf.u);

        int rle = get_rle(buf.c, code->len, 1);
        if (0 > rle) {
                fprintf(stderr,
                        "Error finding rle for %d-th code: 0x%02x(length %u)\n",
                        i, code->value, code->len);
        }
}

static void
test_white()
{
        for (size_t i = 0; i < SIZEOF(cfc_white_term_rle); ++i)
                test_white_rle(cfc_white_term_rle + i, i);
}

int
main(int argc, char **argv)
{
        test_white();
        return 0;
}
