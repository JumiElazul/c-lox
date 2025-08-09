#ifndef JUMI_CLOX_HASH_TABLE_H
#define JUMI_CLOX_HASH_TABLE_H
#include "clox_value.h"
#include "common.h"

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

#endif
