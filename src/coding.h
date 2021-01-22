#ifndef CODING_H
#define CODING_H

#include <stdio.h>
#include <vector>

#include "tree.h"

struct encoding_descriptor {
	int length;
	std::vector<unsigned char> encoding;
	encoding_descriptor(): length { 0 } {};
	void push_bit(int b);
	void pop_bit();
	void print();
};

class i_coding_provider {
public:
	virtual ~i_coding_provider() = default;
	// returns coder type
	// 0 for simple huffman
	// 1 for markov-huffman
	// TODO: just read the simple_huffman flag in main?
	virtual int get_type() = 0;
	virtual void print_table() = 0;
	virtual void print_tree() = 0;
	virtual encoding_descriptor& get_encoding(unsigned char prev, unsigned char c) = 0;
	virtual const tree_node* get_decoding_tree(unsigned char prev) = 0;
	virtual void write_coding_table(FILE* output_fd) = 0;
};

#endif
