#include "bytecode_chunk.h"
#include "memory.h"

void init_bytecode_chunk(bytecode_chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
}

void free_bytecode_chunk(bytecode_chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    init_bytecode_chunk(chunk);
}

void write_to_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte) {
    if (chunk->count >= chunk->capacity) {
        // The count has matched the capacity and we need to grow the dynamic array.
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count++] = byte;
}
