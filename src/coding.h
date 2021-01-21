#ifndef CODING_H
#define CODING_H

#include <vector>

struct encoding_descriptor {
	int length;
	std::vector<unsigned char> encoding;
	encoding_descriptor(): length { 0 } {};
	void push_bit(int b);
	void pop_bit();
};

class i_coding_provider {
public:
	//virtual int get_type() = 0;
	virtual void print_table() = 0;
	virtual void print_tree() = 0;
	//virtual void get_encoding() = 0;
	//virtual void decode() = 0;
};

#endif
