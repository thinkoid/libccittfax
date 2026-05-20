/* -*- mode: c; -*- */

/*
 * mkg4: read raw 1bpp image data (as produced by mkraw) and write a
 * minimal single-page PDF containing a G4-compressed image to stdout.
 *
 * Input format (from mkraw):
 *   int      width
 *   int      height
 *   char[]   1bpp packed pixels, top-down, rows byte-aligned
 *
 * Usage:
 *   mkraw input.bmp | mkg4 > output.pdf
 *   mkg4 input.raw > output.pdf
 */

#include <ccittfax.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/*
 * Byte counter for stdout — ftell(stdout) is unreliable when stdout
 * is a pipe or a non-seekable file.  Track position manually.
 */
static size_t g_pos = 0;

static int
emit(const char *buf, size_t n)
{
        if (fwrite(buf, 1, n, stdout) != n)
                return -1;
        g_pos += n;
        return 0;
}

static int
emitf(const char *fmt, ...)
{
        char buf[4096];
        int n;
        va_list ap;

        va_start(ap, fmt);
        n = vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);

        if (n < 0 || (size_t)n >= sizeof buf)
                return -1;

        return emit(buf, n);
}

static char *
read_stdin_or_file(const char *filename, size_t *len)
{
        FILE *f;
        char *buf = 0;
        size_t cap = 0, pos = 0, n;

        f = (filename && filename[0]) ? fopen(filename, "rb") : stdin;
        if (!f) {
                fprintf(stderr, "mkg4: open %s: %s\n",
                        filename, strerror(errno));
                return 0;
        }

        for (;;) {
                if (pos >= cap) {
                        cap = cap ? cap * 2 : 0x4000;
                        char *tmp = realloc(buf, cap);
                        if (!tmp) {
                                fprintf(stderr, "mkg4: realloc: %s\n",
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
                fprintf(stderr, "mkg4: read error\n");
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
        size_t offsets[6];
        size_t xref_offset;
        char content[256];
        int clen, i;

        raw = read_stdin_or_file(filename, &rawlen);
        if (!raw) return 1;

        if (rawlen < 2 * sizeof(int)) {
                fprintf(stderr, "mkg4: input too short\n");
                goto fail;
        }

        memcpy(&w, raw,               sizeof w);
        memcpy(&h, raw + sizeof(int), sizeof h);

        row_bytes = (w + 7) / 8;

        if (rawlen < 2 * sizeof(int) + (size_t)(row_bytes * h)) {
                fprintf(stderr, "mkg4: input truncated "
                        "(need %zu bytes, got %zu)\n",
                        2 * sizeof(int) + (size_t)(row_bytes * h), rawlen);
                goto fail;
        }

        fprintf(stderr, "mkg4: image %d x %d, %d bytes raw\n",
                w, h, row_bytes * h);

        memset(&params, 0, sizeof params);
        params.k            = -1;   /* G4 */
        params.columns      = w;
        params.rows         = h;
        params.end_of_block = 1;

        enc = cfc(raw + 2 * sizeof(int), &params);
        if (!enc) {
                fprintf(stderr, "mkg4: cfc failed\n");
                goto fail;
        }

        stream_len = (enc->pos + 7) >> 3;
        fprintf(stderr, "mkg4: encoded %zu bits (%zu bytes), ratio %.2f:1\n",
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

        emitf("%%PDF-1.4\n");
        emitf("%%%c%c%c%c\n", 0xe2, 0xe3, 0xcf, 0xd3);

        offsets[1] = g_pos;
        emitf("1 0 obj\n"
              "<< /Type /Catalog /Pages 2 0 R >>\n"
              "endobj\n");

        offsets[2] = g_pos;
        emitf("2 0 obj\n"
              "<< /Type /Pages /Kids [ 3 0 R ] /Count 1 >>\n"
              "endobj\n");

        offsets[3] = g_pos;
        emitf("3 0 obj\n"
              "<< /Type /Page /Parent 2 0 R\n"
              "   /MediaBox [ 0 0 %d %d ]\n"
              "   /Resources << /XObject << /Im1 4 0 R >> >>\n"
              "   /Contents 5 0 R >>\n"
              "endobj\n", w, h);

        offsets[4] = g_pos;
        emitf("4 0 obj\n"
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
        emit(enc->buf, stream_len);
        emitf("\nendstream\nendobj\n");

        clen = snprintf(content, sizeof content,
                        "q %d 0 0 %d 0 0 cm /Im1 Do Q\n", w, h);

        offsets[5] = g_pos;
        emitf("5 0 obj\n"
              "<< /Length %d >>\n"
              "stream\n"
              "%s"
              "endstream\nendobj\n",
              clen, content);

        xref_offset = g_pos;
        emitf("xref\n0 6\n");
        emitf("0000000000 65535 f \n");
        for (i = 1; i <= 5; ++i)
                emitf("%010zu 00000 n \n", offsets[i]);

        emitf("trailer\n"
              "<< /Size 6 /Root 1 0 R >>\n"
              "startxref\n%zu\n"
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
