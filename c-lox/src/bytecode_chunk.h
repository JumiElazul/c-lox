#ifndef JUMI_CLOX_BYTECODE_CHUNK_H
#define JUMI_CLOX_BYTECODE_CHUNK_H
#include "common.h"
#include "value.h"

#define U24T_MAX 0xFFFFFFu

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
} opcode;

typedef struct {
    uint8_t hi;
    uint8_t mid;
    uint8_t lo;
} u24_t;

typedef struct {
    int line;
    int count;
} line_run;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;

    int lr_count;
    int lr_capacity;
    line_run* line_runs;

    value_array constants;
} bytecode_chunk;

void init_bytecode_chunk(bytecode_chunk* chunk);
void free_bytecode_chunk(bytecode_chunk* chunk);
u24_t construct_u24_t(int index);
int deconstruct_u24_t(u24_t format);
void write_to_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte, int line);
void write_constant(bytecode_chunk* chunk, value val, int line);
int get_line(bytecode_chunk* chunk, int index);

#endif
