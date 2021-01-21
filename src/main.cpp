#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <functional>

#include "coding.h"
#include "huffman.h"
#include "markov_huffman.h"
#include "min_pq.h"
#include "null.h"
#include "tree.h"

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

