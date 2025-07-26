#include "virtual_machine.h"
#include "bytecode_chunk.h"
#include "compiler.h"
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
#define BINARY_OP(op)                                                                              \
    do {                                                                                           \
        value b = virtual_machine_stack_pop();                                                     \
        value a = virtual_machine_stack_pop();                                                     \
        virtual_machine_stack_push(a op b);                                                        \
    } while (false);

#ifdef DEBUG_TRACE_EXECUTION
    printf("== virtual machine ==\n");
#endif

    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("stack: ");
        for (value* slot = vm.stack; slot < vm.stack_top; ++slot) {
            printf("[");
            print_value(*slot);
            printf("]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;

        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                value constant = READ_CONSTANT();
                virtual_machine_stack_push(constant);
                printf("\n");
            } break;
            case OP_CONSTANT_LONG: {
                u24_t u24_index;
                u24_index.hi = READ_BYTE();
                u24_index.mid = READ_BYTE();
                u24_index.lo = READ_BYTE();
                int reconstructed_index = construct_u24_t(u24_index);
                virtual_machine_stack_push(vm.chunk->constants.values[reconstructed_index]);
                printf("\n");
            } break;
            case OP_NEGATE: {
                *(vm.stack_top - 1) = -(*(vm.stack_top - 1));
            } break;
            case OP_ADD: {
                BINARY_OP(+);
            } break;
            case OP_SUBTRACT: {
                BINARY_OP(-);
            } break;
            case OP_MULTIPLY: {
                BINARY_OP(*);
            } break;
            case OP_DIVIDE: {
                BINARY_OP(/);
            } break;
            case OP_RETURN: {
                print_value(virtual_machine_stack_pop());
                printf("\n");
                return INTERPRET_OK;
            }
            default: {
                printf("Unknown opcode %d\n", instruction);
                return INTERPRET_RUNTIME_ERROR;
            } break;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

interpret_result virtual_machine_interpret(const char* source_code) {
    compile(source_code);
    return INTERPRET_OK;
}

void virtual_machine_stack_push(value val) {
    // Maybe handle stack overflow here.
    *vm.stack_top = val;
    ++vm.stack_top;
}

value virtual_machine_stack_pop(void) {
    --vm.stack_top;
    return *vm.stack_top;
}
