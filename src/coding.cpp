#include "coding.h"
#include <assert.h>
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
