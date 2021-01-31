#ifndef CODING_H
#define CODING_H

#include <vector>

#include "bitbuffer.h"
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
	virtual void print_table() = 0;
	virtual void print_tree() = 0;
	virtual void write_coding_tree(bitbuffer& buffer) = 0;
	// compression/decompression logic common to all coders
	// this class isn't a "pure interface" but that's ok
	void compress(FILE* input_fd, FILE* output_fd);
	void decompress(FILE* input_fd, FILE* output_fd);
private:
	// returns coder type
	// 0 for simple huffman
	// 1 for markov-huffman
	virtual int get_type() = 0;
	virtual encoding_descriptor& get_encoding(unsigned char prev, unsigned char c) = 0;
	virtual const tree_node* decoding_lookup(unsigned char prev, unsigned char c) = 0;
};

#endif
