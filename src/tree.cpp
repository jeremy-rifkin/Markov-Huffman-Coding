#include "tree.h"
#include <stdio.h>
#include <string>

#include "utils.h"

void tree_node::print() const {
	print(false, 0, "");
}

int tree_node::print(bool subgraph, int n, const std::string& label) const {
	if(subgraph) {
		printf("subgraph clusterG%d {\n", n);
		printf("\tlabel=\"%s\";\n", label.c_str());
		printf("\tcolor=invis;\n");
	} else {
		printf("graph G {\n");
	}
	printf("\tnodesep=0.3;\n");
	printf("\tranksep=0.2;\n");
	printf("\tnode [shape=circle, fixedsize=true];\n");
	printf("\tedge [arrowsize=0.8];\n");
	n = print_nodes(this, n) + 1;
	printf("}\n");
	return n;
}

// passed the next node number
// returns the node number left off at
int tree_node::print_nodes(const tree_node* node, int n) const {
	if(node == null) return -1;
	printf("\tn%d;\n", n);
	if(node->is_internal) {
		//printf("\tn%d [label=\"%d\"];\n", n, node->weight);
		printf("\tn%d [label=\"\"];\n", n);
	} else {
		printf("\tn%d [label=\"%s\"];\n", n, charv(node->value).c_str());
	}
	int next = n + 1;
	int ret = n;
	int lnode = print_nodes(node->left, next);
	if(lnode != -1) {
		printf("\tn%d -- n%d;\n", n, next);
		ret = lnode;
		next = lnode + 1;
	} // else, next = n + 1;
	int rnode = print_nodes(node->right, next);
	if(rnode != -1) {
		printf("\tn%d -- n%d;\n", n, next);
		ret = rnode;
	}
	return ret;
}

