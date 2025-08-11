#ifndef JUMI_CLOX_HASH_TABLE_H
#define JUMI_CLOX_HASH_TABLE_H
#include "clox_object.h"
#include "clox_value.h"
#include "common.h"
#include "memory.h"

typedef struct object_string object_string;

typedef struct {
    object_string* key;
    clox_value val;
} table_entry;

typedef struct {
    int count;
    int capacity;
    table_entry* entries;
} hash_table;

void init_hash_table(hash_table* table);
void free_hash_table(hash_table* table);
bool hash_table_get(hash_table* table, object_string* key, clox_value* val);
bool hash_table_set(hash_table* table, object_string* key, clox_value val);
bool hash_table_delete(hash_table* table, object_string* key);
void hash_table_add_all(hash_table* from, hash_table* to);
object_string* table_find_string(hash_table* table, const char* chars, int length, uint32_t hash);

#endif
