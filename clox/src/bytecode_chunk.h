#ifndef CLOX_BYTECODE_CHUNK_H
#define CLOX_BYTECODE_CHUNK_H
#include "clox_value.h"
#include "common.h"

typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} opcode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    value_array constants;
} bytecode_chunk;

void init_bytecode_chunk(bytecode_chunk* chunk);
void free_bytecode_chunk(bytecode_chunk* chunk);
void write_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte, int line);
int add_constant(bytecode_chunk* chunk, clox_value value);

#endif
