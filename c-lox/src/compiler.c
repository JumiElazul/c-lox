#include "compiler.h"
#include "bytecode_chunk.h"
#include "clox_object.h"
#include "common.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "disassembler.h"
#endif

// declaration → classDecl
//             | funcDecl
//             | varDecl
//             | statement ;

// statement   → exprStmt
//             | forStmt
//             | ifStmt
//             | printStmt
//             | returnStmt
//             | whileStmt
//             | block ;

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
static void statement(void);
static void declaration_statement(void);
static int identifier_constant(token* name);

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

static bool check_token(token_type type) { return parser.current.type == type; }

static bool matches_token(token_type type) {
    if (!check_token(type)) {
        return false;
    }
    advance_parser();
    return true;
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
    int index = add_constant(current_chunk(), val);

    if (index <= 255) {
        emit_bytes2(OP_CONSTANT, index);
    } else {
        u24_t i = construct_u24_t(index);
        emit_bytes4(OP_CONSTANT_LONG, i.hi, i.mid, i.lo);
    }
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

static void string(void) {
    emit_constant(OBJECT_VALUE(copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void named_variable(token name) {
    int arg = identifier_constant(&name);
    if (arg <= 255) {
        emit_bytes2(OP_GET_GLOBAL, arg);
    } else {
        u24_t i = construct_u24_t(arg);
        emit_bytes4(OP_GET_GLOBAL_LONG, i.hi, i.mid, i.lo);
    }
}

static void variable(void) { named_variable(parser.previous); }

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

static void debug_statement(void) {
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after debug statement.");
    emit_byte(OP_DEBUG);
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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
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

    [TOKEN_DEBUG] = {NULL, NULL, PREC_NONE},
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

static int make_constant(clox_value val) {
    int constant = add_constant(current_chunk(), val);

    if ((unsigned int)constant >= U24T_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return constant;
}

static int identifier_constant(token* name) {
    return make_constant(OBJECT_VALUE(copy_string(name->start, name->length)));
}

static int parse_variable(const char* err_msg) {
    consume_if_matches(TOKEN_IDENTIFIER, err_msg);
    return identifier_constant(&parser.previous);
}

static void define_variable(int global) {
    if (global <= 255) {
        emit_bytes2(OP_DEFINE_GLOBAL, global);
    } else {
        u24_t i = construct_u24_t(global);
        emit_bytes4(OP_DEFINE_GLOBAL_LONG, i.hi, i.mid, i.lo);
    }
}

static parse_rule* get_rule(token_type type) { return &rules[type]; }

static void parse_expression(void) { parse_precedence(PREC_ASSIGNMENT); }

static void print_statement(void) {
    parse_expression();
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_PRINT);
}

static void expression_statement(void) {
    parse_expression();
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_POP);
}

static void variable_declaration(void) {
    uint8_t global = parse_variable("Expected variable name.");

    if (matches_token(TOKEN_EQUAL)) {
        parse_expression();
    } else {
        emit_byte(OP_NULL);
    }

    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    define_variable(global);
}

static void synchronize(void) {
    parser.panic_mode = false;

    // When we are in panic mode, we want to continuously discard tokens until we hit a natural
    // point to start again.  This means finding a semicolon, or the beginning of any declaration
    // statements.
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUNC:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:;
        }

        advance_parser();
    }
}

static void declaration_statement(void) {
    if (matches_token(TOKEN_VAR)) {
        variable_declaration();
    } else {
        statement();
    }

    if (parser.panic_mode) {
        synchronize();
    }
}

static void statement(void) {
    if (matches_token(TOKEN_DEBUG)) {
        debug_statement();
        return;
    }

    if (matches_token(TOKEN_PRINT)) {
        print_statement();
    } else {
        expression_statement();
    }
}

bool compile(const char* source_code, bytecode_chunk* chunk) {
    init_lexer(source_code);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance_parser();

    while (!matches_token(TOKEN_EOF)) {
        declaration_statement();
    }

    end_compilation();
    return !parser.had_error;
}
