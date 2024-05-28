/* CRC_for_C.h
   
   Basic definitions to adapt Daniel Bahr's CRC++ library
   for use in C code.

   B. Bird - 2023-05-13
*/

#ifndef CRC_FOR_C_H
#define CRC_FOR_C_H

#ifdef __cplusplus
#include <cstdint>
#define C_FUNCTION_DECLARATION extern "C"
using u32 = std::uint32_t;
#else
#define C_FUNCTION_DECLARATION
typedef uint32_t u32;
#endif 


C_FUNCTION_DECLARATION u32 crc_init(unsigned char first_byte);
C_FUNCTION_DECLARATION u32 crc_continue(unsigned char next_byte, u32 old_crc);



#endif