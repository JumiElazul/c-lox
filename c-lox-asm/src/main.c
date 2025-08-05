#include "asm_lexer.h"
#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLOX_EXTENSION "cloxasm"

// Returns a buffer with the valid file contents, otherwise NULL.
static char* validate_file(const char* path) {
    const char* file_ext = read_file_ext(path);
    if (strcmp(file_ext, CLOX_EXTENSION) != 0) {
        printf("File extension does not match: %s\n", CLOX_EXTENSION);
        exit(EXIT_FAILURE);
    }

    char* assembly_file = read_file(path);
    if (!assembly_file) {
        return NULL;
    }
    return assembly_file;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: c-lox-asm <file>\n");
        exit(EXIT_FAILURE);
    }

    char* file = validate_file(argv[1]);
    printf("%s", file);

    asm_lexer lexer;
    init_asm_lexer(&lexer, file);

    free(file);
    return 0;
}
