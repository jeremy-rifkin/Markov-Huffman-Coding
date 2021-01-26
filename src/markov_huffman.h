#ifndef MARKOV_HUFFMAN_H
#define MARKOV_HUFFMAN_H

#include "coding.h"
#include "huffman.h"

class markov_huffman_table: public i_coding_provider {
	// TODO: maybe don't heap-allocate this? This class itself is always allocated on the heap.
	huffman_table* tables;
	markov_huffman_table(): tables(new huffman_table[256]) {}
public:
	markov_huffman_table(int* counts);
	markov_huffman_table(bitbuffer& buffer);
	~markov_huffman_table() override;
	markov_huffman_table(const markov_huffman_table& other) = delete;
	markov_huffman_table& operator=(const markov_huffman_table& other) = delete;
	markov_huffman_table(markov_huffman_table&& other) = delete;
	markov_huffman_table& operator=(markov_huffman_table&& other) = delete;
	int get_type() override;
	void print_table() override;
	void print_tree() override;
	encoding_descriptor& get_encoding(unsigned char prev, unsigned char c) override;
	const tree_node* get_decoding_tree(unsigned char prev) override;
	void write_coding_tree(bitbuffer& buffer) override;
};

#endif
