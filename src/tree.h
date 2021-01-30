#ifndef TREE_H
#define TREE_H

#include <algorithm>
#include <string>

#include "utils.h"

struct tree_node {
	tree_node* left;
	tree_node* right;
	bool is_internal;
	unsigned char value;
	int weight;
	// height is used cosmetically
	int height;
	// but also indicates the codeword length
	int depth;
	tree_node(tree_node* l, tree_node* r):
		left(l),    right(r),    is_internal(true),  weight(l->weight + r->weight),
		height(std::max(l->height, r->height) + 1), depth(-1) {}
	tree_node(unsigned char v, int w):
		left(null), right(null), is_internal(false), value(v), weight(w), height(0), depth(-1) {}
	~tree_node() {
		delete left;
		delete right;
	}
	// print the tree in graphvis format
	void print() const;
	// print the tree in graphvis format as a subgraph with n = the next node index
	int print(bool subgraph, int n, const std::string& label) const;
private:
	int print_nodes(const tree_node* node, int n) const;
};

#endif
