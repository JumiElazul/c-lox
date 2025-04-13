#ifndef JUMI_CLOX_VALUE_H
#define JUMI_CLOX_VALUE_H
#include "common.h"

typedef struct obj obj;
typedef struct obj_string obj_string;

typedef enum value_type {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER,
    VAL_OBJ,
} value_type;

typedef struct value {
    value_type type;
    union {
        bool boolean;
        double number;
        obj* obj;
    } as;
} value;

// Promotes C values to our internal c-lox value types
#define BOOL_VAL(val)   ((value){ VAL_BOOL,   { .boolean = val   } })
#define NULL_VAL        ((value){ VAL_NULL,   { .number = 0      } })
#define NUMBER_VAL(val) ((value){ VAL_NUMBER, { .number = val    } })
#define OBJ_VAL(val)    ((value){ VAL_OBJ,    { .obj = (obj*)val } })

#define AS_BOOL(val)    ((val).as.boolean)
#define AS_NUMBER(val)  ((val).as.number)
#define AS_OBJ(val)     ((val).as.obj)

#define IS_BOOL(val)    ((val).type == VAL_BOOL)
#define IS_NULL(val)    ((val).type == VAL_NULL)
#define IS_NUMBER(val)  ((val).type == VAL_NUMBER)
#define IS_OBJ(val)     ((val).type == VAL_OBJ)

typedef struct value_array {
    int capacity;
    int count;
    value* values;
} value_array;

bool values_equal(value a, value b);
void init_value_array(value_array* array);
void write_value_array(value_array* array, value val);
void free_value_array(value_array* array);
void print_value(value val);

#endif
