#include "memory.h"
#include "stdlib.h"
#include "virtual_machine.h"
#include <ctype.h>
#include <editline/readline.h>
#include <string.h>
#include <time.h>

#define NATIVE_VARARGS -1, -1
#define NATIVE_ARG_UNBOUNDED -1

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

#define NATIVE_ARG_REQUIRE_STRING(fname, args, index)                                              \
    do {                                                                                           \
        if (!IS_STRING((args)[(index)])) {                                                         \
            NATIVE_FAIL("%s expects argument %d to be of type 'string'.", (fname), (index));       \
        }                                                                                          \
    } while (false)

#define NATIVE_ARG_REQUIRE_NUMBER(fname, args, index)                                              \
    do {                                                                                           \
        if (!IS_NUMBER((args)[(index)])) {                                                         \
            NATIVE_FAIL("%s expects argument %d to be of type 'number'.", (fname), (index));       \
        }                                                                                          \
    } while (false)

#define NATIVE_ARG_REQUIRE_BOOL(fname, args, index)                                                \
    do {                                                                                           \
        if (!IS_BOOL((args)[(index)])) {                                                           \
            NATIVE_FAIL("%s expects argument %d to be of type 'bool'.", (fname), (index));         \
        }                                                                                          \
    } while (false)

#define NATIVE_ARG_REQUIRE_RANGE(fname, argc, min_arity, max_arity)                                \
    do {                                                                                           \
        const int _min = (min_arity);                                                              \
        const int _max = (max_arity);                                                              \
        const int _n = (argc);                                                                     \
        if (((_min) >= 0 && _n < (_min)) || ((_max) >= 0 && _n > (_max))) {                        \
            if ((_min) >= 0 && (_max) >= 0 && (_min) == (_max)) {                                  \
                NATIVE_FAIL("%s expects %d argument%s.", (fname), (_min),                          \
                            ((_min) == 1 ? "" : "s"));                                             \
            } else if ((_min) >= 0 && (_max) >= 0) {                                               \
                NATIVE_FAIL("%s expects %d to %d arguments.", (fname), (_min), (_max));            \
            } else if ((_min) >= 0) {                                                              \
                NATIVE_FAIL("%s expects at least %d argument%s.", (fname), (_min),                 \
                            ((_min) == 1 ? "" : "s"));                                             \
            } else if ((_max) >= 0) {                                                              \
                NATIVE_FAIL("%s expects at most %d argument%s.", (fname), (_max),                  \
                            ((_max) == 1 ? "" : "s"));                                             \
            } else {                                                                               \
            }                                                                                      \
        }                                                                                          \
    } while (false)

static bool same_string(const char* str, int len, int (*char_fn)(int)) {
    for (int i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)str[i];
        if ((char)char_fn(ch) != str[i]) {
            return false;
        }
    }
    return true;
}

static char* char_op_impl(const char* str, int len, int (*char_fn)(int)) {
    char* out = ALLOCATE(char, len + 1);
    for (int i = 0; i < len; ++i) {
        out[i] = (char)char_fn((unsigned char)str[i]);
    }
    out[len] = '\0';
    return out;
}

static clox_value clock_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("clock", argc, 0, 0);
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
    if (argc == 1) {
        NATIVE_ARG_REQUIRE_STRING("get_line", args, 0);
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

static clox_value length_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("length", argc, 1, 1);
    NATIVE_ARG_REQUIRE_STRING("length", args, 0);

    object_string* string = AS_STRING(args[0]);
    return NUMBER_VALUE(string->length);
}

static clox_value to_upper_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("to_upper", argc, 1, 1);
    NATIVE_ARG_REQUIRE_STRING("to_upper", args, 0);

    object_string* s = AS_STRING(args[0]);
    const char* input_str = s->chars;
    int len = s->length;

    if (same_string(input_str, len, toupper)) {
        return OBJECT_VALUE(s);
    }

    char* out = char_op_impl(input_str, len, toupper);
    object_string* res = take_string(out, len);
    return OBJECT_VALUE(res);
}

static clox_value to_lower_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("to_lower", argc, 1, 1);
    NATIVE_ARG_REQUIRE_STRING("to_lower", args, 0);

    object_string* s = AS_STRING(args[0]);
    const char* input_str = s->chars;
    int len = s->length;

    if (same_string(input_str, len, tolower)) {
        return OBJECT_VALUE(s);
    }

    char* out = char_op_impl(input_str, len, tolower);
    object_string* res = take_string(out, len);
    return OBJECT_VALUE(res);
}

static clox_value substring_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("to_lower", argc, 2, 3);
    NATIVE_ARG_REQUIRE_STRING("to_lower", args, 0);
    NATIVE_ARG_REQUIRE_NUMBER("to_lower", args, 1);

    int end_pos = -1;
    if (argc == 3) {
        NATIVE_ARG_REQUIRE_NUMBER("to_lower", args, 2);
        end_pos = AS_NUMBER(args[2]);
    }
}

void stdlib_init(void) {
    virtual_machine_register_native("clock", clock_native, 0, 0);

    virtual_machine_register_native("print", print_native, NATIVE_VARARGS);
    virtual_machine_register_native("println", println_native, NATIVE_VARARGS);
    virtual_machine_register_native("get_line", get_line_native, 0, 1);

    virtual_machine_register_native("length", length_native, 1, 1);
    virtual_machine_register_native("to_upper", to_upper_native, 1, 1);
    virtual_machine_register_native("to_lower", to_lower_native, 1, 1);
    virtual_machine_register_native("substring", substring_native, 2, 3);
}
