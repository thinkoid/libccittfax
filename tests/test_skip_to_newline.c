/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>
#include "../src/cf.c"
#include "../src/cfd_g3_1d.c"

#include "./test.h"

static void
test_skip_to_newline_eof(struct test_t *t)
{
        STEP(("EOF case (no 1-bit terminator)"));

        unsigned char buf[2] = { 0x00, 0x00 }; /* 16 zero bits, no following 1 */
        struct cf_buffer_t cf_buf = { (char*)buf, sizeof(buf), 0 };

        skip_to_newline(&cf_buf);

        TEST(t, cf_buf.pos == (cf_buf.cap << 3),
             "Expected position at end of buffer (%zu), got %zu",
             (size_t)(cf_buf.cap << 3), cf_buf.pos);
}

static void
test_skip_to_newline_valid_eol(struct test_t *t)
{
        STEP(("Nominal EOL with 11 zeros + 1 one"));

        /* Bits: 11110000 00000001
         * -> the zero run starts at bit 4, then 11 zeros, then a 1
         */
        unsigned char buf[] = { 0xF0, 0x01 };
        struct cf_buffer_t cf_buf = { (char*)buf, sizeof(buf), 0 };

        cf_buf.pos = 4; /* position just before zero run */

        skip_to_newline(&cf_buf);

        TEST(t, cf_buf.pos == 4,
             "Expected position before EOL (4), got %zu", cf_buf.pos);
}

static void
test_skip_to_newline_long_zero_eol(struct test_t *t)
{
        STEP(("Long-zero EOL (13 zeros + 1 one)"));

        /* 00000000 00010000 = 13 zeros + 1 one (start at bit 0) */
        unsigned char buf[] = { 0x00, 0x10 };
        struct cf_buffer_t cf_buf = { (char*)buf, sizeof(buf), 0 };

        skip_to_newline(&cf_buf);

        TEST(t, cf_buf.pos == 0,
             "Expected position at 0 (start of EOL), got %zu", cf_buf.pos);
}

int
main(void)
{
        static struct test_t t;
        make_test(&t, "skip_to_newline");

        test_skip_to_newline_eof(&t);
        test_skip_to_newline_valid_eol(&t);
        test_skip_to_newline_long_zero_eol(&t);

        return 0;
}
