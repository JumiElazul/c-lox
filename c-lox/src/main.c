#include "bytecode_chunk.h"
#include "disassembler.h"

int main(int argc, const char* argv[]) {
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int constant = add_constant_to_chunk(&chunk, 15.2);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 1);
    write_to_bytecode_chunk(&chunk, constant, 1);

    write_to_bytecode_chunk(&chunk, OP_RETURN, 2);

    disassemble_bytecode_chunk(&chunk, "test chunk");

    free_bytecode_chunk(&chunk);
}
