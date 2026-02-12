#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H
#include "bytecode_chunk.h"

bool compile(const char* source, bytecode_chunk* chunk);

#endif
