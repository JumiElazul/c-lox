#include "clox_object.h"
#include "memory.h"
#include "virtual_machine.h"
#include <stdio.h>
#include <string.h>

#define ALLOCATE_OBJECT(type, object_type) (type*)allocate_object(sizeof(type), object_type)

static object* allocate_object(size_t size, object_type type) {
    object* obj = (object*)reallocate(NULL, 0, size);
    obj->type = type;

    obj->next = vm.objects;
    vm.objects = obj;

    return obj;
}

// Allocates memory for our clox_object string and assigns the c-style string to it.
static object_string* allocate_string(char* chars, int length, uint32_t hash) {
    object_string* string = ALLOCATE_OBJECT(object_string, OBJECT_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    hash_table_set(&vm.interned_strings, string, NULL_VALUE);
    return string;
}

static uint32_t hash_string(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; ++i) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

object_function* new_function(void) {
    object_function* function = ALLOCATE_OBJECT(object_function, OBJECT_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    init_bytecode_chunk(&function->chunk);
    return function;
}

object_native* new_native(native_fn function, const char* name, int min_arity, int max_arity) {
    object_native* native = ALLOCATE_OBJECT(object_native, OBJECT_NATIVE);
    native->min_arity = min_arity;
    native->max_arity = max_arity;
    native->function = function;
    native->name = name;
    return native;
}

object_string* take_string(char* chars, int length) {
    uint32_t hash = hash_string(chars, length);

    object_string* interned = table_find_string(&vm.interned_strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocate_string(chars, length, hash);
}

// Allocates memory for const char* strings.
object_string* copy_string(const char* chars, int length) {
    uint32_t hash = hash_string(chars, length);

    object_string* interned = table_find_string(&vm.interned_strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    char* heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length, hash);
}

void print_function(object_function* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void print_string(object_string* str) { printf("%s", str->chars); }

void print_object(clox_value val) {
    switch (OBJECT_TYPE(val)) {
        case OBJECT_FUNCTION:
            print_function(AS_FUNCTION(val));
            break;
        case OBJECT_NATIVE:
            printf("<native fn>");
            break;
        case OBJECT_STRING:
            printf("%s", AS_CSTRING(val));
            break;
    }
}
