#include "tree.h"
#include <stdio.h>

#include "utils.h"

void tree_node::print() {
	printf("graph G {\n");
	printf("\tnodesep=0.3;\n");
	printf("\tranksep=0.2;\n");
	printf("\tnode [shape=circle, fixedsize=true];\n");
	printf("\tedge [arrowsize=0.8];\n");
	print_nodes(this, 0);
	printf("}\n");
}

// passed the next node number
// returns the node number left off at
int tree_node::print_nodes(tree_node* node, int n) {
	if(node == null) return -1;
	printf("\tn%d;\n", n);
	if(node->is_internal) {
		printf("\tn%d [label=\"%d\"];\n", n, node->weight);
	} else {
		//printf("\tn%d [label=\"%c %d\"];\n", n, node->value, node->weight);
		printf("\tn%d [label=\"%c\"];\n", n, charv(node->value));
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

