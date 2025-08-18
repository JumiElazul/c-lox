#include "virtual_machine.h"
#include "clox_value.h"
#include "compiler.h"
#include "disassembler.h"
#include "memory.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

virtual_machine vm;

static void dump_constant_table(void) {
    printf("constant table: [");
    bool first = true;

    for (int i = 0; i < vm.chunk->constants.count; ++i) {
        if (!first) {
            printf(", ");
        }

        print_value(vm.chunk->constants.values[i]);
        first = false;
    }
    printf("]\n");
}

static void dump_stack(void) {
    printf("stack: ");
    for (clox_value* slot = vm.stack; slot < vm.stack_top; ++slot) {
        printf("[");
        print_value(*slot);
        printf("]");
    }
    printf("\n");
}

static void dump_global_variables(void) {
    printf("global variables: [");
    bool first = true;

    for (int i = 0; i < vm.global_variables.capacity; ++i) {
        table_entry* entry = &vm.global_variables.entries[i];
        if (entry->key != NULL) {
            if (!first) {
                printf(", ");
            }
            printf("{");
            print_string(entry->key);
            printf(":");
            print_value(entry->val);
            printf("}");
            first = false;
        }
    }
    printf("]\n");
}

static void dump_interned_strings(void) {
    printf("interned strings: [");
    bool first = true;

    for (int i = 0; i < vm.interned_strings.capacity; ++i) {
        table_entry* entry = &vm.interned_strings.entries[i];
        if (entry->key != NULL) {
            if (!first) {
                printf(", ");
            }
            printf("'");
            print_string(entry->key);
            printf("'");
            first = false;
        }
    }
    printf("]\n");
}

static void virtual_machine_debug(void) {
    printf("===== DEBUG =====\n");
    dump_constant_table();
    dump_stack();
    dump_global_variables();
    dump_interned_strings();
    printf("===== END DEBUG =====\n");
}

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

