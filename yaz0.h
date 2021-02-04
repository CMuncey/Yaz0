#ifndef Z64YAZ0_H
#define Z64YAZ0_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint32_t yaz0_rabin_karp(uint8_t*, int, int, uint32_t*);
uint32_t yaz0_find_best(uint8_t*, int, int, uint32_t*, uint32_t*, uint32_t*, uint8_t*);
int      yaz0_internal(uint8_t*, int, uint8_t*);
void     yaz0_compress(uint8_t*, int, uint8_t*, int*);
int      yaz0_decompress(uint8_t*, uint8_t**);

#endif
