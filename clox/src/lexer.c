#include "lexer.h"
#include <string.h>

typedef struct {
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

static bool is_alpha(const char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(const char c) {
    return c >= '0' && c <= '9';
}

static bool is_at_end() {
    return *lex.current == '\0';
}

static char advance_lexer() {
    lex.current++;
    return lex.current[-1];
}

static char peek_char() {
    return *lex.current;
}

static char peek_next_char() {
    if (is_at_end())
        return '\0';

    return lex.current[1];
}

static bool matches_char(const char expected) {
    if (is_at_end())
        return false;
    if (*lex.current != expected)
        return false;

    lex.current++;
    return true;
}

static token make_token(token_type type) {
    token tok;
    tok.type = type;
    tok.start = lex.start;
    tok.length = (size_t)(lex.current - lex.start);
    tok.line = lex.line;
    return tok;
}

static token error_token(const char* message) {
    token tok;
    tok.type = TOKEN_ERROR;
    tok.start = message;
    tok.length = (size_t)strlen(message);
    tok.line = lex.line;
    return tok;
}

static void skip_whitespace() {
    for (;;) {
        char c = peek_char();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance_lexer();
                break;
            case '\n':
                lex.line++;
                advance_lexer();
                break;
            case '/':
                if (peek_next_char() == '/') {
                    while (peek_char() != '\n' && !is_at_end()) {
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

static token_type check_keyword(int start, int length, const char* rest, token_type type) {
    if (lex.current - lex.start == start + length &&
        memcmp(lex.start + start, rest, (size_t)length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static token_type identifier_type() {
    int tok_len = (int)(lex.current - lex.start);

    switch (lex.start[0]) {
        case 'a':
            return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'b':
            return check_keyword(1, 4, "reak", TOKEN_AND);
        case 'c':
            if (tok_len > 1) {
                switch (lex.start[1]) {
                    case 'l':
                        return check_keyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': {
                        if (tok_len == 5) {
                            return check_keyword(2, 3, "nst", TOKEN_CONST);
                        }

                        if (tok_len == 8) {
                            return check_keyword(2, 6, "ntinue", TOKEN_CONTINUE);
                        }

                        return TOKEN_IDENTIFIER;
                    }
                }
            }
            break;
        case 'd':
            return check_keyword(1, 4, "ebug", TOKEN_DEBUG);
        case 'e':
            return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (tok_len > 1) {
                switch (lex.start[1]) {
                    case 'a':
                        return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return check_keyword(2, 2, "nc", TOKEN_FUNC);
                }
            }
            break;
        case 'g':
            return check_keyword(1, 5, "lobal", TOKEN_GLOBAL);
        case 'i':
            return check_keyword(1, 1, "f", TOKEN_IF);
        case 'n':
            return check_keyword(1, 3, "ull", TOKEN_NULL);
        case 'o':
            return check_keyword(1, 1, "r", TOKEN_OR);
        case 'p':
            return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r':
            return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 's':
            return check_keyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (tok_len > 1) {
                switch (lex.start[1]) {
                    case 'h':
                        return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r':
                        return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v':
            return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
            return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static token identifier() {
    while (is_alpha(peek_char()) || is_digit(peek_char())) {
        advance_lexer();
    }

    return make_token(identifier_type());
}

static token number() {
    while (is_digit(peek_char())) {
        advance_lexer();
    }

    if (peek_char() == '.' && is_digit(peek_next_char())) {
        advance_lexer();

        while (is_digit(peek_char())) {
            advance_lexer();
        }
    }

    return make_token(TOKEN_NUMBER);
}

static token string() {
    while (peek_char() != '"' && !is_at_end()) {
        if (peek_char() == '\n') {
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

token scan_token() {
    skip_whitespace();
    lex.start = lex.current;

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
            return make_token(matches_char('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(matches_char('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make_token(matches_char('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(matches_char('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return string();
    }

    return error_token("Unexpected character.");
}
