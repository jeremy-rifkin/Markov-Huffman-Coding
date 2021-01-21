#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <functional>

#include "min_pq.h"

#define null NULL

#define BUFFER_SIZE 32768

void print_counts(int* counts) {
	for(int i = 0; i < 256; i++) {
		if(counts[i]) {
			printf("%c %d\n", i, counts[i]);
		}
	}
}

void print_help() {
	printf("markov-huffman <input> [-o output] [options]\n");
	printf("\t-o output_file\n");
	printf("\t-e encoding_file\n");
	printf("\t-d output_encoding_file\n");
	printf("\t-g debug / demo encoding\n");
	printf("\t-h just use naive huffman\n");
	printf("\t-x extract\n");
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
		left = null;
		right = null;
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
		if(node == null) return -1;
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

class i_coding_provider {
public:
	//virtual int get_type() = 0;
	virtual void print_table() = 0;
	virtual void print_tree() = 0;
	//virtual void get_encoding() = 0;
	//virtual void decode() = 0;
};

class huffman_table: public i_coding_provider {
	tree_node* huffman_tree;
	encoding_descriptor encoding_table[256];
	int counts[256];
	int total = 0;
public:
	huffman_table(int* counts) : huffman_tree { null } {
		for(int i = 0; i < 256; i++) {
			this->counts[i] = counts[i];
			total += counts[i];
		}
		build();
	}
	[[deprecated]] void increment(int c) {
		counts[c]++;
		total++;
	}
	bool empty() {
		return total == 0;
	}
	int get_total() {
		return total;
	}
	virtual void print_table() override {
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
	virtual void print_tree() override {
		huffman_tree->print();
	}
private:
	void build_huffman_encoding_table(tree_node* node, encoding_descriptor& descriptor) {
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
	void build() {
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
			q.insert(a->weight + b->weight, new tree_node { a, b });
		}
		huffman_tree = q.pop_min();
		// edge case where the tree has height 0
		if(!huffman_tree->is_internal) {
			// TODO: weights
			huffman_tree->left  = new tree_node { null, null, false, huffman_tree->value, huffman_tree->weight };
			//huffman_tree->right = new tree_node { null, null, false, huffman_tree->value, 0 };
			huffman_tree->is_internal = true;
		}
		// recursive builder will continuously update this descriptor to build the tree
		encoding_descriptor working_descriptor;
		build_huffman_encoding_table(huffman_tree, working_descriptor);
		//huffman_tree->print();
		//print_encoding_table();
	}
};

class markov_huffman_table: public i_coding_provider {
	huffman_table** tables;
	int totals[256];
public:
	markov_huffman_table(int* counts) {
		tables = new huffman_table*[256];
		for(int i = 0; i < 256; i++) {
			tables[i] = new huffman_table(counts + 256 * i);
			totals[i] = tables[i]->get_total();
		}
	}
	virtual void print_table() override {
		for(int i = 0; i < 256; i++) {
			printf("Prev '%c' table:\n", i);
			tables[i]->print_table();
		}
	}
	virtual void print_tree() override {
		// TODO: subgraphs.
		for(int i = 0; i < 256; i++) {
			printf("Prev '%c' table:\n", i);
			tables[i]->print_table();
		}
	}
};

void construct_table(FILE* input_fd, std::function<void(unsigned char, unsigned char)> counter) {
	size_t bytes_read;
	unsigned char buffer[BUFFER_SIZE];
	int prev = ' ';
	while(bytes_read = fread(buffer, 1, BUFFER_SIZE, input_fd)) {
		for(int i = 0; i < bytes_read; i++) {
			counter(prev, buffer[i]);
			prev = buffer[i];
		}
	}
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		print_help();
		return 1;
	}
	// Parameters
	bool extract = false;
	bool debug = false;
	bool simple_huffman = false;
	char* input = null;
	char* encoding = null;
	char* output = null;
	char* encoding_output = null;
	// Process arguments
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			int chomp = 0;
			for(int j = 0; argv[i][++j] != 0x0; )
				switch(argv[i][j]) {
					case 'o':
						if(i + 1 < argc) {
							output = argv[i + chomp++ + 1];
						} else {
							fprintf(stderr, "[Error] Expected output file following -o.\n");
						}
						break;
					case 'e':
						if(i + 1 < argc) {
							encoding = argv[i + chomp++ + 1];
						} else {
							fprintf(stderr, "[Error] Expected encoding file following -e.\n");
						}
						break;
					case 'd':
						if(i + 1 < argc) {
							encoding_output = argv[i + chomp++ + 1];
						} else {
							fprintf(stderr, "[Error] Expected encoding output file following -d.\n");
						}
						break;
					case 'x':
						extract = true;
						break;
					case 'h':
						simple_huffman = true;
						break;
					case 'g':
						debug = true;
						break;
					default:
						fprintf(stderr, "[Warning] Unknown option %c\n", argv[i][j]);
				}
			i += chomp;
		} else {
			if(input == null) {
				input = argv[i];
			} else {
				fprintf(stderr, "[Warning] Unknown positional argument %s\n", argv[i]);
			}
		}
	}

	if(input == null) {
		printf("error must provide input\n");
		exit(1);
	}

	FILE* input_fd = fopen(input, "r");
	if(input_fd == null) {
		printf("error opening input errno %s\n", strerror(errno));
		exit(1);
	}

	// file needs to indicate whether it has the encoding embedded (todo: good idea or not??)
	// file needs to indicate whether it is naive huffman or fancy huffman
	i_coding_provider* coder = null;
	if(encoding) {
		// load encoding
		fprintf(stderr, "--error--\n");
	} else {
		// build encoding
		if(simple_huffman) {
			int counts[256];
			for(int i = 0; i < 256; i++) counts[i] = 0;
			construct_table(input_fd, [&](unsigned char prev, unsigned char c) {
				counts[c]++;
			});
			coder = new huffman_table(counts);
		} else {
			int* counts = new int[256 * 256];
			for(int i = 0; i < 256 * 256; i++) counts[i] = 0;
			construct_table(input_fd, [&](unsigned char prev, unsigned char c) {
				counts[256 * prev + c]++;
			});
			coder = new markov_huffman_table(counts);
		}
		fseek(input_fd, 0, SEEK_SET);
	}

	if(debug) {
		coder->print_table();
		coder->print_tree();
	}

	if(extract) {
		fprintf(stderr, "--error--\n");
	} else {
		if(debug) {

		} else {
			
		}
	}

	/*char* input = "input_a.txt";
	FILE* input_fd = fopen(input, "r");
	
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

	fclose(input_fd);*/
}

