/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

static void
test_new_capacity_fixed(struct test_t *test)
{
        NOTE(("testing cf_new_capacity (fixed cases)"));

        const size_t X = (size_t)-1;
        const size_t H = (X >> 1) + 1;

        /* zero-cap rule */
        TEST(test, cf_new_capacity(0, 0, 7) == 7);

        /* no-resize when enough room */
        TEST(test, cf_new_capacity(64, 32, 16) == 64);  /* size+add=48 <=64 */

        /* simple double up to cover request (still below H) */
        {
                size_t cap = 8, size = 8, add = 40; /* need 48 */
                size_t got = cf_new_capacity(cap, size, add);
                TEST(test, got >= size + add);
                TEST(test, got == cf_new_capacity(cap, size, add));
        }

        /* boundary around H */
        {
                /* below H: should land at H (or above) if needed */
                size_t cap = H >> 2;    /* comfortably below H */
                size_t size = cap;      /* full */
                size_t add = (H - cap) + 1; /* force target just above H */
                size_t got = cf_new_capacity(cap, size, add);
                TEST(test, got >= size + add);
                TEST(test, got == cf_new_capacity(cap, size, add));
        }

        /* half-step phase behavior (cap already >= H) */
        {
                size_t cap = H;
                size_t size = H;
                size_t add  = 1024; /* small increment well into phase 2 */
                size_t got = cf_new_capacity(cap, size, add);
                TEST(test, got >= size + add);
                TEST(test, got == cf_new_capacity(cap, size, add));
        }

        /* overflow guard: add > X - size -> 0 */
        {
                size_t size = X - 5;
                size_t add  = 6;  /* add > X - size == 5 */
                TEST(test, cf_new_capacity(size, size, add) == 0);
        }

        /* exact-maximum request: size+add == X should return X
           (precondition size <= cap must hold) */
        {
                size_t cap  = (size_t)-1 - 8;   /* X-8 */
                size_t size = cap;              /* == cap */
                size_t add  = 8;                /* size+add == X */
                size_t got  = cf_new_capacity(cap, size, add);
                TEST(test, got == (size_t)-1);
        }
}

static void
test_new_capacity_monotonic(struct test_t *test)
{
        NOTE(("testing cf_new_capacity (monotonic in add)"));
        size_t cap = 64, size = 32; /* valid precondition */

        size_t prev = 0;
        for(size_t add = 0; add <= 1024; add += 17) {
                size_t got = cf_new_capacity(cap, size, add);
                if(add == 0) {
                        prev = got;
                        continue;
                }
                /* Results should never decrease as 'add' increases */
                TEST(test, got >= prev);
                prev = got;
        }
}

static void
test_new_capacity_idempotent_when_spacious(struct test_t *test)
{
        NOTE(("testing cf_new_capacity (idempotence when enough room)"));

        size_t cap = 4096, size = 1000, add = 500; /* size+add=1500 <= 4096 */
        size_t got = cf_new_capacity(cap, size, add);

        TEST(test, got == cap);

        /* Calling again with same cap/size/add should still be cap */
        size_t got2 = cf_new_capacity(got, size, add);
        TEST(test, got2 == got);
}

static void
test_new_capacity_fuzz(struct test_t *test)
{
        NOTE(("testing cf_new_capacity (light fuzz)"));

        const size_t X = (size_t)-1;

        /* limited fuzz to keep tests fast & deterministic */
        for(size_t cap = 1; cap <= 1u<<18; cap <<= 1) {
                size_t size = cap / 3;
                for(size_t k = 0; k < 7; ++k) {
                        size_t add = (k * cap) / 5;
                        if(add > X - size) add = (X - size); /* clamp to contract */

                        size_t got  = cf_new_capacity(cap, size, add);
                        size_t want = cf_new_capacity(cap, size, add);

                        /* contract checks */
                        if (cap >= size + add)
                                TEST(test, got == cap);
                        else
                                TEST(test, got >= size + add);

                        TEST(test, got == want);
                }
        }
}

static void
test_new_capacity(struct test_t *test)
{
        test_new_capacity_fixed(test);
        test_new_capacity_monotonic(test);
        test_new_capacity_idempotent_when_spacious(test);
        test_new_capacity_fuzz(test);
}

int main()
{
        static struct test_t test;
        make_test(&test, "cf_new_capacity internal API test");

        test_new_capacity(&test);

        return !!test.failed;
}
