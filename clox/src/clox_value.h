#ifndef CLOX_CLOX_VALUE_H
#define CLOX_CLOX_VALUE_H
#include "common.h"

typedef double clox_value;

typedef struct {
    int capacity;
    int count;
    clox_value* values;
} value_array;

void init_value_array(value_array* array);
void free_value_array(value_array* array);
void write_value_array(value_array* array, clox_value value);
void print_value(clox_value value);

#endif
