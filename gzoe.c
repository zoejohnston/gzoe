/* gzoe.c

   Based on starter code by Bill Bird.
   Oversees collection of input into blocks and block writing.

   Zoe Johnston - 2023/06/25
*/

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "stdint.h"
#include "output_stream.h"
#include "lzss.h"
#include "prefix_code.h"
#include "CRC_for_C.h"

#define MAX_BLOCK_SIZE (1<<16) - 1

/* Global Variables */

uint16_t ll_code_table[2][288];
uint16_t dist_code_table[2][32];

/* Function Declaration */

/* From Bill Bird's starter code. Pushes a basic gzip header. 
 */
void push_gzip_header(bitstream_t* stream) {
    unsigned char initial_bytes[] = {0x1f, 0x8b,
        0x08,
        0x00, 
        0x00, 0x00, 0x00, 0x00,
        0x00,
        0x03 
    };

    for(unsigned int i = 0; i < 10; i++)
        bitstream_push_byte(stream, initial_bytes[i]);
}

/* Sets up the global code tables used for block type 1, storing them in a global variable.
 */
void setup_default_code_tables() {
    unsigned int i; 

    for (i = 0; i < 144; i++) 
        ll_code_table[1][i] = 8;
    for (i = 144; i < 256; i++) 
        ll_code_table[1][i] = 9;
    for (i = 256; i < 280; i++) 
        ll_code_table[1][i] = 7;
    for (i = 280; i < 288; i++) 
        ll_code_table[1][i] = 8;
    for (i = 0; i < 32; i++) 
        dist_code_table[1][i] = 5;

    construct_canonical_code(288, ll_code_table[1], ll_code_table[0]);
    construct_canonical_code(32, dist_code_table[1], dist_code_table[0]);
}

/* Counts the number of code lengths which are not trailing zeros, assuming that the first offset amount of them
 * are not trailing zeros.
 */
uint16_t count_codes(uint16_t code_lengths[], uint16_t num_codes, uint16_t offset) {
    uint16_t count = 0, num = 0;

    for (unsigned int i = offset; i < num_codes; i++) {
        count++;
        
        if (code_lengths[i] != 0)
            num = count;
    }

    return num + offset;
}

/* Counts the number of code lengths which are not trailing zeros, assuming that the first offset amount of them
 * are not trailing zeros. Applies a permutation as this is done.
 */
uint16_t count_cl_codes(uint16_t code_lengths[], uint16_t num_codes, uint16_t permutation[], uint16_t offset) {
    uint16_t count = 0, num = 0;

    for (unsigned int i = offset; i < num_codes; i++) {
        count++;
        
        if (code_lengths[permutation[i]] != 0)
            num = count;
    }

    return num + offset;
}

/* Applies RLE to code lengths using CL symbols 16, 17, and 18
 */
uint16_t rle(uint16_t num_codes, uint16_t code_lengths[], uint16_t storage[]) {
    uint16_t i = 0, j = 0, length, current;

    while (i < num_codes) {
        current = code_lengths[i];

        // Checks for a run of three zeros
        if (current == 0 && i + 2 < num_codes && code_lengths[i + 1] == 0 && code_lengths[i + 2] == 0) {
            length = 3;
            
            while (length + i < num_codes) {
                if (code_lengths[length + i] != 0)
                    break;

                if (length == 138)
                    break;

                length++;
            }
            
            if (length < 11) {
                storage[j] = 17;
                storage[j + 1] = length - 3;
            } else {
                storage[j] = 18;
                storage[j + 1] = length - 11;
            }

            j += 2;
            i += length;

        // Checks for a run of three of the current character
        } else if (i + 3 < num_codes && current == code_lengths[i + 1] 
        && current == code_lengths[i + 2] && current == code_lengths[i + 3]) {
            storage[j] = current;
            length = 3;
            
            while (length + i + 1 < num_codes) {
                if (code_lengths[length + i + 1] != current)
                    break;

                if (length == 6)
                    break;

                length++;
            }

            storage[j + 1] = 16;
            storage[j + 2] = length - 3;

            j += 3;
            i += length + 1;
        
        } else {
            storage[j] = current;

            j++;
            i++;
        }
    }

    return j;
}

/* Writes the code length data.
 */
