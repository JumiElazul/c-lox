#include "virtual_machine.h"
#include "bytecode_chunk.h"
#include "common.h"
#include "compiler.h"
#include "disassembler.h"
#include "hash_table.h"
#include "object.h"
#include "memory.h"
#include "value.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

virtual_machine vm;

static void reset_stack(void) {
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
}

static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    call_frame* frame = &vm.frames[vm.frame_count - 1];
    size_t instruction = frame->ip - frame->function->chunk.code - 1;
    int line = frame->function->chunk.lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

void init_virtual_machine(void) {
    reset_stack();
    vm.objects = NULL;
    hash_table_init(&vm.global_variables);
    hash_table_init(&vm.interned_strings);
}

void free_virtual_machine(void) {
    hash_table_free(&vm.global_variables);
    hash_table_free(&vm.interned_strings);
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
    call_frame* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

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

    if (debug_trace_execution) {
        printf("== vm runtime bytecode ==\n");
    }

    for (;;) {
        if (debug_trace_execution) {
            printf("stack:  ");
            for (value* slot = vm.stack; slot < vm.stack_top; ++slot) {
                printf("[");
                print_value(*slot);
                printf("]");
            }
            printf("\n");
            disassemble_instruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
        }

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
            case OP_POP: {
                virtual_machine_stack_pop();
            } break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                virtual_machine_stack_push(frame->slots[slot]);
            } break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = virtual_machine_stack_peek(0);
            } break;
            case OP_GET_GLOBAL: {
                obj_string* name = READ_STRING();
                value val;
                if (!hash_table_get(&vm.global_variables, name, &val)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                virtual_machine_stack_push(val);
            } break;
            case OP_DEFINE_GLOBAL: {
                obj_string* name = READ_STRING();
                hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0));
                virtual_machine_stack_pop();
            } break;
            case OP_SET_GLOBAL: {
                obj_string* name = READ_STRING();
                if (hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0))) {
                    hash_table_delete(&vm.global_variables, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
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
            case OP_PRINT: {
                print_value(virtual_machine_stack_pop());
                printf("\n");
            } break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
            } break;
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (is_falsey(virtual_machine_stack_peek(0))) {
                    frame->ip += offset;
                }
            } break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
            } break;
            case OP_RETURN: {
                return INTERPRET_OK;
            } break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

// First we pass the source code to the compiler.  It returns us an obj_function* containing
// the compiled top level code.  If we get NULL, there was a reported compiler error.
// Otherwise, we store the function on the stack and prepare an initial call_frame to execute
// its code.  This is why we reserve the first index of the local_variables stack in the compiler
// struct, to store this function.  In the new call_frame, we point to the functcion, initialize
// its ip to point to the beginning of the function's bytecode, and set up its stack window to start
// at the very bottom of the vm's value stack.
interpret_result virtual_machine_interpret(const char* source) {
    obj_function* function = compile(source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    virtual_machine_stack_push(OBJ_VAL(function));
    call_frame* frame = &vm.frames[vm.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stack;

    return virtual_machine_run();
}

