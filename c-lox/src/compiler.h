#ifndef JUMI_CLOX_COMPILER_H
#define JUMI_CLOX_COMPILER_H
#include "bytecode_chunk.h"
#include "object.h"

obj_function* compile(const char* source);

#endif
