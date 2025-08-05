#ifndef JUMI_CLOX_VALUE_H
#define JUMI_CLOX_VALUE_H
#include "common.h"

typedef double value;

typedef struct {
    int count;
    int capacity;
    value* values;
} value_array;

void init_value_array(value_array* array);
void free_value_array(value_array* array);
void write_to_value_array(value_array* array, value val);
void print_value(value val);

#endif
