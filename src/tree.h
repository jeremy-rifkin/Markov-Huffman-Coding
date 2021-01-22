#ifndef TREE_H
#define TREE_H

#include <algorithm>

#include "utils.h"

struct tree_node {
	tree_node* left;
	tree_node* right;
	bool is_internal;
	unsigned char value;
	int weight;
	// height is purely used cosmetically
	int height;
public:
	tree_node(tree_node* l, tree_node* r):
		left(l),    right(r),    is_internal(true),  weight(l->weight + r->weight),
		height(std::max(l->height, r->height) + 1) {}
	tree_node(unsigned char v, int w):
		left(null), right(null), is_internal(false), value(v), weight(w), height(0) {}
	~tree_node() {
		delete left;
		delete right;
	}
	void print();
private:
	int print_nodes(tree_node* node, int n);
};

#endif
