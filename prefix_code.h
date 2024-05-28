/* prefix_code.h

   Zoe Johnston - 2023/06/25
*/ 

#ifndef PREFIX_CODE_H
#define PREFIX_CODE_H

#include "stdio.h"
#include "stdint.h"
#include "assert.h"
#include "string.h"

typedef struct {
    uint16_t symbol;
    uint32_t cost;
    int merged;
} item_t;

void get_frequencies(uint32_t* ll_storage, uint32_t* dist_storage, uint16_t* contents, uint32_t size);
void get_cl_frequencies(uint32_t* storage, uint16_t* ll_lengths, uint32_t ll_size, uint16_t* dist_lengths, uint32_t dist_size);
int frequency_analysis(uint32_t* ll_storage, uint32_t* dist_storage);
void construct_canonical_code(uint16_t num_symbols, uint16_t lengths[], uint16_t result_codes[]);
void package_merge(uint8_t max_len, uint16_t num_symbols, uint32_t* frequencies, uint16_t* storage);

#endif 