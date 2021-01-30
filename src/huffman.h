#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "bitbuffer.h"
#include "coding.h"
#include "tree.h"

class huffman_table: public i_coding_provider {
	tree_node* huffman_tree;
	encoding_descriptor encoding_table[256];
	tree_node* decoding_lookup_table[256];
public:
	huffman_table();
	huffman_table(int* counts);
	huffman_table(bitbuffer& buffer);
	~huffman_table() override;
	huffman_table(const huffman_table& other) = delete;
	huffman_table& operator=(const huffman_table& other) = delete;
	huffman_table(huffman_table&& other) = delete;
	huffman_table& operator=(huffman_table&& other);
	bool empty();
	int get_type() override;
	void print_table() override;
	void print_tree() override;
	int print_tree(bool subgraph, int n, const std::string& label);
	encoding_descriptor& get_encoding(unsigned char prev, unsigned char c) override;
	const tree_node* get_decoding_tree(unsigned char prev) override;
	const tree_node* decoding_lookup(unsigned char prev, unsigned char c) override;
	void write_coding_tree(bitbuffer& buffer) override;
private:
	void build_huffman_encoding_table();
	void build_huffman_encoding_table(tree_node* node, encoding_descriptor& descriptor, int depth);
	void build(int* counts);
	tree_node* build_tree_from_buffer(bitbuffer& buffer);
	void write_coding_tree_traversal(tree_node* node, bitbuffer& buffer);
};

#endif
