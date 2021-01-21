#include "markov_huffman.h"
#include <stdio.h>
#include <stdlib.h>

#include "coding.h"
#include "huffman.h"

markov_huffman_table::markov_huffman_table(int* counts) {
	// todo: definitely want to rework the allocation/initialization
	// doing it this way because huffman_table doesn't have a default constructor
	tables = (huffman_table*) malloc(256 * sizeof(huffman_table));
	for(int i = 0; i < 256; i++) {
		new (tables + i) huffman_table(counts + 256 * i);
		totals[i] = tables[i].get_total();
	}
}

markov_huffman_table::~markov_huffman_table() {
	delete[] tables;
}

void markov_huffman_table::print_table() {
	for(int i = 0; i < 256; i++) {
		printf("Prev '%c' table:\n", i);
		tables[i].print_table();
	}
}

void markov_huffman_table::print_tree() {
	// TODO: subgraphs.
	for(int i = 0; i < 256; i++) {
		printf("Prev '%c' table:\n", i);
		tables[i].print_table();
	}
}

encoding_descriptor& markov_huffman_table::get_encoding(unsigned char prev, unsigned char c) {
	return tables[prev].get_encoding(0, c);
}
