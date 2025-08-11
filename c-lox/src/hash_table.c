#include "hash_table.h"
#include <string.h>

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
    table_entry* tombstone = NULL;

    while (true) {
        table_entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NULL(entry->val)) {
                // Empty entry
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
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

    table->count = 0;
    for (int i = 0; i < table->capacity; ++i) {
        table_entry* entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        // Rehash the old entries into their new position
        table_entry* destination = find_entry(entries, capacity, entry->key);
        destination->key = entry->key;
        destination->val = entry->val;
        ++table->count;
    }

    FREE_ARRAY(table_entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool hash_table_get(hash_table* table, object_string* key, clox_value* val) {
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

bool hash_table_set(hash_table* table, object_string* key, clox_value val) {
    if (table->count >= table->capacity * HASH_TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    table_entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == NULL;

    // Only increment the count if it's truly a new entry and not a tombstone.
    if (is_new_key && IS_NULL(entry->val)) {
        ++table->count;
    }

    entry->key = key;
    entry->val = val;
    return is_new_key;
}

bool hash_table_delete(hash_table* table, object_string* key) {
    if (table->count == 0) {
        return false;
    }

    table_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    // Place a tombstone
    entry->key = NULL;
    entry->val = BOOL_VALUE(true);
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

object_string* table_find_string(hash_table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }

    uint32_t index = hash % table->capacity;
    while (true) {
        table_entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            if (IS_NULL(entry->val)) {
                return NULL;
            }
        } else if (entry->key->length == length && entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}
