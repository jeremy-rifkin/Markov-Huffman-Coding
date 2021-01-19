#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "min_pq.h"

#define BUFFER_SIZE 32768

void print_counts(int* counts) {
	for(int i = 0; i < 256; i++) {
		if(counts[i]) {
			printf("%c %d\n", i, counts[i]);
		}
	}
}

struct tree_node {
	tree_node* left;
	tree_node* right;
	bool is_internal;
	unsigned char value;
	int weight;
public:
	tree_node(tree_node* left, tree_node* right, bool is_internal, unsigned char value, int weight):
		left(left), right(right), is_internal(is_internal), value(value), weight(weight) {};
	tree_node() {
		is_internal = false;
		left = NULL;
		right = NULL;
	}
	tree_node(tree_node* l, tree_node* r) {
		is_internal = true;
		left = l;
		right = r;
		weight = l->weight + r->weight;
	}
	tree_node(unsigned char v, int w): tree_node() {
		is_internal = false;
		value = v;
		weight = w;
	}
	~tree_node() {
		delete left;
		delete right;
	}
	void print() {
		printf("graph G {\n");
		printf("\tnodesep=0.3;\n");
		printf("\tranksep=0.2;\n");
		printf("\tnode [shape=circle, fixedsize=true];\n");
		printf("\tedge [arrowsize=0.8];\n");
		//int counter = 0;
		print_nodes(this, 0);
		printf("}\n");
	}
private:
	// passed the next node number
	// returns the node number left off at
	int print_nodes(tree_node* node, int n) {
		if(node == NULL) return -1;
		printf("\tn%d;\n", n);
		if(node->is_internal) {
			printf("\tn%d [label=\"%d\"];\n", n, node->weight);
		} else {
			printf("\tn%d [label=\"%c %d\"];\n", n, node->value, node->weight);
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
};

struct encoding_descriptor {
	int length;
	std::vector<unsigned char> encoding;
	encoding_descriptor(): length { 0 } {};
	void push_bit(int b) {
		if(length % 8 == 0) {
			encoding.push_back(b << 7);
		} else {
			encoding[length / 8] |= b << (8 - length % 8 - 1);
		}
		length++;
	}
	void pop_bit() {
		assert(length > 0);
		length--;
		if((length + 7) / 8 < encoding.size())
			encoding.pop_back();
	}
};

class huffman_table {
	tree_node* huffman_tree;
	encoding_descriptor encoding_table[256];
	int counts[256];
	int total = 0;
public:
	huffman_table() : huffman_tree { NULL } {
		for(int i = 0; i < 256; i++) {
			counts[i] = 0;
		}
	}
	void increment(int c) {
		counts[c]++;
		total++;
	}
	bool empty() {
		return total == 0;
	}
	void build() {
		if(empty()) {
			return;
		}
		// build huffman tree from the counts
		min_pq<int, tree_node*> q;
		for(int i = 0; i < 256; i++) {
			if(counts[i]) {
				q.insert(counts[i], new tree_node { i, counts[i] });
			}
		}
		while(q.size() > 1) {
			tree_node* a = q.pop_min();
			tree_node* b = q.pop_min();
			q.insert(a->weight + b->weight, new tree_node { a, b });
		}
		huffman_tree = q.pop_min();
		// edge case where the tree has height 0
		if(!huffman_tree->is_internal) {
			// TODO: weights
			huffman_tree->left  = new tree_node { NULL, NULL, false, huffman_tree->value, huffman_tree->weight };
			//huffman_tree->right = new tree_node { NULL, NULL, false, huffman_tree->value, 0 };
			huffman_tree->is_internal = true;
		}
		// recursive builder will continuously update this descriptor to build the tree
		encoding_descriptor working_descriptor;
		build_huffman_encoding_table(huffman_tree, working_descriptor);
		huffman_tree->print();
		print_encoding_table();
	}
	void rebuild() {
		delete huffman_tree;
		build();
	}
	void print_encoding_table() {
		printf("Table:\n");
		for(int i = 0; i < 256; i++) {
			if(encoding_table[i].length) {
				printf("%c %d ", i, encoding_table[i].length);
				for(int j = 0; j < encoding_table[i].length; j++) {
					//int mask = 1 << (8 - j % 8 - 1);
					//printf("%d", encoding_table[i].encoding[j / 8] & mask);
					printf("%d", (encoding_table[i].encoding[j / 8] >> (8 - j % 8 - 1)) & 1);
				}
				printf("\n");
			}
		}
	}
private:
	void build_huffman_encoding_table(tree_node* node, encoding_descriptor& descriptor) {
		if(node == NULL) return;
		if(node->is_internal) {
			descriptor.push_bit(0);
			build_huffman_encoding_table(node->left, descriptor);
			descriptor.pop_bit();
			descriptor.push_bit(1);
			build_huffman_encoding_table(node->right, descriptor);
			descriptor.pop_bit();
		} else {
			encoding_table[node->value] = descriptor;
		}
	}
};

class markov_huffman_table {
	huffman_table* tables;
public:
	markov_huffman_table() {
		tables = new huffman_table[256];
	}
	void increment(int c, int prev) {
		tables[prev].increment(c);
	}
	void build() {
		for(int i = 0; i < 256; i++) {
			if(!tables[i].empty())
				printf("[[building '%c']]\n", i);
			tables[i].build();
		}
	}
};

int main() {
	char* input = "input_a.txt";
	FILE* input_fd = fopen(input, "r");
	size_t bytes_read;
	unsigned char buffer[BUFFER_SIZE];
	printf("%d %d\n", sizeof(huffman_table), sizeof(markov_huffman_table));
	huffman_table h_table;
	markov_huffman_table mh_table;
	int prev = ' ';
	while(bytes_read = fread(buffer, 1, BUFFER_SIZE, input_fd)) {
		for(int i = 0; i < bytes_read; i++) {
			h_table.increment(buffer[i]);
			mh_table.increment(buffer[i], prev);
			prev = buffer[i];
		}
	}
	h_table.build();
	mh_table.build();
	//print_counts(counts);
	printf("---\n");
	//h_table.print_encoding_table();

	fseek(input_fd, 0, SEEK_SET);

	//std::vector<unsigned char> data;
	//while(bytes_read = fread(buffer, 1, BUFFER_SIZE, input_fd)) {
	//	for(int i = 0; i < bytes_read; i++) {
	//		// get codeword
	//		tree_node* node = huffman_tree;
	//	}
	//}

	fclose(input_fd);
}