void write_cl_data(bitstream_t* stream, uint16_t ll_code_lengths[], uint16_t num_ll_codes, uint16_t dist_code_lengths[], uint16_t num_dist_codes) {
    // Cutting off ending runs of zeros
    num_ll_codes = count_codes(ll_code_lengths, num_ll_codes, 257);
    num_dist_codes = count_codes(dist_code_lengths, num_dist_codes, 1);

    unsigned int HLIT = num_ll_codes - 257;
    unsigned int HDIST = num_dist_codes - 1;
    
    bitstream_push_bits(stream, HLIT,  5);
    bitstream_push_bits(stream, HDIST, 5);

    // Applying RLE using CL Symbols 16, 17, and 18
    uint16_t new_ll_code_lengths[286] = {0};
    uint16_t new_dist_code_lengths[30] = {0};
    num_ll_codes = rle(num_ll_codes, ll_code_lengths, new_ll_code_lengths);
    num_dist_codes = rle(num_dist_codes, dist_code_lengths, new_dist_code_lengths);

    // Computing CL code
    uint32_t frequencies[19] = {0};
    get_cl_frequencies(frequencies, new_ll_code_lengths, num_ll_codes, new_dist_code_lengths, num_dist_codes);
    
    uint16_t cl_code[19];
    uint16_t cl_code_lengths[19] = {0};
    uint16_t cl_permutation[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    package_merge(7, 19, frequencies, cl_code_lengths);
    construct_canonical_code(19, cl_code_lengths, cl_code);

    // Cutting off ending runs of zeros
    uint16_t num_cl_codes = count_cl_codes(cl_code_lengths, 19, cl_permutation, 4);
    unsigned int HCLEN = num_cl_codes - 4; 
    bitstream_push_bits(stream, HCLEN, 4);

    // Pushing CL code
    for (unsigned int i = 0; i < num_cl_codes; i++)
        bitstream_push_bits(stream, cl_code_lengths[cl_permutation[i]], 3);
    
    uint16_t bits, code, len, offset;

    // Pushing encoded ll code
    // See rle to see how offsets are stored
    for (unsigned int k = 0; k < num_ll_codes; k++) {
        len = new_ll_code_lengths[k];
        bits = cl_code_lengths[len];
        code = cl_code[len];
        bitstream_push_encoding(stream, code, bits);

        if (len > 15) {
            k++;
            offset = new_ll_code_lengths[k];
        }

        if (len == 16) {
            bitstream_push_bits(stream, offset, 2);
        } else if (len == 17) {
            bitstream_push_bits(stream, offset, 3);
        } else if (len == 18) {
            bitstream_push_bits(stream, offset, 7);
        }
    }

    // Pushing encoded distance code
    if (num_dist_codes == 0) {
        bitstream_push_bits(stream,0,5);

    } else {
        for (unsigned int k = 0; k < num_dist_codes; k++) {
            len = new_dist_code_lengths[k];
            bits = cl_code_lengths[len];
            code = cl_code[len];
            bitstream_push_encoding(stream, code, bits);

            if (len > 15) {
                k++;
                offset = new_dist_code_lengths[k];
            }

            if (len == 16) {
                bitstream_push_bits(stream, offset, 2);
            } else if (len == 17) {
                bitstream_push_bits(stream, offset, 3);
            } else if (len == 18) {
                bitstream_push_bits(stream, offset, 7);
            }
        }
    }
}

/* Pushes a block of type 2.
 */
void block_2(bitstream_t* stream, uint16_t* contents, uint32_t block_size, uint32_t* ll_frequencies, uint32_t* dist_frequencies){
    uint16_t ll_code_lengths[286] = {0}, ll_code[286];
    uint16_t dist_code_lengths[30] = {0}, dist_code[30];

    bitstream_push_bits(stream, 2, 2); 

    package_merge(15, 286, ll_frequencies, ll_code_lengths);
    package_merge(15, 30, dist_frequencies, dist_code_lengths);

    construct_canonical_code(286, ll_code_lengths, ll_code);
    construct_canonical_code(30, dist_code_lengths, dist_code);

    write_cl_data(stream, ll_code_lengths, 286, dist_code_lengths, 30);

    uint16_t bits;
    uint16_t code;
    uint32_t index = 0;
    uint16_t current_symbol, offset;

    while (index < block_size) {
        current_symbol = contents[index];
        index++;

        bits = ll_code_lengths[current_symbol];
        code = ll_code[current_symbol];
        bitstream_push_encoding(stream, code, bits);

        // See lzss in lzss.c to see how offsets and distance symbols are stored
        if (current_symbol > 256) {
            assert(index + 1 < MAX_BLOCK_SIZE);
            offset = contents[index];
            index++;

            bitstream_push_bits(stream, offset >> 5, offset_bits(current_symbol));

            current_symbol = offset & 31;
            assert(current_symbol < 30);

            bits = dist_code_lengths[current_symbol];
            code = dist_code[current_symbol];
            bitstream_push_encoding(stream, code, bits);

            offset = contents[index];
            index++;
            
            bitstream_push_bits(stream, offset, offset_bits(current_symbol));
        }
    }

    bits = ll_code_lengths[256];
    code = ll_code[256];
    bitstream_push_encoding(stream, code, bits);

}

/* Pushes a block of type 1.
 */
void block_1(bitstream_t* stream, uint16_t* contents, uint32_t block_size) {
    uint16_t current_symbol, offset;
    uint32_t bits, code, index = 0;

    bitstream_push_bits(stream, 1, 2);

    while (index < block_size) {
        current_symbol = contents[index];
        index++;

        bits = ll_code_table[1][current_symbol];
        code = ll_code_table[0][current_symbol];
        bitstream_push_encoding(stream, code, bits); 

        // See lzss in lzss.c to see how offsets and distance symbols are stored
        if (current_symbol > 256) {
            offset = contents[index];
            index++;
            bitstream_push_bits(stream, offset >> 5, offset_bits(current_symbol));

            current_symbol = offset & 31;
            assert(current_symbol < 32);

            bits = dist_code_table[1][current_symbol];
            code = dist_code_table[0][current_symbol];
            bitstream_push_encoding(stream, code, bits);

            assert(index < MAX_BLOCK_SIZE);
            offset = contents[index];
            index++;
            
            bitstream_push_bits(stream, offset, offset_bits(current_symbol));
        }
    } 
    
    bits = ll_code_table[1][256];
    code = ll_code_table[0][256];
    bitstream_push_encoding(stream, code, bits); 
}

/* From Bill Bird's starter code. Pushes a block of type 0. 
 */
uint32_t block_0(bitstream_t* stream, uint8_t* contents, uint32_t block_size) {
    bitstream_push_bits(stream, 0, 2);
    bitstream_push_bits(stream, 0, 5); 
    bitstream_push_u16(stream, block_size);
    bitstream_push_u16(stream, ~block_size);

    for(unsigned int i = 0; i < block_size; i++)
        bitstream_push_byte(stream, contents[i]); 

    return 0;
}

/* Preforms LZSS, then checks if the LZSS will make an improvement. If it won't, uses block type 0. If it will, then a 
 * freqeuncy count is performed. The results of the frequency count are then used to decide if using block type 2 will be
 * advantageous. 
 */
uint32_t write_block(bitstream_t* stream, window_t* window, uint8_t* contents, uint32_t block_size) {
    uint32_t bits_used;
    uint16_t post_lzss_contents[MAX_BLOCK_SIZE];
    uint32_t post_lzss_size = lzss(post_lzss_contents, window, contents, block_size, &bits_used);

    if (bits_used > (block_size * 8) + 40) {
        block_0(stream, contents, block_size);
        return 0;
    }

    uint32_t ll_frequencies[288] = {0};
    uint32_t dist_frequencies[32] = {0};
    get_frequencies(ll_frequencies, dist_frequencies, post_lzss_contents, post_lzss_size);

    if (frequency_analysis(ll_frequencies, dist_frequencies) == 1) {
        block_1(stream, post_lzss_contents, post_lzss_size);
        return 0;
    }

    block_2(stream, post_lzss_contents, post_lzss_size, ll_frequencies, dist_frequencies);
    return 0;
}

/* Initializes the bitstream, default code tables, and sliding window. Collects a block of size MAX_BLOCK_SIZE
 * and then pushes that block.
 */
int main() {
    bitstream_t stream;
    bitstream_init(&stream, stdout);

    push_gzip_header(&stream);
    setup_default_code_tables();

    uint8_t block_contents[MAX_BLOCK_SIZE];
    
    window_t window;
    window_init(&window);

    uint32_t block_size = 0;
    uint32_t bytes_read = 0;

    int next_byte;
    uint32_t crc = 0;

    if ((next_byte = fgetc(stdin)) == EOF) {
        
    } else {
        bytes_read++;
        crc = crc_init((unsigned char) next_byte);

        while(1){
            block_contents[block_size++] = (unsigned char) next_byte;

            if ((next_byte = fgetc(stdin)) == EOF)
                break;

            bytes_read++;
            crc = crc_continue((unsigned char) next_byte, crc);

            if (block_size == MAX_BLOCK_SIZE){
                bitstream_push_bit(&stream, 0);
                block_size = write_block(&stream, &window, block_contents, block_size);
            }
        }
    }

    if (block_size > 0) {
        bitstream_push_bit(&stream, 1); 
        block_size = write_block(&stream, &window, block_contents, block_size);
    }

    bitstream_flush_to_byte(&stream);
    bitstream_push_u32(&stream, crc);
    bitstream_push_u32(&stream, bytes_read);
    bitstream_finalize(&stream);

    return 0;
}