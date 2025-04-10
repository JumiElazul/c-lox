#include "bytecode_chunk.h"
#include "disassembler.h"
#include "virtual_machine.h"

int main(int argc, const char* argv[]) {
    init_virtual_machine();

    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    int constant1 = add_constant_to_chunk(&chunk, 1);
    int constant2 = add_constant_to_chunk(&chunk, 2);
    int constant3 = add_constant_to_chunk(&chunk, 3);

    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 53);
    write_to_bytecode_chunk(&chunk, constant1, 53);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 53);
    write_to_bytecode_chunk(&chunk, constant2, 53);
    write_to_bytecode_chunk(&chunk, OP_MULTIPLY, 53);
    write_to_bytecode_chunk(&chunk, OP_CONSTANT, 53);
    write_to_bytecode_chunk(&chunk, constant3, 53);
    write_to_bytecode_chunk(&chunk, OP_ADD, 54);

    write_to_bytecode_chunk(&chunk, OP_RETURN, 54);

    disassemble_bytecode_chunk(&chunk, "test chunk");

    virtual_machine_interpret(&chunk);

    free_virtual_machine();
    free_bytecode_chunk(&chunk);
}
