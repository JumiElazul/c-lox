#include "virtual_machine.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void run_repl(void) {
    // This could be improved by not hard coding a line length limit.
    printf("clox repl mode ('q' or 'quit' to quit)\n");
    char line[1024];
    for (;;) {
        printf("clox > ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) {
            return;
        }

        virtual_machine_interpret(line);
    }
}

static char* read_file(const char* filepath) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        perror("File could not be opened\n");
        exit(74);
    }

    fseek(fp, 0L, SEEK_END);
    unsigned long filesize = ftell(fp);
    rewind(fp);

    char* buffer = (char*)malloc(filesize + 1);
    if (!buffer) {
        fprintf(stderr, "Error allocating memory for file with filepath %s\n", filepath);
        fclose(fp);
        exit(74);
    }

    size_t bytes_read = fread(buffer, 1, filesize, fp);

    if (bytes_read < filesize) {
        fprintf(stderr, "Failed to read contents from file with filepath %s", filepath);
        fclose(fp);
        exit(74);
    }

    buffer[bytes_read] = '\0';
    fclose(fp);
    return buffer;
}

static void run_file(const char* filepath) {
    char* source = read_file(filepath);
    interpret_result result = virtual_machine_interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) {
        exit(65);
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
        exit(70);
    }
}

static bool matches_ndebug(const char* candidate) {
    return strcmp(candidate, "--ndebug") == 0;
}

static void turn_off_debug(void) {
    debug_print_code = false;
    debug_trace_execution = false;
}

int main(int argc, const char* argv[]) {
    init_virtual_machine();

    if (argc == 1) {
        run_repl();
    } else if (argc == 2) {
        if (matches_ndebug(argv[1])) {
            turn_off_debug();
            run_repl();
        } else {
            run_file(argv[1]);
        }
    } else if (argc == 3) {
        int ndebug_index = -1;
        if (matches_ndebug(argv[1])) {
            ndebug_index = 1;
            turn_off_debug();
        } else if (matches_ndebug(argv[2])) {
            ndebug_index = 2;
            turn_off_debug();
        }
        int file_index = ndebug_index == 1 ? 2 : 1;
        run_file(argv[file_index]);
    } else {
        fprintf(stderr, "usage: clox [path] [--ndebug]\n");
        fprintf(stderr, "not providing path will run in repl mode.\n");
        exit(EXIT_FAILURE);
    }

    free_virtual_machine();
}