static bool is_falsey(clox_value value) {
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate_string(void) {
    object_string* b = AS_STRING(virtual_machine_stack_pop());
    object_string* a = AS_STRING(virtual_machine_stack_pop());

    int length = b->length + a->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    object_string* result = take_string(chars, length);
    virtual_machine_stack_push(OBJECT_VALUE(result));
}

void init_virtual_machine(void) {
    reset_stack();
    vm.objects = NULL;
    init_hash_table(&vm.global_variables);
    init_hash_table(&vm.global_consts);
    init_hash_table(&vm.interned_strings);
}

void free_virtual_machine(void) {
    free_hash_table(&vm.global_variables);
    free_hash_table(&vm.global_consts);
    free_hash_table(&vm.interned_strings);
    free_objects();
}

void virtual_machine_stack_push(clox_value val) {
    // Maybe handle stack overflow here.
    *vm.stack_top = val;
    ++vm.stack_top;
}

clox_value virtual_machine_stack_pop(void) {
    --vm.stack_top;
    return *vm.stack_top;
}

static int READ_U24(void) {
    u24_t u24_index;
    u24_index.hi = *vm.ip++;
    u24_index.mid = *vm.ip++;
    u24_index.lo = *vm.ip++;
    return deconstruct_u24_t(u24_index);
}

static interpret_result virtual_machine_run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
    dump_constant_table();
#endif

    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        dump_stack();
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;

        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                clox_value constant = READ_CONSTANT();
                virtual_machine_stack_push(constant);
            } break;
            case OP_CONSTANT_LONG: {
                int reconstructed_index = READ_U24();
                virtual_machine_stack_push(vm.chunk->constants.values[reconstructed_index]);
            } break;
            case OP_NULL: {
                virtual_machine_stack_push(NULL_VALUE);
            } break;
            case OP_TRUE: {
                virtual_machine_stack_push(BOOL_VALUE(true));
            } break;
            case OP_FALSE: {
                virtual_machine_stack_push(BOOL_VALUE(false));
            } break;
            case OP_POP: {
                virtual_machine_stack_pop();
            } break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                virtual_machine_stack_push(vm.stack[slot]);
            } break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = virtual_machine_stack_peek(0);
            } break;
            case OP_GET_GLOBAL: {
                object_string* name = READ_STRING();
                clox_value val;
                if (!hash_table_get(&vm.global_variables, name, &val)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                virtual_machine_stack_push(val);
            } break;
            case OP_GET_GLOBAL_LONG: {
                int reconstructed_index = READ_U24();
                object_string* name = AS_STRING(vm.chunk->constants.values[reconstructed_index]);
                clox_value val;
                if (!hash_table_get(&vm.global_variables, name, &val)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                virtual_machine_stack_push(val);
            } break;
            case OP_DEFINE_GLOBAL: {
                object_string* name = READ_STRING();
                hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0));
                virtual_machine_stack_pop();
            } break;
            case OP_DEFINE_GLOBAL_CONST: {
                object_string* name = READ_STRING();
                hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0));
                hash_table_set(&vm.global_consts, name, BOOL_VALUE(true));
                virtual_machine_stack_pop();
            } break;
            case OP_DEFINE_GLOBAL_LONG: {
                int reconstructed_index = READ_U24();
                object_string* name = AS_STRING(vm.chunk->constants.values[reconstructed_index]);
                hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0));
                virtual_machine_stack_pop();
            } break;
            case OP_DEFINE_GLOBAL_LONG_CONST: {
                int reconstructed_index = READ_U24();
                object_string* name = AS_STRING(vm.chunk->constants.values[reconstructed_index]);
                hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0));
                hash_table_set(&vm.global_consts, name, BOOL_VALUE(true));
                virtual_machine_stack_pop();
            } break;
            case OP_SET_GLOBAL: {
                object_string* name = READ_STRING();

                if (hash_table_get(&vm.global_consts, name, &(clox_value){0})) {
                    runtime_error("Cannot reassign to a global variable marked 'const'.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0))) {
                    hash_table_delete(&vm.global_variables, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_SET_GLOBAL_LONG: {
                int reconstructed_index = READ_U24();
                object_string* name = AS_STRING(vm.chunk->constants.values[reconstructed_index]);
                if (hash_table_set(&vm.global_variables, name, virtual_machine_stack_peek(0))) {
                    hash_table_delete(&vm.global_variables, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
            } break;
            case OP_EQUAL: {
                clox_value b = virtual_machine_stack_pop();
                clox_value a = virtual_machine_stack_pop();
                virtual_machine_stack_push(BOOL_VALUE(values_equal(a, b)));
            } break;
            case OP_GREATER: {
                BINARY_OP(BOOL_VALUE, >);
            } break;
            case OP_LESS: {
                BINARY_OP(BOOL_VALUE, <);
            } break;
            case OP_ADD: {
                if (IS_STRING(virtual_machine_stack_peek(0)) &&
                    IS_STRING(virtual_machine_stack_peek(1))) {
                    concatenate_string();
                } else if (IS_NUMBER(virtual_machine_stack_peek(0)) &&
                           IS_NUMBER(virtual_machine_stack_peek(1))) {
                    BINARY_OP(NUMBER_VALUE, +);
                } else {
                    runtime_error("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
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
            case OP_NOT: {
                virtual_machine_stack_push(BOOL_VALUE(is_falsey(virtual_machine_stack_pop())));
            } break;
            case OP_NEGATE: {
                if (!IS_NUMBER(virtual_machine_stack_peek(0))) {
                    runtime_error("Operand must be a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                virtual_machine_stack_push(NUMBER_VALUE(-AS_NUMBER(virtual_machine_stack_pop())));
            } break;
            case OP_PRINT: {
                print_value(virtual_machine_stack_pop());
                printf("\n");
            } break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
            } break;
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (is_falsey(virtual_machine_stack_peek(0))) {
                    vm.ip += offset;
                }
            } break;
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                vm.ip -= offset;
            } break;
            case OP_RETURN: {
                return INTERPRET_OK;
            }
            case OP_DEBUG: {
                virtual_machine_debug();
                return INTERPRET_OK;
            } break;
            default: {
                printf("Unknown opcode %d\n", instruction);
                return INTERPRET_RUNTIME_ERROR;
            } break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
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
