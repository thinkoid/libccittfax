/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

static void
test_new_capacity(struct test_t *test)
{
        NOTE(("testing cf_new_capacity (fixed cases)"));

        const size_t X = (size_t)-1;
        const size_t H = (X >> 1) + 1;

        /* zero-cap rule */
        TEST(test, cf_new_capacity(0, 0, 4) == 4);

        /* no-resize when enough room */
        TEST(test, cf_new_capacity(32, 24,  1) == 32);
        TEST(test, cf_new_capacity(32, 24,  2) == 32);
        TEST(test, cf_new_capacity(32, 24,  3) == 32);
        TEST(test, cf_new_capacity(32, 24,  4) == 32);
        TEST(test, cf_new_capacity(32, 24,  5) == 32);
        TEST(test, cf_new_capacity(32, 24,  6) == 32);
        TEST(test, cf_new_capacity(32, 24,  7) == 32);
        TEST(test, cf_new_capacity(32, 24,  8) == 32);

        /* yes-resize when not enough room */
        TEST(test, cf_new_capacity(32, 24,  9) == 64);
        TEST(test, cf_new_capacity(32, 24, 10) == 64);

        TEST(test, cf_new_capacity( 32,  32,  1) ==  64);
        TEST(test, cf_new_capacity( 64,  64,  1) == 128);
        TEST(test, cf_new_capacity(128, 128,  1) == 256);

        /* boundary around H */
        TEST(test, cf_new_capacity(H - 1, H - 1, 1) == (H - 1) << 1);
        TEST(test, cf_new_capacity(H, H, 1) == (H + ((X - H) >> 1)));
        TEST(test, cf_new_capacity(H + H/2, H + H/2, 1) ==
             (H + H / 2) + ((X - H - H/2) >> 1));

        /* boundary around X */
        TEST(test, cf_new_capacity(X - 2, X - 2, 1) == X - 1);
        TEST(test, cf_new_capacity(X - 1, X - 1, 1) == X);

        TEST(test, 0 == cf_new_capacity(X - 1, X - 1, 2));
}

int main()
{
        static struct test_t test;
        make_test(&test, "cf_new_capacity internal API test");

        test_new_capacity(&test);

        return !!test.failed;
}
