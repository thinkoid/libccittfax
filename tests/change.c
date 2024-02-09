/* -*- mode: c; -*- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

static inline int
get_color(const char *arr, size_t pos)
{
        /* Convention is: 0 is white, 1 is black */
        return (unsigned char)arr[pos >> 3] & (0x80 >> (pos & 7));
}

static int
find_changing(const char *buf, int pos, int endpos)
{
        int color = 0;

        if (pos < 0)
                pos = 0;
        else
                color = get_color(buf, pos);

        for(; pos < endpos; ++pos) {
                if (color != get_color(buf, pos))
                        return pos;
        }

        return endpos;
}

static void
do_test_beg(unsigned char a, unsigned char b, unsigned char c, unsigned char d,
            int expected)
{
        int a0 = -1;

        const unsigned char buf[] = { a, b, c, d };
        a0 = find_changing((const char *)buf, a0, sizeof(buf) << 3);

        if (a0 != expected) {
                fprintf(stderr, "--> error: expected %d, got %d\n", expected, a0);
        }

}

static void
test_beg()
{
        do_test_beg(   0, 0, 0, 0, 32);
        do_test_beg(0x80, 0, 0, 0, 0);
        do_test_beg(0x40, 0, 0, 0, 1);
        do_test_beg(0x20, 0, 0, 0, 2);
        do_test_beg(0x10, 0, 0, 0, 3);
        do_test_beg(0x08, 0, 0, 0, 4);
        do_test_beg(0x04, 0, 0, 0, 5);
        do_test_beg(0x02, 0, 0, 0, 6);
        do_test_beg(0x01, 0, 0, 0, 7);

        do_test_beg(   0, 0x80, 0, 0, 8);
        do_test_beg(   0, 0x40, 0, 0, 9);
        do_test_beg(   0, 0x20, 0, 0, 10);
        do_test_beg(   0, 0x10, 0, 0, 11);
        do_test_beg(   0, 0x08, 0, 0, 12);
        do_test_beg(   0, 0x04, 0, 0, 13);
        do_test_beg(   0, 0x02, 0, 0, 14);
        do_test_beg(   0, 0x01, 0, 0, 15);

        do_test_beg(   0,    0, 0x80, 0, 16);
        do_test_beg(   0,    0, 0x40, 0, 17);
        do_test_beg(   0,    0, 0x20, 0, 18);
        do_test_beg(   0,    0, 0x10, 0, 19);
}

int main()
{
        test_beg();
        return 0;
}
