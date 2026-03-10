#ifndef CLOX_VM_H
#define CLOX_VM_H
#include "hash_table.h"
#include "object.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    object_function* function;
    uint8_t* ip;
    clox_value* slots;
} stack_frame;

typedef struct {
    stack_frame frames[FRAMES_MAX];
    int frame_count;

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
