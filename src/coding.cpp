#include "coding.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "utils.h"

void encoding_descriptor::push_bit(int b) {
	if(length % 8 == 0) {
		encoding.push_back(b << 7);
	} else {
		encoding[length / 8] |= b << (8 - length % 8 - 1);
	}
	length++;
}

void encoding_descriptor::pop_bit() {
	assert(length > 0);
	length--;
	if((length + 7) / 8 < encoding.size()) {
		encoding.pop_back();
	} else {
		// reset popped bit
		encoding[length / 8] &= ~(1 << (8 - length % 8 - 1));
	}
}

void encoding_descriptor::print() {
	for(int i = 0; i < length; i++) {
		printf("%d", (encoding[i / 8] >> (8 - i % 8 - 1)) & 1);
	}
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

void i_coding_provider::compress(FILE* input_fd, FILE* output_fd) {
	size_t bytes_read;
	unsigned char input_buffer[BUFFER_SIZE];
	bitbuffer output_buffer(output_fd, bitbuffer::write);
	// push temp header byte
	output_buffer.push_byte(1 << 7);
	unsigned char prev = ' ';
	while(bytes_read = read_buffer(input_buffer, 1, BUFFER_SIZE, input_fd)) {
		for(int input_buffer_index = 0; input_buffer_index < bytes_read; input_buffer_index++) {
			// get encoding for character in input
			encoding_descriptor& e = get_encoding(prev, input_buffer[input_buffer_index]);
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
	unsigned char header = 0x30 | (~get_type() & 1) << 3 | (8 - bi) % 8;
	write_buffer(&header, 1, 1, output_fd);
	// output_buffer manual flush guarantees internal state i=0 so the buffer won't be flushed on
	// destruction here
	// output_fd will be handled by the output bitbuffer
	fclose(input_fd);
}

void i_coding_provider::decompress(FILE* input_fd, FILE* output_fd) {
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
	if((~(header & 1<<3)>>3 & 1) != get_type()) {
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
		const tree_node* node = decoding_lookup(prev, w);
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
	// bitbuffers will close the file descriptors
}
