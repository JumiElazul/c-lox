#ifndef JUMI_CLOX_CLOX_OBJECT_H
#define JUMI_CLOX_CLOX_OBJECT_H
#include "clox_value.h"

#define OBJECT_TYPE(val) (AS_OBJECT(val)->type)

#define IS_STRING(val) is_object_type(val, OBJECT_STRING)

#define AS_STRING(val) ((object_string*)AS_OBJECT(val))

#define AS_CSTRING(val) (((object_string*)AS_OBJECT(val))->chars)

typedef enum { OBJECT_STRING } object_type;

struct object {
    object_type type;
    struct object* next;
};

struct object_string {
    object obj;
    int length;
    char* chars;
};

object_string* take_string(char* chars, int length);
object_string* copy_string(const char* chars, int length);
void print_object(clox_value val);

static inline bool is_object_type(clox_value val, object_type type) {
    return IS_OBJECT(val) && AS_OBJECT(val)->type == type;
}

#endif
