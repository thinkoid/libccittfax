/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

#include "../src/cfc_common.c"
#include "../src/cfd_g3_1d.c"

#include "./test.h"

static inline void
test_cfc_put_rle(struct test_t *t)
{
        size_t pos, rle, rle_ver;
        struct cf_buffer_t cf_buffer = { 0, 0, 0 };

        char buf[256];
        memset(buf, 0, sizeof buf);

        cf_buffer.buf = buf;
        cf_buffer.cap = sizeof buf;

        for(pos = 0; pos < sizeof(unsigned) << 3; pos++) {
                /* shift starting position at each iteration */
                for(rle = 0; rle < 16384; ++rle) {
                        cf_buffer.pos = pos;
                        cfc_put_rle(&cf_buffer, rle, 0);
                        /* dump_buffer(cf_buffer.buf, cf_buffer.cap); */

                        cf_buffer.pos = pos;
                        rle_ver = get_rle(&cf_buffer, 0);

                        TEST(t, rle == rle_ver, "expected rle %lu, got %lu",
                             rle, rle_ver);

                        memset(buf, 0, sizeof buf);
                }
        }
}

int main()
{
        static struct test_t test;
        make_test(&test, "src/cfc_common.c internal API test");

        test_cfc_put_rle(&test);

        return !!test.failed;
}
