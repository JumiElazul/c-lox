#include "stdlib.h"
#include "virtual_machine.h"
#include <editline/readline.h>
#include <string.h>
#include <time.h>

#define NATIVE_VARARGS -1, -1

#define NATIVE_FAIL(...)                                                                           \
    do {                                                                                           \
        virtual_machine_native_errorf(__VA_ARGS__);                                                \
        return NULL_VALUE;                                                                         \
    } while (false)
#define NATIVE_REQUIRE(condition, ...)                                                             \
    do {                                                                                           \
        if (!(condition))                                                                          \
            NATIVE_FAIL(__VA_ARGS__);                                                              \
    } while (false)

static clox_value clock_native(int argc, clox_value* args) {
    (void)args;
    return NUMBER_VALUE((double)clock() / CLOCKS_PER_SEC);
}

static clox_value print_native(int argc, clox_value* args) {
    for (int i = 0; i < argc; ++i) {
        print_value(args[i]);
    }
    return NULL_VALUE;
}

static clox_value println_native(int argc, clox_value* args) {
    for (int i = 0; i < argc; ++i) {
        print_value(args[i]);
    }
    printf("\n");
    return NULL_VALUE;
}

static clox_value get_line_native(int argc, clox_value* args) {
    NATIVE_REQUIRE(argc <= 1, "get_line takes 0 or 1 arguments.");

    char* prompt = NULL;
    if (argc == 1) {
        print_value(args[0]);
    }

    char* line = readline("");

    if (!line) {
        NATIVE_FAIL("input cancelled (EOF)");
    }

    object_string* s = copy_string(line, (int)strlen(line));
    free(line);
    return OBJECT_VALUE(s);
}

void stdlib_init(void) {
    virtual_machine_register_native("clock", clock_native, 0, 0);
    virtual_machine_register_native("print", print_native, NATIVE_VARARGS);
    virtual_machine_register_native("println", println_native, NATIVE_VARARGS);
    virtual_machine_register_native("get_line", get_line_native, 0, 1);
}
