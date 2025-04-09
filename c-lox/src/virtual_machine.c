#include "virtual_machine.h"
#include "bytecode_chunk.h"
#include "disassembler.h"
#include <stdio.h>

virtual_machine vm;

static void reset_stack(void) {
    vm.stack_top = vm.stack;
}

void init_virtual_machine(void) {
    reset_stack();
}

void free_virtual_machine(void) {

}

static interpret_result virtual_machine_run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.current_chunk->constants.values[READ_BYTE()])

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("        ");
        for (value* slot = vm.stack; slot < vm.stack_top; ++slot) {
            printf("[");
            print_value(*slot);
            printf("]");
        }
        printf("\n");
        disassemble_instruction(vm.current_chunk, (int)(vm.ip - vm.current_chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                value constant = READ_CONSTANT();
                virtual_machine_stack_push(constant);
                printf("\n");
            } break;
            case OP_RETURN: {
                print_value(virtual_machine_stack_pop());
                printf("\n");
                return INTERPRET_OK;
            } break;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

interpret_result virtual_machine_interpret(bytecode_chunk* chunk) {
    vm.current_chunk = chunk;
    vm.ip = vm.current_chunk->code;
    return virtual_machine_run();
}

void virtual_machine_stack_push(value val) {
    *vm.stack_top = val;
    ++vm.stack_top;
}

value virtual_machine_stack_pop(void) {
    --vm.stack_top;
    return *vm.stack_top;
}
