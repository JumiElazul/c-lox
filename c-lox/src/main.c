#include "editline/history.h"
#include "editline/readline.h"
#include "virtual_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    ERR_FILE_UNOPENABLE = 1,
    ERR_MEM_ALLOC,
    ERR_COMPILE,
    ERR_RUNTIME,
} err_code;

static void run_repl(void) {
    char* line = NULL;
    while ((line = readline("clox > ")) != NULL) {
        add_history(line);

        if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) {
            free(line);
            break;
        }

        virtual_machine_interpret(line);

        free(line);
    }
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "File with path \"%s\" could not be opened for reading.\n", path);
        exit(ERR_FILE_UNOPENABLE);
    }

    fseek(file, 0L, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(filesize + 1);
    if (!buffer) {
        fprintf(stderr, "Memory could not be allocated for filepath \"%s\"\n", path);
        fclose(file);
        exit(ERR_MEM_ALLOC);
    }

    size_t bytes_read = fread(buffer, sizeof(char), filesize, file);
    if (bytes_read < filesize) {
        fprintf(stderr, "Could not read file \"%s\" properly.\n", path);
        fclose(file);
        exit(ERR_MEM_ALLOC);
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

static void run_file(const char* path) {
    char* source_code = read_file(path);
    interpret_result result = virtual_machine_interpret(source_code);
    free(source_code);

    if (result == INTERPRET_COMPILE_ERROR) {
        exit(ERR_COMPILE);
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
        exit(ERR_RUNTIME);
    }
}

int main(int argc, const char* argv[]) {
    init_virtual_machine();

    if (argc == 1) {
        run_repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
    }

    return 0;
}
