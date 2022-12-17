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

## How

CCITTFAX recommendations deal with both encoding and with the embedding of images in a surrounding stream. Therefore we encounter the concepts of terminal size, number of lines, length of lines in mm, timing of signals, Return to Control (RTC), etc. Also, because it is lacking the concept of bytes (the encoding is a stream of bits) it does not have padding.

PDF specification is a bit different. It takes the standard and trims it to accommodate the PDF paradigms. Because PDF is a byte stream and not a bit stream it mandates:

* unencoded data is treated as a collection of scanlines, each ending on a byte boundary (padding, if necessary),
* encoded data is treated as a continuous bit stream with an option to add padding bits to each scanline.

Also, end of data (EOD) skips to the next byte boundary.

Flags that customize the processing of CCITTFaxDecode decoding are:

* EOL codes are optional (but always accepted and processed in input)
* Encoded Byte Align indicates the presence of padding bits after an encoded scanline
* EndOfBlock pattern indicates whether row processing ends at the special EOB (or RTC) codes or after processing a fixed (parameterized) number of rows


## References

<a id="1">[1]</a> CCITT ITU-T Recommendation T.4 - Standardization of Group 3 facsimile terminals for document transmission

<a id="2">[2]</a> CCITT ITU-T Recommendation T.6 - Facsimile Coding Schemes And Coding Control Functions For Group 4 Facsimile Apparatus
