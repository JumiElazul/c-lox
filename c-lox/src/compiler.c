#include "compiler.h"
#include "bytecode_chunk.h"
#include "common.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "disassembler.h"
#endif

typedef struct {
    token current;
    token previous;
    bool had_error;
    bool panic_mode;
} token_parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} precedence;

typedef void (*parse_fn)(void);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    precedence prec;
} parse_rule;

token_parser parser;
bytecode_chunk* compiling_chunk;

static bytecode_chunk* current_chunk(void) { return compiling_chunk; }

static void parse_expression(void);

static void error_at(token* t, const char* message) {
    if (parser.panic_mode) {
        return;
    }

    parser.panic_mode = true;

    fprintf(stderr, "[line %d] Error", t->line);

    if (t->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (t->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at '%.*s'", t->length, t->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void error(const char* message) { error_at(&parser.previous, message); }

static void error_at_current(const char* message) { error_at(&parser.current, message); }

static void advance_parser(void) {
    parser.previous = parser.current;

    while (true) {
        parser.current = lexer_scan_token();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        error_at_current(parser.current.start);
    }
}

static void consume_if_matches(token_type type, const char* message) {
    if (parser.current.type == type) {
        advance_parser();
        return;
    }

    error_at_current(message);
}

static void emit_byte(uint8_t byte) {
    write_to_bytecode_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes2(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_bytes3(uint8_t byte1, uint8_t byte2, uint8_t byte3) {
    emit_byte(byte1);
    emit_byte(byte2);
    emit_byte(byte3);
}

static void emit_bytes4(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {
    emit_byte(byte1);
    emit_byte(byte2);
    emit_byte(byte3);
    emit_byte(byte4);
}

static void emit_return(void) { emit_byte(OP_RETURN); }

static void emit_constant(clox_value val) {
    write_constant(current_chunk(), val, parser.previous.line);
}

static void end_compilation(void) {
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void parse_expression(void);
static parse_rule* get_rule(token_type type);
static void parse_precedence(precedence prec);

static void binary(void) {
    token_type operator_type = parser.previous.type;
    parse_rule* rule = get_rule(operator_type);
    parse_precedence((precedence)(rule->prec + 1));

    switch (operator_type) {
        case TOKEN_BANG_EQUAL:
            emit_bytes2(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_byte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emit_byte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_bytes2(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emit_byte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emit_bytes2(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emit_byte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emit_byte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emit_byte(OP_DIVIDE);
            break;
        default:
            return;
    }
}

static void literal(void) {
    switch (parser.previous.type) {
        case TOKEN_NULL: {
            emit_byte(OP_NULL);
        } break;
        case TOKEN_TRUE: {
            emit_byte(OP_TRUE);
        } break;
        case TOKEN_FALSE: {
            emit_byte(OP_FALSE);
        } break;
        default:
            return;
    }
}

static void grouping(void) {
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void number(void) {
    double val = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VALUE(val));
}

static void unary(void) {
    token_type operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY);

    switch (operator_type) {
        case TOKEN_BANG: {
            emit_byte(OP_NOT);
        } break;
        case TOKEN_MINUS: {
            emit_byte(OP_NEGATE);
        } break;
        default:
            return;
    }
}

parse_rule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNC] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NULL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static void parse_precedence(precedence prec) {
    advance_parser();

    parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expected expression.");
        return;
    }

    prefix_rule();

    while (prec <= get_rule(parser.current.type)->prec) {
        advance_parser();
        parse_fn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
    }
}

static parse_rule* get_rule(token_type type) { return &rules[type]; }

static void parse_expression(void) { parse_precedence(PREC_ASSIGNMENT); }

bool compile(const char* source_code, bytecode_chunk* chunk) {
    init_lexer(source_code);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance_parser();
    parse_expression();
    consume_if_matches(TOKEN_EOF, "Expect end of expression.");
    end_compilation();
    return !parser.had_error;
}
