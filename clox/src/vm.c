#include "vm.h"
#include "assert.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>

VM vm;

static void vm_reset_stack() {
    vm.sp = vm.stack;
}

void init_vm() {
    vm_reset_stack();
}

void free_vm() {
}

static interpret_result run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("        ");
        for (clox_value* slot = vm.stack; slot < vm.sp; ++slot) {
            printf("[ ");
            print_value(*slot);
            printf("]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                clox_value constant = READ_CONSTANT();
                vm_stack_push(constant);
            } break;
            case OP_RETURN: {
                print_value(vm_stack_pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

interpret_result interpret(bytecode_chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

void vm_stack_push(clox_value value) {
    assert(vm.sp < vm.stack + STACK_MAX && "== vm stack overflow ==");
    *vm.sp = value;
    vm.sp++;
}

clox_value vm_stack_pop() {
    assert(vm.sp > vm.stack && "== vm stack underflow ==");
    vm.sp--;
    return *vm.sp;
}
