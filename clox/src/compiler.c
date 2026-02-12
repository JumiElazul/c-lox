#include "compiler.h"
#include "bytecode_chunk.h"
#include "common.h"
#include "lexer.h"
#include <stdio.h>

typedef struct {
    token current;
    token previous;
    bool had_error;
    bool panic_mode;
} parser;

parser parse;
bytecode_chunk* compiling_chunk;

static bytecode_chunk* current_chunk() {
    return compiling_chunk;
}

static void error_at(token* tok, const char* message) {
    if (parse.panic_mode) {
        return;
    }

    parse.panic_mode = true;
    fprintf(stderr, "[line %d] Error", tok->line);

    if (tok->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (tok->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at '%.*s'", tok->length, tok->start);
    }

    fprintf(stderr, ": %s\n", message);
    parse.had_error = true;
}

static void error(const char* message) {
    error_at(&parse.previous, message);
}

static void error_at_current(const char* message) {
    error_at(&parse.current, message);
}

static void advance_parser() {
    parse.previous = parse.current;

    for (;;) {
        parse.current = scan_token();
        if (parse.current.type != TOKEN_ERROR) {
            break;
        }

        error_at_current(parse.current.start);
    }
}

static void consume_if_matches(token_type type, const char* message) {
    if (parse.current.type == type) {
        advance_parser();
        return;
    }

    error_at_current(message);
}

static void emit_byte(uint8_t byte) {
    write_bytecode_chunk(current_chunk(), byte, parse.previous.line);
}

static void emit_bytes2(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_return() {
    emit_byte(OP_RETURN);
}

static void end_compiler() {
    emit_return();
}

bool compile(const char* source, bytecode_chunk* chunk) {
    init_lexer(source);

    compiling_chunk = chunk;
    parse.had_error = false;
    parse.panic_mode = false;

    advance_parser();
    expression();
    consume_if_matches(TOKEN_EOF, "Expect end of expression.");
    end_compiler();
    return !parse.had_error;
}
