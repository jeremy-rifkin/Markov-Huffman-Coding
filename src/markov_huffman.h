#ifndef MARKOV_HUFFMAN_H
#define MARKOV_HUFFMAN_H

#include "coding.h"
#include "huffman.h"

class markov_huffman_table: public i_coding_provider {
	huffman_table* tables;
	int totals[256];
public:
	markov_huffman_table(int* counts);
	~markov_huffman_table() override;
	void print_table() override;
	void print_tree() override;
	encoding_descriptor& get_encoding(unsigned char prev, unsigned char c) override;
};

#endif
