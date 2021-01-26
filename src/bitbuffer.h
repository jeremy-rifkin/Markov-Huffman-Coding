#ifndef BITBUFFER_H
#define BITBUFFER_H

#include <stdio.h>
#include <vector>


// This is an abstraction for working with bitstreams and sub-byte data storage.
// This is a handy abstraction for use when reading/writing the encoding trees to files.
// TODO: Maybe rework the encode/decode logic to make use of this abstraction? This was written
// after the compression logic. Would need to make this buffer more robust to get the same
// performance.

// Note:
// - This is a unidirection buffer.
// - This buffer will panic if errors occur.
// - This buffer will read the entire contents of a file into memory.

class bitbuffer {
public:
	enum e_mode { read, write };
private:
	std::vector<unsigned char> data;
	int bi;
	FILE* file;
	e_mode mode;
public:
	bitbuffer(FILE* file, e_mode mode): bi(0), file(file), mode(mode) {
		if(mode == read) load();
	};
	void push(int b);
	void push_byte(unsigned char b);
	unsigned char peek();
	unsigned char pop();
	unsigned char pop_byte();
	int remaining();
	// NOTE: This flush will round up to the nearest byte
	void flush();
private:
	void load();
};

#endif
