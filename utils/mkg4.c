/* -*- mode: c; -*- */

/*
 * mkpdf: read raw 1bpp image data (as produced by mkraw) and write a
 * minimal single-page PDF containing a G4-compressed image to stdout.
 *
 * Input format (from mkraw):
 *   int      width
 *   int      height
 *   char[]   1bpp packed pixels, top-down, rows byte-aligned
 *
 * Usage:
 *   mkraw input.bmp | mkpdf > output.pdf
 *   mkpdf input.raw > output.pdf
 */

#include <ccittfax.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *
read_stdin_or_file(const char *filename, size_t *len)
{
        FILE *f;
        char *buf = 0;
        size_t cap = 0, pos = 0, n;

        f = (filename && filename[0]) ? fopen(filename, "rb") : stdin;
        if (!f) {
                fprintf(stderr, "mkpdf: open %s: %s\n",
                        filename, strerror(errno));
                return 0;
        }

        for (;;) {
                if (pos >= cap) {
                        cap = cap ? cap * 2 : 0x4000;
                        char *tmp = realloc(buf, cap);
                        if (!tmp) {
                                fprintf(stderr, "mkpdf: realloc: %s\n",
                                        strerror(errno));
                                goto fail;
                        }
                        buf = tmp;
                }

                n = fread(buf + pos, 1, cap - pos, f);
                if (n == 0) break;
                pos += n;
        }

        if (ferror(f)) {
                fprintf(stderr, "mkpdf: read error\n");
                goto fail;
        }

        if (f != stdin)
                fclose(f);

        *len = pos;
        return buf;

fail:
        free(buf);
        if (f && f != stdin)
                fclose(f);
        return 0;
}

int
main(int argc, char **argv)
{
        const char *filename = argc > 1 ? argv[1] : 0;
        char *raw = 0;
        size_t rawlen = 0;
        int w, h, row_bytes;
        struct cf_params_t params;
        struct cf_buffer_t *enc = 0;
        size_t stream_len;
        long offsets[6], xref_offset;
        char content[256];
        int clen, i;

        raw = read_stdin_or_file(filename, &rawlen);
        if (!raw) return 1;

        if (rawlen < 2 * sizeof(int)) {
                fprintf(stderr, "mkpdf: input too short\n");
                goto fail;
        }

        memcpy(&w, raw,                 sizeof w);
        memcpy(&h, raw + sizeof(int),   sizeof h);

        row_bytes = (w + 7) / 8;

        if (rawlen < 2 * sizeof(int) + (size_t)(row_bytes * h)) {
                fprintf(stderr, "mkpdf: input truncated "
                        "(need %zu bytes, got %zu)\n",
                        2 * sizeof(int) + (size_t)(row_bytes * h), rawlen);
                goto fail;
        }

        fprintf(stderr, "mkpdf: image %d x %d, %d bytes raw\n",
                w, h, row_bytes * h);

        memset(&params, 0, sizeof params);
        params.k            = -1;   /* G4 */
        params.columns      = w;
        params.rows         = h;
        params.end_of_block = 1;

        enc = cfc(raw + 2 * sizeof(int), &params);
        if (!enc) {
                fprintf(stderr, "mkpdf: cfc failed\n");
                goto fail;
        }

        stream_len = (enc->pos + 7) >> 3;
        fprintf(stderr, "mkpdf: encoded %zu bits (%zu bytes), ratio %.2f:1\n",
                enc->pos, stream_len,
                (double)(row_bytes * h) / stream_len);

        /*
         * Minimal PDF structure:
         *   1 0 obj  catalog
         *   2 0 obj  pages
         *   3 0 obj  page
         *   4 0 obj  image XObject (CCITTFaxDecode stream)
         *   5 0 obj  content stream
         */

#define EMIT(...) fprintf(stdout, __VA_ARGS__)
#define TELL()    ftell(stdout)

        EMIT("%%PDF-1.4\n");
        EMIT("%%%c%c%c%c\n", 0xe2, 0xe3, 0xcf, 0xd3);

        offsets[1] = TELL();
        EMIT("1 0 obj\n"
             "<< /Type /Catalog /Pages 2 0 R >>\n"
             "endobj\n");

        offsets[2] = TELL();
        EMIT("2 0 obj\n"
             "<< /Type /Pages /Kids [ 3 0 R ] /Count 1 >>\n"
             "endobj\n");

        offsets[3] = TELL();
        EMIT("3 0 obj\n"
             "<< /Type /Page /Parent 2 0 R\n"
             "   /MediaBox [ 0 0 %d %d ]\n"
             "   /Resources << /XObject << /Im1 4 0 R >> >>\n"
             "   /Contents 5 0 R >>\n"
             "endobj\n", w, h);

        offsets[4] = TELL();
        EMIT("4 0 obj\n"
             "<< /Type /XObject /Subtype /Image\n"
             "   /Width %d /Height %d\n"
             "   /ColorSpace /DeviceGray\n"
             "   /BitsPerComponent 1\n"
             "   /Filter /CCITTFaxDecode\n"
             "   /DecodeParms << /K -1 /Columns %d /Rows %d"
             " /EndOfBlock true >>\n"
             "   /Length %zu >>\n"
             "stream\n",
             w, h, w, h, stream_len);
        fwrite(enc->buf, 1, stream_len, stdout);
        EMIT("\nendstream\nendobj\n");

        clen = snprintf(content, sizeof content,
                        "q %d 0 0 %d 0 0 cm /Im1 Do Q\n", w, h);

        offsets[5] = TELL();
        EMIT("5 0 obj\n"
             "<< /Length %d >>\n"
             "stream\n"
             "%s"
             "endstream\nendobj\n",
             clen, content);

        xref_offset = TELL();
        EMIT("xref\n0 6\n");
        EMIT("0000000000 65535 f \n");
        for (i = 1; i <= 5; ++i)
                EMIT("%010ld 00000 n \n", offsets[i]);

        EMIT("trailer\n"
             "<< /Size 6 /Root 1 0 R >>\n"
             "startxref\n%ld\n"
             "%%%%EOF\n",
             xref_offset);

        free(enc->buf);
        free(enc);
        free(raw);
        return 0;

fail:
        if (enc) { free(enc->buf); free(enc); }
        free(raw);
        return 1;
}
