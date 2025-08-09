#include "hash_table.h"

void init_hash_table(hash_table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}
