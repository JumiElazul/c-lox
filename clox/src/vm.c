#include "vm.h"
#include "assert.h"
#include "bytecode_chunk.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "hash_table.h"
#include "memory.h"
#include "object.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

VM vm;

static clox_value peek_stack(int distance) {
    return vm.sp[-1 - distance];
}

static bool is_falsey(clox_value value) {
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    object_string* b = AS_STRING(vm_stack_pop());
    object_string* a = AS_STRING(vm_stack_pop());

    size_t length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    object_string* result = take_string(chars, length);
    vm_stack_push(OBJECT_VAL(result));
}

static void vm_reset_stack() {
    vm.sp = vm.stack;
}

static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = (size_t)(vm.ip - vm.chunk->code - 1);
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    vm_reset_stack();
}

void init_vm() {
    vm_reset_stack();
    vm.objects = NULL;
    init_hash_table(&vm.strings);
}

void free_vm() {
    free_hash_table(&vm.strings);
    free_objects();
}

static interpret_result run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op) \
    do { \
        if (!IS_NUMBER(peek_stack(0)) || !IS_NUMBER(peek_stack(1))) { \
            runtime_error("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(vm_stack_pop());\
        double a = AS_NUMBER(vm_stack_pop());\
        vm_stack_push(value_type(a op b));\
    } while (false);

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("        ");
        for (clox_value* slot = vm.stack; slot < vm.sp; ++slot) {
            printf("[");
            print_value(*slot);
            printf("]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (size_t)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                clox_value constant = READ_CONSTANT();
                vm_stack_push(constant);
            } break;
            case OP_NULL: {
                vm_stack_push(NULL_VAL);
            } break;
            case OP_TRUE: {
                vm_stack_push(BOOL_VAL(true));
            } break;
            case OP_FALSE: {
                vm_stack_push(BOOL_VAL(false));
            } break;
            case OP_EQUAL: {
                clox_value b = vm_stack_pop();
                clox_value a = vm_stack_pop();
                vm_stack_push(BOOL_VAL(values_equal(a, b)));
            } break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek_stack(0))) {
                    runtime_error("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_stack_push(NUMBER_VAL(-AS_NUMBER(vm_stack_pop())));
            } break;
            case OP_GREATER: {
                BINARY_OP(BOOL_VAL, >);
            } break;
            case OP_LESS: {
                BINARY_OP(BOOL_VAL, <);
            } break;
            case OP_ADD: {
                if (IS_STRING(peek_stack(0)) && IS_STRING(peek_stack(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek_stack(0)) && IS_NUMBER(peek_stack(1))) {
                    double b = AS_NUMBER(vm_stack_pop());
                    double a = AS_NUMBER(vm_stack_pop());
                    vm_stack_push(NUMBER_VAL(a + b));
                } else {
                    runtime_error("Operands must be two numbers or two strings.");
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
                vm_stack_push(BOOL_VAL(is_falsey(vm_stack_pop())));
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
#undef BINARY_OP
}

interpret_result interpret(const char* source) {
    bytecode_chunk chunk;
    init_bytecode_chunk(&chunk);

    if (!compile(source, &chunk)) {
        free_bytecode_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    interpret_result result = run();

    free_bytecode_chunk(&chunk);
    return result;
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
