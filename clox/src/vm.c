#include "vm.h"
#include "assert.h"
#include "bytecode_chunk.h"
#include "clox_value.h"
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

static void vm_reset_stack() {
    vm.sp = vm.stack;
    vm.frame_count = 0;
}

static void print_stack_trace() {
    fprintf(stderr, "== stack trace ==\n");

    for (int i = vm.frame_count - 1; i >= 0; --i) {
        stack_frame* frame = &vm.frames[i];
        object_function* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
}

static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    print_stack_trace();

    vm_reset_stack();
}

static bool call_function(object_function* function, int arg_count) {
    if (arg_count != function->arity) {
        runtime_error("Expected %d arguments for <func %s> but got %d.", function->arity,
                      function->name->chars, arg_count);
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtime_error("== vm stack overflow ==");
        return false;
    }

    stack_frame* frame = &vm.frames[vm.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->stack_window = vm.sp - arg_count - 1;
    return true;
}

static bool call_value(clox_value callee, int arg_count) {
    if (IS_OBJECT(callee)) {
        switch (OBJECT_TYPE(callee)) {
            case OBJECT_FUNCTION: {
                return call_function(AS_FUNCTION(callee), arg_count);
            } break;
            case OBJECT_NATIVE: {
                native_fn native = AS_NATIVE(callee);
                clox_value result = native(arg_count, vm.sp - arg_count);
                vm.sp -= arg_count + 1;
                vm_stack_push(result);
                return true;
            } break;
            default:
                break;
        }
    }
    runtime_error("Can only call functions and classes.");
    return false;
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

void init_vm() {
    vm_reset_stack();
    vm.objects = NULL;
    init_hash_table(&vm.globals);
    init_hash_table(&vm.const_globals);
    init_hash_table(&vm.strings);
}

void free_vm() {
    free_hash_table(&vm.globals);
    free_hash_table(&vm.const_globals);
    free_hash_table(&vm.strings);
    free_objects();
}

static interpret_result run() {
    stack_frame* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
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
        printf("stack:  ");
        for (clox_value* slot = vm.stack; slot < vm.sp; ++slot) {
            printf("[");
            print_value(*slot);
            printf("]");
        }
        printf("\n");
        disassemble_instruction(&frame->function->chunk,
                                (int)(frame->ip - frame->function->chunk.code));
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
            case OP_POP: {
                vm_stack_pop();
            } break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm_stack_push(frame->stack_window[slot]);
            } break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->stack_window[slot] = peek_stack(0);
            } break;
            case OP_GET_GLOBAL: {
                object_string* name = READ_STRING();
                clox_value value;
                if (!hash_table_get(&vm.const_globals, name, &value)) {
                    if (!hash_table_get(&vm.globals, name, &value)) {
                        runtime_error("Undefined variable '%s'.", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                vm_stack_push(value);
            } break;
            case OP_DEFINE_GLOBAL: {
                object_string* name = READ_STRING();
                hash_table_set(&vm.globals, name, peek_stack(0));
                vm_stack_pop();
            } break;
            case OP_DEFINE_CONST_GLOBAL: {
                object_string* name = READ_STRING();

                clox_value val;
                bool exists = hash_table_get(&vm.const_globals, name, &val);
                if (exists) {
                    runtime_error("Const variable '%s' already defined.", name->chars);
                    break;
                }

                hash_table_set(&vm.const_globals, name, peek_stack(0));
                vm_stack_pop();
            } break;
            case OP_SET_GLOBAL: {
                object_string* name = READ_STRING();
                clox_value value;
                if (hash_table_get(&vm.const_globals, name, &value)) {
                    runtime_error("Cannot reassign to variable marked 'const'.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (hash_table_set(&vm.globals, name, peek_stack(0))) {
                    hash_table_delete(&vm.globals, name);
                    runtime_error("Undefined variable '%s'.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_EQUAL: {
                clox_value b = vm_stack_pop();
                clox_value a = vm_stack_pop();
                vm_stack_push(BOOL_VAL(values_equal(a, b)));
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
            case OP_NEGATE: {
                if (!IS_NUMBER(peek_stack(0))) {
                    runtime_error("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_stack_push(NUMBER_VAL(-AS_NUMBER(vm_stack_pop())));
            } break;
            case OP_PRINT: {
                print_value(vm_stack_pop());
                printf("\n");
            } break;
            case OP_DEBUG: {
                printf("global variables:\n");
                for (size_t i = 0; i < vm.globals.capacity; ++i) {
                    table_entry* entry = &vm.globals.entries[i];
                    if (entry->key) {
                        printf("%s ", entry->key->chars);
                    }
                }
                printf("\n");
            } break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
            } break;
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (is_falsey(peek_stack(0))) {
                    frame->ip += offset;
                }
            } break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
            } break;
            case OP_CALL: {
                int arg_count = READ_BYTE();
                if (!call_value(peek_stack(arg_count), arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
            } break;
            case OP_RETURN: {
                clox_value result = vm_stack_pop();
                vm.frame_count--;

                if (vm.frame_count == 0) {
                    vm_stack_pop();
                    return INTERPRET_OK;
                }

                vm.sp = frame->stack_window;
                vm_stack_push(result);
                frame = &vm.frames[vm.frame_count - 1];
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}

interpret_result interpret(const char* source) {
    object_function* function = compile(source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    vm_stack_push(OBJECT_VAL(function));
    call_function(function, 0);

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
