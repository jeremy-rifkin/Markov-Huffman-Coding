#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <functional>

#include "coding.h"
#include "huffman.h"
#include "markov_huffman.h"
#include "min_pq.h"
#include "null.h"
#include "tree.h"

#define BUFFER_SIZE 32768

void print_counts(int* counts) {
	for(int i = 0; i < 256; i++) {
		if(counts[i]) {
			printf("%c %d\n", i, counts[i]);
		}
	}
}

void print_counts_2d(int* counts) {
	printf("  ");
	for(int i = 0; i < 256; i++) printf("%c", i < 0x20 || i >= 127 ? ' ' : i); printf("\n");
	for(int i = 0; i < 256; i++) {
		printf("%c ", i < 0x20 || i >= 127 ? ' ' : i);
		for(int j = 0; j < 256; j++) {
			printf("%d", counts[256 * i + j]);
		}
		printf("\n");
	}
}

void print_byte(unsigned char c) {
	for(int i = 8; i--; ) {
		printf("%d", (c >> i) & 1);
	}
}

void print_help() {
	printf("markov-huffman <input> [-o output] [options]\n");
	printf("\t-o output_file\n");
	printf("\t-e encoding_file\n");
	printf("\t-d output_encoding_file\n");
	printf("\t-g debug / demo encoding\n");
	printf("\t-h just use naive huffman\n");
	printf("\t-x extract\n");
}

/*
 * Output file format:
 * [metadata: 1byte] [data: .........]
 * metadata:
 *  1 bit complete | 3 bits unused | 1 bit encoder | 3 bits remainder
 *  complete: 1 if the header is currently blank, 0 if it the header is valid
 *  encoder: Indicates the encoding method and stores the inverse of get_type. I'm storing the
 *           inverse here so I can do a cool ascii hack.
 *  remainder: Number of bits of the last byte unused
 * 0 0 1  1 E R R R
 *    32 16 8 4 2 1
 * The unused bits take value 0x30 so that with markov-huffman coding, the ascii value of the
 * header byte itself takes on a '0' to '9' value equal to the remainder.
 * The data length in bits can be found from the file length and partial byte.
 * TODO: Currently have to seek back to write the last byte. Consider putting header byte at the
 * end...
 * TODO: CRC? Probably not needed for this proof of concept..
 *
 * NOTE: right bytes are filled with codewords left to right. Math might be simpler if filled
 * from right to left...
 * TODO: clean up all this math
 * TODO: get rid of all asserts
 */

void compress(i_coding_provider* coder, FILE* input_fd, FILE* output_fd) {
	size_t bytes_read;
	unsigned char input_buffer[BUFFER_SIZE];
	unsigned char output_buffer[BUFFER_SIZE] = { 1<<7 };
	int output_buffer_index = 1; // skip header byte
	unsigned char byte = 0x0;
	int output_bit_index = 0;
	int prev = ' ';
	while(bytes_read = fread(input_buffer, 1, BUFFER_SIZE, input_fd)) {
		for(int input_buffer_index = 0; input_buffer_index < bytes_read; input_buffer_index++) {
			// get encoding for character in input
			encoding_descriptor& e = coder->get_encoding(prev, input_buffer[input_buffer_index]);
			printf("[encoding character '%c': %d ", input_buffer[input_buffer_index], e.length);
			e.print();
			printf("]\n");
			// update state
			prev = input_buffer[input_buffer_index];
			// write encoding
			for(int i = 0; i < (e.length + 7) / 8; i++) {
				// how many bits we're working with this loop
				int working_bits = std::min(8, e.length - i * 8);
				print_byte(byte); printf("\n");
				for(int _ = 0; _ < output_bit_index; _++) printf(" "); printf("^\n");
				// three cases
				if(output_bit_index + working_bits == 8) { // fits perfectly
					// insert
					byte |= e.encoding[i] >> (8 - working_bits);
					printf("--fits perfectly--\n"); print_byte(byte); printf("\n");
					// flush
					output_buffer[output_buffer_index++] = byte;
					// reset
					byte = 0;
					output_bit_index = 0;
				} else if(output_bit_index + working_bits < 8) { // falls short of fitting
					//byte |= (e.encoding[i] >> (8 - working_bits)) << (8 - (output_bit_index + working_bits));
					byte |= e.encoding[i] >> output_bit_index;
					output_bit_index += working_bits;
					printf("--under fills--\n"); print_byte(byte); printf("\n");
					for(int _ = 0; _ < output_bit_index; _++) printf(" "); printf("^\n");
					continue; // skip the output_buffer_index check TODO: micro-optimization?
				} else { // code exceeds byte size
					// fill
					//byte |= (e.encoding[i] >> (8 - (8 - output_bit_index)));
					byte |= e.encoding[i] >> output_bit_index;
					printf("--over fills--\n");
					printf("first byte:\n");
					print_byte(byte); printf("\n");
					// flush
					output_buffer[output_buffer_index++] = byte;
					// insert and reset
					byte = e.encoding[i] << (8 - output_bit_index);
					output_bit_index = working_bits - (8 - output_bit_index);
					printf("--partial byte--\n"); print_byte(byte); printf("\n");
					for(int _ = 0; _ < output_bit_index; _++) printf(" "); printf("^\n");
				}
				// flush buffer if necessary
				if(output_buffer_index == BUFFER_SIZE) {
					//fwrite(output_buffer, 1, BUFFER_SIZE, output_fd);
					output_buffer_index = 0;
				}
			}
		}
	}
	// Check for read errors
	if(bytes_read == -1) {
		fprintf(stderr, "error occurred while reading input (errno: %d).\n", errno);
		exit(1);
	}
	// remainder byte
	if(output_bit_index > 0)
		output_buffer[output_buffer_index++] = byte;
	// write whatever is left
	if(output_buffer_index > 0)
		assert(fwrite(output_buffer, 1, output_buffer_index, output_fd) == output_buffer_index);
	// go back and write header....
	fseek(output_fd, 0, SEEK_SET);
	assert(output_bit_index >= 0 && output_bit_index < 8);
	printf("--remainder:-- %d\n", (8 - output_bit_index) % 8);
	printf("--type:-- %d\n", coder->get_type());
	printf("--type:-- %d\n", (~coder->get_type() & 1) << 3);
	unsigned char header = 0x30 | (~coder->get_type() & 1) << 3 | (8 - output_bit_index) % 8;
	assert(fwrite(&header, 1, 1, output_fd) == 1);
}

