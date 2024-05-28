/* output_stream.c

   Definitions of the functions declared in output_stream.h

   This code was written by Bill Bird - 2023-05-12
   and later modified by Zoe Johnston - 2023-06-25
*/ 


#include "stdio.h"
#include "stdint.h"
#include "output_stream.h"

static void output_byte(bitstream_t *stream){
    fputc((unsigned char) stream->bitvec, stream->output_file);
    stream->bitvec = 0;
    stream->numbits = 0;
}

/* Initialize an bitstream_t structure. MUST be called before any of the below functions are used. */
void bitstream_init(bitstream_t* stream, FILE* output_file){
    stream->output_file = output_file;
    stream->numbits = 0;
    stream->bitvec = 0;
}

void bitstream_finalize(bitstream_t* stream){
    if (stream->numbits > 0)
        output_byte(stream);
}

/* Push an entire byte into the stream, with the least significant bit pushed first */
void bitstream_push_byte(bitstream_t* stream, unsigned char b){
    bitstream_push_bits(stream, b, 8);
}

/* Push a 32 bit unsigned integer value (LSB first) */
void bitstream_push_u32(bitstream_t* stream, uint32_t i){
    bitstream_push_bits(stream, i, 32);
}

/* Push a 16 bit unsigned integer value (LSB first) */
void bitstream_push_u16(bitstream_t* stream, uint16_t i){
    bitstream_push_bits(stream, i, 16);
}

/* Push the lowest order num_bits bits from b into the stream
   with the least significant bit pushed first */
void bitstream_push_bits(bitstream_t* stream, unsigned int b, unsigned int num_bits){
    for(unsigned int i = 0; i < num_bits; i++)
        bitstream_push_bit(stream, (b >> i) & 1);
}

/* Push the lowest order num_bits bits from b into the stream
   with the most significant bit pushed first*/
void bitstream_push_encoding(bitstream_t* stream, unsigned int b, unsigned int num_bits) {
    for(int i = num_bits - 1; i >= 0; i--) {
        bitstream_push_bit(stream, (b >> (unsigned int) i) & 1);
    }
}

/* Push a single bit b (stored as the LSB of an unsigned int)
    into the stream */ 
void bitstream_push_bit(bitstream_t* stream, unsigned int b){
    stream->bitvec |= (b & 1) << stream->numbits;
    stream->numbits++;
    if (stream->numbits == 8)
        output_byte(stream);
}

/* Flush the currently stored bits to the output stream */
void bitstream_flush_to_byte(bitstream_t* stream){
    if (stream->numbits > 0)
        output_byte(stream);
}