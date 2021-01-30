#include "huffman.h"
#include <assert.h>
#include <stdio.h>
#include <string>

#include "coding.h"
#include "min_pq.h"
#include "tree.h"
#include "utils.h"

huffman_table::huffman_table(): huffman_tree(null) {
	for(int i = 0; i < 256; i++) {
		decoding_lookup_table[i] = 0;
	}
}

huffman_table::huffman_table(int* counts): huffman_table() {
	build(counts);
}

huffman_table::huffman_table(bitbuffer& buffer): huffman_table() {
	huffman_tree = build_tree_from_buffer(buffer);
	build_huffman_encoding_table();
}

huffman_table::~huffman_table() {
	delete huffman_tree;
}

huffman_table& huffman_table::operator=(huffman_table&& other) {
	if(this != &other) {
		// if this huffman tree is not null, it'll be cleaned up in other's destructor
		std::swap(huffman_tree, other.huffman_tree);
		// copy array contents
		for(int i = 0; i < 256; i++) {
			encoding_table[i] = other.encoding_table[i];
			decoding_lookup_table[i] = other.decoding_lookup_table[i];
		}
	}
	return *this;
}

bool huffman_table::empty() {
	return huffman_tree == null;
}

int huffman_table::get_type() {
	return 0;
}

void huffman_table::print_table() {
	printf("Table:\n");
	for(int i = 0; i < 256; i++) {
		if(encoding_table[i].length) {
			printf("%s %d ", charv(i).c_str(), encoding_table[i].length);
			encoding_table[i].print();
			printf("\n");
		}
	}
}

void huffman_table::print_tree() {
	huffman_tree->print();
}

int huffman_table::print_tree(bool subgraph, int n, const std::string& label) {
	return huffman_tree->print(subgraph, n, label);
}

encoding_descriptor& huffman_table::get_encoding(unsigned char, unsigned char c) {
	return encoding_table[c];
}

/*
 * Output file format:
 * - Traverse the tree
 * - If an internal node is reached, output a 0
 * - If an external node is reached, output a 1 followed by the 8 bit value
 *
 */

void huffman_table::write_coding_tree(bitbuffer& buffer) {
	write_coding_tree_traversal(huffman_tree, buffer);
}

const tree_node* huffman_table::decoding_lookup(unsigned char, unsigned char c) {
	return decoding_lookup_table[c];
}

void huffman_table::build_huffman_encoding_table() {
	// Recursive builder will continuously update this descriptor while traversing the tree
	encoding_descriptor working_descriptor;
	build_huffman_encoding_table(huffman_tree, working_descriptor, 0);
}

void huffman_table::build_huffman_encoding_table(tree_node* node, encoding_descriptor& descriptor, int depth) {
	// This function does two things:
	// - Populates the encoding table.
	// - Builds the decoding lookup table.
	// Note: height should always be equal to the descriptor length
	if(node == null) return;
	node->depth = depth;
	if(node->is_internal) {
		descriptor.push_bit(0);
		build_huffman_encoding_table(node->left, descriptor, depth + 1);
		descriptor.pop_bit();
		descriptor.push_bit(1);
		build_huffman_encoding_table(node->right, descriptor, depth + 1);
		descriptor.pop_bit();
		if(depth == 8) {
			decoding_lookup_table[descriptor.encoding[0]] = node;
		}
	} else {
		encoding_table[node->value] = descriptor;
		if(depth <= 8) {
			unsigned char codeword = descriptor.encoding[0];
			for(int i = 0; i < 1 << 8 - depth; i++) {
				decoding_lookup_table[codeword + i] = node;
			}
		}
	}
}

template<typename T> void swap(T& a, T& b) {
	T tmp = a;
	a = b;
	b = tmp;
}

void huffman_table::build(int* counts) {
	// build huffman tree from the counts
	min_pq<int, tree_node*> q;
	for(int i = 0; i < 256; i++) {
		if(counts[i]) {
			q.insert(counts[i], new tree_node { (unsigned char) i, counts[i] });
		}
	}
	// if no character has counts, we're empty
	if(q.empty()) {
		return;
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
		// TODO: weights?
		// This is a hack to make encoding/decoding and tree serializing easy
		huffman_tree->left  = new tree_node { huffman_tree->value, huffman_tree->weight };
		huffman_tree->right = new tree_node { huffman_tree->value, huffman_tree->weight };
		// height will never be touched outside of this method but just in case
		huffman_tree->height = 1;
		huffman_tree->is_internal = true;
	}
	build_huffman_encoding_table();
}

tree_node* huffman_table::build_tree_from_buffer(bitbuffer& buffer) {
	if(buffer.pop_bit()) {
		return new tree_node(buffer.pop_byte(), 0);
	} else {
		return new tree_node(build_tree_from_buffer(buffer), build_tree_from_buffer(buffer));
	}
}

void huffman_table::write_coding_tree_traversal(tree_node* node, bitbuffer& buffer) {
	if(node == null) {
		return;
	}
	if(node->is_internal) {
		buffer.push_bit(0);
	} else {
		buffer.push_bit(1);
		buffer.push_byte(node->value);
	}
	if(node->left == null || node->right == null)
		assert(node->left == null && node->right == null);
	write_coding_tree_traversal(node->left, buffer);
	write_coding_tree_traversal(node->right, buffer);
}
