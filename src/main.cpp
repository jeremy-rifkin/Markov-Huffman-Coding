#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <functional>

#include "bitbuffer.h"
#include "coding.h"
#include "huffman.h"
#include "markov_huffman.h"
#include "min_pq.h"
#include "utils.h"
#include "tree.h"

#undef BUFFER_SIZE // previously defined in bitbuffer.h
#define BUFFER_SIZE 32768

void print_byte(unsigned char c) {
	for(int i = 8; i--; ) {
		eprintf("%d", (c >> i) & 1);
	}
}

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

/*
 * Output file format:
 * [metadata: 1byte] [data: .........]
 *
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
 *
 * Using the unused bytes like this also allows them to serve as a file signature check of sorts.
 *
 * The data length in bits can be found from the file length and partial byte.
 *
 * TODO: Currently have to seek back to write the last byte. Consider putting header byte at the
 * end...
 * TODO: CRC? Probably not needed for this proof of concept..
 * TODO: Store length of decoded data as another data integrity check? Probably not for the same
 * reason as above..
 */

void compress(i_coding_provider* coder, FILE* input_fd, FILE* output_fd) {
	size_t bytes_read;
	unsigned char input_buffer[BUFFER_SIZE];
	bitbuffer output_buffer(output_fd, bitbuffer::write);
	// push temp header byte
	output_buffer.push_byte(1 << 7);
	unsigned char prev = ' ';
	while(bytes_read = read_buffer(input_buffer, 1, BUFFER_SIZE, input_fd)) {
		for(int input_buffer_index = 0; input_buffer_index < bytes_read; input_buffer_index++) {
			// get encoding for character in input
			encoding_descriptor& e = coder->get_encoding(prev, input_buffer[input_buffer_index]);
			assert(e.length != 0);
			// update state
			prev = input_buffer[input_buffer_index];
			// write encoding
			output_buffer.push_encoding_descriptor(e);
		}
	}
	// Check for read errors
	if(bytes_read == -1) {
		eprintf("Error occurred while reading input; %s.\n", strerror(errno));
		exit(1);
	}
	// go back and write header....
	int bi = output_buffer.get_bi();
	output_buffer.flush();
	fseek(output_fd, 0, SEEK_SET);
	unsigned char header = 0x30 | (~coder->get_type() & 1) << 3 | (8 - bi) % 8;
	write_buffer(&header, 1, 1, output_fd);
	// output_buffer manual flush guarantees internal state i=0 so the buffer won't be flushed on
	// destruction here
}

void decompress(i_coding_provider* coder, FILE* input_fd, FILE* output_fd) {
	bitbuffer input_buffer(input_fd, bitbuffer::read);
	bitbuffer output_buffer(output_fd, bitbuffer::write);
	// header
	unsigned char header = input_buffer.pop_byte();
	// only necessary to check header & 1<<7, however, checking the 0x30 serves as a file signature
	// of sorts
	if((header & 0xF0) != 0x30) {
		eprintf("Error while decoding file: Input appears corrupt.\n");
		exit(1);
	}
	if((~(header & 1<<3)>>3 & 1) != coder->get_type()) {
		eprintf("Error: File encoding method does not match provided encoding table.\n");
		exit(1);
	}
	int remainder = header & 7;
	// need to be careful with seeking in a file owned by the bitbuffer
	long pos = ftell(input_fd);
	fseek(input_fd, 0, SEEK_END);
	int length = (ftell(input_fd) - 1) * 8 - remainder; // data length in bits
	fseek(input_fd, pos, SEEK_SET);
	// main decoder body
	unsigned char prev = ' ';
	//const tree_node* node = null;
	int bi = 0;
	// the working byte and working byte index
	unsigned char w = 0;
	int wi = 0;
	while(bi < length) {
		w = input_buffer.pop_rest(w, wi);
		const tree_node* node = coder->decoding_lookup(prev, w);
		assert(node != null);
		assert(node->depth >= 1);
		if(node->is_internal) {
			bi += 8;
			// need to traverse
			while(node != null) {
				int bit = input_buffer.pop_bit();
				bi++;
				if(bit) {
					node = node->right;
				} else {
					node = node->left;
				}
				assert(node != null);
				if(!node->is_internal) {
					output_buffer.push_byte(node->value);
					prev = node->value;
					node = null;
				}
			}
			// reset working byte
			w = 0;
			wi = 0;
		} else {
			output_buffer.push_byte(node->value);
			prev = node->value;
			w <<= node->depth;
			wi = 8 - node->depth;
			bi += node->depth;
		}
	}
	assert(bi == length);
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
	// todo: just use access()?
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
		decompress(coder, input_fd, output_fd);
		// bitbuffers in decompress take ownership of input_fd and output_fd
	} else {
		eprintf("Compressing %s ===> %s...\n", input, output);
		compress(coder, input_fd, output_fd);
		// bitbuffer in compress takes ownership of the output_fd
		fclose(input_fd);
	}

	delete coder;
	
	eprintf("Done.\n");
}
