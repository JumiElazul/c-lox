#include "bytecode_chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char* argv[]) {
    init_vm();

    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int constant = add_constant(&chunk, 2.2);

    write_bytecode_chunk(&chunk, OP_CONSTANT, 123);
    write_bytecode_chunk(&chunk, constant, 123);

    constant = add_constant(&chunk, 3.4);

    write_bytecode_chunk(&chunk, OP_CONSTANT, 123);
    write_bytecode_chunk(&chunk, constant, 123);

    write_bytecode_chunk(&chunk, OP_ADD, 123);

    constant = add_constant(&chunk, 5.6);

    write_bytecode_chunk(&chunk, OP_CONSTANT, 123);
    write_bytecode_chunk(&chunk, constant, 123);
    write_bytecode_chunk(&chunk, OP_DIVIDE, 123);
    write_bytecode_chunk(&chunk, OP_NEGATE, 123);
    write_bytecode_chunk(&chunk, OP_RETURN, 123);

    interpret(&chunk);

    free_vm();
    free_bytecode_chunk(&chunk);
    return 0;
}
