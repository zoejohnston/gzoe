/* lzss.h

   Zoe Johnston - 2023/06/25
*/ 

#ifndef LZSS_H
#define LZSS_H

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "assert.h"

/* A sliding window. The chars array stores the characters in the window.
 */
typedef struct {
    uint16_t hash[256];
    uint16_t indices[33026];
    uint8_t chars[33026];

    uint16_t current;
    uint16_t oldest;
} window_t;

void window_init(window_t* window);
uint16_t offset_bits(uint16_t symbol);
uint32_t lzss(uint16_t* storage, window_t* window, uint8_t* contents, uint32_t block_size, uint32_t* bits_used_ptr);

#endif 