#include "markov_huffman.h"
#include <stdio.h>
#include <stdlib.h>

#include "coding.h"
#include "huffman.h"

markov_huffman_table::markov_huffman_table(int* counts): markov_huffman_table() {
	for(int i = 0; i < 256; i++) {
		tables[i] = huffman_table(counts + 256 * i);
	}
}

markov_huffman_table::markov_huffman_table(bitbuffer& buffer): markov_huffman_table() {
	while(buffer.remaining() >= 8) {
		unsigned char c = buffer.pop_byte();
		tables[c] = huffman_table(buffer);
	}
}

markov_huffman_table::~markov_huffman_table() {
	delete[] tables;
}

int markov_huffman_table::get_type() {
	return 1;
}

void markov_huffman_table::print_table() {
	for(int i = 0; i < 256; i++) {
		if(!tables[i].empty()) {
			printf("Prev '%s' table:\n", charv(i).c_str());
			tables[i].print_table();
		}
	}
}

void markov_huffman_table::print_tree() {
	printf("graph G {\n");
	printf("\tpackmode=\"cluster\";\n");
	for(int i = 0, n = 0; i < 256; i++) {
		if(!tables[i].empty()) {
			printf("/* Prev '%s' tree: */\n", charv(i).c_str());
			n = tables[i].print_tree(true, n, "Prev: " + charv(i));
		}
	}
	printf("}\n");
}

encoding_descriptor& markov_huffman_table::get_encoding(unsigned char prev, unsigned char c) {
	return tables[prev].get_encoding(0, c);
}

const tree_node* markov_huffman_table::get_decoding_tree(unsigned char prev) {
	return tables[prev].get_decoding_tree(0);
}

/*
 * Output file format:
 * Huffman tree files will always start with a 0 because of the tree root. Markov-huffman files will
 * start with a 1 followed by {(8 bit prev)(encoding tree)}* as many times as necessary.
 *
 * TODO: Currently we are just storing non-empty trees and putting an 8-bit header with the value
 * of prev. This is a maximum overhead of 256 bytes. It may be better to store every tree regardless
 * of being empty or not and just use a 1-bit header. Overhead would be a constant 32 bytes.
 *
 * Don't need to store the partial byte in the huffman tree dump because the minimum huffman
 * tree size > 8 bytes. We can just stop at the first partial byte.
 *
 */

void markov_huffman_table::write_coding_tree(bitbuffer& buffer) {
	buffer.push(1);
	for(int i = 0; i < 256; i++) {
		if(!tables[i].empty()) {
			buffer.push_byte((unsigned char) i);
			tables[i].write_coding_tree(buffer);
		}
	}
}
