#include "bytecode_chunk.h"
#include "disassembler.h"
#include <stdio.h>

int main(int argc, const char* argv[]) {
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int constant = add_constant_to_chunk(&chunk, 15.0);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT);
    write_to_bytecode_chunk(&chunk, constant);

    write_to_bytecode_chunk(&chunk, OP_RETURN);

    disassemble_bytecode_chunk(&chunk, "test chunk");

    free_bytecode_chunk(&chunk);
}
