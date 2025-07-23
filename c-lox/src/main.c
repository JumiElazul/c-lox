#include "bytecode_chunk.h"
#include "disassembler.h"
#include "virtual_machine.h"

int main(int argc, const char* argv[]) {
    init_virtual_machine();

    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    write_constant(&chunk, 1.2, 1);
    write_constant(&chunk, 3.9, 2);
    write_constant(&chunk, 0.5, 2);
    write_constant(&chunk, 25, 2);
    write_constant(&chunk, 420, 3);
    write_to_bytecode_chunk(&chunk, OP_RETURN, 4);

    disassemble_chunk(&chunk, "test chunk");

    virtual_machine_interpret(&chunk);

    free_bytecode_chunk(&chunk);
    free_virtual_machine();
    return 0;
}
