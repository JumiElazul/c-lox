#include "compiler.h"
#include "bytecode_chunk.h"
#include "common.h"
#include "lexer.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    token current;
    token previous;
    bool had_error;
    bool panic_mode;
} parser;

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

typedef void (*parse_fn)(bool can_assign);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    precedence prec;
} parse_rule;

parser parse;
bytecode_chunk* compiling_chunk;

static void parse_expression();
static parse_rule* get_rule(token_type type);
static void parse_precedence(precedence prec);
static void declaration();
static void statement();
static uint8_t identifier_constant(token* name);

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
        fprintf(stderr, " at '%.*s'", (uint32_t)tok->length, tok->start);
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

static void must_consume_token(token_type type, const char* message) {
    if (parse.current.type == type) {
        advance_parser();
        return;
    }

    error_at_current(message);
}

static bool check_token(token_type type) {
    return parse.current.type == type;
}

static bool matches_token(token_type expected) {
    if (!check_token(expected)) {
        return false;
    }

    advance_parser();
    return true;
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

static uint8_t make_constant(clox_value value) {
    int constant = add_constant(current_chunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

static void emit_constant(clox_value value) {
    emit_bytes2(OP_CONSTANT, make_constant(value));
}

static void end_compiler() {
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parse.had_error) {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void binary(bool can_assign) {
    token_type operator_type = parse.previous.type;
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

static void literal(bool can_assign) {
    switch (parse.previous.type) {
        case TOKEN_FALSE: {
            emit_byte(OP_FALSE);
        } break;
        case TOKEN_NULL: {
            emit_byte(OP_NULL);
        } break;
        case TOKEN_TRUE: {
            emit_byte(OP_TRUE);
        } break;
        default:
            return;
    }
}

static void grouping(bool can_assign) {
    parse_expression();
    must_consume_token(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void number(bool can_assign) {
    double value = strtod(parse.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void string(bool can_assign) {
    object_string* str = copy_string(parse.previous.start + 1, parse.previous.length - 2);
    emit_constant(OBJECT_VAL(str));
}

static void named_variable(token name, bool can_assign) {
    uint8_t arg = identifier_constant(&name);

    if (can_assign && matches_token(TOKEN_EQUAL)) {
        parse_expression();
        emit_bytes2(OP_SET_GLOBAL, arg);
    } else {
        emit_bytes2(OP_GET_GLOBAL, arg);
    }
}

static void variable(bool can_assign) {
    named_variable(parse.previous, can_assign);
}

static void unary(bool can_assign) {
    token_type operator_type = parse.previous.type;

    parse_precedence(PREC_UNARY);

    switch (operator_type) {
        case TOKEN_BANG:
            emit_byte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_NEGATE);
            break;
        default:
            return;
    }
}

// clang-format off
parse_rule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,    PREC_NONE       },
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_COMMA]         = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_DOT]           = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_MINUS]         = {unary,    binary,  PREC_TERM       },
    [TOKEN_PLUS]          = {NULL,     binary,  PREC_TERM       },
    [TOKEN_SEMICOLON]     = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_SLASH]         = {NULL,     binary,  PREC_FACTOR     },
    [TOKEN_STAR]          = {NULL,     binary,  PREC_FACTOR     },
    [TOKEN_BANG]          = {unary,    NULL,    PREC_NONE       },
    [TOKEN_BANG_EQUAL]    = {NULL,     binary,  PREC_EQUALITY   },
    [TOKEN_EQUAL]         = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,  PREC_EQUALITY   },
    [TOKEN_GREATER]       = {NULL,     binary,  PREC_COMPARISON },
    [TOKEN_GREATER_EQUAL] = {NULL,     binary,  PREC_COMPARISON },
    [TOKEN_LESS]          = {NULL,     binary,  PREC_COMPARISON },
    [TOKEN_LESS_EQUAL]    = {NULL,     binary,  PREC_COMPARISON },
    [TOKEN_IDENTIFIER]    = {variable, NULL,    PREC_NONE       },
    [TOKEN_STRING]        = {string,   NULL,    PREC_NONE       },
    [TOKEN_NUMBER]        = {number,   NULL,    PREC_NONE       },
    [TOKEN_AND]           = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_CLASS]         = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_ELSE]          = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_FALSE]         = {literal,  NULL,    PREC_NONE       },
    [TOKEN_FOR]           = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_FUN]           = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_IF]            = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_NULL]          = {literal,  NULL,    PREC_NONE       },
    [TOKEN_OR]            = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_PRINT]         = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_RETURN]        = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_SUPER]         = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_THIS]          = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_TRUE]          = {literal,  NULL,    PREC_NONE       },
    [TOKEN_VAR]           = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_WHILE]         = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_ERROR]         = {NULL,     NULL,    PREC_NONE       },
    [TOKEN_EOF]           = {NULL,     NULL,    PREC_NONE       },
};
// clang-format on

