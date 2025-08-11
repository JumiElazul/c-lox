#include "hash_table.h"

#define HASH_TABLE_MAX_LOAD 0.75

void init_hash_table(hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_hash_table(hash_table* table) {
    FREE_ARRAY(table_entry, table->entries, table->capacity);
    init_hash_table(table);
}

static table_entry* find_entry(table_entry* entries, int capacity, object_string* key) {
    uint32_t index = key->hash % capacity;

    while (true) {
        table_entry* entry = &entries[index];
        if (entry->key == key || entry->key == NULL) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(hash_table* table, int capacity) {
    table_entry* entries = ALLOCATE(table_entry, capacity);
    for (int i = 0; i < capacity; ++i) {
        entries[i].key = NULL;
        entries[i].val = NULL_VALUE;
    }

    for (int i = 0; i < table->capacity; ++i) {
        table_entry* entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        table_entry* destination = find_entry(entries, capacity, entry->key);
        destination->key = entry->key;
        destination->val = entry->val;
    }

    FREE_ARRAY(table_entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool hash_table_set(hash_table* table, object_string* key, clox_value val) {
    if (table->count >= table->capacity * HASH_TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    table_entry* entry = find_entry(table->entries, table->capacity, key);
}
