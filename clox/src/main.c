#include "bytecode_chunk.h"
#include "common.h"
#include "debug.h"

int main(int argc, char* argv[]) {
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int constant = add_constant(&chunk, 1.2);
    int constant2 = add_constant(&chunk, 8.4);
    write_bytecode_chunk(&chunk, OP_CONSTANT, 123);
    write_bytecode_chunk(&chunk, constant, 123);
    write_bytecode_chunk(&chunk, OP_CONSTANT, 123);
    write_bytecode_chunk(&chunk, constant2, 123);
    write_bytecode_chunk(&chunk, OP_RETURN, 124);

    disassemble_chunk(&chunk, "test chunk");

    free_bytecode_chunk(&chunk);
    return 0;
}
