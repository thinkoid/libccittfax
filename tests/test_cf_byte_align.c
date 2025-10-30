/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

static void
test_cf_byte_align(struct test_t *test)
{
        struct cf_buffer_t cf_buffer = { 0, 0, 0 };

        char buf[8] = { 0 };

        NOTE(("testing cf_byte_align"));
        cf_buffer.buf = buf;
        cf_buffer.cap = sizeof(buf);

        cf_byte_align(&cf_buffer);
        TEST(test, 0 == cf_buffer.pos);

        for(size_t i = 1; i < sizeof(char) << 3; i++) {
                cf_buffer.pos = i;
                cf_byte_align(&cf_buffer);
                TEST(test, 8 == cf_buffer.pos);
        }

        cf_buffer.pos = 9;
        cf_byte_align(&cf_buffer);
        TEST(test, 16 == cf_buffer.pos);
}

int main()
{
        static struct test_t test;
        make_test(&test, "cf_byte_align internal API test");

        test_cf_byte_align(&test);

        return !!test.failed;
}
