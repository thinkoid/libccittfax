/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

static void
do_test_buffer_color(struct test_t *test, const char *buf, size_t beg,
                     size_t end, int color)
{
        for(size_t pos = beg; pos < end; pos++) {
                int actual_color = cf_getbit(buf, pos);
                TEST(test, color == actual_color,
                     "expected bit color %d at position %lu: got %d",
                     color, pos, actual_color);
        }
}

static void
test_buffer_color(struct test_t *test,
                  const char *buf, size_t cap, size_t beg, size_t end,
                  int color)
{
        do_test_buffer_color(test, buf,   0, beg, !color);
        do_test_buffer_color(test, buf, beg, end, color);
        do_test_buffer_color(test, buf, end, cap << 3, !color);
}

static void
test_byte_align(struct test_t *test)
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

static void
do_test_cf_setbits(struct test_t *test, int color)
{
        size_t beg, end;

        char buf[8] = { 0 };
        memset(buf, color ? 0 : -1, sizeof buf);

        for(end = 1; end < 13; end++) {
                cf_setbits(buf, 0, end, color);
                test_buffer_color(test, buf, sizeof buf, 0, end, color);
                memset(buf, color ? 0 : -1, sizeof buf);
        }

        for(beg = 0; end < (sizeof(buf) << 3); beg++, end++) {
                cf_setbits(buf, beg, end, color);
                test_buffer_color(test, buf, sizeof buf, beg, end, color);
                memset(buf, color ? 0 : -1, sizeof buf);
        }

        for(; beg < (sizeof(buf) << 3); beg++) {
                cf_setbits(buf, beg, end, color);
                test_buffer_color(test, buf, sizeof buf, beg, end, color);
                memset(buf, color ? 0 : -1, sizeof buf);
        }
}

static void
test_cf_setbits(struct test_t *test)
{
        do_test_cf_setbits(test, 0);
        do_test_cf_setbits(test, 1);
}


int main()
{
        static struct test_t test;
        make_test(&test, "cf_setbits internal API test");

        test_cf_setbits(&test);

        return !!test.failed;
}
