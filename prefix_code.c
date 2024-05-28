/* prefix_code.c

   Definitions of the functions declared in prefix_code.h

   Zoe Johnston - 2023/06/25
*/ 

#include "stdio.h"
#include "stdint.h"
#include "assert.h"
#include "string.h"
#include "prefix_code.h"

/* Function Declaration */

/* Counts the number of occurences of each ll symbol in the array pointed to by contents, storing the results in 
   ll_storage. Adds one to the count for symbol 256. Counts the number of occurences of each distance symbol in 
   the array pointed to by contents, storing the results in dist_storage.
 */
void get_frequencies(uint32_t* ll_storage, uint32_t* dist_storage, uint16_t* contents, uint32_t size) {
    uint16_t current;
    unsigned int i = 0;

    while (i  < size) {
        current = contents[i];
        ll_storage[current]++;

        if (current > 256) {
            current = contents[i + 1];
            dist_storage[current & 31]++;
            i += 2;
        }

        i++;
    }

    ll_storage[256]++;
}

/* Counts the number of occurences of each CL symbol in both the array pointed to by ll_lengths and the array pointed to 
   by dist_lengths, storing the results in storage.
 */
void get_cl_frequencies(uint32_t* storage, uint16_t* ll_lengths, uint32_t ll_size, uint16_t* dist_lengths, uint32_t dist_size) {
    uint16_t current;
    unsigned int i = 0;

    while (i  < ll_size) {
        current = ll_lengths[i];
        storage[current]++;
        i++;

        if (current > 15)
            i++;
    }

    i = 0;

    while (i  < dist_size) {
        current = dist_lengths[i];
        storage[current]++;
        i++;

        if (current > 15)
            i++;
    }
}

/* Computes the variance of the values in the values array
 */
float variance(uint32_t* values, float n) {
    unsigned int i;
    float mean, temp, sum = 0;

    for(i = 0; i < n; i++) {
        sum += (float) values[i];
    }

    mean = sum / n;
    sum = 0;

    for(i = 0; i < n; i++) {
        temp = (float) values[i];
        sum += (temp - mean) * (temp - mean);
    }

    return sum / n;
}

/* Returns 1 if most characters appear with about the same frequency, 0 otherwise.
 */
int frequency_analysis(uint32_t* ll_storage, uint32_t* dist_storage) {
    float ll_variance = variance(ll_storage, 288);
    float dist_variance = variance(dist_storage, 32);

    if (ll_variance > 1.5 || dist_variance > 1.5) {
        return 0;
    } else {
        return 1;
    }
}

/* The algorithm used in this function follows the pseudocode in RFC 1951.
   Code provided by Bill Bird.
 */
void construct_canonical_code(uint16_t num_symbols, uint16_t lengths[], uint16_t result_codes[]) {
    uint16_t length_counts[16] = {0};
    uint16_t max_length = 0;

    for(unsigned int i = 0; i < num_symbols; i++){
        assert(lengths[i] <= 15);
        length_counts[lengths[i]]++;

        if (lengths[i] > max_length)
            max_length = lengths[i];
    }

    length_counts[0] = 0; 
    memset(result_codes,0,sizeof(result_codes[0])*num_symbols);
    uint16_t next_code[max_length+1];
    memset(next_code,0,sizeof(next_code[0])*(max_length+1));
    
    {
        unsigned int code = 0;
        for(unsigned int i = 1; i <= max_length; i++){
            code = (code+length_counts[i-1])<<1;
            next_code[i] = code;
        }
    }
    {
        for(unsigned int symbol = 0; symbol < num_symbols; symbol++){
            unsigned int length = lengths[symbol];
            if (length > 0)
                result_codes[symbol] = next_code[length]++;
        }  
    } 
}

