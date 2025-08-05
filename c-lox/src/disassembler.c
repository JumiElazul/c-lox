#include "disassembler.h"
#include "bytecode_chunk.h"
#include "clox_value.h"
#include <stdio.h>
#include <stdlib.h>

void disassemble_chunk(bytecode_chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

static int constant_instruction(const char* name, bytecode_chunk* chunk, int offset,
                                bool is_long_instr) {
    int constant;
    if (is_long_instr) {
        if (offset + 3 >= chunk->count) {
            printf("Truncated OP_CONSTANT_LONG at %d\n", offset);
            exit(EXIT_FAILURE);
        }

        uint8_t hi = chunk->code[offset + 1];
        uint8_t mid = chunk->code[offset + 2];
        uint8_t lo = chunk->code[offset + 3];
        constant = deconstruct_u24_t((u24_t){.hi = hi, .mid = mid, .lo = lo});
    } else {
        constant = chunk->code[offset + 1];
    }
    printf("%-16s %6d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return is_long_instr ? offset + 4 : offset + 2;
}

static int simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int disassemble_instruction(bytecode_chunk* chunk, int offset) {
    printf("%06d ", offset);

    int line = get_line(chunk, offset);
    if (offset > 0 && line == get_line(chunk, offset - 1)) {
        printf("     | ");
    } else {
        printf("%6d ", line);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset, false);
        case OP_CONSTANT_LONG:
            return constant_instruction("OP_CONSTANT_LONG", chunk, offset, true);
        case OP_ADD:
            return simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_instruction("OP_DIVIDE", offset);
        case OP_NEGATE:
            return simple_instruction("OP_NEGATE", offset);
        case OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        default: {
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
        }
    }
}
