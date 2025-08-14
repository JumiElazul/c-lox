#ifndef JUMI_CLOX_IDENTIFIER_CACHE_H
#define JUMI_CLOX_IDENTIFIER_CACHE_H
#include "clox_object.h"
#include "clox_value.h"
#include "common.h"
#include "memory.h"

typedef struct object_string object_string;

typedef struct {
    object_string* key;
    int index;
} cache_entry;

typedef struct {
    int count;
    int capacity;
    cache_entry* entries;
} identifier_cache;

void init_identifier_cache(identifier_cache* table);
void free_identifier_cache(identifier_cache* table);
bool identifier_cache_get(identifier_cache* table, object_string* key, int* val);
bool identifier_cache_set(identifier_cache* table, object_string* key, int val);
bool identifier_cache_delete(identifier_cache* table, object_string* key);

#endif
