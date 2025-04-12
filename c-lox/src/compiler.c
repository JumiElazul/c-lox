#include "compiler.h"
#include "bytecode_chunk.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "disassembler.h"
#endif

typedef struct token_parser {
    token current;
    token previous;
    bool had_error;
    bool panic_mode;
} token_parser;

typedef enum precedence {
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
    PREC_PRIMARY
} precedence;

typedef void (*parse_fn)(void);

typedef struct parse_rule {
    parse_fn prefix;
    parse_fn infix;
    precedence prec;
} parse_rule;

token_parser parser;
bytecode_chunk* compiling_chunk;

static parse_rule* get_rule(token_type type);
static void parse_expression(void);
static void parse_precedence(precedence prec);

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

static void emit_bytes2(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_return(void) {
    emit_byte(OP_RETURN);
}

// Adds a constant to the current chunks constant table, and then returns the index at which
// that constant is found.
static uint8_t make_constant(value val) {
    int constant = add_constant_to_chunk(current_chunk(), val);

    if (constant > UINT8_MAX) {
        error("Too many constants in current chunk\n");
        return 0;
    }

    return (uint8_t)constant;
}

// Writes two bytes to the current chunk.  The first byte is the necessary OP_CONSTANT opcode, and the
// second is the index at which the constant is found in the chunk's constant table.
static void emit_constant(value val) {
    emit_bytes2(OP_CONSTANT, make_constant(val));
}

static void end_compiler(void) {
    emit_return();

#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        disassemble_bytecode_chunk(current_chunk(), "code");
    }
#endif
}

static void binary(void) {
    // When a prefix parser function is called, the leading token has already been consumed.  In an infix
    // parser function, the entire left-hand operand expression has already been compiled and the subsequent infix
    // operator consumed.
    token_type operator_type = parser.previous.type;
    parse_rule* rule = get_rule(operator_type);
    parse_precedence((precedence)(rule->prec + 1));

    switch (operator_type) {
        case TOKEN_PLUS:  emit_byte(OP_ADD);      break;
        case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
        case TOKEN_STAR:  emit_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(OP_DIVIDE);   break;
        default: return;
    }
}

static void grouping(void) {
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after grouping expression.");
}

// When we find a number literal, there's nothing else to do but add the constant to the chunk's
// constant table and write the proper opcode.
static void number(void) {
    double val = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(val));
}

static void literal(void) {
    switch (parser.previous.type) {
        case TOKEN_NULL:  emit_byte(OP_NULL);  break;
        case TOKEN_TRUE:  emit_byte(OP_TRUE);  break;
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        default: return;
    }
}

static void unary(void) {
    token_type operator_type = parser.previous.type;

    // Given that unary should only match with expressions of itself or higher, we need to parse
    // an expression of the same precedence or higher level.  Think of an example like
    // -1.  Unary should match with 1 since a literal is of a higher precedence than unary.
    // However, in something like -a.b + c, the dot call expression is of higher, so we should
    // match to that, but then the following + is of lower precedence, so we don't care about that
    // part of the expression here.
    parse_precedence(PREC_UNARY);

    switch (operator_type) {
        case TOKEN_BANG:  emit_byte(OP_NOT);    break;
        case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
        default: return;
    }
}

static parse_rule rules[] = {
    [TOKEN_LEFT_PAREN]    = { grouping, NULL,     PREC_NONE   },
    [TOKEN_RIGHT_PAREN]   = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_LEFT_BRACE]    = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_RIGHT_BRACE]   = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_COMMA]         = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_DOT]           = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_MINUS]         = { unary,    binary,   PREC_TERM   },
    [TOKEN_PLUS]          = { NULL,     binary,   PREC_TERM   },
    [TOKEN_SEMICOLON]     = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_SLASH]         = { NULL,     binary,   PREC_FACTOR },
    [TOKEN_STAR]          = { NULL,     binary,   PREC_FACTOR },
    [TOKEN_BANG]          = { unary,    NULL,     PREC_NONE   },
    [TOKEN_BANG_EQUAL]    = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_EQUAL]         = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_EQUAL_EQUAL]   = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_GREATER]       = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_GREATER_EQUAL] = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_LESS]          = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_LESS_EQUAL]    = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_IDENTIFIER]    = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_STRING]        = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_NUMBER]        = { number,   NULL,     PREC_NONE   },
    [TOKEN_AND]           = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_CLASS]         = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_ELSE]          = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_FALSE]         = { literal,  NULL,     PREC_NONE   },
    [TOKEN_FOR]           = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_FUNC]          = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_IF]            = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_NULL]          = { literal,  NULL,     PREC_NONE   },
    [TOKEN_OR]            = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_PRINT]         = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_RETURN]        = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_SUPER]         = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_THIS]          = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_TRUE]          = { literal,  NULL,     PREC_NONE   },
    [TOKEN_VAR]           = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_WHILE]         = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_ERROR]         = { NULL,     NULL,     PREC_NONE   },
    [TOKEN_EOF]           = { NULL,     NULL,     PREC_NONE   },
};

static void parse_precedence(precedence prec) {
    advance_parser();
    // By definition, the previous token needs to have a prefix rule or else its not a valid expression.
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

static parse_rule* get_rule(token_type type) {
    return &rules[type];
}

// We map each token type to a different kind of expression. We define a function
// for each expression that outputs the appropriate bytecode. Then we build an
// array of function pointers. The indexes in the array correspond to the
// TokenType enum values, and the function at each index is the code to compile
// an expression of that token type.
static void parse_expression(void) {
    parse_precedence(PREC_ASSIGNMENT);
}

bool compile(const char* source, bytecode_chunk* chunk) {
    init_lexer(source);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance_parser();
    parse_expression();
    consume_if_matches(TOKEN_EOF, "Expected end of expression.");
    end_compiler();
    return !parser.had_error;
}
