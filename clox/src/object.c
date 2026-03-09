#include "object.h"
#include "bytecode_chunk.h"
#include "hash_table.h"
#include "memory.h"
#include "vm.h"
#include <stdio.h>
#include <string.h>

#define ALLOCATE_OBJECT(type, object_type) \
    (type*)allocate_object(sizeof(type), object_type)

static object* allocate_object(size_t size, object_type type) {
    object* obj = (object*)reallocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

object_function* new_function() {
    object_function* function = ALLOCATE_OBJECT(object_function, OBJECT_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    init_bytecode_chunk(&function->chunk);
    return function;
}

static object_string* allocate_string(char* chars, size_t length, uint32_t hash) {
    object_string* string = ALLOCATE_OBJECT(object_string, OBJECT_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    // Intern the string
    hash_table_set(&vm.strings, string, NULL_VAL);

    return string;
}

static uint32_t fnv1a_hash_string(const char* key, size_t length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < (int)length; ++i) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

object_string* take_string(char* chars, size_t length) {
    uint32_t hash = fnv1a_hash_string(chars, length);

    object_string* interned = hash_table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocate_string(chars, length, hash);
}

object_string* copy_string(const char* chars, size_t length) {
    uint32_t hash = fnv1a_hash_string(chars, length);

    object_string* interned = hash_table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    char* heap_chars = ALLOCATE(char, (size_t)length + 1);
    memcpy(heap_chars, chars, (size_t)length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length, hash);
}

static void print_function(object_function* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<func %s>", function->name->chars);
}

void print_object(clox_value value) {
    switch (OBJECT_TYPE(value)) {
        case OBJECT_FUNCTION: {
            print_function(AS_FUNCTION(value));
        } break;
        case OBJECT_STRING: {
            printf("%s", AS_CSTRING(value));
        } break;
    }
}
