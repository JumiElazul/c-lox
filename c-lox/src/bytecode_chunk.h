#ifndef JUMI_CLOX_BYTECODE_CHUNK_H
#define JUMI_CLOX_BYTECODE_CHUNK_H
#include "clox_value.h"

#define U24T_MAX 0xFFFFFFu

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,

    OP_DEBUG,
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
int add_constant(bytecode_chunk* chunk, clox_value val);
u24_t construct_u24_t(int index);
int deconstruct_u24_t(u24_t format);
void write_to_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte, int line);
int get_line(bytecode_chunk* chunk, int index);

#endif
