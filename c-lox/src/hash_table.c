#include "hash_table.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

void hash_table_init(hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void hash_table_free(hash_table* table) {
    FREE_ARRAY(table_entry, table->entries, table->capacity);
    hash_table_init(table);
}

float hash_table_load_factor(hash_table* table) {
    if (table->capacity == 0) {
        return 1.0;
    }

    return (float)table->count / (float)table->capacity;
}

static table_entry* find_entry(table_entry* entries, int capacity, obj_string* key) {
    uint32_t index = key->hash % capacity;
    for (;;) {
        table_entry* entry = &entries[index];
        table_entry* tombstone = NULL;

        if (entry->key == NULL) {
            if (IS_NULL(entry->val)) {
                // Empty entry, if tombstone was passed to get here return that spot instead
                return tombstone != NULL ? tombstone : entry;
            } else {
                // Found tombstone, set it and continue
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            // Found the key
            return entry;
        }

        // Linear probing to find the next available spot the key could reside
        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(hash_table* table, int capacity) {
    table_entry* entries = ALLOCATE(table_entry, capacity);

    // Set a default NULL value for every "table_entry" in the new hash_table
    for (int i = 0; i < capacity; ++i) {
        entries[i].key = NULL;
        entries[i].val = NULL_VAL;
    }

    // Loop over the old table, rehashing in find_entry and inserting into its new
    // proper place.
    table->count = 0;

    for (int i = 0; i < table->capacity; ++i) {
        table_entry* entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        table_entry* destination = find_entry(entries, capacity, entry->key);
        destination->key = entry->key;
        destination->val = entry->val;
        ++table->count;
    }

    FREE_ARRAY(table_entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool hash_table_get(hash_table* table, obj_string* key, value* val) {
    if (table->count == 0) {
        return false;
    }

    table_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    *val = entry->val;
    return true;
}

bool hash_table_set(hash_table* table, obj_string* key, value val) {
    if (hash_table_load_factor(table) > TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    table_entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == NULL;
    if (is_new_key && IS_NULL(entry->val)) {
        ++table->count;
    }

    entry->key = key;
    entry->val = val;

    return is_new_key;
}

bool hash_table_delete(hash_table* table, obj_string* key) {
    if (table->count == 0) {
        return false;
    }

    table_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    entry->key = NULL;
    entry->val = BOOL_VAL(true);
    return true;
}

void hash_table_add_all(hash_table* from, hash_table* to) {
    for (int i = 0; i < from->capacity; ++i) {
        table_entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            hash_table_set(to, entry->key, entry->val);
        }
    }
}

obj_string* hash_table_find_string(hash_table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;

    for (;;) {
        table_entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NULL(entry->val)) {
                return NULL;
            }
        } else if (entry->key->length == length &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            // We found it.
            return entry->key;
        }
        index = (index + 1) % table->capacity;
    }
}
