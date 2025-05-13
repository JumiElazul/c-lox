#include "value.h"
#include "memory.h"
#include "object.h"
#include <stdio.h>
#include <string.h>

void init_value_array(value_array* array) {
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void write_value_array(value_array* array, value val) {
    if (array->count >= array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count++] = val;
}

void free_value_array(value_array* array) {
    FREE_ARRAY(value, array->values, array->capacity);
}

void print_value(value val) {
    switch (val.type) {
        case VAL_BOOL: {
            printf(AS_BOOL(val) ? "true" : "false");
        } break;
        case VAL_NULL: {
            printf("null");
        } break;
        case VAL_NUMBER: {
            printf("%g", AS_NUMBER(val));
        } break;
        case VAL_OBJ: {
            print_object(val);
        } break;
    }
}

bool values_equal(value a, value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NULL:   return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
        default:         return false;
    }
}
