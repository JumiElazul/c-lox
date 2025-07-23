#include "value.h"
#include "memory.h"
#include <stdio.h>

void init_value_array(value_array* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void free_value_array(value_array* array) {
    FREE_ARRAY(uint8_t, array->values, array->capacity);
    init_value_array(array);
}

void write_to_value_array(value_array* array, value val) {
    if (array->count >= array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = val;
    ++array->count;
}

void print_value(value val) {
    printf("%g", val);
}
