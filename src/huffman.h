#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "coding.h"
#include "tree.h"

class huffman_table: public i_coding_provider {
	tree_node* huffman_tree;
	encoding_descriptor encoding_table[256];
	int counts[256];
	int total = 0;
public:
	huffman_table(int* counts);
	~huffman_table() override;
	bool empty();
	int get_total();
	int get_type() override;
	void print_table() override;
	void print_tree() override;
	encoding_descriptor& get_encoding(unsigned char prev, unsigned char c) override;
	void write_coding_table(FILE* output_fd) override;
private:
	void build_huffman_encoding_table(tree_node* node, encoding_descriptor& descriptor);
	void build();
};

#endif
