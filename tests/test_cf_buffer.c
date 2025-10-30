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
test_make_buffer(struct test_t *test)
{
        struct cf_buffer_t *cf_buffer = cf_make_buffer();

        NOTE(("testing cf_make_buffer"));

        TEST(test, 0 != cf_buffer);
        TEST(test, 0 != cf_buffer->buf);

        TEST(test, cf_buffer->cap > 0);
        TEST(test, cf_buffer->pos == 0);

        if(cf_buffer) {
                if(cf_buffer->buf)
                        free(cf_buffer->buf);
                free(cf_buffer);
        }
}

static void
do_test_resize_buffer(struct test_t *test, size_t cap, size_t pos,
                      int resized)
{
        struct cf_buffer_t cf_buffer = { 0 }, *resized_cf_buffer;

        char *custom_buffer = malloc(cap);
        if (0 == custom_buffer)
                goto fail;

        memset(custom_buffer, 0, cap);

        cf_buffer.buf = custom_buffer;

        cf_buffer.cap = cap;
        cf_buffer.pos = pos;

        cf_setbits(cf_buffer.buf, 0, pos, 1);
        /* dump_buffer(cf_buffer.buf, cap); */

        resized_cf_buffer = cf_resize_buffer(&cf_buffer);

        TEST(test, resized_cf_buffer && resized_cf_buffer == &cf_buffer);
        TEST(test, resized ^ (resized_cf_buffer->buf == custom_buffer));
        TEST(test, resized ^ (resized_cf_buffer->cap == cap));
        TEST(test, pos == resized_cf_buffer->pos);

        if(resized) {
                test_buffer_color(test, resized_cf_buffer->buf, cap,   0,      pos, 1);
                test_buffer_color(test, resized_cf_buffer->buf, cap, pos, cap << 3, 0);
        }

        free(resized_cf_buffer->buf);

        if (resized_cf_buffer != &cf_buffer)
                free(resized_cf_buffer);

        return;

fail:
        TEST(test, 0 != custom_buffer);
}

static void
test_resize_buffer(struct test_t *test)
{
        const size_t max_cap = 64;

        NOTE(("testing cf_resize_buffer"));
        for(size_t cap = 1; cap < max_cap; cap++) {
                const size_t max_pos = cap << 3;
                for(size_t pos = 0; pos < max_pos; pos++) {
                        const int written = (pos + 7) >> 3;
                        do_test_resize_buffer(
                                test, cap, pos, sizeof(int) > cap - written);
                }
        }
}

static void
test_buffer(struct test_t *test)
{
        test_make_buffer(test);
        test_resize_buffer(test);
}


int main()
{
        static struct test_t test;
        make_test(&test, "struct cf_buffer_t management API test");

        test_buffer(&test);

        return !!test.failed;
}
