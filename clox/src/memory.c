#include "memory.h"
#include "bytecode_chunk.h"
#include "object.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (!result) {
        perror("error with memory reallocation in 'reallocate'.");
        exit(1);
    }
    return result;
}

static void free_object(object* obj) {
    switch (obj->type) {
        case OBJECT_FUNCTION: {
            object_function* function = (object_function*)obj;
            free_bytecode_chunk(&function->chunk);
            FREE(object_function, obj);
        } break;
        case OBJECT_NATIVE: {
            FREE(object_native, obj);
        } break;
        case OBJECT_STRING: {
            object_string* string = (object_string*)obj;
            FREE_ARRAY(char, string->chars, string->length);
            FREE(object_string, string);
        } break;
    }
}

void free_objects() {
    object* obj = vm.objects;

    while (obj != NULL) {
        object* next = obj->next;
        free_object(obj);
        obj = next;
    }
}
