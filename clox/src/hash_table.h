#ifndef CLOX_HASH_TABLE_H
#define CLOX_HASH_TABLE_H
#include "clox_value.h"
#include "common.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
    object_string* key;
    clox_value value;
} table_entry;

typedef struct {
    size_t count;
    size_t capacity;
    table_entry* entries;
} hash_table;

void init_hash_table(hash_table* table);
void free_hash_table(hash_table* table);
void hash_table_set(hash_table* table, object_string* key, clox_value value);

#endif
