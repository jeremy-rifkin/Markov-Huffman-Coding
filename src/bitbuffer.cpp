#include "bitbuffer.h"

#include <assert.h>
#include <stdio.h>
#include <vector>

#include "utils.h"

void bitbuffer::push(int b) {
	assert(b == b & 1);
	assert(mode == write);
	buffer[i] |= b << (7 - bi);
	bi++;
	if(bi == 8) {
		i++;
		bi = 0;
		check_flush();
	}
}

void bitbuffer::push_byte(unsigned char b) {
	assert(mode == write);
	// Previously used a loop:
	//unsigned char mask = ~((unsigned char)~0>>1);
	//while(mask) {
	//	push(b & mask ? 1 : 0);
	//	mask >>= 1;
	//}
	// Now using bitwise logic
	// Is this micro-optimization? The compiler does optimize this loop a lot but I don't think it's
	// micro-optimization.
	if(bi == 0) {
		buffer[i++] = b;
		check_flush();
	} else {
		buffer[i++] |= b >> bi;
		int _bi = bi;
		bi = 0;
		check_flush();
		buffer[i] |= b << (8 - _bi);
		bi = _bi;
	}
}

unsigned char bitbuffer::peek() {
	assert(mode == read);
	check_load();
	assert(i < bytes_read);
	return (buffer[i] >> (7 - bi)) & 1;
}

unsigned char bitbuffer::pop() {
	assert(mode == read);
	unsigned char b = peek();
	if(++bi == 8) {
		i++;
		bi = 0;
	}
	return b;
}

unsigned char bitbuffer::pop_byte() {
	assert(mode == read);
	//unsigned char b = 0;
	//unsigned char mask = ~((unsigned char)~0>>1);
	//while(mask) {
	//	if(pop())
	//		b |= mask;
	//	mask >>= 1;
	//}
	//return b;
	check_load();
	if(bi == 0) {
		return buffer[i++];
	} else {
		unsigned char b = 0;
		b |= buffer[i++] << bi;
		int _bi = bi;
		bi = 0;
		check_load();
		b |= buffer[i] >> (8 - _bi);
		bi = _bi;
		return b;
	}
}

void bitbuffer::check_load() {
	assert(mode == read);
	assert(i <= bytes_read);
	if(i == bytes_read) {
		load();
	}
}

void bitbuffer::load() {
	assert(mode == read);
	assert(!feof(file));
	bytes_read = read_buffer(buffer, 1, BUFFER_SIZE, file);
	i = 0;
	bi = 0;
}

void bitbuffer::check_flush() {
	assert(mode == write);
	assert(i <= BUFFER_SIZE);
	if(i == BUFFER_SIZE) {
		flush();
	}
}

void bitbuffer::flush() {
	assert(mode == write);
	// if the buffer is being flushed because it's full, there shouldn't be a partial byte
	assert(i != BUFFER_SIZE || bi == 0);
	// if there is a partial byte, round up
	if(bi) i++;
	write_buffer(buffer, 1, i, file);
	zero();
	i = 0;
	bi = 0;
}

void bitbuffer::zero() {
	for(int i = 0; i < BUFFER_SIZE; i++) {
		buffer[i] = 0;
	}
}
