#include "bytecode_chunk.h"
#include "memory.h"
#include "value.h"

// Initialize the chunk
void init_bytecode_chunk(bytecode_chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    init_value_array(&chunk->constants);
}

// Free the
void free_bytecode_chunk(bytecode_chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    free_value_array(&chunk->constants);
    init_bytecode_chunk(chunk);
}

// Write a single byte to the chunk, growing the storage if necessary.
void write_to_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte) {
    if (chunk->count >= chunk->capacity) {
        // The count has matched the capacity and we need to grow the dynamic array.
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count++] = byte;
}

// Adds a value to the chunk's constant table.
// Returns the index where the constant was added.
int add_constant_to_chunk(bytecode_chunk* chunk, value val) {
    write_value_array(&chunk->constants, val);
    return chunk->constants.count - 1;
}
