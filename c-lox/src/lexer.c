#include "lexer.h"
#include <stdbool.h>
#include <string.h>

typedef struct lexer {
    const char* start;
    const char* current;
    int line;
} lexer;

lexer lex;

void init_lexer(const char* source) {
    lex.start = source;
    lex.current = source;
    lex.line = 1;
}

// We ensure that our incoming source string is always null-terminated
// to make sure that this function works correctly.
static bool is_at_end(void) {
    return *lex.current == '\0';
}

static token make_token(token_type type) {
    token tok;
    tok.type = type;
    tok.start = lex.start;
    tok.length = (int)(lex.current - lex.start);
    tok.line = lex.line;
    return tok;
}

static token error_token(const char* msg) {
    token tok;
    tok.type = TOKEN_ERROR;
    tok.start = msg;
    tok.length = (int)strlen(msg);
    tok.line = lex.line;
    return tok;
}

static char advance_lexer(void) {
    return *lex.current++;
}

static bool matches_token(const char expected_char) {
    if (is_at_end()) {
        return false;
    }
    if (*lex.current != expected_char) {
        return false;
    }
    ++lex.current;
    return true;
}

static char peek_current_char(void) {
    return *lex.current;
}

static char peek_next_char(void) {
    if (is_at_end()) {
        return '\0';
    }
    return lex.current[1];
}

static void skip_whitespace(void) {
    // Peek at the next char as many times as is necessary to skip all whitespace.
    // We just need to increment the line number in the lexer when we find a newline.
    for (;;) {
        char c = peek_current_char();
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
                if (peek_next_char() == '/') {
                    while (!is_at_end() && peek_current_char() != '\n') {
                        advance_lexer();
                    }
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

static token make_string(void) {
    // We eat characters, including newlines (making sure to increment the line number when we
    // find those), until we either reach the end of the file, where the string is considered 
    // unterminated, or we find the closing quote and the string is complete.
    while (!is_at_end() && peek_current_char() != '"') {
        if (peek_current_char() == '\n') {
            ++lex.line;
        }
        advance_lexer();
    }

    if (is_at_end()) {
        return error_token("Unterminated string.");
    }

    advance_lexer();
    return make_token(TOKEN_STRING);
}

token scan_token(void) {
    skip_whitespace();
    // By the time the above function returns, we know that the next character
    // is significant to us in some way.
    lex.start = lex.current;

    if (is_at_end()) {
        return make_token(TOKEN_EOF);
    }

    // Advance the lexer by one character to start
    char c = advance_lexer();

    switch (c) {
        case '(': return make_token(TOKEN_LEFT_PAREN);
        case ')': return make_token(TOKEN_RIGHT_PAREN);
        case '{': return make_token(TOKEN_LEFT_BRACE);
        case '}': return make_token(TOKEN_RIGHT_BRACE);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '-': return make_token(TOKEN_MINUS);
        case '+': return make_token(TOKEN_PLUS);
        case '/': return make_token(TOKEN_SLASH);
        case '*': return make_token(TOKEN_STAR);
        case '!': {
            return make_token(matches_token('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        }
        case '=': {
            return make_token(matches_token('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        }
        case '<': {
            return make_token(matches_token('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        }
        case '>': {
            return make_token(matches_token('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        }
        case '"': return make_string();
        default:
            break;
    }

    return error_token("Unexpected character.");
}
