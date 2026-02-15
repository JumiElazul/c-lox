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
bool hash_table_get(hash_table* table, object_string* key, clox_value* value);
bool hash_table_set(hash_table* table, object_string* key, clox_value value);
bool hash_table_delete(hash_table* table, object_string* key);
void table_add_all(hash_table* from, hash_table* to);

#endif
