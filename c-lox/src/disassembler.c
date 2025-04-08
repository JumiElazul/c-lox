#include "disassembler.h"
#include "bytecode_chunk.h"
#include <stdio.h>

void disassemble_bytecode_chunk(bytecode_chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

static int simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int disassemble_instruction(bytecode_chunk* chunk, int offset) {
    printf("%06d ", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN: {
            return simple_instruction("OP_RETURN", offset);
        }
        default: {
            printf("Unknown opcode %d", instruction);
            return offset + 1;
        }
    }
}
