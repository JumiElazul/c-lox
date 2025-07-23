#ifndef JUMI_CLOX_BYTECODE_CHUNK_H
#define JUMI_CLOX_BYTECODE_CHUNK_H
#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} opcode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    value_array constants;
} bytecode_chunk;

void init_bytecode_chunk(bytecode_chunk* chunk);
void free_bytecode_chunk(bytecode_chunk* chunk);
void write_to_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte);
int add_constant(bytecode_chunk* chunk, value val);

#endif
