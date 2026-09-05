/* -*- mode: c; -*- */

/*
 * Corpus round-trip oracle: encode a generated raster, decode it back, and
 * require the image to survive bit for bit.
 *
 * This is the gate that was missing. test_cfc_g4.c and test_cfd_g4.c pin
 * hand-built streams and code tables, and both stayed green for months while
 * the 2D encoder emitted b1 values that were not changing elements -- a
 * defect no unit test could see, because encoder and decoder shared the bug
 * and agreed with each other. Only a whole-image round trip over varied
 * geometry catches that class of fault.
 *
 * Only the meaningful `columns' bits of each row are compared. The bits
 * padding a row out to a byte boundary carry no image and the two sides are
 * free to disagree about them.
 *
 * The corpus is generated rather than stored: a fixed LCG keeps it identical
 * on every machine and every run without a fixture file to go stale.
 */

#include <stdlib.h>
#include <string.h>

#include <ccittfax/ccittfax.h>

#include "test.h"

/*
 * A private LCG rather than rand(): the corpus must be the same everywhere,
 * and rand()'s sequence is not fixed across implementations.
 */
static unsigned long g_seed;

static void
seed(unsigned long s)
{
        g_seed = s;
}

static int
next_pct(void)
{
        g_seed = g_seed * 6364136223846793005UL + 1442695040888963407UL;
        return (int)((g_seed >> 33) % 100);
}

enum pattern_t {
        PATTERN_CHECKER,
        PATTERN_WHITE,
        PATTERN_BLACK,
        PATTERN_VSTRIPE,
        PATTERN_HSTRIPE
};

struct case_t {
        const char     *name;
        enum pattern_t  pattern;
        int             columns, rows;
        int             block;      /* checkerboard square, in pixels */
        int             noise;      /* percent of pixels flipped      */
};

/*
 * The geometry is the point of the corpus. Widths that are not multiples of
 * eight exercise the row tail, single-pixel blocks and stripes force a
 * changing element at nearly every position, and the solid images are the
 * degenerate reference lines.
 */
static const struct case_t cases[] = {
        { "checker8-clean",   PATTERN_CHECKER,  64,  48,  8,   0 },
        { "checker8-noisy",   PATTERN_CHECKER,  64,  48,  8,  10 },
        { "checker3-noisy",   PATTERN_CHECKER, 127,  53,  3,  25 },
        { "checker1-chaos",   PATTERN_CHECKER,  32,  16,  1,  50 },
        { "checker16-sparse", PATTERN_CHECKER, 200, 100, 16,   5 },
        { "checker4-narrow",  PATTERN_CHECKER,  17,   9,  4,  30 },
        { "checker2-short",   PATTERN_CHECKER,  65,   3,  2,  40 },
        { "checker8-tiny",    PATTERN_CHECKER,   8,   8,  8,   0 },
        { "solid-white",      PATTERN_WHITE,    40,  20,  0,   0 },
        { "solid-black",      PATTERN_BLACK,    40,  20,  0,   0 },
        { "vertical-stripe",  PATTERN_VSTRIPE,  64,  20,  0,   0 },
        { "horizontal-strip", PATTERN_HSTRIPE,  64,  20,  0,   0 },
};

/* 1 is white, matching cf_get_color. */
static int
pixel_of(const struct case_t *c, int x, int y)
{
        int white;

        switch (c->pattern) {
        case PATTERN_WHITE:   return 1;
        case PATTERN_BLACK:   return 0;
        case PATTERN_VSTRIPE: return (x & 1) ? 0 : 1;
        case PATTERN_HSTRIPE: return (y & 1) ? 0 : 1;
        default:              break;
        }

        white = ((x / c->block + y / c->block) & 1) ? 1 : 0;

        if (c->noise && next_pct() < c->noise)
                white = !white;

        return white;
}

static void
set_bit(char *buf, size_t pos, int value)
{
        unsigned char *p = (unsigned char *)buf + (pos >> 3);
        unsigned char  m = (unsigned char)(0x80 >> (pos & 7));

        *p = (unsigned char)(value ? (*p | m) : (*p & ~m));
}