/* Constructs an item_t for each symbol with a non-zero frequency and sorts them into the originals array. 
*/
uint16_t setup_originals(uint16_t num_symbols, uint32_t* frequencies, item_t* originals) {
    uint16_t non_zero = 0, last = 0;

    for (unsigned int i = 0; i < num_symbols; i++) {
        if (frequencies[i] == 0)
            continue;

        last = i;
        originals[non_zero].symbol = i;
        originals[non_zero].cost = frequencies[i];
        originals[non_zero].merged = 0;
        non_zero++;
    }

    // Avoids instances where there are no or only one non-zero frequency
    if (non_zero == 0) {
        originals[non_zero].symbol = last;
        originals[non_zero].cost = 1;
        originals[non_zero].merged = 0;
        non_zero++;
    }

    if (non_zero == 1) {
        last = (last + 1) % num_symbols;
        originals[non_zero].symbol = last;
        originals[non_zero].cost = 1;
        originals[non_zero].merged = 0;
        non_zero++;
    }

    // Bubble sort code from https://www.geeksforgeeks.org/bubble-sort/
    item_t temp;
    int no_swap;

    for (unsigned int i = 0; i < non_zero - 1; i++) {
        no_swap = 1;

        for (unsigned int j = 0; j < non_zero - i - 1; j++) {
            if (originals[j].cost > originals[j + 1].cost) {
                temp = originals[j];
                originals[j] = originals[j + 1];
                originals[j + 1] = temp;

                no_swap = 0;
            }
        }

        if (no_swap)
            break;
    }

    return non_zero;
}

/* Packages the item_ts in array into temp, then merges temp with originals, maintaining ordering.
 */
uint16_t package_and_merge(item_t* array, uint16_t array_len, item_t* originals, uint16_t num_symbols, item_t* merged) {
    uint16_t maximum = 2 * num_symbols - 2;
    uint16_t temp_size = array_len / 2;
    uint32_t temp[temp_size];
    unsigned int i, j, k;

    for (i = 0; i < temp_size; i++)
        temp[i] = array[2 * i].cost + array[(2 * i) + 1].cost;

    i = 0;
    j = 0;
    k = 0;

    while (i < maximum && j < temp_size && k < num_symbols) {
        if (temp[j] < originals[k].cost) {
            merged[i].merged = 1;
            merged[i].cost = temp[j];
            j++;
        } else {
            merged[i].merged = 0; 
            merged[i].cost = originals[k].cost;
            merged[i].symbol = originals[k].symbol;
            k++;
        }
        
        i++;
    }

    while (i < maximum && j < temp_size) {
        merged[i].merged = 1;
        merged[i].cost = temp[j];
        j++;
        i++;
    }

    while (i < maximum && k < num_symbols) {
        merged[i].merged = 0; 
        merged[i].cost = originals[k].cost;
        merged[i].symbol = originals[k].symbol;
        k++;
        i++;
    }

    return i;
}

/* Counts the number of item_t which were the result of packaging and returns twice that number. For each item_t
 * which was not the result of packaging, increase the code length of its symbol by one.  
 */
uint16_t interpret(item_t* array, uint16_t num, uint16_t* storage) {
    uint16_t num_merged = 0;

    for (unsigned int i = 0; i < num; i++) {
        if (array[i].merged == 1) {
            num_merged++;

        } else {
            storage[array[i].symbol]++;
        }
    }

    return 2 * num_merged;
}

/* This function implements the package merge algorithm. It generates code lengths less than or equal to max_len
 * and stores them in the array pointed to by storage. My implementation is based on the description of the
 * algorithm given by Stephan Brumme at https://create.stephan-brumme.com/length-limited-prefix-codes/. See 
 * README.md for more information.
 */
void package_merge(uint8_t max_len, uint16_t num_symbols, uint32_t* frequencies, uint16_t* storage) {
    // Constructs an item_t for each symbol with a non-zero frequency and sorts them into the originals array
    item_t originals[num_symbols];
    uint16_t actual_num = setup_originals(num_symbols, frequencies, originals);

    uint16_t maximum = 2 * actual_num - 2;
    uint16_t sizes[max_len - 1];
    item_t items_at_iter[max_len - 1][maximum];
    unsigned int i;

    // Package the contents of originals and merge the packaged item_ts with those in the original array
    sizes[0] = package_and_merge(originals, actual_num, originals, actual_num, items_at_iter[0]);

    // Repeat max_len - 2 times, packaging the results of the previous merge and merging them with those in the original array
    for (i = 0; i < max_len - 2; i++) 
        sizes[i + 1] = package_and_merge(items_at_iter[i], sizes[i], originals, actual_num, items_at_iter[i + 1]);

    // Interpret the arrays created at each of the package and merge steps, storing results in the array pointed to by storage
    uint16_t num = interpret(items_at_iter[max_len - 2], maximum, storage);

    for (i = max_len - 3; i > 0; i--) 
        num = interpret(items_at_iter[i], num, storage);
    
    num = interpret(items_at_iter[0], num, storage);
    num = interpret(originals, num, storage);
}