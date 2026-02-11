#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H
#include "bytecode_chunk.h"

void disassemble_chunk(bytecode_chunk* chunk, const char* name);
int disassemble_instruction(bytecode_chunk* chunk, int offset);

#endif
