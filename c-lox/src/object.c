#include "object.h"
#include "bytecode_chunk.h"
#include "hash_table.h"
#include "memory.h"
#include "value.h"
#include "virtual_machine.h"
#include <stdio.h>
#include <string.h>

#define ALLOCATE_OBJ(type, object_type) \
    (type*)allocate_object(sizeof(type), object_type)

static obj* allocate_object(size_t size, obj_type type) {
    // Allocate enough memory for the size of the struct being passed in, and then set the field.
    obj* object = (obj*)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;

    return object;
}

obj_function* new_function(void) {
    obj_function* function = ALLOCATE_OBJ(obj_function, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    init_bytecode_chunk(&function->chunk);
    return function;
}

static obj_string* allocate_string(char* chars, int length, uint32_t hash) {
    // Allocate enough for the type we need in the ALLOCATE_OBJ call, in this case, obj_string.
    // Then we set the fields we just allocated memory for, and return the pointer.
    obj_string* string = ALLOCATE_OBJ(obj_string, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    hash_table_set(&vm.interned_strings, string, NULL_VAL);
    return string;
}

static uint32_t hash_string(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

// In this function we have already allocated the memory from the virtual machine for the string 
// concatenation, and we just need to produce an obj_string* object from it.
obj_string* take_string(char* chars, int length) {
    uint32_t hash = hash_string(chars, length);

    obj_string* interned_string = hash_table_find_string(&vm.interned_strings, chars, length, hash);
    if (interned_string != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned_string;
    }

    return allocate_string(chars, length, hash);
}

// In this function we need to actually allocate new memory for the obj_string->chars portion of the string.
obj_string* copy_string(const char* chars, int length) {
    uint32_t hash = hash_string(chars, length);

    obj_string* interned_string = hash_table_find_string(&vm.interned_strings, chars, length, hash);
    if (interned_string != NULL) {
        return interned_string;
    }

    char* buffer = ALLOCATE(char, length + 1);
    memcpy(buffer, chars, length + 1);
    buffer[length] = '\0';
    return allocate_string(buffer, length, hash);
}

static void print_function(obj_function* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void print_object(value val) {
    switch (OBJ_TYPE(val)) {
        case OBJ_FUNCTION: {
            print_function(AS_FUNCTION(val));
        } break;
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(val));
        } break;
    }
}
