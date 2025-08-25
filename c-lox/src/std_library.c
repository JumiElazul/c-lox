#include "memory.h"
#include "stdlib.h"
#include "utility.h"
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

static object_string* null_char(void) {
    char* nc = ALLOCATE(char, 1);
    object_string* s = take_string(nc, 0);
    return s;
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
    NATIVE_ARG_REQUIRE_RANGE("substring", argc, 2, 3);
    NATIVE_ARG_REQUIRE_STRING("substring", args, 0);
    NATIVE_ARG_REQUIRE_NUMBER("substring", args, 1);

    if (argc == 3) {
        NATIVE_ARG_REQUIRE_NUMBER("substring", args, 2);
    }

    object_string* src = AS_STRING(args[0]);
    const char* s = src->chars;
    int n = src->length;

    int start = (int)AS_NUMBER(args[1]);
    int end = (argc == 3) ? (int)AS_NUMBER(args[2]) : n;

    if (start < 0)
        start += n;
    if (end < 0)
        end += n;

    if (start < 0)
        start = 0;
    if (start > n)
        start = n;
    if (end < 0)
        end = 0;
    if (end > n)
        end = n;

    if (end < start)
        end = start;

    int sub_len = end - start;

    object_string* out = copy_string(s + start, sub_len);
    return OBJECT_VALUE(out);
}

static clox_value file_exists_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("file_exists", argc, 1, 1);
    NATIVE_ARG_REQUIRE_STRING("file_exists", args, 0);

    object_string* str = AS_STRING(args[0]);
    bool exists = file_exists(str->chars);
    return BOOL_VALUE(exists);
}

static clox_value read_file_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("read_file", argc, 1, 1);
    NATIVE_ARG_REQUIRE_STRING("read_file", args, 0);

    object_string* filepath = AS_STRING(args[0]);
    char* file = read_file(filepath->chars);

    if (file == NULL) {
        return OBJECT_VALUE(null_char());
    }

    size_t s = strlen(file);

    object_string* file_contents = take_string(file, s);
    return OBJECT_VALUE(file_contents);
}

static clox_value create_file_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("create_file", argc, 1, 1);
    NATIVE_ARG_REQUIRE_STRING("create_file", args, 0);

    object_string* filepath = AS_STRING(args[0]);
    if (!file_exists(filepath->chars)) {
        create_file(filepath->chars);
        return BOOL_VALUE(true);
    } else {
        return BOOL_VALUE(false);
    }
}

static clox_value append_file_native(int argc, clox_value* args) {
    NATIVE_ARG_REQUIRE_RANGE("append_file", argc, 2, 2);
    NATIVE_ARG_REQUIRE_STRING("append_file", args, 0);
    NATIVE_ARG_REQUIRE_STRING("append_file", args, 1);

    object_string* filepath = AS_STRING(args[0]);
    object_string* write = AS_STRING(args[1]);

    if (file_exists(filepath->chars)) {
        append_file(filepath->chars, write->chars);
        return BOOL_VALUE(true);
    } else {
        return BOOL_VALUE(false);
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

    virtual_machine_register_native("file_exists", file_exists_native, 1, 1);
    virtual_machine_register_native("read_file", read_file_native, 1, 1);
    virtual_machine_register_native("create_file", create_file_native, 1, 1);
    virtual_machine_register_native("append_file", append_file_native, 2, 2);
}
