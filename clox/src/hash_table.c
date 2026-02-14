#include "hash_table.h"
#include "clox_value.h"
#include "memory.h"
#include "object.h"
#include <stdlib.h>
#include <string.h>

void init_hash_table(hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_hash_table(hash_table* table) {
    FREE_ARRAY(table_entry, table->entries, table->count);
    init_hash_table(table);
}

static table_entry* find_entry(hash_table* table, size_t capacity, object_string* key) {
    uint32_t index = key->hash % capacity;

    for (;;) {
        table_entry* entry = &table->entries[index];
        if (entry->key == key || entry->key == NULL) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(hash_table* table, size_t capacity) {
}

void hash_table_set(hash_table* table, object_string* key, clox_value value) {
    if (table->count >= table->capacity * TABLE_MAX_LOAD) {
        size_t capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    table_entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == NULL;

    if (is_new_key) {
        table->count++;
    }

    entry->key = key;
    entry->value = value;
}
