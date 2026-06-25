/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

static void
test_cf_find_changing(struct test_t *test)
{
        NOTE(("testing cf_find_changing"));

        /* Byte: 0b10110000 = 0xB0
           bit idx: 0 1 2 3 4 5 6 7
                    1 0 1 1 0 0 0 0
         */
        const unsigned char b0[] = { 0xB0 };

        /* pos < 0 -> treated as 0 internally; color inferred at pos */
        TEST(test, cf_find_changing((const char*)b0, -5, 8) == 1); /* 1->0 at bit 1 */

        TEST(test, cf_find_changing((const char*)b0, 0, 8) == 1);  /* 1->0 */
        TEST(test, cf_find_changing((const char*)b0, 1, 8) == 2);  /* 0->1 */
        TEST(test, cf_find_changing((const char*)b0, 2, 8) == 4);  /* 1->0 at 4 */
        TEST(test, cf_find_changing((const char*)b0, 4, 8) == 8);  /* no more -> end */

        /* All zeros: no change */
        const unsigned char z[] = { 0x00, 0x00 };
        TEST(test, cf_find_changing((const char*)z, 0, 16) == 16);
        TEST(test, cf_find_changing((const char*)z, 7, 16) == 16);

        /* All ones: no change */
        const unsigned char o[] = { 0xFF, 0xFF };
        TEST(test, cf_find_changing((const char*)o, 0, 16) == 16);
        TEST(test, cf_find_changing((const char*)o, 9, 16) == 16);

        /* From pos, find where the run starting at pos ends.  edge bit 8 is 0
         * (start of a 1-pixel run); it ends at bit 9 where the colour flips. */
        const unsigned char edge[] = { 0xFF, 0x7F }; /* ... 11111111 01111111 */
        TEST(test, cf_find_changing((const char*)edge, 8, 16) == 9); /* 0->1 at bit 9 */
}

int main()
{
        static struct test_t test;
        make_test(&test, "cf_find_changing internal API test");

        test_cf_find_changing(&test);

        return !!test.failed;
}
