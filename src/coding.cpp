#include "coding.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

void encoding_descriptor::push_bit(int b) {
	if(length % 8 == 0) {
		encoding.push_back(b << 7);
	} else {
		encoding[length / 8] |= b << (8 - length % 8 - 1);
	}
	length++;
}

void encoding_descriptor::pop_bit() {
	assert(length > 0);
	length--;
	if((length + 7) / 8 < encoding.size())
		encoding.pop_back();
}

void encoding_descriptor::print() {
	for(int i = 0; i < length; i++) {
		printf("%d", (encoding[i / 8] >> (8 - i % 8 - 1)) & 1);
	}
}
