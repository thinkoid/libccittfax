/* -*- mode: c; -*- */

#ifndef LIBCCITTFAX_TEST_TEST_H
#define LIBCCITTFAX_TEST_TEST_H

#include <stdio.h>
#include <stdlib.h>

struct test_t {
        unsigned passed, failed, warned;
        const char *name;
};

static struct test_t *g_test = 0;

static inline void
summarize_test(void)
{
        const struct test_t *p = g_test;

        if (0 == p)
                return;

        printf("\n# Summary for %s:\n", p->name);
        printf("#   Passed : %u\n", p->passed);
        printf("#   Failed : %u\n", p->failed);
        printf("#   Warned : %u\n", p->warned);
        printf("#   Total  : %u\n", p->passed + p->failed + p->warned);

        fflush(stdout);
}

static inline void
make_test(struct test_t *p, const char *name)
{
        p->passed = p->failed = p->warned = 0;
        p->name = name;
        g_test = p;
        atexit(summarize_test);
        printf("# Starting test suite: %s\n", name);
}

#define TEST_DETAIL(t, expr, line, fmt, ...)                            \
        do {                                                            \
                if (!(expr)) {                                          \
                        printf("# FAIL [%s:%d]\n#   Expression: %s\n",  \
                               (t)->name, line, #expr);                 \
                        if (fmt && fmt[0]) {                            \
                                printf("#   ");                         \
                                printf(fmt, ##__VA_ARGS__);             \
                                printf("\n");                           \
                        }                                               \
                        printf("\n");                                   \
                        ++(t)->failed;                                  \
                } else {                                                \
                        ++(t)->passed;                                  \
                }                                                       \
        } while(0)

#define WARN_DETAIL(t, expr, line, fmt, ...)                           \
        do {                                                           \
                if (!(expr)) {                                         \
                        printf("# WARN [%s:%d]\n#   Expression: %s\n", \
                               (t)->name, line, #expr);                \
                        if (fmt && fmt[0]) {                           \
                                printf("#   ");                        \
                                printf(fmt, ##__VA_ARGS__);            \
                                printf("\n");                          \
                        }                                              \
                        printf("\n");                                  \
                        ++(t)->warned;                                 \
                }                                                      \
        } while (0)

/* These allow the message to be optional */
#define TEST(t, expr, ...) TEST_DETAIL(t, expr, __LINE__, "" __VA_ARGS__)
#define WARN(t, expr, ...) WARN_DETAIL(t, expr, __LINE__, "" __VA_ARGS__)

#define NOTE(x)                       \
        do {                          \
                printf("\n# Note: "); \
                printf(x);            \
                printf("\n\n");       \
        } while (0)

#define STEP(x)                       \
        do {                          \
                printf("\n# Step: "); \
                printf x;             \
                printf("\n\n");       \
        } while (0)

static inline void
dump_buffer(const char *buf, size_t n)
{
        printf("## buffer dump:\n## ");
        for(size_t i = 0; i < n; i++) {
                if (i && 0 == i % 16)
                        printf("\n## ");
                printf(" %02X ", ((unsigned char *)buf)[i]);
        }
        printf("\n## \n");
}

#endif /* LIBCCITTFAX_TEST_TEST_H */
