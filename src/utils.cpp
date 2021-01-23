#include "utils.h"

#ifdef _WIN32
#include <io.h>
#elif __linux__
#include <unistd.h>
#else
#error "Unsupported platform."
#endif
#include "errno.h"
#include "string.h"

char charv(char c) {
	if(c > 32 && c < 127) {
		return c;
	} else {
		return ' ';
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
		write ? "writing" : "reading",
		strerror(errno));
}
