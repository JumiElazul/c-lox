#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H
#include "bytecode_chunk.h"
#include "object.h"

object_function* compile(const char* source);

#endif
