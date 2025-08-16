#ifndef JUMI_CLOX_VIRTUAL_MACHINE_H
#define JUMI_CLOX_VIRTUAL_MACHINE_H
#include "bytecode_chunk.h"
#include "clox_value.h"
#include "hash_table.h"

#define STACK_MAX 256

typedef struct {
    bytecode_chunk* chunk;
    uint8_t* ip;
    clox_value stack[STACK_MAX];
    clox_value* stack_top;
    hash_table global_variables;
    hash_table global_consts;
    hash_table interned_strings;
    object* objects;
} virtual_machine;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} interpret_result;

extern virtual_machine vm;

void init_virtual_machine(void);
void free_virtual_machine(void);
void virtual_machine_stack_push(clox_value val);
clox_value virtual_machine_stack_pop(void);
interpret_result virtual_machine_interpret(const char* source_code);

#endif
