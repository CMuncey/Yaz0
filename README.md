# Yaz0 Library

A small library for compressing and decompressing data using Nintendo's Yaz0 algorithm written in C.
I still need to ensure that it will compile on Windows and MacOS, I've only tested on Linux.

To use this the decompression, use `yaz0_decompress()` with the source data, and an <u>unallocated</u> pointer for the decompressed data.
It will find the size using the yaz0 header and allocate the needed space.
It will return the size of the decompressed data as an int.

To use the compression, use `yaz0_compress()` with the source data, the size of the source data, and an <u>allocated</u> chunk of memory for the compressed data to go into.
Ideally, this chunk of memory will be the size of the source + 1 byte for every 8 bytes of source + 16 bytes.
This will allow for even the worst case scenario to be "compressed" and not go out of bounds.
This function will return the actual size of the compressed data after the algorithm has run, rounded up to the nearest multiple of 32 bytes.
