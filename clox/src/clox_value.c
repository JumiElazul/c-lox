#include "clox_value.h"
#include "memory.h"
#include "object.h"
#include "stdio.h"
#include <string.h>

bool values_equal(clox_value a, clox_value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case VAL_BOOL: {
            return AS_BOOL(a) == AS_BOOL(b);
        }
        case VAL_NULL: {
            return true;
        }
        case VAL_NUMBER: {
            return AS_NUMBER(a) == AS_NUMBER(b);
        }
        case VAL_OBJECT: {
            return AS_OBJECT(a) == AS_OBJECT(b);
        } break;
        default:
            return false;
    }
}

void init_value_array(value_array* array) {
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void free_value_array(value_array* array) {
    FREE_ARRAY(clox_value, array->values, array->capacity);
    init_value_array(array);
}

void write_value_array(value_array* array, clox_value value) {
    if (array->count >= array->capacity) {
        size_t old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(clox_value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void print_value(clox_value value) {
    switch (value.type) {
        case VAL_BOOL: {
            printf(AS_BOOL(value) ? "true" : "false");
        } break;
        case VAL_NULL: {
            printf("null");
        } break;
        case VAL_NUMBER: {
            printf("%g", AS_NUMBER(value));
        } break;
        case VAL_OBJECT: {
            print_object(value);
        } break;
    }
}
