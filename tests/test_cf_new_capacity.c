/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

#define X  ((size_t)-1)
#define H  ((X >> 1) + 1)

static void
test_new_capacity(struct test_t *test)
{
        NOTE(("testing cf_new_capacity (fixed cases)"));

        static const struct {
                size_t cap, size, add, expected;
        } arr[] = {
                { 8, 1, 2,  8 },
                { 8, 2, 2,  8 },
                { 8, 3, 2,  8 },
                { 8, 4, 2,  8 },
                { 8, 5, 2,  8 },
                { 8, 6, 2,  8 },
                { 8, 7, 2, 16 },

                { H - 1, H - 1, 1, ((H - 1) << 1) },
                { H,     H - 1, 1,   H },

                { H,     H, H, 0 },

                { H,     H - 1, 2, H + ((X - H) >> 1) },
                { H,     H - 1, 3, H + ((X - H) >> 1) },
                { H,     H - 1, 4, H + ((X - H) >> 1) },

                { X - 8,      X - 8,      1,  (X - 4) },
                { X - 7,      X - 8,      2,  (X - 4) },
                { X - 6,      X - 8,      3,  (X - 3) },
                { X - 5,      X - 8,      4,  (X - 3) },
                { X - 4,      X - 8,      5,  (X - 2) },
                { X - 3,      X - 8,      6,  (X - 2) },
                { X - 2,      X - 8,      7,  (X - 1) },
                { X - 1,      X - 8,      8,  X },
                { X,          X,          1,  0 }
        };

        for(size_t i = 0; i < sizeof arr / sizeof *arr; i++) {
                size_t cap, size, add, expected;

                cap      = arr[i].cap;
                size     = arr[i].size;
                add      = arr[i].add;
                expected = arr[i].expected;

                TEST(test, expected == cf_new_capacity(cap, size, add));
        }
}

int main()
{
        static struct test_t test;
        make_test(&test, "cf_new_capacity internal API test");

        test_new_capacity(&test);

        return !!test.failed;
}
