/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

#include "../src/cf.h"

struct test_t {
        unsigned passed;
        unsigned failed;
        unsigned warned;
};

static inline void
make_test(struct test_t *t)
{
        t->passed = t->failed = t->warned = 0;
}

static inline void summarize_test(const struct test_t *t)
{
        printf("# Summary:\n");
        printf("#   Passed : %u\n", t->passed);
        printf("#   Failed : %u\n", t->failed);
        printf("#   Warned : %u\n", t->warned);
}

#define TEST_DETAIL(t, expr, line, ...)                         \
    do {                                                        \
        if (!(expr)) {                                          \
                printf("# Test failed, line: %d\n# ", line);    \
                printf(__VA_ARGS__);                            \
                printf("\n\n");                                 \
                ++(t)->failed;                                  \
        } else {                                                \
                ++(t)->passed;                                  \
        }                                                       \
    } while(0)

#define WARN_DETAIL(t, expr, line, ...)                         \
    do {                                                        \
        if (!(expr)) {                                          \
                printf("# Warning, line: %d\n# ", line);        \
                printf(__VA_ARGS__);                            \
                printf("\n\n");                                 \
                ++(t)->warned;                                  \
        }                                                       \
    } while(0)

#define TEST(t, expr, ...) TEST_DETAIL(t, expr, __LINE__, __VA_ARGS__)
#define WARN(t, expr, ...) WARN_DETAIL(t, expr, __LINE__, __VA_ARGS__)

static void
test_make_buffer(struct test_t *test)
{
        struct cf_buffer_t *ptr = cf_make_buffer();

        TEST(test, ptr, "expected cf_make_buffer success");
        TEST(test, ptr->buf, "expected valid cf_buffer_t");
        TEST(test, ptr->cap > 0, "expected non-zero cf_buffer_t capacity");
        TEST(test, ptr->pos == 0, "expected cf_buffer_t position 0");

        if(ptr) {
                if(ptr->buf)
                        free(ptr->buf);
                free(ptr);
        }
}

static void
test_buffer_color(struct test_t *test, const char *buf, size_t beg, size_t end,
                  int color)
{
        for(size_t pos = beg; pos < end; pos++) {
                int actual_color = cf_getbit(buf, pos);
                TEST(test, color == actual_color,
                     "expected bit color %d at position %lu: got %d",
                     color, pos, actual_color);
        }
}

static void
dump_buffer(const char *pbuf, size_t cap)
{
        printf("## buffer dump:\n## ");
        for(size_t i = 0; i < cap; i++) {
                if (i && 0 == i % 16)
                        printf("\n##  ");
                printf(" %02X ", ((unsigned char *)pbuf)[i]);
        }
        printf("\n## \n");
}

static void
do_test_resize_buffer(struct test_t *test, size_t cap, size_t pos,
                      int resized)
{
        struct cf_buffer_t cf_buf = { 0 }, *pcf_buf;

        char *p = malloc(cap);
        if (0 == p)
                goto fail;

        memset(p, 0, cap);

        cf_buf.buf = p;

        cf_buf.cap = cap;
        cf_buf.pos = pos;

        cf_setbits(cf_buf.buf, 0, pos, 1);
        /* dump_buffer(cf_buf.buf, cap); */

        pcf_buf = cf_resize_buffer(&cf_buf);

        TEST(test, pcf_buf && pcf_buf == &cf_buf,
             "expected cf_resize_buffer success");

        TEST(test, resized ^ (pcf_buf->buf == p),
             "expected %s cf_buffer_t::buf", resized ? "new" : "same");

        TEST(test, resized ^ (pcf_buf->cap == cap),
             "expected %s cf_buffer_t::cap", resized ? "increased" : "same");

        TEST(test, pos == pcf_buf->pos,
             "expected cf_buffer_t::pos %lu; got %lu", pos, pcf_buf->pos);

        if(resized) {
                test_buffer_color(test, pcf_buf->buf, 0, pos, 1);
                test_buffer_color(test, pcf_buf->buf, pos, cap << 3, 0);
        }

        free(pcf_buf->buf);

        if (pcf_buf != &cf_buf)
                free(pcf_buf);

        return;

fail:
        TEST(test, 0, "do_test_simple_resize_buffer fail");
}

static void
test_resize_buffer(struct test_t *test)
{
        const size_t max_cap = 64;
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
        struct test_t test;
        make_test(&test);

        test_buffer(&test);

        printf("# Summary:\n");
        printf("# --> tests:  %u\n", test.passed);
        printf("# --> failed: %u\n", test.failed);
        printf("# --> warned: %u\n", test.warned);

        return 0;
}
