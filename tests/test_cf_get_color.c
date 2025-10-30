/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

static void
test_cf_get_color(struct test_t *test)
{
        NOTE(("testing cf_get_color"));

        {
                /* 10000000b */
                const unsigned char buf[] = { 0x80 };
                TEST(test, cf_get_color((const char *)buf, 0) == 1);
                for(size_t i = 1; i < 8; i++)
                        TEST(test, cf_get_color((const char *)buf, i) == 0);
        }

        {
                /* 00000001b */
                const unsigned char buf[] = { 0x01 };
                TEST(test, cf_get_color((const char *)buf, 7) == 1);
                for(size_t i = 0; i < 7; i++)
                        TEST(test, cf_get_color((const char *)buf, i) == 0);
        }

        {
                /* alternating 10101010b */
                const unsigned char buf[] = { 0xAA };
                for(size_t i = 0; i < 8; i++)
                        TEST(test, cf_get_color((const char *)buf, i) == (i % 2 == 0));
        }

        {
                /* 16 bits of 11111111 00000000 */
                const unsigned char buf[] = { 0xFF, 0x00 };
                for(size_t i = 0; i < 8; i++)
                        TEST(test, cf_get_color((const char *)buf, i) == 1);
                for(size_t i = 8; i < 16; i++)
                        TEST(test, cf_get_color((const char *)buf, i) == 0);
        }
}

int main()
{
        static struct test_t test;
        make_test(&test, "cf_get_color internal API test");

        test_cf_get_color(&test);

        return !!test.failed;
}