void construct_table(FILE* input_fd, std::function<void(unsigned char, unsigned char)> counter) {
	size_t bytes_read;
	unsigned char buffer[BUFFER_SIZE];
	int prev = ' ';
	while(bytes_read = fread(buffer, 1, BUFFER_SIZE, input_fd)) {
		for(int i = 0; i < bytes_read; i++) {
			counter(prev, buffer[i]);
			prev = buffer[i];
		}
	}
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		print_help();
		return 1;
	}
	// Parameters
	bool extract = false;
	bool debug = false;
	bool simple_huffman = false;
	char* input = null;
	char* encoding = null;
	char* output = null;
	char* encoding_output = null;
	// Process arguments
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			int chomp = 0;
			for(int j = 0; argv[i][++j] != 0x0; )
				switch(argv[i][j]) {
					case 'o':
						if(i + 1 < argc) {
							output = argv[i + chomp++ + 1];
						} else {
							fprintf(stderr, "[Error] Expected output file following -o.\n");
						}
						break;
					case 'e':
						if(i + 1 < argc) {
							encoding = argv[i + chomp++ + 1];
						} else {
							fprintf(stderr, "[Error] Expected encoding file following -e.\n");
						}
						break;
					case 'd':
						if(i + 1 < argc) {
							encoding_output = argv[i + chomp++ + 1];
						} else {
							fprintf(stderr, "[Error] Expected encoding output file following -d.\n");
						}
						break;
					case 'x':
						extract = true;
						break;
					case 'h':
						simple_huffman = true;
						break;
					case 'g':
						debug = true;
						break;
					default:
						fprintf(stderr, "[Warning] Unknown option %c\n", argv[i][j]);
				}
			i += chomp;
		} else {
			if(input == null) {
				input = argv[i];
			} else {
				fprintf(stderr, "[Warning] Unknown positional argument %s\n", argv[i]);
			}
		}
	}

	if(input == null) {
		printf("error must provide input\n");
		exit(1);
	}

	if(output == null) {
		printf("error must provide output file\n");
		exit(1);
	}

	FILE* input_fd = fopen(input, "r");
	if(input_fd == null) {
		printf("error opening input errno: %s\n", strerror(errno));
		exit(1);
	}

	// open output file early to catch errors
	// todo: just use access()?
	FILE* output_fd = fopen(output, "wb");
	if(output_fd == null) {
		printf("error opening output errno: %s\n", strerror(errno));
		exit(1);
	}

	// file needs to indicate whether it has the encoding embedded (todo: good idea or not??)
	// file needs to indicate whether it is naive huffman or fancy huffman
	i_coding_provider* coder = null;
	if(encoding) {
		// load encoding
		fprintf(stderr, "--error--\n");
		exit(1);
	} else {
		// build encoding
		if(simple_huffman) {
			int counts[256];
			memset(counts, 0, 256 * sizeof(int));
			construct_table(input_fd, [&](unsigned char prev, unsigned char c) {
				counts[c]++;
			});
			coder = new huffman_table(counts);
		} else {
			int* counts = new int[256 * 256];
			memset(counts, 0, 256 * 256 * sizeof(int));
			construct_table(input_fd, [&](unsigned char prev, unsigned char c) {
				counts[256 * prev + c]++;
			});
			//print_counts_2d(counts);
			coder = new markov_huffman_table(counts);
			delete[] counts;
		}
		fseek(input_fd, 0, SEEK_SET);
	}

	if(debug) {
		coder->print_table();
		coder->print_tree();
	}

	// if outputing the encoding table, do so here
	// todo: check access early with
	if(encoding_output) {
		fprintf(stderr, "--error--\n");
		exit(1);
	}

	if(extract) {
		fprintf(stderr, "--error--\n");
		exit(1);
	} else {
		compress(coder, input_fd, output_fd);
	}
	fclose(input_fd);
	fclose(output_fd);

	delete coder;
}

