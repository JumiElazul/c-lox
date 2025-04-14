#include "virtual_machine.h"
#include "bytecode_chunk.h"
#include "compiler.h"
#include "disassembler.h"
#include "object.h"
#include "value.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

virtual_machine vm;

static void reset_stack(void) {
    vm.stack_top = vm.stack;
}

static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.current_chunk->code - 1;
    int line = vm.current_chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

void init_virtual_machine(void) {
    reset_stack();
    vm.objects = NULL;
}

void free_virtual_machine(void) {
    free_objects();
}

void virtual_machine_stack_push(value val) {
    *vm.stack_top = val;
    ++vm.stack_top;
}

value virtual_machine_stack_pop(void) {
    --vm.stack_top;
    return *vm.stack_top;
}

static value virtual_machine_stack_peek(int distance) {
    return vm.stack_top[-1 - distance];
}

static bool is_falsey(value val) {
    // null and false are falsey, and everything else is truthy
    return IS_NULL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}

static void concatenate(void) {
    obj_string* b = AS_STRING(virtual_machine_stack_pop());
    obj_string* a = AS_STRING(virtual_machine_stack_pop());

    int length = a->length + b->length;
    char* buffer = ALLOCATE(char, length + 1);
    memcpy(buffer, a->chars, a->length);
    memcpy(buffer + a->length, b->chars, b->length);
    buffer[length] = '\0';

    obj_string* result = take_string(buffer, length);
    virtual_machine_stack_push(OBJ_VAL(result));
}

static interpret_result virtual_machine_run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.current_chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op)                          \
    do {                                                   \
        if (!IS_NUMBER(virtual_machine_stack_peek(0)) ||   \
            !IS_NUMBER(virtual_machine_stack_peek(1))) {   \
            runtime_error("Operands must be numbers.\n");  \
            return INTERPRET_RUNTIME_ERROR;                \
        }                                                  \
        double b = AS_NUMBER(virtual_machine_stack_pop()); \
        double a = AS_NUMBER(virtual_machine_stack_pop()); \
        virtual_machine_stack_push(value_type(a op b));    \
    } while (false)

#ifdef DEBUG_TRACE_EXECUTION
    printf("== vm runtime bytecode ==\n");
#endif

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("stack:  ");
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
            } break;
            case OP_NULL: {
                virtual_machine_stack_push(NULL_VAL);
            } break;
            case OP_TRUE: {
                virtual_machine_stack_push(BOOL_VAL(true));
            } break;
            case OP_FALSE: {
                virtual_machine_stack_push(BOOL_VAL(false));
            } break;
            case OP_EQUAL: {
                value b = virtual_machine_stack_pop();
                value a = virtual_machine_stack_pop();
                virtual_machine_stack_push(BOOL_VAL(values_equal(a, b)));
            } break;
            case OP_GREATER: {
                BINARY_OP(BOOL_VAL, >);
            } break;
            case OP_LESS: {
                BINARY_OP(BOOL_VAL, <);
            } break;
            case OP_ADD: {
                if (IS_STRING(virtual_machine_stack_peek(0)) && IS_STRING(virtual_machine_stack_peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(virtual_machine_stack_peek(0)) && IS_NUMBER(virtual_machine_stack_peek(1))) {
                    BINARY_OP(NUMBER_VAL, +);
                } else {
                    runtime_error("Operands of a '+' operator must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VAL, -);
            } break;
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VAL, *);
            } break;
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VAL, /);
            } break;
            case OP_NOT: {
                virtual_machine_stack_push(BOOL_VAL(is_falsey(virtual_machine_stack_pop())));
            } break;
            case OP_NEGATE: {
                if (!IS_NUMBER(virtual_machine_stack_peek(0))) {
                    runtime_error("Operand for unary negation '-' must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                virtual_machine_stack_push(NUMBER_VAL(-AS_NUMBER(virtual_machine_stack_pop())));
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

interpret_result virtual_machine_interpret(const char* source) {
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    if (!compile(source, &chunk)) {
        free_bytecode_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.current_chunk = &chunk;
    vm.ip = vm.current_chunk->code;

    interpret_result result = virtual_machine_run();
    free_bytecode_chunk(&chunk);
    return result;
}

