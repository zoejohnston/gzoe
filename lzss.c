/* lzss.c

   Definitions of the functions declared in lzss.h
   
   Preforms LZSS on a given block of data, maintaining a static amount of
   information about past blocks for the creation of backreferences.

   Zoe Johnston - 2023/06/25
*/ 

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "assert.h"
#include "lzss.h"

#define WINDOW_SIZE 33026
#define FUTURE_SIZE 258
#define PAST_SIZE 32768
#define NIL 33026

/* Global variables */

uint16_t actual_future;
uint16_t actual_past;

uint16_t distance_code_ranges[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
uint16_t distance_offsets[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
uint16_t length_code_ranges[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
uint16_t length_offsets[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

/* Function Declaration */

/* Takes a length as a parameter and returns the symbol associated with it
 */
uint16_t length_to_symbol(uint16_t length) {
    for (unsigned int i = 0; i < 28; i++) {
        if (length_code_ranges[i + 1] > length) 
            return i + 257;
    }

    return 285;
}

/* Takes a distance as a parameter and returns the symbol associated with it
 */
uint16_t distance_to_symbol(uint16_t distance) {
    for (unsigned int i = 0; i < 29; i++) {
        if (distance_code_ranges[i + 1] > distance) 
            return i;
    }

    return 29;
}

/* Takes a length and the length symbol associated with that length minus 257 and returns 
 * the required offset value.
 */
uint16_t length_offset(uint16_t length, uint16_t len_ind) {
    uint16_t len_range_start = length_code_ranges[len_ind];
    return length - len_range_start;
}

/* Takes a distance and the distance symbol associated with that distance and returns 
 * the required offset value.
 */
uint16_t distance_offset(uint16_t distance, uint16_t dist_ind) {
    uint16_t dist_range_start = distance_code_ranges[dist_ind];
    return distance - dist_range_start;
}

/* Takes either a length of distance symbol and returns the number of offset bits that follow it
 */
uint16_t offset_bits(uint16_t symbol) {
    if (symbol < 30) {
        return distance_offsets[symbol]; 
    } else {
        return length_offsets[symbol - 257];
    }
}

/* Returns the number of bits used to encode a given length symbol in block 1
 */
uint32_t length_bits(uint16_t symbol) {
    if (symbol < 144) {
        return 8;
    } else if (symbol < 256) {
        return 9;
    } else if (symbol < 280) {
        return 7;
    } else {
        return 8;
    }
}

/* Initializes the sliding window. Pulls the first 258 characters into the sliding window buffer and
 * sets unused slots in the buffer and hash to NIL.
 */
void window_init(window_t* window) {
    window->current = 0;
    window->oldest = 0;
    actual_past = 0;
    actual_future = 0;
    
    for (uint32_t i = 0; i < 256; i++) {
        window->hash[i] = NIL;
    }

    for (unsigned int i = 0; i < WINDOW_SIZE; i++) {
        window->indices[i] = NIL;
        window->chars[i] = 0;
    }
}

/* Enters the first WINDOW_SIZE characters into the sliding window. 
 */
void setup_future(window_t* window, uint8_t* contents, uint32_t block_size) {
    uint32_t limit;

    if (FUTURE_SIZE < block_size) {
        limit = FUTURE_SIZE;
    } else {
        limit = block_size;
    }

    for (unsigned int i = 0; i < limit; i++) {
        window->chars[window->oldest] = contents[i];
        window->oldest = (window->oldest + 1) % WINDOW_SIZE;
        actual_future++;
    }
}

/* Moves the sliding window forward by the amount provided in the parameter num. 
 */
void move_window(window_t* window, uint8_t* contents, uint32_t block_size, uint32_t index, uint16_t num) {
    uint8_t current_char, oldest_char;

    for (unsigned int i = 0; i < num; i++) {
        current_char = window->chars[window->current];

        if (current_char < 256) {
            window->indices[window->current] = window->hash[current_char];
            window->hash[current_char] = window->current;
        }

        window->current = (window->current + 1) % WINDOW_SIZE;
        oldest_char = window->chars[window->oldest];

        if (window->hash[oldest_char] == window->indices[window->oldest])
            window->hash[oldest_char] = NIL;
            
        window->indices[window->oldest] = NIL;

        if (i + index < block_size) {
            window->chars[window->oldest] = (uint16_t) contents[i + index];
            window->oldest = (window->oldest + 1) % WINDOW_SIZE;

        } else if (actual_future > 0) {
            actual_future--;
        }

        if (actual_past < PAST_SIZE)
            actual_past++;
    }
}

/* Returns 1 if the current charcater and the two characters that follow it are respectively equal to the charcater at 
 * index most_recent_index and the two characters that follow it. Returns 0 otherwise. 
 */
int three_are_equal(window_t* window, uint16_t most_recent_index) {
    return window->chars[(most_recent_index + 1) % WINDOW_SIZE] == window->chars[(window->current + 1) % WINDOW_SIZE]
        && window->chars[(most_recent_index + 2) % WINDOW_SIZE] == window->chars[(window->current + 2) % WINDOW_SIZE];
}

/* Returns 0 if the character at index0 is equal to the character at index1, returns 1 otherwise.
 */
int are_not_equal(window_t* window, uint16_t index0, uint16_t index1) {
    return window->chars[index0 % WINDOW_SIZE] != window->chars[index1 % WINDOW_SIZE];
}

/* Computes the distance between the current index and the most recenty index
 */
uint16_t compute_distance(window_t* window, uint16_t most_recent_index) {
    int diff = (signed) window->current - most_recent_index;
        
    while (diff < 0)
        diff += WINDOW_SIZE;

    return (uint16_t) diff % WINDOW_SIZE;
}

/* Locates a backreference. If no backreference is found, returns 0. Otherwise returns 1. Updates the values pointed to 
 * by distance and length with the disntace and length of the backreference. 
 */
int find_backreference(window_t* window, uint32_t num_left, uint16_t* distance, uint16_t* length) {
    int attempts = 500; 
    
    if (actual_past == 0 || num_left < 3)
        return 0;

    uint16_t current_char = window->chars[window->current];
    uint16_t most_recent_index = window->hash[current_char];
    uint16_t longest_len = 0, longest_len_distance = 0;
    uint16_t cur_len, cur_dist;

    while (most_recent_index != NIL && attempts > 0) {
        if (window->chars[most_recent_index] != window->chars[window->current])
            break;

        cur_dist = compute_distance(window, most_recent_index);

        if (cur_dist > PAST_SIZE || cur_dist > actual_past || cur_dist == 0) 
            break;

        if (three_are_equal(window, most_recent_index)) {
            for (cur_len = 3; cur_len < FUTURE_SIZE && cur_len < num_left; cur_len++) {
                if (are_not_equal(window, most_recent_index + cur_len, window->current + cur_len))
                    break;
            }

            if (cur_len > 50) {
                *distance = cur_dist;
                *length = cur_len;
                return 1;
            }
            
            if (cur_len > longest_len) {
                longest_len = cur_len;
                longest_len_distance = cur_dist;
            }
        }

        most_recent_index = window->indices[most_recent_index];
        attempts--;
    }

    if (longest_len > 0 && longest_len_distance > 0) {
        *length = longest_len;
        *distance = longest_len_distance;
        return 1;
    }

    return 0;
}

/* Applies LZSS to the contents of the block pointed to by the contents parameter. Stores the result in the array 
 * pointed to by the storage parameter.
 */
uint32_t lzss(uint16_t* storage, window_t* window, uint8_t* contents, uint32_t block_size, uint32_t* bits_used_ptr) {
    uint16_t distance_symbol, length_symbol, distance, length;
    uint32_t bits_used = 0;
    uint32_t i = 0, j = 0;

    // Puts future characters into the sliding window
    setup_future(window, contents, block_size);

    // Iterates over the block while moving through the window 
    while (i < block_size) {
        if (find_backreference(window, block_size - i, &distance, &length) == 1) {
            distance_symbol = distance_to_symbol(distance);
            length_symbol = length_to_symbol(length);

            // Because the length will always be at least 3, we can safely store the length and distance symbols and offsets
            // in three array entries 
            storage[j] = length_symbol;
            storage[j + 1] = (length_offset(length, length_symbol - 257) << 5) | distance_symbol;
            storage[j + 2] = distance_offset(distance, distance_symbol);

            move_window(window, contents, block_size, i + FUTURE_SIZE, length);

            bits_used += length_bits(length_symbol) + 5 + offset_bits(length_symbol) + offset_bits(distance_symbol);

            j += 3;
            i += length;

        } else {
            assert(contents[i] == window->chars[window->current]);
            storage[j] = (uint16_t) window->chars[window->current];
            move_window(window, contents, block_size, i + FUTURE_SIZE, 1);

            bits_used += 8;

            j++;
            i++;
        }
    }

    *bits_used_ptr = bits_used;
    return j;
}