EXTRA_CXXFLAGS=
EXTRA_CFLAGS=
CXXFLAGS=-O3 -Wall -std=c++20 $(EXTRA_CXXFLAGS)
CFLAGS=-O3 -Wall -std=c18 $(EXTRA_CFLAGS)

.PHONY all:
all: gzoe

gzoe: CRC_for_C.o gzoe.o output_stream.o lzss.o prefix_code.o
	gcc -o $@ $^

.PHONY clean:
clean:
	rm -f gzoe *.o
