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

## References

<a id="1">[1]</a> CCITT ITU-T Recommendation T.4 - Standardization of Group 3 facsimile terminals for document transmission

<a id="2">[2]</a> CCITT ITU-T Recommendation T.6 - Facsimile Coding Schemes And Coding Control Functions For Group 4 Facsimile Apparatus
