#include "memory.h"
#include <stdlib.h>

// oldSize           newSize                  Operation
// 0                 Non‑zero                 Allocate new block.
// Non‑zero          0                        Free allocation.
// Non‑zero          Smaller than oldSize     Shrink existing allocation.
// Non‑zero          Larger than oldSize      Grow existing allocation.
void* reallocate(void* pointer, [[maybe_unused]] size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) {
        exit(EXIT_FAILURE);
    }
    return result;
}