static int
get_bit(const char *buf, size_t pos)
{
        return !!(*((const unsigned char *)buf + (pos >> 3)) &
                  (0x80 >> (pos & 7)));
}

static char *
make_raster(const struct case_t *c, int stride)
{
        char *raster = calloc((size_t)c->rows * stride, 1);
        int   x, y;

        if (0 == raster)
                return 0;

        /* Reseed per image so a case's content never depends on its
         * predecessors -- adding a case must not perturb the others. */
        seed(0x9E3779B97F4A7C15UL ^ (unsigned long)c->columns << 8 ^
             (unsigned long)c->rows);

        for (y = 0; y < c->rows; ++y)
                for (x = 0; x < c->columns; ++x)
                        set_bit(raster, (size_t)y * stride * 8 + x,
                                pixel_of(c, x, y));

        return raster;
}

/*
 * Compare the image proper: `columns' bits per row, ignoring row padding.
 * Returns the bit offset of the first difference, or -1 when they agree.
 */
static long
first_difference(const char *a, const char *b, int columns, int rows,
                 int stride)
{
        int x, y;

        for (y = 0; y < rows; ++y) {
                for (x = 0; x < columns; ++x) {
                        size_t pos = (size_t)y * stride * 8 + x;

                        if (get_bit(a, pos) != get_bit(b, pos))
                                return (long)pos;
                }
        }

        return -1;
}

static void
run_case(struct test_t *t, const struct case_t *c, int k)
{
        struct cf_params_t   params = { 0 };
        struct cf_buffer_t  *enc = 0, *dec = 0;

        const int stride = (c->columns + 7) / 8;
        const size_t rlen = (size_t)c->rows * stride;

        char *raster = make_raster(c, stride);
        long  diff;

        TEST(t, 0 != raster, "%s k=%d: out of memory", c->name, k);
        if (0 == raster)
                return;

        params.k            = k;
        params.columns      = c->columns;
        params.rows         = c->rows;
        params.end_of_block = 1;

        enc = cfc(raster, &params);
        TEST(t, 0 != enc, "%s k=%d: cfc failed", c->name, k);
        if (0 == enc)
                goto done;

        dec = cfd(enc->buf, (enc->pos + 7) >> 3, &params);
        TEST(t, 0 != dec, "%s k=%d: cfd failed", c->name, k);
        if (0 == dec)
                goto done;

        /*
         * A short or long decode is itself the bug's signature: feeding the
         * decoder a malformed 2D stream ran it past the end of the image and
         * produced one byte more than the raster holds.
         */
        TEST(t, ((dec->pos + 7) >> 3) == rlen,
             "%s k=%d: decoded %lu bytes, raster is %lu",
             c->name, k, (unsigned long)((dec->pos + 7) >> 3),
             (unsigned long)rlen);

        diff = first_difference(raster, dec->buf, c->columns, c->rows, stride);
        TEST(t, -1 == diff,
             "%s k=%d: first differing pixel at row %ld col %ld",
             c->name, k, diff / (stride * 8), diff % (stride * 8));

done:
        if (dec)
                cf_free_buffer(dec);
        if (enc)
                cf_free_buffer(enc);
        free(raster);
}

int
main(void)
{
        /* static: summarize_test() runs from atexit(), after main's frame
         * is gone, and g_test still points here. */
        static struct test_t t;

        /*
         * k = -1 G4, k = 0 G3 1D, k = 4 G3 2D. The k > 0 pass pins this
         * library against itself only: with end_of_line = 0 the per-row
         * 1D/2D tag has nowhere to ride, so the stream is not conformant
         * T.4 and other decoders reject it. It still guards the 2D encoder,
         * which is what this file exists for.
         */
        static const int ks[] = { -1, 0, 4 };

        size_t i, j;

        make_test(&t, "test_roundtrip");

        for (i = 0; i < sizeof cases / sizeof *cases; ++i)
                for (j = 0; j < sizeof ks / sizeof *ks; ++j)
                        run_case(&t, &cases[i], ks[j]);

        return t.failed ? 1 : 0;
}
