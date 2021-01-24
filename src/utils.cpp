#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#elif __linux__
#include <unistd.h>
#else
#error "Unsupported platform."
#endif

std::string charv(unsigned char c) {
	if(c > 32 && c < 127) {
		return {(char) c, 0};
	} else {
		switch(c) {
			case ' ':
				return "\\\\space";
			case '\t':
				return "\\\\t";
			case '\r':
				return "\\\\r";
			case '\n':
				return "\\\\n";
		}
		std::stringstream stream;
		stream<<"\\\\"<<std::hex<<(int)c;
		return stream.str();
	}
}

void check_access(const char* path, bool write) {
	if(
#ifdef _WIN32
	_access(path, write ? 2 : 4)
#else
	access(path, write ? W_OK : R_OK)
#endif
	== -1)
	eprintf("Error: Unable to open \"%s\" for %s; %s.",
	    path,
		write ? "writing" : "reading",
		strerror(errno));
}

int read_buffer(void* ptr, size_t size, size_t count, FILE* stream) {
	// make sure we don't have an error coming in
	assert(!ferror(stream));
	int r = fread(ptr, size, count, stream);
	if(r != count && ferror(stream)) {
		eprintf("Error occurred while reading file.\n");
		exit(1);
	}
	return r;
}

int write_buffer(void* ptr, size_t size, size_t count, FILE* stream) {
	// make sure we don't have an error coming in
	assert(!ferror(stream));
	int r = fwrite(ptr, size, count, stream);
	if(r != count && ferror(stream)) {
		eprintf("Error occurred while writing file.\n");
		exit(1);
	}
	return r;
}
