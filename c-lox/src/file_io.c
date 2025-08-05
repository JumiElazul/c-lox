#include "file_io.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* read_file_ext(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext || ext == path) {
        return "";
    }
    return ext + 1;
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "File with path \"%s\" could not be opened for reading.\n", path);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(filesize + 1);
    if (!buffer) {
        fprintf(stderr, "Memory could not be allocated for filepath \"%s\"\n", path);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(buffer, sizeof(char), filesize, file);
    if (bytes_read < filesize) {
        fprintf(stderr, "Could not read file \"%s\" properly.\n", path);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}
