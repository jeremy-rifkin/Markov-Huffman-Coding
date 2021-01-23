#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define null 0

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

// Returns a printable version of a character.
char charv(char c);

// Cross-platform wrapper to check for read/write (specified by the write parameter) access to a
// file.
void check_access(const char* path, bool write);

#endif
