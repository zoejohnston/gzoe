/* CRC_for_C.cpp
   
   Basic definitions to adapt Daniel Bahr's CRC++ library
   for use in C code.

   B. Bird - 2023-05-13
*/

#include "CRC_for_C.h"

#define CRCPP_USE_CPP11
#include "CRC.h"

//Construct the CRC table as a global variable
static auto crc_table = CRC::CRC_32().MakeTable();


u32 crc_init(unsigned char first_byte){
    return CRC::Calculate(&first_byte, 1, crc_table);
}

u32 crc_continue(unsigned char next_byte, u32 old_crc){
    return CRC::Calculate(&next_byte, 1, crc_table, old_crc);
}