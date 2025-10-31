/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"

#include "./test.h"

static void
test_cf_setbit(struct test_t *test)
{
        NOTE(("testing cf_setbit"));

        char buf[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        unsigned char *pbuf = (unsigned char *)buf;

        cf_setbit(buf, 3, 0);
        TEST(test, 0xEF == pbuf[0]);

        buf[0] = 0xFF;

        cf_setbit(buf, 0, 0);
        TEST(test, 0x7F == pbuf[0]);

        buf[0] = 0xFF;

        cf_setbit(buf, 13, 0);
        TEST(test, 0xFB == pbuf[1]);
}

int main()
{
        static struct test_t test;
        make_test(&test, "cf_setbit internal API test");

        test_cf_setbit(&test);

        return !!test.failed;
}
