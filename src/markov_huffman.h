#ifndef MARKOV_HUFFMAN_H
#define MARKOV_HUFFMAN_H

#include "bitbuffer.h"
#include "coding.h"
#include "huffman.h"
#include "tree.h"

class markov_huffman_table: public i_coding_provider {
	huffman_table tables[256];
public:
	markov_huffman_table(int* counts);
	markov_huffman_table(bitbuffer& buffer);
	markov_huffman_table(const markov_huffman_table& other) = delete;
	markov_huffman_table& operator=(const markov_huffman_table& other) = delete;
	markov_huffman_table(markov_huffman_table&& other) = delete;
	markov_huffman_table& operator=(markov_huffman_table&& other) = delete;
	int get_type() override;
	void print_table() override;
	void print_tree() override;
	encoding_descriptor& get_encoding(unsigned char prev, unsigned char c) override;
	const tree_node* decoding_lookup(unsigned char prev, unsigned char c) override;
	void write_coding_tree(bitbuffer& buffer) override;
};

#endif
