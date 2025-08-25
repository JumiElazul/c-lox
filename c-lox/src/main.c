#include "editline/readline.h"
#include "utility.h"
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

static void run_file(const char* path) {
    char* source_code = read_file(path);

    if (source_code == NULL) {
        fprintf(stderr, "Could not read file with filepath %s\n", path);
        return;
    }

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
