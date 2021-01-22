#include "huffman.h"
#include <assert.h>
#include <stdio.h>

#include "coding.h"
#include "min_pq.h"
#include "tree.h"
#include "utils.h"

huffman_table::huffman_table(int* counts): huffman_tree { null } {
	for(int i = 0; i < 256; i++) {
		this->counts[i] = counts[i];
		total += counts[i];
	}
	build();
}

huffman_table::~huffman_table() {
	delete huffman_tree;
}

bool huffman_table::empty() {
	return total == 0;
}

int huffman_table::get_total() {
	return total;
}

int huffman_table::get_type() {
	return 0;
}

void huffman_table::print_table() {
	printf("Table:\n");
	for(int i = 0; i < 256; i++) {
		if(encoding_table[i].length) {
			printf("%c %d ", charv(i), encoding_table[i].length);
			encoding_table[i].print();
			printf("\n");
		}
	}
}

void huffman_table::print_tree() {
	huffman_tree->print();
}

encoding_descriptor& huffman_table::get_encoding(unsigned char prev, unsigned char c) {
	return encoding_table[c];
}

void huffman_table::write_coding_table(FILE* output_fd) {
	assert(fwrite(counts, sizeof(int), 256, output_fd) == 256);
}

const tree_node* huffman_table::get_decoding_tree(unsigned char) {
	return huffman_tree;
}

void huffman_table::build_huffman_encoding_table(tree_node* node, encoding_descriptor& descriptor) {
	if(node == null) return;
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

template<typename T> void swap(T& a, T& b) {
	T tmp = a;
	a = b;
	b = tmp;
}

void huffman_table::build() {
	if(empty()) {
		return;
	}
	// build huffman tree from the counts
	min_pq<int, tree_node*> q;
	for(int i = 0; i < 256; i++) {
		if(counts[i]) {
			q.insert(counts[i], new tree_node { (unsigned char) i, counts[i] });
		}
	}
	while(q.size() > 1) {
		tree_node* a = q.pop_min();
		tree_node* b = q.pop_min();
		// This is purely cosmetic and I should make a flag to turn it off.
		if(a->height > b->height) {
			swap(a, b);
		}
		q.insert(a->weight + b->weight, new tree_node { a, b });
	}
	huffman_tree = q.pop_min();
	// edge case where the tree has height 0
	if(!huffman_tree->is_internal) {
		// TODO: weights
		huffman_tree->left  = new tree_node { huffman_tree->value, huffman_tree->weight };
		// height will never be touched outside of this method but just in case
		huffman_tree->height = 1;
		//huffman_tree->right = new tree_node { null, null, false, huffman_tree->value, 0 };
		huffman_tree->is_internal = true;
	}
	// recursive builder will continuously update this descriptor to build the tree
	encoding_descriptor working_descriptor;
	build_huffman_encoding_table(huffman_tree, working_descriptor);
	//huffman_tree->print();
	//print_encoding_table();
}
