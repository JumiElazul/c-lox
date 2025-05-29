#ifndef JUMI_CLOX_VIRTUAL_MACHINE_H
#define JUMI_CLOX_VIRTUAL_MACHINE_H
#include "bytecode_chunk.h"
#include "common.h"
#include "object.h"
#include "hash_table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    obj_function* function;
    uint8_t* ip;
    value* slots;
} call_frame;

typedef struct virtual_machine {
    call_frame frames[FRAMES_MAX];
    int frame_count;
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
