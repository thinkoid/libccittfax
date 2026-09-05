/* -*- mode: c; -*- */

/*
 * Tests for cfc_g4 (T.6 MMR encoder) -- the real ones.
 *
 * Until 4f49362 this file was a verbatim copy of test_cfd_g4.c: it made
 * seven cfd() calls, none to cfc(), and registered under the same "cfd_g4"
 * suite name, so those decoder cases simply ran twice. Nothing in tests/
 * called the encoder at all, which is how a 2D encoder that emitted b1
 * values that were not changing elements shipped into a package with the
 * suite green.
 *
 * Each case pins the exact bitstream cfc() emits for a known raster. The
 * mode codes come from the table at the top of cfc_2d.c:
 *
 *   V(0)   1           H      001         Pass   0001
 *   VR(1)  011         VL(1)  010
 *   VR(2)  000011      VL(2)  000010
 *
 * and the run-length codewords are the ones already hand-verified in
 * test_cfd_g4.c (white 4 = 1011, black 8 = 000101), so the two files agree
 * on the same bits from opposite directions: what the decoder is fed here
 * is what the encoder is expected to produce.
 *
 * Image convention: 1 = white, 0 = black (black_is_1 = 0, the default);
 * the coding line starts white at a0 = -1, and the reference line above
 * row 0 is imaginary all-white. No EOL between rows in G4; EOFB is two
 * 12-bit EOL codewords, appended only when end_of_block is set.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

#include "test.h"

/* ------------------------------------------------------------------ */
/* Bit-packing helper for constructing expected streams                */
/* ------------------------------------------------------------------ */

struct bits_t {
        unsigned char buf[256];
        int           pos;      /* next bit to write */
};

static void
bits_init(struct bits_t *b)
{
        memset(b->buf, 0, sizeof b->buf);
        b->pos = 0;
}

/* Append `len' MSB-first bits from `val'. */
static void
bits_put(struct bits_t *b, unsigned val, int len)
{
        int i;

        for (i = len - 1; i >= 0; --i) {
                if ((val >> i) & 1)
                        b->buf[b->pos >> 3] |= 0x80 >> (b->pos & 7);
                ++b->pos;
        }
}

/* The mode codes, spelled once. */
static void put_v0(struct bits_t *b)   { bits_put(b, 0x1, 1); }
static void put_vr1(struct bits_t *b)  { bits_put(b, 0x3, 3); }
static void put_vl2(struct bits_t *b)  { bits_put(b, 0x2, 6); }
static void put_h(struct bits_t *b)    { bits_put(b, 0x1, 3); }
static void put_pass(struct bits_t *b) { bits_put(b, 0x1, 4); }

/* Terminating run codewords used below, from test_cfd_g4.c. */
static void put_white4(struct bits_t *b) { bits_put(b, 0xB,  4); }
static void put_black8(struct bits_t *b) { bits_put(b, 0x05, 6); }

/* EOFB: two 12-bit EOL codewords. */
static void
put_eofb(struct bits_t *b)
{
        bits_put(b, 0x1, 12);
        bits_put(b, 0x1, 12);
}

/* ------------------------------------------------------------------ */
/* Raster helpers                                                      */
/* ------------------------------------------------------------------ */

/*
 * Write one row from a picture of it: 'W' white, 'B' black, one character
 * per column. Spelling the rows out keeps each case readable as an image
 * rather than as hexadecimal.
 */
static void
row_from_string(char *raster, int row, int stride, const char *s)
{
        size_t x;

        for (x = 0; s[x]; ++x) {
                size_t         pos = (size_t)row * stride * 8 + x;
                unsigned char *p   = (unsigned char *)raster + (pos >> 3);
                unsigned char  m   = (unsigned char)(0x80 >> (pos & 7));

                *p = (unsigned char)('W' == s[x] ? (*p | m) : (*p & ~m));
        }
}

static struct cf_params_t
make_params(int columns, int rows, int end_of_block)
{
        struct cf_params_t p;

        memset(&p, 0, sizeof p);

        p.k            = -1;    /* G4 */
        p.columns      = columns;
        p.rows         = rows;
        p.end_of_block = end_of_block;

        return p;
}

/*
 * Encode `rows' spelled-out rows and require the emitted stream to equal
 * `expect' exactly -- same bit count, same bits.
 */
static void
check_encoding(struct test_t *t, const char *what,
               const char *const *rows, int nrows, int columns,
               int end_of_block, const struct bits_t *expect)
{
        struct cf_params_t  params = make_params(columns, nrows, end_of_block);
        struct cf_buffer_t *enc;

        const int stride = (columns + 7) / 8;

        char *raster = calloc((size_t)nrows * stride, 1);
        int   i;

        TEST(t, 0 != raster, "%s: out of memory", what);
        if (0 == raster)
                return;

        for (i = 0; i < nrows; ++i)
                row_from_string(raster, i, stride, rows[i]);

        enc = cfc(raster, &params);

        TEST(t, 0 != enc, "%s: cfc returned non-null", what);
        if (0 == enc) {
                free(raster);
                return;
        }

        TEST(t, (int)enc->pos == expect->pos,
             "%s: emitted %d bits, expected %d",
             what, (int)enc->pos, expect->pos);

        if ((int)enc->pos == expect->pos) {
                int n = (expect->pos + 7) >> 3;

                TEST(t, 0 == memcmp(enc->buf, expect->buf, (size_t)n),
                     "%s: bitstream matches", what);

                if (0 != memcmp(enc->buf, expect->buf, (size_t)n)) {
                        printf("## emitted:\n");
                        dump_buffer(enc->buf, (size_t)n);
                        printf("## expected:\n");
                        dump_buffer((const char *)expect->buf, (size_t)n);
                }
        }

        cf_free_buffer(enc);
        free(raster);
}

