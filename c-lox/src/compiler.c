#include "compiler.h"
#include "bytecode_chunk.h"
#include "lexer.h"
#include <stdio.h>

typedef struct token_parser {
    token current;
    token previous;
    bool had_error;
    bool panic_mode;
} token_parser;

token_parser parser;
bytecode_chunk* compiling_chunk;

static bytecode_chunk* current_chunk(void) {
    return compiling_chunk;
}

static void error_at(token* tok, const char* msg) {
    if (parser.panic_mode) {
        return;
    }
    parser.panic_mode = true;

    fprintf(stderr, "[line %d] Error", tok->line);

    if (tok->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (tok->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at '%.*s'", tok->length, tok->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser.had_error = true;
}

static void error(const char* msg) {
    error_at(&parser.previous, msg);
}

static void error_at_current(const char* msg) {
    error_at(&parser.current, msg);
}

static void advance_parser(void) {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        error_at_current(parser.current.start);
    }
}

static void consume_if_matches(token_type type, const char* msg) {
    if (parser.current.type == type) {
        advance_parser();
        return;
    }

    error_at_current(msg);
}

static void emit_byte(uint8_t byte) {
    write_to_bytecode_chunk(current_chunk(), byte, parser.previous.line);
}

static void expression(void) {

}

bool compile(const char* source, bytecode_chunk* chunk) {
    init_lexer(source);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance_parser();
    expression();
    consume_if_matches(TOKEN_EOF, "Expected end of expression.");

    return !parser.had_error;
}
