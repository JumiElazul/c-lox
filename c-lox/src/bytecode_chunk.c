#include "bytecode_chunk.h"
#include "common.h"
#include "memory.h"
#include "value.h"
#include <stdlib.h>

void init_bytecode_chunk(bytecode_chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_value_array(&chunk->constants);
}

void free_bytecode_chunk(bytecode_chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    free_value_array(&chunk->constants);
    init_bytecode_chunk(chunk);
}

void write_to_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte, int line) {
    if (chunk->count >= chunk->capacity) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    ++chunk->count;
}

int add_constant(bytecode_chunk* chunk, value val) {
    write_to_value_array(&chunk->constants, val);
    return chunk->constants.count - 1;
}
