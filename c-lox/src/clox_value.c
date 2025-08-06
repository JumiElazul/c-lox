#include "clox_value.h"
#include "clox_object.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

bool values_equal(clox_value a, clox_value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case CLOX_VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case CLOX_VAL_NULL:
            return true;
        case CLOX_VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case CLOX_VAL_OBJECT: {
            object_string* a_string = AS_STRING(a);
            object_string* b_string = AS_STRING(b);
            return a_string->length == b_string->length &&
                   memcmp(a_string->chars, b_string->chars, a_string->length) == 0;
        }
        default:
            return false;
    }
}

void init_value_array(value_array* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void free_value_array(value_array* array) {
    FREE_ARRAY(uint8_t, array->values, array->capacity);
    init_value_array(array);
}

void write_to_value_array(value_array* array, clox_value val) {
    if (array->count >= array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(clox_value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = val;
    ++array->count;
}

void print_value(clox_value val) {
    switch (val.type) {
        case CLOX_VAL_BOOL: {
            printf(AS_BOOL(val) ? "true" : "false");
        } break;
        case CLOX_VAL_NULL: {
            printf("null");
        } break;
        case CLOX_VAL_NUMBER: {
            printf("%g", AS_NUMBER(val));
        } break;
        case CLOX_VAL_OBJECT: {
            if (IS_STRING(val)) {
                printf("%s", AS_CSTRING(val));
            }
        } break;
    }
}
