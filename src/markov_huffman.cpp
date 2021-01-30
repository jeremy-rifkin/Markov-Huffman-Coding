#include "markov_huffman.h"
#include <stdio.h>
#include <stdlib.h>

#include "coding.h"
#include "huffman.h"

markov_huffman_table::markov_huffman_table(int* counts) {
	for(int i = 0; i < 256; i++) {
		tables[i] = huffman_table(counts + 256 * i);
	}
}

markov_huffman_table::markov_huffman_table(bitbuffer& buffer) {
	// pop leading indicator bit
	buffer.pop_bit();
	// load trees
	for(int i = 0; i < 256; i++) {
		if(buffer.pop_bit()) {
			tables[i] = huffman_table(buffer);
		}
		// else: no action required
	}
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
	return tables[prev].get_encoding(prev, c);
}

const tree_node* markov_huffman_table::decoding_lookup(unsigned char prev, unsigned char c) {
	return tables[prev].decoding_lookup(prev, c);
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
 * Storing 1-bit headers for each prev value also makes it much easier to get away with not storing
 * the partial byte count without having to use a janky system for determining when to stop reading.
 *
 * TODO: There's an edge case where every tree is empty. Currently empty files are not handled...
 *
 */

void markov_huffman_table::write_coding_tree(bitbuffer& buffer) {
	buffer.push_bit(1);
	for(int i = 0; i < 256; i++) {
		buffer.push_bit(!tables[i].empty());
		if(!tables[i].empty()) {
			tables[i].write_coding_tree(buffer);
		}
	}
}
