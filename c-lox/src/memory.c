#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

// The two size arguments passed to reallocate() control which operation to perform:
// old_size---new_size----------------operation
// 0          Non-zero                Allocate new block
// Non-zero   0                       Free allocation
// Non-zero   Smaller than old_size   Shrink existing allocation
// Non-zero   Larger than old_size    Grow existing allocation
void* reallocate(void* pointer, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) {
        fprintf(stderr, "Error allocating memory in reallocate()\n");
        exit(EXIT_FAILURE);
    }

    return result;
}
