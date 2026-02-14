#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H
#include "clox_value.h"
#include "common.h"

#define OBJECT_TYPE(val) (AS_OBJECT(val)->type)

#define IS_STRING(val)   is_object_type(val, OBJECT_STRING)
#define AS_STRING(val)   ((object_string*)AS_OBJECT(val))
#define AS_CSTRING(val)  (((object_string*)AS_OBJECT(val))->chars)

typedef enum {
    OBJECT_STRING
} object_type;

typedef struct object {
    object_type type;
    struct object* next;
} object;

typedef struct object_string {
    object obj;
    size_t length;
    char* chars;
} object_string;

object_string* take_string(char* chars, size_t length);
object_string* copy_string(const char* chars, size_t length);
void print_object(clox_value value);

static inline bool is_object_type(clox_value value, object_type type) {
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif
