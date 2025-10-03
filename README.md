# CCITTFAX

## Synopsis

The library deals with CCITTFAX encoding as detailed in CCITT T.4 [[1]](#1) and
T.6 [[2]](#2) recommendations in the way they matter to processing of PDF images.

## Why

There are codecs embedded in other software, e.g., LibTIFF. For LibTIFF, using
that codec is quite straightforwardly putting together an appropriate TIFF image
header together with the encrypted data and passing it through the LibTIFF API.

I need a codec that is simple enough so it can be used as a C library, as well
as a C++ `boost::iostreams` filter.

## Background

CCITTFAX recommendations deal with both encoding and with the embedding of images in a surrounding stream. Therefore we encounter the concepts of terminal size, number of lines, length of lines in mm, timing of signals, Return to Control (RTC), etc. Also, because it is lacking the concept of bytes (the encoding is a stream of bits) it does not have padding.

PDF specification is a bit different. It takes the standard and trims it to accommodate the PDF paradigms. Because PDF is a byte stream and not a bit stream it mandates:

* unencoded data is treated as a collection of scanlines, each ending on a byte boundary (padding, if necessary),
* encoded data is treated as a continuous bit stream with an option to add padding bits to each scanline.

Also, end of data (EOD) skips to the next byte boundary.

Flags that customize the processing of CCITTFaxDecode decoding are:

* EOL codes are optional (but always accepted and processed in input)
* Encoded Byte Align indicates the presence of padding bits after an encoded scanline
* EndOfBlock pattern indicates whether row processing ends at the special EOB (or RTC) codes or after processing a fixed (parameterized) number of rows

## How

Low-level codec for CCITT Fax (G3 1D, G3 2D, G4) with parameters tailored
for PDF’s CCITTFaxDecode filter. The library operates on bitstreams with
buffer-in / buffer-out semantics.

### Data Structures

```c
struct cf_buffer_t {
        char*  buf;   /* memory buffer */
        size_t cap;   /* total capacity in bytes */
        size_t pos;   /* current bit position */
};

struct cf_params_t {
        int k;
        int end_of_line;
        int encoded_byte_align;
        int columns;
        int rows;
        int end_of_block;
        int black_is_1;
        int damage_limit;
};
```

- `cf_buffer_t.pos` counts **bits**, not bytes.
- Bits are packed **MSB-first** in each byte.

### Parameter Mapping (PDF → cf_params_t)

- **k**
  - `0` → G3 1D (Modified Huffman).
  - `>0` → G3 2D (Modified READ).
  - `<0` → G4 (pure 2D).
- **end_of_line**
  - If true, each scan line is preceded by an EOL code.
  - On decode, EOL is always accepted if present.
- **encoded_byte_align**
  - If true, pad after each line to next byte boundary.
  - Decoder skips padding accordingly.
- **columns** → image width in pixels.
- **rows** → number of scanlines.
- **end_of_block**
  - If true, encoder appends RTC at end (G3 only).
  - If false, end after fixed `rows`.
- **black_is_1** → pixel polarity.
- **damage_limit** → maximum consecutive malformed rows tolerated.

### Bit/Byte Semantics (PDF Constraints)

- Encoded stream = continuous bitstream, MSB-first.
- Per-row byte alignment optional via `encoded_byte_align`.
- End-of-data aligns to next byte boundary.
- Raw (unencoded) scanlines end on byte boundaries, but library core
  handles encoded form.

### Raster Interface Contract

- Input image = bit-packed scanlines, MSB-first per byte.
- Stride = `((columns+7)/8)` bytes per row.
- Polarity per `black_is_1`.
- Output = encoded CCITT bitstream per flags.

### Decoder Behavior

- Accept optional EOL even if disabled.
- Skip padding if `encoded_byte_align`.
- Termination:
  - If `end_of_block==0` and `rows>0`, stop after `rows`.
  - If `end_of_block==1`, stop on RTC (G3).
  - For G4, stop after `rows` or input exhaustion.
- Error recovery: tolerate up to `damage_limit` consecutive bad rows.

### Valid Combinations

- **G3 1D (k==0)**
  - EOL optional, RTC optional, byte-align optional.
- **G3 2D (k>0)**
  - First line 1D, then up to k 2D lines, repeat.
  - Flags same as G3 1D.
- **G4 (k<0)**
  - `end_of_line=0`, `end_of_block=0`.
  - `encoded_byte_align` normally 0, but honored if set.

### Defaults (PDF-Aligned)

- rows = 0 (unspecified)
- k = 0 (G3 1D)
- end_of_line = 0
- encoded_byte_align = 0
- end_of_block = 1 for G3, 0 for G4
- black_is_1 = 0 (0=black)
- damage_limit = 0 (strict)

### Internal Mechanics

- Bitwriter/bitreader for `cf_buffer_t`.
- Huffman tables for MH/MR/MMR.
- State machine with changing elements lists.
- Resync via EOL or scanning, bounded by `damage_limit`.

## References

<a id="1">[1]</a> CCITT ITU-T Recommendation T.4 - Standardization of Group 3 facsimile terminals for document transmission

<a id="2">[2]</a> CCITT ITU-T Recommendation T.6 - Facsimile Coding Schemes And Coding Control Functions For Group 4 Facsimile Apparatus
