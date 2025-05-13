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
    return (float)table->count / (float)table->capacity;
}

static table_entry* find_entry(table_entry* entries, int capacity, obj_string* key) {
    uint32_t index = key->hash % capacity;
    // If the key at the index is NULL, the bucket is empty.  If we are using find_entry to look up
    // something in the hash table, this means it isn't there.  If we're using it to insert, it means
    // we've found the place to add the new entry.

    // If the key in the bucket is equal to the key we're looking for, then that key is already present
    // in the table.  If we're doing a lookup, we've found the key we seek.  If we're doing an insert,
    // we will replace the value for that key instead of adding a new entry.

    // Otherwise, the bucket has an entry with it, but with a different key (collision).  We start our linear
    // probing to find the next avaiable spot for the key.
    for (;;) {
        table_entry* entry = &entries[index];
        if (entry->key == key || entry->key == NULL) {
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
    if (is_new_key) {
        ++table->count;
    }

    entry->key = key;
    entry->val = val;

    return is_new_key;
}

void hash_table_add_all(hash_table* from, hash_table* to) {
    for (int i = 0; i < from->capacity; ++i) {
        table_entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            hash_table_set(to, entry->key, entry->val);
        }
    }
}
