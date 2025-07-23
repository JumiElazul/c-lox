#include "bytecode_chunk.h"
#include "disassembler.h"
#include "common.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) {
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int constant = add_constant(&chunk, 1.2);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 123);
    write_to_bytecode_chunk(&chunk, constant, 123);
    write_to_bytecode_chunk(&chunk, OP_RETURN, 123);
    disassemble_chunk(&chunk, "test chunk");
    free_bytecode_chunk(&chunk);

    return 0;
}
