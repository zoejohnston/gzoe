# Gzip Style Data Compression

This compressor is designed such that it can be used interchangeably with gzip. That is, gzip can be used to decompress files produced by this compressor. The compressor uses the DEFLATE algorithm, making use of two compression techniques: LZSS and prefix coding.

My implementation is broken up into a few different files. Functions relating to LZSS are found in *lzss.c* and functions relating to prefix coding are found in *prefix_code.c*. The file *gzoe.c* imports the headers for both of these files. Headers for outside code are also imported by *gzoe.c*. These include the *output_steam* files by Bill Bird, and CRC++ by Daniel Bahr. More detailed citation can be found in each file.

To use the compressor, run the following commands. 

```
make
./gzoe < file_to_compress.txt > compressed_file.gz
```

## Compression Ratio and Speed

My test data was comprised mainly of the Canterbury and Calgary corpuses. My implementation is able achieve a compression ratio higher than gzip -1 for every piece of test data and it is able to compress the entire collection of test data in under 10 seconds.

## LZSS

Like other Lempel-Ziv schemes, LZSS uses backreferences. To this end, the compressor maintains a sliding window of stored bytes as it works through the file input from stdin. I used a "suspension bridge" approach to speed up my backreference lookups.

My code uses a char array of size 33026 for the sliding window. This array contains 32768 past characters and 258 future characters. As new characters are introduced, they overwrite characters that have moved out of the sliding window. Two additional arrays, one of size 33026 and one of size 256, keep track of indices in the char array. The array of size 256 has an entry corresponding to each character. If the character has not yet been encountered, it contains a NIL value (a value defined at the top of *lzss.h*). Otherwise, it contains the index of the char array where that character was most recently seen in the character array. Similarly, the arrray of size 33026 has entries which correspond to the character array and contain the previous index that the character at that index is seen at. As characters leave the window, their corresponding entries are set to NIL.

When searching for a backreference, the *find_backreference* function first checks the array of size 256 to see where the current character most recently occured. It then uses the array of size 33026 to iterate through the rest of the occurences of that character. At each occurence, it checks to see if a backreference could be generated and if that backreference would be longer than any found thus far. To ensure timeliness, the *find_backreference* function will iterate a maximum of 500 times. 

## Choosing Block Types Based on Data

Gzip uses three different block types (0, 1, and 2) to encode data. Each block type suits itself best to certain types of data. Block 0 delivers the block in its uncompressed form, this is used whenever adding backreferences increases the size of the block. If block type 0 is not chosen, the frequencies of symbols in the LZSS compressed data are computed. If the variance is low, the dynamic prefix code would likely be similar to the default one. So, in that case, block type 1 is used in order to avoid the overhead associated with block type 2. Otherwise, block type 2 is used.

## Dynamic Prefix Codes

My implementation generates dynamic LL and distance codes for block type 2. After LZSS has been applied to the data, the frequencies for length and distance symbols are computed. These are used by the package merge and canonical code algorithms to contruct the dynamic codes.

### The Package Merge Algorithm

My implementation is based on the description of the package merge algorithm by Stephan Brumme found [here](https://create.stephan-brumme.com/length-limited-prefix-codes/). Stephan Brumme provides his own implementation in the article, but my code does not reference his implementation. My implementation can be found in the function *package_merge* in *prefix_code.c*. This function is called as needed by functions in *gzoe.c* to generate code lengths.
