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

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
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

static token_type check_keyword(int start, int length, const char* rest, token_type type) {
    // Ensure that the lexeme is exactly as long as the keyword AND that the remaining
    // characters match exactly.  If we are looking for the keyword 'super', the first
    // letter is 's', the lexeme could be 'sup' or 'superb', or the 'super' keyword.
    if (lex.current - lex.start == start + length
            && memcmp(lex.start + start, rest, length) == 0) {
        return type;
    }

    // It's not a keyword, it's a user defined identifier.
    return TOKEN_IDENTIFIER;
}

static int current_lexeme_len(void) {
    return lex.current - lex.start;
}

static token_type identifier_type(void) {
    switch (lex.start[0]) {
        case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'f': {
            if (current_lexeme_len() > 1) {
                switch (lex.start[1]) {
                    case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(2, 2, "nc", TOKEN_FUNC);
                }
            }
            break;
        }
        case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(1, 3, "ull", TOKEN_NULL);
        case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
        case 't': {
            if (current_lexeme_len() > 1) {
                switch (lex.start[1]) {
                    case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        }
        case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static token make_identifier(void) {
    while (is_alpha(peek_current_char()) || is_digit(peek_current_char())) {
        advance_lexer();
    }
    return make_token(identifier_type());
}

static token make_number(void) {
    // Advance until we find a non digit
    while (is_digit(peek_current_char())) {
        advance_lexer();
    }

    // Look for the fractional part '.'
    if (peek_current_char() == '.' && is_digit(peek_next_char())) {
        // Consume the dot character
        advance_lexer();

        while (is_digit(peek_current_char())) {
            // Advance until we find a non-digit char
            advance_lexer();
        }
    }

    return make_token(TOKEN_NUMBER);
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

    if (is_alpha(c)) {
        return make_identifier();
    }
    if (is_digit(c)) {
        return make_number();
    }

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
