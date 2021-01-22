#include "markov_huffman.h"
#include <assert.h>
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

int markov_huffman_table::get_type() {
	return 1;
}

void markov_huffman_table::print_table() {
	for(int i = 0; i < 256; i++) {
		if(!tables[i].empty()) {
			printf("Prev '%c' table:\n", i);
			tables[i].print_table();
		}
	}
}

void markov_huffman_table::print_tree() {
	// TODO: subgraphs.
	for(int i = 0; i < 256; i++) {
		if(!tables[i].empty()) {
			printf("Prev '%c' tree:\n", i);
			tables[i].print_tree();
		}
	}
}

encoding_descriptor& markov_huffman_table::get_encoding(unsigned char prev, unsigned char c) {
	return tables[prev].get_encoding(0, c);
}

void markov_huffman_table::write_coding_table(FILE* output_fd) {
	for(int i = 0; i < 256; i++) {
		//unsigned char c = i;
		//assert(fwrite(&c, 1, 1, output_fd) == 1);
		tables[i].write_coding_table(output_fd);
	}
}
