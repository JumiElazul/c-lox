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
bool hash_table_set(hash_table* table, object_string* key, clox_value val);

#endif
