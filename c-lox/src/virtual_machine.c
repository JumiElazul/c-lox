#include "virtual_machine.h"
#include "bytecode_chunk.h"
#include "disassembler.h"
#include "value.h"
#include <stdio.h>

virtual_machine vm;

static void reset_stack(void) { vm.stack_top = vm.stack; }

void init_virtual_machine(void) { reset_stack(); }

void free_virtual_machine(void) {}

static interpret_result virtual_machine_run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

#ifdef DEBUG_TRACE_EXECUTION
    printf("== virtual machine ==\n");
#endif

    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;

        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                value constant = READ_CONSTANT();
                print_value(constant);
                printf("\n");
            } break;
            case OP_CONSTANT_LONG: {
                u24_t u24_index;
                u24_index.hi = READ_BYTE();
                u24_index.mid = READ_BYTE();
                u24_index.lo = READ_BYTE();
                int reconstructed_index = construct_u24_t(u24_index);
                print_value(vm.chunk->constants.values[reconstructed_index]);
                printf("\n");
            } break;
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

interpret_result virtual_machine_interpret(bytecode_chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return virtual_machine_run();
}
