#ifndef JUMI_CLOX_ASM_ASM_LEXER_H
#define JUMI_CLOX_ASM_ASM_LEXER_H

typedef struct {
    const char* start;
    const char* current;
    int line;
} asm_lexer;

typedef enum {
    ASM_TOKEN_SECTION,
    ASM_TOKEN_DIRECTIVE,
    ASM_TOKEN_IDENTIFIER,
    ASM_TOKEN_NUMBER,
    ASM_TOKEN_SEMICOLON,
    ASM_TOKEN_EOF,
    ASM_TOKEN_ERROR,
} asm_token_type;

typedef struct {
    asm_token_type type;
    const char* start;
    int length;
    int line;
} asm_token;

void init_asm_lexer(asm_lexer* lexer, const char* source_code);
asm_token scan_token(asm_lexer* lexer);

#endif
