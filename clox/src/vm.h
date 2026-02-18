#ifndef CLOX_VM_H
#define CLOX_VM_H
#include "bytecode_chunk.h"
#include "hash_table.h"
#include "object.h"

#define STACK_MAX 256

typedef struct {
    bytecode_chunk* chunk;
    uint8_t* ip;
    clox_value stack[STACK_MAX];
    clox_value* sp;
    hash_table globals;
    hash_table const_globals;
    hash_table strings;
    object* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} interpret_result;

extern VM vm;

void init_vm();
void free_vm();
interpret_result interpret(const char* source);
void vm_stack_push(clox_value);
clox_value vm_stack_pop();

#endif
