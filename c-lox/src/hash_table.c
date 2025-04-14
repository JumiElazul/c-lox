#include "hash_table.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

void init_hash_table(hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_hash_table(hash_table* table) {
    FREE_ARRAY(table_entry, table->entries, table->capacity);
    init_hash_table(table);
}

bool set_hash_table(hash_table* table, obj_string* key, value val) {

}
