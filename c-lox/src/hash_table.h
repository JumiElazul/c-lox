#ifndef JUMI_CLOX_HASH_TABLE_H
#define JUMI_CLOX_HASH_TABLE_H
#include "value.h"

typedef struct {
    obj_string* key;
    value val;
} table_entry;

typedef struct {
    int count;
    int capacity;
    table_entry* entries;
} hash_table;

void init_hash_table(hash_table* table);
void free_hash_table(hash_table* table);
bool set_hash_table(hash_table* table, obj_string* key, value val);

#endif
