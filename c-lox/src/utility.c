#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool file_exists(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(filesize + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), filesize, file);
    if (bytes_read < filesize) {
        fclose(file);
        free(buffer);
        return NULL;
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

void create_file(const char* path) {
    FILE* file = fopen(path, "wb");
    fclose(file);
}

void append_file(const char* path, const char* app) {
    if (!path || !app) {
        return;
    }

    FILE* file = fopen(path, "ab");
    if (!file) {
        return;
    }

    size_t len = strlen(app);
    fwrite(app, 1, len, file);
    fwrite("\n", 1, 1, file);
    fclose(file);
}
