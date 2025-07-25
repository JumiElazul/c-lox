#ifndef JUMI_CLOX_VIRTUAL_MACHINE_H
#define JUMI_CLOX_VIRTUAL_MACHINE_H
#include "bytecode_chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    bytecode_chunk* chunk;
    uint8_t* ip;
    value stack[STACK_MAX];
    value* stack_top;
} virtual_machine;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} interpret_result;

void init_virtual_machine(void);
void free_virtual_machine(void);
interpret_result virtual_machine_interpret(bytecode_chunk* chunk);
void virtual_machine_stack_push(value val);
value virtual_machine_stack_pop(void);

#endif
