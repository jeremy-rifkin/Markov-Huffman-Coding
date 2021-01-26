#include "bitbuffer.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

#include "utils.h"

#define BUFFER_SIZE 32768

void bitbuffer::push(int b) {
	assert(mode == write);
	if(bi % 8 == 0) {
		data.push_back((unsigned char)(b << 7));
	} else {
		data[bi / 8] |= b << (8 - bi % 8 - 1);
	}
	bi++;
}

// "surely the compiler will optimize this"
// TODO: I should probably implement this better...
void bitbuffer::push_byte(unsigned char b) {
	assert(mode == write);
	unsigned char mask = ~((unsigned char)~0>>1);
	while(mask) {
		push(b & mask ? 1 : 0);
		mask >>= 1;
	}
}

unsigned char bitbuffer::peek() {
	assert(mode == read);
	assert(bi < 8 * data.size());
	return (data[bi / 8] >> (7 - (bi % 8))) & 1;
}

unsigned char bitbuffer::pop() {
	unsigned char b;
	return b = peek(), bi++, b;
}

// "surely the compiler will optimize this"
// TODO: I should probably implement this better...
unsigned char bitbuffer::pop_byte() {
	assert(mode == read);
	// TODO: double-check boundary here or just rely on ::pop()'s boundary check?
	assert(bi < 8 * data.size() - 7);
	unsigned char b = 0;
	unsigned char mask = ~((unsigned char)~0>>1);
	while(mask) {
		if(pop())
			b |= mask;
		mask >>= 1;
	}
	return b;
}

int bitbuffer::remaining() {
	assert(mode == read);
	return 8 * data.size() - bi;
}

void bitbuffer::flush() {
	assert(mode == write);
	write_buffer(&data[0], 1, data.size(), file);
	data.clear();
	bi = 0;
}

void bitbuffer::load() {
	assert(mode == read);
	size_t bytes_read;
	unsigned char input_buffer[BUFFER_SIZE];
	while(bytes_read = read_buffer(input_buffer, 1, BUFFER_SIZE, file)) {
		int size_0 = data.size();
		data.reserve(size_0 + bytes_read);
		for(int i = 0; i < bytes_read; i++) {
			data.push_back(input_buffer[i]);
		}
	}
}
