#include "virtual_machine.h"
#include "bytecode_chunk.h"
#include "clox_value.h"
#include "compiler.h"
#include "disassembler.h"
#include <stdarg.h>
#include <stdio.h>

virtual_machine vm;

static void reset_stack(void) { vm.stack_top = vm.stack; }

static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = get_line(vm.chunk, instruction);
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

static clox_value virtual_machine_stack_peek(int distance) { return vm.stack_top[-1 - distance]; }

void init_virtual_machine(void) { reset_stack(); }

void free_virtual_machine(void) {}

void virtual_machine_stack_push(clox_value val) {
    // Maybe handle stack overflow here.
    *vm.stack_top = val;
    ++vm.stack_top;
}

clox_value virtual_machine_stack_pop(void) {
    --vm.stack_top;
    return *vm.stack_top;
}

static interpret_result virtual_machine_run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op)                                                                  \
    do {                                                                                           \
        if (!IS_NUMBER(virtual_machine_stack_peek(0)) ||                                           \
            !IS_NUMBER(virtual_machine_stack_peek(1))) {                                           \
            runtime_error("Operands must be numbers.");                                            \
            return INTERPRET_RUNTIME_ERROR;                                                        \
        }                                                                                          \
        double b = AS_NUMBER(virtual_machine_stack_pop());                                         \
        double a = AS_NUMBER(virtual_machine_stack_pop());                                         \
        virtual_machine_stack_push(value_type(a op b));                                            \
    } while (false);

#ifdef DEBUG_TRACE_EXECUTION
    printf("== virtual machine ==\n");
#endif

    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("stack: ");
        for (clox_value* slot = vm.stack; slot < vm.stack_top; ++slot) {
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
                clox_value constant = READ_CONSTANT();
                virtual_machine_stack_push(constant);
                printf("\n");
            } break;
            case OP_CONSTANT_LONG: {
                u24_t u24_index;
                u24_index.hi = READ_BYTE();
                u24_index.mid = READ_BYTE();
                u24_index.lo = READ_BYTE();
                int reconstructed_index = deconstruct_u24_t(u24_index);
                virtual_machine_stack_push(vm.chunk->constants.values[reconstructed_index]);
                printf("\n");
            } break;
            case OP_NEGATE: {
                if (!IS_NUMBER(virtual_machine_stack_peek(0))) {
                    runtime_error("Operand must be a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                virtual_machine_stack_push(NUMBER_VALUE(-AS_NUMBER(virtual_machine_stack_pop())));
            } break;
            case OP_ADD: {
                BINARY_OP(NUMBER_VALUE, +);
            } break;
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VALUE, -);
            } break;
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VALUE, *);
            } break;
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VALUE, /);
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
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    if (!compile(source_code, &chunk)) {
        free_bytecode_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    interpret_result result = virtual_machine_run();
    free_bytecode_chunk(&chunk);

    return result;
}
