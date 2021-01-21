#ifndef TREE_H
#define TREE_H

#include "null.h"

struct tree_node {
	tree_node* left;
	tree_node* right;
	bool is_internal;
	unsigned char value;
	int weight;
public:
	tree_node(tree_node* left, tree_node* right, bool is_internal, unsigned char value, int weight):
		left(left), right(right), is_internal(is_internal), value(value), weight(weight) {};
	tree_node():
		left(null), right(null), is_internal(false) {}
	tree_node(tree_node* l, tree_node* r):
		left(l),    right(r),    is_internal(true), weight(l->weight + r->weight) {}
	tree_node(unsigned char v, int w):
		left(null), right(null), is_internal(false), value(v), weight(w) {}
	~tree_node() {
		delete left;
		delete right;
	}
	void print();
private:
	int print_nodes(tree_node* node, int n);
};

#endif
