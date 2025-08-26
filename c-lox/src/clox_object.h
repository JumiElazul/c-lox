#ifndef JUMI_CLOX_CLOX_OBJECT_H
#define JUMI_CLOX_CLOX_OBJECT_H
#include "bytecode_chunk.h"
#include "clox_value.h"

#define OBJECT_TYPE(val) (AS_OBJECT(val)->type)

#define IS_CLOSURE(val) is_object_type(val, OBJECT_CLOSURE)
#define IS_FUNCTION(val) is_object_type(val, OBJECT_FUNCTION)
#define IS_NATIVE(val) is_object_type(val, OBJECT_NATIVE)
#define IS_STRING(val) is_object_type(val, OBJECT_STRING)

#define AS_CLOSURE(val) ((object_closure*)AS_OBJECT(val))
#define AS_FUNCTION(val) ((object_function*)AS_OBJECT(val))
#define AS_NATIVE(val) ((object_native*)AS_OBJECT(val))
#define AS_STRING(val) ((object_string*)AS_OBJECT(val))

#define AS_CSTRING(val) (((object_string*)AS_OBJECT(val))->chars)

typedef enum {
    OBJECT_CLOSURE,
    OBJECT_FUNCTION,
    OBJECT_NATIVE,
    OBJECT_STRING,
    OBJECT_UPVALUE
} object_type;

struct object {
    object_type type;
    struct object* next;
};

typedef struct {
    object obj;
    int arity;
    int upvalue_count;
    bytecode_chunk chunk;
    object_string* name;
} object_function;

typedef clox_value (*native_fn)(int arg_count, clox_value* args);

typedef struct {
    object obj;
    native_fn function;
    const char* name;
    int min_arity;
    int max_arity;
} object_native;

struct object_string {
    object obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct object_upvalue {
    object obj;
    clox_value* location;
} object_upvalue;

typedef struct {
    object obj;
    object_function* function;
    object_upvalue** upvalues;
    int upvalue_count;
} object_closure;

object_closure* new_closure(object_function* function);
object_function* new_function(void);
object_native* new_native(native_fn function, const char* name, int min_arity, int max_arity);
object_string* take_string(char* chars, int length);
object_string* copy_string(const char* chars, int length);
object_upvalue* new_upvalue(clox_value* slot);
void print_function(object_function* val);
void print_object(clox_value val);
void print_string(object_string* str);

static inline bool is_object_type(clox_value val, object_type type) {
    return IS_OBJECT(val) && AS_OBJECT(val)->type == type;
}

#endif
