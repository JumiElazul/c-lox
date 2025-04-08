#include "disassembler.h"
#include "bytecode_chunk.h"
#include "value.h"
#include <stdio.h>

void disassemble_bytecode_chunk(bytecode_chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

// Just print the instruction name and return the index of the next byte
static int simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

// Get the next byte from the chunk, which holds the index of where to find the
// value in the constants table.
static int constant_instruction(const char* name, bytecode_chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-24s %6d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset+ + 2;
}

int disassemble_instruction(bytecode_chunk* chunk, int offset) {
    printf("%06d ", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT: {
            return constant_instruction("OP_CONSTANT", chunk, offset);
        }
        case OP_RETURN: {
            return simple_instruction("OP_RETURN", offset);
        }
        default: {
            printf("Unknown opcode %d", instruction);
            return offset + 1;
        }
    }
}
