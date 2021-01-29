#ifndef BITBUFFER_H
#define BITBUFFER_H

#include <stdio.h>
#include <vector>

#define BUFFER_SIZE 32768

// coding.h includes this file and we need the encoding_descriptor, so forward dec
class encoding_descriptor;

// This is an abstraction for working with bitstreams and sub-byte data storage.
// This is a handy abstraction for use when reading/writing the encoding trees to files.
// TODO: Maybe rework the encode/decode logic to make use of this abstraction? This was written
// after the compression logic. Would need to make this buffer more robust to get the same
// performance.

// Note:
// - This is a unidirection buffer.
// - This buffer will panic if errors occur.
// - Ownership of the file pointer is transferred into this buffer.

class bitbuffer {
public:
	enum e_mode { read, write };
private:
	// buffer contents are lazy-loaded in read mode
	unsigned char buffer[BUFFER_SIZE];
	int i;
	int bi;
	int bytes_read;
	FILE* file;
	e_mode mode;
public:
	bitbuffer(FILE* file, e_mode mode): i(0), bi(0), bytes_read(0), file(file), mode(mode) {
		if(mode == write)
			zero();
	}
	~bitbuffer() {
		if(mode == write)
			flush();
		if(file != stdout)
			fclose(file);
	}
	void push(int b); // TODO: Rename push_bit/pop_bit/peek_bit
	void push_byte(unsigned char b);
	void push_encoding_descriptor(encoding_descriptor& descriptor);
	unsigned char peek(); // TODO: Rename push_bit/pop_bit/peek_bit
	unsigned char pop(); // TODO: Rename push_bit/pop_bit/peek_bit
	unsigned char pop_byte();
	int get_bi();
	// NOTE: This flush will round up to the nearest byte
	void flush();
private:
	// loads data if buffer has been consumed
	void check_load();
	// loads data from file into the buffer
	void load();
	// flushes if the buffer is full
	void check_flush();
	// zeroes buffer
	void zero(int n = BUFFER_SIZE);
};

#endif
