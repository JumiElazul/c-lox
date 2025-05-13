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

void hash_table_init(hash_table* table);
void hash_table_free(hash_table* table);
float hash_table_load_factor(hash_table* table);
bool hash_table_get(hash_table* table, obj_string* key, value* val);
bool hash_table_set(hash_table* table, obj_string* key, value val);
bool hash_table_delete(hash_table* table, obj_string* key);
void hash_table_add_all(hash_table* from, hash_table* to);
obj_string* hash_table_find_string(hash_table* table, const char* chars, int length, uint32_t hash);

#endif
