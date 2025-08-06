#ifndef JUMI_CLOX_CLOX_VALUE_H
#define JUMI_CLOX_CLOX_VALUE_H
#include "common.h"

typedef struct object object;
typedef struct object_string object_string;

typedef enum {
    CLOX_VAL_BOOL,
    CLOX_VAL_NULL,
    CLOX_VAL_NUMBER,
    CLOX_VAL_OBJECT,
} clox_value_type;

typedef struct {
    clox_value_type type;
    union {
        bool boolean;
        double number;
        object* obj;
    } as;
} clox_value;

#define IS_BOOL(val) ((val).type == CLOX_VAL_BOOL)
#define IS_NULL(val) ((val).type == CLOX_VAL_NULL)
#define IS_NUMBER(val) ((val).type == CLOX_VAL_NUMBER)
#define IS_OBJECT(val) ((val).type == CLOX_VAL_OBJECT)

#define AS_OBJECT(val) ((val).as.obj)
#define AS_BOOL(val) ((val).as.boolean)
#define AS_NUMBER(val) ((val).as.number)

#define BOOL_VALUE(val) ((clox_value){CLOX_VAL_BOOL, {.boolean = val}})
#define NULL_VALUE ((clox_value){CLOX_VAL_NULL, {.number = 0}})
#define NUMBER_VALUE(val) ((clox_value){CLOX_VAL_NUMBER, {.number = val}})
#define OBJECT_VALUE(val) ((clox_value){CLOX_VAL_OBJECT, {.obj = (object*)val}})

typedef struct {
    int count;
    int capacity;
    clox_value* values;
} value_array;

bool values_equal(clox_value a, clox_value b);
void init_value_array(value_array* array);
void free_value_array(value_array* array);
void write_to_value_array(value_array* array, clox_value val);
void print_value(clox_value val);

#endif
