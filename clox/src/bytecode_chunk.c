#include "bytecode_chunk.h"
#include "clox_value.h"
#include "memory.h"
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

void write_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte, int line) {
    if (chunk->count >= chunk->capacity) {
        size_t old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int add_constant(bytecode_chunk* chunk, clox_value value) {
    // Do a simple linear scan to see if the value is already in the constant table, and return
    // the index if it is.  This will slow down compile time a bit but make us far less likely
    // to hit our current 255 constant limit per chunk.
    for (size_t index = 0; index < chunk->constants.count; ++index) {
        clox_value* entry = &chunk->constants.values[index];
        if (values_equal(*entry, value)) {
            return index;
        }
    }

    write_value_array(&chunk->constants, value);
    return (int)chunk->constants.count - 1;
}
