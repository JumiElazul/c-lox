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

static bool is_at_end(void) { return *lex.current == '\0'; }

token lexer_scan_token(void) {
    lex.start = lex.current;
    if (is_at_end()) {
        return make_token(TOKEN_EOF);
    }

    return error_token("Unexpected character.");
}
