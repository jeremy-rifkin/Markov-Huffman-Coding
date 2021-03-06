#include "bitbuffer.h"

#include <assert.h>
#include <stdio.h>

#include "coding.h"
#include "utils.h"

void bitbuffer::push_bit(int b) {
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

void bitbuffer::push_encoding_descriptor(encoding_descriptor& descriptor) {
	// note floor division
	for(int j = 0; j < descriptor.length / 8; j++) {
		push_byte(descriptor.encoding[j]);
	}
	// push partial byte
	if(descriptor.length % 8) {
		int w = descriptor.length % 8;
		unsigned char l = descriptor.encoding[descriptor.encoding.size() - 1];
		// 3 cases
		if(bi + w < 8) { // falls short
			buffer[i] |= l >> bi;
			bi += w;
		} else if(bi + w == 8) { // fits perfectly
			buffer[i++] |= l >> bi;
			bi = 0;
			check_flush();
		} else {
			// fill
			buffer[i++] |= l >> bi;
			int _bi = bi;
			bi = 0;
			check_flush();
			// insert and reset
			buffer[i] = l << (8 - _bi);
			bi = w - (8 - _bi);
		}
	}
}

unsigned char bitbuffer::peek_bit() {
	assert(mode == read);
	check_load();
	assert(i < bytes_read);
	return (buffer[i] >> (7 - bi)) & 1;
}

unsigned char bitbuffer::pop_bit() {
	assert(mode == read);
	unsigned char b = peek_bit();
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
		check_load();
		b |= buffer[i] >> (8 - _bi);
		bi = _bi;
		return b;
	}
}

unsigned char bitbuffer::try_pop_bit() {
	assert(mode == read);
	if(!feof(file))
		check_load();
	if(bytes_read == 0)
		assert(feof(file));
	if(i < bytes_read) {
		return pop_bit();
	} else {
		return 0;
	}
}

unsigned char bitbuffer::pop_rest(unsigned char byte, int byte_i) {
	// TODO: optimize this or rely on the compiler figuring it out?
	assert(mode == read);
	assert(byte_i < 8 && byte_i >= 0);
	unsigned char mask = 1 << (7 - byte_i);
	while(mask) {
		if(try_pop_bit())
			byte |= mask;
		mask >>= 1;
	}
	return byte;
}

int bitbuffer::get_bi() {
	return bi;
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
	zero(i);
	i = 0;
	bi = 0;
}

void bitbuffer::zero(int n) {
	for(int j = 0; j < n; j++) {
		buffer[j] = 0;
	}
}
