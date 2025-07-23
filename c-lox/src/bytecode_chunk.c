#include "bytecode_chunk.h"
#include "common.h"
#include "memory.h"
#include "value.h"
#include <stdlib.h>

static void encode_line_run(bytecode_chunk* chunk, int line) {
    // Case 1: The line_run continues, increment the count.
    if (chunk->lr_count > 0) {
        int last_index = chunk->lr_count - 1;
        if (chunk->line_runs[last_index].line == line) {
            ++chunk->line_runs[last_index].count;
            return;
        }
    }

    // Case 2: Grow the array if needed, and create a new run.
    if (chunk->lr_count >= chunk->lr_capacity) {
        int old = chunk->lr_capacity;
        chunk->lr_capacity = GROW_CAPACITY(old);
        chunk->line_runs = GROW_ARRAY(line_run, chunk->line_runs, old, chunk->lr_capacity);
    }
    chunk->line_runs[chunk->lr_count++] = (line_run){ .line = line, .count = 1 };
}

void init_bytecode_chunk(bytecode_chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lr_count = 0;
    chunk->lr_capacity = 0;
    chunk->line_runs = NULL;
    init_value_array(&chunk->constants);
}

void free_bytecode_chunk(bytecode_chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(line_run, chunk->line_runs, chunk->lr_capacity);
    free_value_array(&chunk->constants);
    init_bytecode_chunk(chunk);
}

void write_to_bytecode_chunk(bytecode_chunk* chunk, uint8_t byte, int line) {
    if (chunk->count >= chunk->capacity) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    ++chunk->count;

    encode_line_run(chunk, line);
}

int add_constant(bytecode_chunk* chunk, value val) {
    write_to_value_array(&chunk->constants, val);
    return chunk->constants.count - 1;
}

int get_line(bytecode_chunk* chunk, int instr_index) {
    if (instr_index < 0 || instr_index >= chunk->count) {
        return -1;
    }

    int offset = 0;
    for (int i = 0; i < chunk->lr_count; ++i) {
        line_run* curr_run = &chunk->line_runs[i];

        if (instr_index < offset + curr_run->count) {
            return curr_run->line;
        }
        offset += curr_run->count;
    }

    return -1;
}
