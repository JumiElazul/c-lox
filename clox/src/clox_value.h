#ifndef CLOX_CLOX_VALUE_H
#define CLOX_CLOX_VALUE_H
#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER,
    VAL_OBJECT,
} value_type;

typedef struct object object;
typedef struct object_string object_string;

typedef struct {
    value_type type;
    union {
        bool boolean;
        double number;
        object* obj;
    } as;
} clox_value;

#define IS_BOOL(val)    ((val).type == VAL_BOOL)
#define IS_NULL(val)    ((val).type == VAL_NULL)
#define IS_NUMBER(val)  ((val).type == VAL_NUMBER)
#define IS_OBJECT(val)  ((val).type == VAL_OBJECT)

#define AS_BOOL(val)    ((val).as.boolean)
#define AS_NUMBER(val)  ((val).as.number)
#define AS_OBJECT(val)  ((val).as.obj)

#define BOOL_VAL(val)   ((clox_value){VAL_BOOL,   {.boolean = val}})
#define NULL_VAL        ((clox_value){VAL_NULL,   {.number = 0}})
#define NUMBER_VAL(val) ((clox_value){VAL_NUMBER, {.number = val}})
#define OBJECT_VAL(val) ((clox_value){VAL_OBJECT, {.obj = (object*)val}})

typedef struct {
    size_t capacity;
    size_t count;
    clox_value* values;
} value_array;

bool values_equal(clox_value a, clox_value b);
void init_value_array(value_array* array);
void free_value_array(value_array* array);
void write_value_array(value_array* array, clox_value value);
void print_value(clox_value value);

#endif
