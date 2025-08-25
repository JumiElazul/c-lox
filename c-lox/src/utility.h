#ifndef JUMI_CLOX_UTILITY_H
#define JUMI_CLOX_UTILITY_H
#include "common.h"

bool file_exists(const char* path);
char* read_file(const char* path);
void create_file(const char* path);
void append_file(const char* path, const char* app);

#endif
