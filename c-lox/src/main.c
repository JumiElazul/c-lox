#include "bytecode_chunk.h"
#include "disassembler.h"
#include <stdio.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) {
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int c0 = add_constant(&chunk, 1.2);
    int c1 = add_constant(&chunk, 3.9);
    int c2 = add_constant(&chunk, 0.5);
    int c3 = add_constant(&chunk, 25);

    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 1);
    write_to_bytecode_chunk(&chunk, c0, 1);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 2);
    write_to_bytecode_chunk(&chunk, c1, 2);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 2);
    write_to_bytecode_chunk(&chunk, c2, 2);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 3);
    write_to_bytecode_chunk(&chunk, c3, 3);
    write_to_bytecode_chunk(&chunk, OP_RETURN, 4);

    disassemble_chunk(&chunk, "test chunk");
    free_bytecode_chunk(&chunk);

    return 0;
}
