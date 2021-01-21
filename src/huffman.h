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
	[[deprecated]] void increment(int c);
	bool empty();
	int get_total();
	virtual void print_table() override;
	virtual void print_tree() override;
private:
	void build_huffman_encoding_table(tree_node* node, encoding_descriptor& descriptor);
	void build();
};

#endif
