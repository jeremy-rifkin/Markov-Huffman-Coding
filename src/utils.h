#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string>

#define null 0

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

// Returns a printable representation of a character
// Returns the character if it's printable, otherwise an escape sequence.
std::string charv(unsigned char c);

// Cross-platform wrapper to check for read/write (specified by the write parameter) access to a
// file.
void check_access(const char* path, bool write);

// Shallow wrappers for fread and fwrite with builtin error handling
int read_buffer(void* ptr, size_t size, size_t count, FILE* stream);
int write_buffer(void* ptr, size_t size, size_t count, FILE* stream);

#endif
