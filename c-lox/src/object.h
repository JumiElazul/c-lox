#ifndef JUMI_CLOX_OBJECT_H
#define JUMI_CLOX_OBJECT_H
#include "common.h"
#include "value.h"

typedef enum {
    OBJ_STRING,
} obj_type;

struct obj {
    obj_type type;
    struct obj* next;
};

struct obj_string {
    obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

obj_string* take_string(char* chars, int length);
obj_string* copy_string(const char* chars, int length);
void print_object(value val);

static inline bool is_obj_type(value val, obj_type type) {
    return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)

#define IS_STRING(val)  is_obj_type(val, OBJ_STRING)

#define AS_STRING(val)  ((obj_string*)AS_OBJ(val))

#define AS_CSTRING(val) (((obj_string*)AS_OBJ(val))->chars)

#endif
