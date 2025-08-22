#ifndef JUMI_CLOX_COMPILER_H
#define JUMI_CLOX_COMPILER_H
#include "bytecode_chunk.h"
#include "clox_object.h"

object_function* compile(const char* source_code);

#endif
