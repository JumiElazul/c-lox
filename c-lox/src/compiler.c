#include "compiler.h"
#include "common.h"
#include "lexer.h"
#include <stdio.h>

void compile(const char* source_code) {
    init_lexer(source_code);

    int line = -1;
    while (true) {
        token tok = lexer_scan_token();
        if (tok.line != line) {
            printf("%6d ", tok.line);
            line = tok.line;
        } else {
            printf("     | ");
        }
        printf("%2d '%.*s'\n", tok.type, tok.length, tok.start);

        if (tok.type == TOKEN_EOF) {
            break;
        }
    }
}
