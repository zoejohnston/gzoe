/* output_stream.h

   Definitions for a bitstream type which complies with the bit ordering
   required by the gzip format.

   This code was written by Bill Bird - 2023-05-12
   and later modified by Zoe Johnston - 2023-06-25
*/ 

#ifndef OUTPUT_STREAM_H
#define OUTPUT_STREAM_H

#include "stdio.h"
#include "stdint.h"

typedef struct {
    uint32_t bitvec, numbits;
    FILE* output_file;
} bitstream_t;


/* Initialize an bitstream_t structure. MUST be called before any of the below functions are used. */
void bitstream_init(bitstream_t* stream, FILE* output_file);

void bitstream_finalize(bitstream_t* stream);

/* Push an entire byte into the stream, with the least significant bit pushed first */
void bitstream_push_byte(bitstream_t* stream, unsigned char b);

/* Push a 32 bit unsigned integer value (LSB first) */
void bitstream_push_u32(bitstream_t* stream, uint32_t i);

/* Push a 16 bit unsigned integer value (LSB first) */
void bitstream_push_u16(bitstream_t* stream, uint16_t i);

/* Push the lowest order num_bits bits from b into the stream
   with the least significant bit pushed first */
void bitstream_push_bits(bitstream_t* stream, unsigned int b, unsigned int num_bits);

/* Push the lowest order num_bits bits from b into the stream
   with the most significant bit pushed first*/
void bitstream_push_encoding(bitstream_t* stream, unsigned int b, unsigned int num_bits);

/* Push a single bit b (stored as the LSB of an unsigned int)
    into the stream */ 
void bitstream_push_bit(bitstream_t* stream, unsigned int b);

/* Flush the currently stored bits to the output stream */
void bitstream_flush_to_byte(bitstream_t* stream);

#endif 