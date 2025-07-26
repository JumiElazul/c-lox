#include "lexer.h"
#include "common.h"
#include <string.h>

typedef struct {
    const char* start;
    const char* current;
    int line;
} lexer;

lexer lex;

void init_lexer(const char* source_code) {
    lex.start = source_code;
    lex.current = source_code;
    lex.line = 1;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static token make_token(token_type type) {
    token tok;
    tok.type = type;
    tok.start = lex.start;
    tok.length = (int)(lex.current - lex.start);
    tok.line = lex.line;
    return tok;
}

static token error_token(const char* message) {
    token tok;
    tok.type = TOKEN_ERROR;
    tok.start = message;
    tok.length = (int)strlen(message);
    tok.line = lex.line;
    return tok;
}

static char advance_lexer(void) {
    ++lex.current;
    return lex.current[-1];
}

static bool is_at_end(void) { return *lex.current == '\0'; }

static char peek(void) { return *lex.current; }

static char peek_next(void) {
    if (is_at_end()) {
        return '\0';
    }

    return lex.current[1];
}

static void skip_whitespace(void) {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance_lexer();
                break;
            case '\n':
                ++lex.line;
                advance_lexer();
                break;
            case '/':
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_at_end()) {
                        advance_lexer();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static token_type identifier_type(void) { return TOKEN_IDENTIFIER; }

static token identifier(void) {
    while (is_alpha(peek()) || is_digit(peek())) {
        advance_lexer();
    }
    return make_token(identifier_type());
}

static token number(void) {
    while (is_digit(peek())) {
        advance_lexer();
    }

    if (peek() == '.' && is_digit(peek_next())) {
        advance_lexer();

        while (is_digit(peek())) {
            advance_lexer();
        }
    }

    return make_token(TOKEN_NUMBER);
}

static token string(void) {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            ++lex.line;
        }
        advance_lexer();
    }

    if (is_at_end()) {
        return error_token("Uneterminated string.");
    }

    advance_lexer();
    return make_token(TOKEN_STRING);
}

static bool matches_token(char expected) {
    if (is_at_end()) {
        return false;
    }
    if (*lex.current != expected) {
        return false;
    }

    ++lex.current;
    return true;
}

token lexer_scan_token(void) {
    skip_whitespace();

    lex.start = lex.current;
    ;
    if (is_at_end()) {
        return make_token(TOKEN_EOF);
    }

    char c = advance_lexer();
    if (is_alpha(c)) {
        return identifier();
    }
    if (is_digit(c)) {
        return number();
    }

    switch (c) {
        case '(':
            return make_token(TOKEN_LEFT_PAREN);
        case ')':
            return make_token(TOKEN_RIGHT_PAREN);
        case '{':
            return make_token(TOKEN_LEFT_BRACE);
        case '}':
            return make_token(TOKEN_RIGHT_BRACE);
        case ';':
            return make_token(TOKEN_SEMICOLON);
        case ',':
            return make_token(TOKEN_COMMA);
        case '.':
            return make_token(TOKEN_DOT);
        case '-':
            return make_token(TOKEN_MINUS);
        case '+':
            return make_token(TOKEN_PLUS);
        case '/':
            return make_token(TOKEN_SLASH);
        case '*':
            return make_token(TOKEN_STAR);
        case '!':
            return make_token(matches_token('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(matches_token('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make_token(matches_token('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(matches_token('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return string();
    }

    return error_token("Unexpected character.");
}
