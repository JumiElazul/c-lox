#ifndef JUMI_CLOX_COMPILER_H
#define JUMI_CLOX_COMPILER_H
#include "bytecode_chunk.h"

bool compile(const char* source_code, bytecode_chunk* chunk);

#endif
