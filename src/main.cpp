#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <functional>

#include "bitbuffer.h"
#include "coding.h"
#include "huffman.h"
#include "markov_huffman.h"
#include "utils.h"

#undef BUFFER_SIZE // previously defined in bitbuffer.h
#define BUFFER_SIZE 32768

void print_help() {
	eprintf("markov-huffman <input> [-o output] [options]\n");
	eprintf("\t-o output_file\n");
	eprintf("\t-h use simple huffman coding\n");
	eprintf("\n");
	eprintf("\t-e encoding_file\n");
	eprintf("\t-d output_encoding_file\n");
	eprintf("\n");
	eprintf("\t-g print huffman trees and tables\n");
	eprintf("\t-x extract\n");
}

void construct_table(FILE* input_fd, std::function<void(unsigned char, unsigned char)> counter) {
	size_t bytes_read;
	unsigned char buffer[BUFFER_SIZE];
	int prev = ' ';
	while(bytes_read = read_buffer(buffer, 1, BUFFER_SIZE, input_fd)) {
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
	char* output = null;
	char* encoding_input = null;
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
							eprintf("Error: Expected output file following -o.\n");
						}
						break;
					case 'e':
						if(i + 1 < argc) {
							encoding_input = argv[i + chomp++ + 1];
						} else {
							eprintf("Error: Expected encoding file following -e.\n");
						}
						break;
					case 'd':
						if(i + 1 < argc) {
							encoding_output = argv[i + chomp++ + 1];
						} else {
							eprintf("Error: Expected encoding output file following -d.\n");
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
						eprintf("Warning: Unknown option %c.\n", argv[i][j]);
				}
			i += chomp;
		} else {
			if(input == null) {
				input = argv[i];
			} else {
				eprintf("Warning: Unexpected positional argument %s.\n", argv[i]);
			}
		}
	}

	// argument validation
	if(input == null) {
		eprintf("Error: Must provide input file.\n");
		exit(1);
	}
	if(encoding_input && encoding_output) {
		eprintf("Error: Don't provide an encoding input and an encoding output. Just use cp.\n");
		exit(1);
	}
	if(extract && !encoding_input) {
		eprintf("Error: Must provide encoding file input while in decompress mode.\n");
		exit(1);
	}

	// check access on inputs/outputs
	                    check_access(input, false);
	if(output)          check_access(output, true);
	if(encoding_input)  check_access(encoding_input, false);
	if(encoding_output) check_access(encoding_output, false);

	FILE* input_fd = fopen(input, "rb");
	if(input_fd == null) {
		eprintf("Error while opening input; %s.\n", strerror(errno));
		exit(1);
	}

	// open output file early to catch errors
	FILE* output_fd = output == null ? stdout : fopen(output, "wb");
	if(output_fd == null) {
		eprintf("Error while opening output; %s.\n", strerror(errno));
		exit(1);
	}

	i_coding_provider* coder = null;
	if(encoding_input) {
		eprintf("Loading encoding table from file...\n");
		// load encoding
		FILE* encoding_input_fd = fopen(encoding_input, "rb");
		if(encoding_input_fd == null) {
			eprintf("Error while opening encoding input; %s.\n", strerror(errno));
			exit(1);
		}
		bitbuffer buffer(encoding_input_fd, bitbuffer::read);
		// check that the correct encoding file was provided for our operation
		if(buffer.peek_bit() != !simple_huffman) {
			assert(encoding_input);
			eprintf("Error: Incorrect encoding table provided for current operation; "
					"expected %s, found %s.\n",
					simple_huffman ? "simple Huffman" : "Markov-Huffman",
					buffer.peek_bit() ? "Markov-Huffman" : "simple Huffman");
			exit(1);
		}
		if(buffer.peek_bit() == 0) {
			// simple huffman
			coder = new huffman_table(buffer);
		} else {
			// markov-huffman
			coder = new markov_huffman_table(buffer);
		}
	} else {
		// build encoding tables
		if(simple_huffman) {
			eprintf("Building simple Huffman encoding table from input...\n");
			int counts[256];
			memset(counts, 0, 256 * sizeof(int));
			construct_table(input_fd, [&](unsigned char, unsigned char c) {
				counts[c]++;
			});
			coder = new huffman_table(counts);
		} else {
			eprintf("Building Markov-Huffman encoding table from input...\n");
			int* counts = new int[256 * 256];
			memset(counts, 0, 256 * 256 * sizeof(int));
			construct_table(input_fd, [&](unsigned char prev, unsigned char c) {
				counts[256 * prev + c]++;
			});
			coder = new markov_huffman_table(counts);
			delete[] counts;
		}
		// return pointer to beginning
		fseek(input_fd, 0, SEEK_SET);
	}

	// Print tree and table for debug view
	if(debug) {
		coder->print_table();
		coder->print_tree();
	}

	// if outputing the encoding table, do so here
	if(encoding_output) {
		FILE* encoding_output_fd = fopen(encoding_output, "wb");
		eprintf("Writing encoding table to %s...\n", encoding_output);
		if(encoding_output_fd == null) {
			eprintf("Error while opening encoding file output; %s.\n", strerror(errno));
			exit(1);
		}
		bitbuffer buffer(encoding_output_fd, bitbuffer::write);
		coder->write_coding_tree(buffer);
	}

	if(extract) {
		eprintf("Extracting %s ===> %s...\n", input, output);
		// file descriptor ownership transferred into this method
		coder->decompress(input_fd, output_fd);
	} else {
		eprintf("Compressing %s ===> %s...\n", input, output);
		// file descriptor ownership transferred into this method
		coder->compress(input_fd, output_fd);
	}

	delete coder;
	
	eprintf("Done.\n");
}
