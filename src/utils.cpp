#include "utils.h"

char charv(char c) {
	if(c > 32 && c < 127) {
		return c;
	} else {
		return ' ';
	}
}
