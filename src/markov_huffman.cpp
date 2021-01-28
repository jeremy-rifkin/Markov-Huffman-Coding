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
	// pop leading indicator bit
	buffer.pop();
	// load trees
	unsigned char i = 0; // current prev symbol
	while(buffer.remaining() >= 8) {
		if(buffer.pop()) {
			tables[i] = huffman_table(buffer);
		}
		// else: no action required
		i++;
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
 * start with a 1.
 * The leading 1 will be followed by 256 entries of the following form:
 *  [0]              : empty tree
 *  [1][huffman tree]: tree with entries
 *
 * Previously we stored trees with the file format [1]{[8-bit symbol][huffman tree]}+ where the
 * 8-bit symbol was the previous character for the tree. This method had a maximum overhead of 256
 * bytes for the symbols.
 * Storing all 256 trees regardless with a 1-bit header for each indicating full/empty status
 * results in a constant 32 byte overhead, which is better in most cases.
 *
 * TODO: There's an edge case where every tree is empty. Currently empty files are not handled...
 *
 * Don't need to store the partial byte in the huffman tree dump because the minimum huffman
 * tree size > 8 bytes. We can just stop at the first partial byte.
 *
 */

void markov_huffman_table::write_coding_tree(bitbuffer& buffer) {
	buffer.push(1);
	for(int i = 0; i < 256; i++) {
		buffer.push(!tables[i].empty());
		if(!tables[i].empty()) {
			tables[i].write_coding_tree(buffer);
		}
	}
}