static void parse_precedence(precedence prec) {
    advance_parser();

    parse_fn prefix_rule = get_rule(parse.previous.type)->prefix;
    if (!prefix_rule) {
        error("Expected expression.");
        return;
    }

    bool can_assign = prec <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while (prec <= get_rule(parse.current.type)->prec) {
        advance_parser();
        parse_fn infix_rule = get_rule(parse.previous.type)->infix;
        if (!infix_rule) {
            error("Expected valid infix rule.");
            return;
        }
        infix_rule(can_assign);
    }

    if (can_assign && matches_token(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint8_t identifier_constant(token* name) {
    return make_constant(OBJECT_VAL(copy_string(name->start, name->length)));
}

static uint8_t parse_variable(const char* err_message) {
    must_consume_token(TOKEN_IDENTIFIER, err_message);
    return identifier_constant(&parse.previous);
}

static void define_variable(uint8_t global, bool is_const) {
    uint8_t instr = is_const ? OP_DEFINE_CONST_GLOBAL : OP_DEFINE_GLOBAL;
    emit_bytes2(instr, global);
}

static parse_rule* get_rule(token_type type) {
    return &rules[type];
}

static void parse_expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

static void variable_declaration(bool is_const) {
    uint8_t global = parse_variable("Expected variable name.");

    if (matches_token(TOKEN_EQUAL)) {
        parse_expression();
    } else {
        if (is_const) {
            error_at_current("Variable marked 'const' must have an initializer expression.");
        } else {
            emit_byte(OP_NULL);
        }
    }

    must_consume_token(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

    define_variable(global, is_const);
}

static void const_variable_declaration() {
    if (!matches_token(TOKEN_VAR)) {
        error_at_current("Expected 'var' keyword after const declaration.");
    } else {
        variable_declaration(true);
    }
}

static void print_statement() {
    must_consume_token(TOKEN_LEFT_PAREN, "Expected '(' after print statement.");
    parse_expression();
    must_consume_token(TOKEN_RIGHT_PAREN, "Expected ')' to close print statement.");
    must_consume_token(TOKEN_SEMICOLON, "Expected ';' after print statement closing.");
    emit_byte(OP_PRINT);
}

static void debug_statement() {
    emit_byte(OP_DEBUG);
    must_consume_token(TOKEN_SEMICOLON, "Expected ';' after debug statement.");
}

static void synchronize() {
    parse.panic_mode = false;

    while (parse.current.type != TOKEN_EOF) {
        if (parse.previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (parse.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
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

static void expression_statement() {
    parse_expression();
    must_consume_token(TOKEN_SEMICOLON, "Expected ';' after expression.");
    emit_byte(OP_POP);
}

static void declaration() {
    if (matches_token(TOKEN_CONST)) {
        const_variable_declaration();
    } else if (matches_token(TOKEN_VAR)) {
        variable_declaration(false);
    } else {
        statement();
    }

    if (parse.panic_mode) {
        synchronize();
    }
}

static void statement() {
    if (matches_token(TOKEN_PRINT)) {
        print_statement();
    } else if (matches_token(TOKEN_DEBUG)) {
        debug_statement();
    } else {
        expression_statement();
    }
}

bool compile(const char* source, bytecode_chunk* chunk) {
    init_lexer(source);

    compiling_chunk = chunk;
    parse.had_error = false;
    parse.panic_mode = false;

    advance_parser();

    while (!matches_token(TOKEN_EOF)) {
        declaration();
    }

    end_compiler();
    return !parse.had_error;
}
