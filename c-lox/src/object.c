#include "object.h"
#include "memory.h"
#include "value.h"
#include <string.h>

#define ALLOCATE_OBJ(type, object_type) \
    (type*)allocate_object(sizeof(type), object_type)

static obj* allocate_object(size_t size, obj_type type) {
    // Allocate enough memory for the size of the struct being passed in, and then set the field.
    obj* object = (obj*)reallocate(NULL, 0, size);
    object->type = type;
    return object;
}

static obj_string* allocate_string(char* chars, int length) {
    // Allocate enough for the type we need in the ALLOCATE_OBJ call, in this case, obj_string.
    // Then we set the fields we just allocated memory for, and return the pointer.
    obj_string* string = ALLOCATE_OBJ(obj_string, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

obj_string* copy_string(const char* chars, int length) {
    // Allocate enough for just the characters that will be stored in obj_string->chars.
    char* buffer = ALLOCATE(char, length + 1);
    memcpy(buffer, chars, length + 1);
    buffer[length] = '\0';
    return allocate_string(buffer, length);
}
