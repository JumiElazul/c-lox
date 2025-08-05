#include "bytecode_chunk.h"
#include "disassembler.h"
#include "file_io.h"
#include "virtual_machine.h"
#include <stdio.h>
#include <stdlib.h>

static void run_repl(void) {
    char line[1024];
    while (true) {
        printf("clox > ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        virtual_machine_interpret(line);
    }
}

static void run_file(const char* path) {
    char* source_code = read_file(path);
    interpret_result result = virtual_machine_interpret(source_code);
    free(source_code);

    if (result == INTERPRET_COMPILE_ERROR) {
        exit(EXIT_FAILURE);
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
        exit(EXIT_FAILURE);
    }
}

int main(int argc, const char* argv[]) {
    init_virtual_machine();

    if (argc == 1) {
        run_repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
    }

    // bytecode_chunk chunk;
    // init_bytecode_chunk(&chunk);

    // write_constant(&chunk, 1.2, 1);
    // write_constant(&chunk, 3.9, 2);
    // write_to_bytecode_chunk(&chunk, OP_ADD, 2);
    // write_constant(&chunk, 0.9, 3);
    // write_to_bytecode_chunk(&chunk, OP_ADD, 2);
    // write_to_bytecode_chunk(&chunk, OP_NEGATE, 3);
    // write_to_bytecode_chunk(&chunk, OP_NEGATE, 3);
    // write_constant(&chunk, 3, 3);
    // write_to_bytecode_chunk(&chunk, OP_MULTIPLY, 4);
    // write_constant(&chunk, 2, 4);
    // write_to_bytecode_chunk(&chunk, OP_DIVIDE, 4);
    // write_to_bytecode_chunk(&chunk, OP_RETURN, 5);

    // disassemble_chunk(&chunk, "test chunk");

    // virtual_machine_interpret(&chunk);

    // free_bytecode_chunk(&chunk);
    // free_virtual_machine();
    return 0;
}
