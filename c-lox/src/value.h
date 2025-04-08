#ifndef JUMI_CLOX_VALUE_H
#define JUMI_CLOX_VALUE_H

typedef double value;

typedef struct value_array {
    int capacity;
    int count;
    value* values;
} value_array;

void init_value_array(value_array* array);
void write_value_array(value_array* array, value val);
void free_value_array(value_array* array);
void print_value(value val);

#endif