/* The reference row every two-row case below codes against. */
#define REF_ROW "WWWWBBBBBBBBWWWW"

/*
 * Row 0 of those cases, coded against the imaginary all-white line:
 * a1 = 4 and b1 = 16, so |a1 - b1| is far past vertical range and Pass
 * does not apply -- H, then the trailing white run closes with V(0).
 */
static void
put_ref_row(struct bits_t *b)
{
        put_h(b);
        put_white4(b);
        put_black8(b);
        put_v0(b);
}

/* ------------------------------------------------------------------ */
/* Cases                                                               */
/* ------------------------------------------------------------------ */

/*
 * An all-white image against an all-white reference: every row has no
 * changing element, so a1 = b1 = columns and each row is a single V(0).
 */
static void
test_all_white_image(struct test_t *t)
{
        static const char *const rows[] = {
                "WWWWWWWWWWWWWWWW", "WWWWWWWWWWWWWWWW",
                "WWWWWWWWWWWWWWWW", "WWWWWWWWWWWWWWWW"
        };

        struct bits_t expect;
        int i;

        NOTE(("test_all_white_image: 16x4 all white, one V(0) per row"));

        bits_init(&expect);
        for (i = 0; i < 4; ++i)
                put_v0(&expect);

        check_encoding(t, "16x4 all-white", rows, 4, 16, 0, &expect);
}

/*
 * Horizontal mode on the first row, which is the only mode that can start
 * an image whose first changing element is far from the imaginary line.
 */
static void
test_horizontal_mode(struct test_t *t)
{
        static const char *const rows[] = { REF_ROW };

        struct bits_t expect;

        NOTE(("test_horizontal_mode: W4 B8 W4 as H + runs, closed by V(0)"));

        bits_init(&expect);
        put_ref_row(&expect);

        check_encoding(t, "16x1 H mode", rows, 1, 16, 0, &expect);
}

/*
 * Vertical mode: row 1's first changing element sits one to the right of
 * the reference's, which is VR(1); the remaining two align exactly.
 */
static void
test_vertical_mode(struct test_t *t)
{
        static const char *const rows[] = { REF_ROW, "WWWWWBBBBBBBWWWW" };

        struct bits_t expect;

        NOTE(("test_vertical_mode: VR(1) then two V(0)"));

        bits_init(&expect);
        put_ref_row(&expect);
        put_vr1(&expect);
        put_v0(&expect);
        put_v0(&expect);

        check_encoding(t, "16x2 VR(1)", rows, 2, 16, 0, &expect);
}

/*
 * Pass mode: the reference's black run ends (b2 = 12) before the coding
 * line's next changing element (a1 = 16), so the run is passed over.
 */
static void
test_pass_mode(struct test_t *t)
{
        static const char *const rows[] = { REF_ROW, "WWWWWWWWWWWWWWWW" };

        struct bits_t expect;

        NOTE(("test_pass_mode: white row over a black run passes it"));

        bits_init(&expect);
        put_ref_row(&expect);
        put_pass(&expect);
        put_v0(&expect);

        check_encoding(t, "16x2 Pass", rows, 2, 16, 0, &expect);
}

/*
 * Regression for the encoder half of the find_b1 defect (4f49362), the
 * mirror of test_cfd_g4.c's test_find_b1_changing_element.
 *
 * Coding row 1's final V(0) calls find_b1 with a0 = 10 and coding colour
 * white. ref[11] is black -- the opposite colour -- but it is mid-run: the
 * black run began at 4, left of a0, so it has no changing element here and
 * b1 must be 16, not 11.
 *
 * With b1 = 11 the encoder computes b2 = 12 < a1 = 16 and emits Pass where
 * V(0) belongs, so the stream decodes to the wrong image. This case fails
 * on the old encoder and passes on the fixed one.
 */
static void
test_b1_must_be_changing_element(struct test_t *t)
{
        static const char *const rows[] = { REF_ROW, "WWWWBBBBBBWWWWWW" };

        struct bits_t expect;

        NOTE(("test_b1_must_be_changing_element: b1 skips a mid-run pixel"));

        bits_init(&expect);
        put_ref_row(&expect);
        put_v0(&expect);
        put_vl2(&expect);
        put_v0(&expect);

        check_encoding(t, "16x2 b1 changing element", rows, 2, 16, 0, &expect);
}

/*
 * end_of_block appends EOFB and nothing else -- G4 emits no EOL between
 * rows, so the row coding is byte-for-byte what it was without it.
 */
static void
test_eofb_appended(struct test_t *t)
{
        static const char *const rows[] = { "WWWWWWWWWWWWWWWW" };

        struct bits_t expect;

        NOTE(("test_eofb_appended: V(0) then two 12-bit EOL codewords"));

        bits_init(&expect);
        put_v0(&expect);
        put_eofb(&expect);

        check_encoding(t, "16x1 with EOFB", rows, 1, 16, 1, &expect);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int
main(void)
{
        /* static: summarize_test() runs from atexit(), after main's frame
         * is gone, and g_test still points here. */
        static struct test_t t;

        make_test(&t, "cfc_g4");

        test_all_white_image(&t);
        test_horizontal_mode(&t);
        test_vertical_mode(&t);
        test_pass_mode(&t);
        test_b1_must_be_changing_element(&t);
        test_eofb_appended(&t);

        return t.failed ? 1 : 0;
}
