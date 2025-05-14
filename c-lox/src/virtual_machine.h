#ifndef JUMI_CLOX_VIRTUAL_MACHINE_H
#define JUMI_CLOX_VIRTUAL_MACHINE_H
#include "bytecode_chunk.h"
#include "common.h"
#include "hash_table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct virtual_machine {
    bytecode_chunk* current_chunk;
    uint8_t* ip;
    value stack[STACK_MAX];
    value* stack_top;
    hash_table global_variables;
    hash_table interned_strings;
    obj* objects;
} virtual_machine;

typedef enum interpret_result {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} interpret_result;

extern virtual_machine vm;

void init_virtual_machine(void);
void free_virtual_machine(void);
void virtual_machine_stack_push(value val);
value virtual_machine_stack_pop(void);
interpret_result virtual_machine_interpret(const char* source);

#endif
