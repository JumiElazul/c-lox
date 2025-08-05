#include "asm_lexer.h"
#include "common.h"
#include <string.h>

void init_asm_lexer(asm_lexer* lexer, const char* source_code) {
    lexer->start = source_code;
    lexer->current = source_code;
    lexer->line = 1;
}

static asm_token make_token(asm_lexer* lexer, asm_token_type type) {
    asm_token tok;
    tok.type = type;
    tok.start = lexer->start;
    tok.length = (int)(lexer->current - lexer->start);
    tok.line = lexer->line;
    return tok;
}

static asm_token error_token(asm_lexer* lexer, const char* message) {
    asm_token tok;
    tok.type = ASM_TOKEN_ERROR;
    tok.start = message;
    tok.length = (int)strlen(message);
    tok.line = lexer->line;
    return tok;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static char advance_lexer(asm_lexer* lexer) {
    ++lexer->current;
    return lexer->current[-1];
}

static bool is_at_end(asm_lexer* lexer) { return *lexer->current == '\0'; }

static char peek(asm_lexer* lexer) { return *lexer->current; }

static char peek_next(asm_lexer* lexer) {
    if (is_at_end(lexer)) {
        return '\0';
    }

    return lexer->current[1];
}

static void skip_whitespace(asm_lexer* lexer) {
    while (true) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance_lexer(lexer);
                break;
            case '\n':
                ++lexer->line;
                advance_lexer(lexer);
                break;
            case ';':
                while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                    advance_lexer(lexer);
                }
                break;
            default:
                return;
        }
    }
}
