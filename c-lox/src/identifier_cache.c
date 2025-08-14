#include "identifier_cache.h"
#include <string.h>

#define IDENTIFIER_CACHE_MAX_LOAD 0.75
#define TOMBSTONE (object_string*)0x1

void init_identifier_cache(identifier_cache* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_identifier_cache(identifier_cache* table) {
    FREE_ARRAY(cache_entry, table->entries, table->capacity);
    init_identifier_cache(table);
}

static cache_entry* find_entry(cache_entry* entries, int capacity, object_string* key) {
    uint32_t index = key->hash % capacity;
    cache_entry* tombstone = NULL;

    while (true) {
        cache_entry* entry = &entries[index];

        if (entry->key == NULL) {
            return tombstone ? tombstone : entry;
        } else if (entry->key == TOMBSTONE) {
            if (tombstone == NULL) {
                tombstone = entry;
            }
        } else {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(identifier_cache* table, int capacity) {
    cache_entry* entries = ALLOCATE(cache_entry, capacity);
    for (int i = 0; i < capacity; ++i) {
        entries[i].key = NULL;
        entries[i].index = -1;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; ++i) {
        cache_entry* entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        // Rehash the old entries into their new position
        cache_entry* destination = find_entry(entries, capacity, entry->key);
        destination->key = entry->key;
        destination->index = entry->index;
        ++table->count;
    }

    FREE_ARRAY(cache_entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool identifier_cache_get(identifier_cache* table, object_string* key, int* index) {
    if (table->count == 0) {
        return false;
    }

    cache_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    *index = entry->index;
    return true;
}

bool identifier_cache_set(identifier_cache* table, object_string* key, int index) {
    if (table->count >= table->capacity * IDENTIFIER_CACHE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    cache_entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = (entry->key == NULL) || (entry->key == TOMBSTONE);
    if (is_new_key) {
        ++table->count;
    }

    entry->key = key;
    entry->index = index;
    return is_new_key;
}

bool identifier_cache_delete(identifier_cache* table, object_string* key) {
    if (table->count == 0) {
        return false;
    }

    cache_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    // Place a tombstone
    entry->key = TOMBSTONE;
    entry->index = 0;
    return true;
}
