#include "bytecode_chunk.h"
#include "disassembler.h"
#include "virtual_machine.h"

int main(int argc, const char* argv[]) {
    init_virtual_machine();

    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int constant = add_constant_to_chunk(&chunk, 15.2);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 1);
    write_to_bytecode_chunk(&chunk, constant, 1);

    write_to_bytecode_chunk(&chunk, OP_RETURN, 2);

    disassemble_bytecode_chunk(&chunk, "test chunk");

    virtual_machine_interpret(&chunk);

    free_virtual_machine();
    free_bytecode_chunk(&chunk);
}
