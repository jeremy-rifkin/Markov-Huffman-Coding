#include "markov_huffman.h"
#include <stdio.h>

#include "coding.h"
#include "huffman.h"

markov_huffman_table::markov_huffman_table(int* counts) {
	tables = new huffman_table*[256];
	for(int i = 0; i < 256; i++) {
		tables[i] = new huffman_table(counts + 256 * i);
		totals[i] = tables[i]->get_total();
	}
}

void markov_huffman_table::print_table() {
	for(int i = 0; i < 256; i++) {
		printf("Prev '%c' table:\n", i);
		tables[i]->print_table();
	}
}

void markov_huffman_table::print_tree() {
	// TODO: subgraphs.
	for(int i = 0; i < 256; i++) {
		printf("Prev '%c' table:\n", i);
		tables[i]->print_table();
	}
}
