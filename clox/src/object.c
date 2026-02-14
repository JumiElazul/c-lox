#include "object.h"
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

static object_string* allocate_string(char* chars, size_t length) {
    object_string* string = ALLOCATE_OBJECT(object_string, OBJECT_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

object_string* take_string(char* chars, size_t length) {
    return allocate_string(chars, length);
}

object_string* copy_string(const char* chars, size_t length) {
    char* heap_chars = ALLOCATE(char, (size_t)length + 1);
    memcpy(heap_chars, chars, (size_t)length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length);
}

void print_object(clox_value value) {
    switch (OBJECT_TYPE(value)) {
        case OBJECT_STRING: {
            printf("%s", AS_CSTRING(value));
        } break;
    }
}
