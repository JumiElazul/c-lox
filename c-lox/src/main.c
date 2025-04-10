#include "virtual_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void run_repl(void) {
    // This could be improved by not hard coding a line length limit.
    char line[1024];
    for (;;) {
        printf("clox > ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
    }

    virtual_machine_interpret(line);
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

int main(int argc, const char* argv[]) {
    init_virtual_machine();

    if (argc == 1) {
        run_repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "usage: clox [path]\n");
        exit(EXIT_FAILURE);
    }

    free_virtual_machine();
}
