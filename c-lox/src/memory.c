#include "memory.h"
#include "clox_object.h"
#include "virtual_machine.h"
#include <stdlib.h>

// oldSize           newSize                  Operation
// 0                 Non‑zero                 Allocate new block.
// Non‑zero          0                        Free allocation.
// Non‑zero          Smaller than oldSize     Shrink existing allocation.
// Non‑zero          Larger than oldSize      Grow existing allocation.
void* reallocate(void* pointer, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) {
        exit(EXIT_FAILURE);
    }
    return result;
}

static void free_object(object* obj) {
    switch (obj->type) {
        case OBJECT_STRING: {
            object_string* string = (object_string*)obj;
            FREE_ARRAY(char, string->chars, string->length);
            FREE(object_string, obj);
        } break;
    }
}

void free_objects(void) {
    object* obj = vm.objects;
    while (obj != NULL) {
        object* next = obj->next;
        free_object(obj);
        obj = next;
    }
}
