#include "compiler.h"
#include "bytecode_chunk.h"
#include "clox_object.h"
#include "common.h"
#include "identifier_cache.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_PRINT_CODE
#include "disassembler.h"
#endif

// program      → declaration_statement* EOF ;
//
// declaration  → classDecl
//              | funcDecl
//              | varDecl
//              | statement ;
//
// classDecl    → "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
// funcDecl     → "func" function ;
// varDecl      → ( "const" )? "var" IDENTIFIER ( "=" expression )? ";" ;
//
// statement    → exprStmt
//              | forStmt
//              | ifStmt
//              | printStmt
//              | returnStmt
//              | whileStmt
//              | blockStmt
//              | switchStmt
//              | breakStmt
//              | continueStmt ;
//
// exprStmt     → expression ";" ;
// forStmt      → "for" "(" ( varDecl | exprStmt | ";" )
//                            expression? ";"
//                            expression? ")" statement;
// ifStmt       → "if" "(" expression ")" statement ( "else" statement )? ;
// printStmt    → "print" expression ";" ;
// returnStmt   → "return" expression? ";" ;
// whileStmt    → "while" "(" expression ")" statement ;
// blockStmt    → "{" declaration* "}" ;
// switchStmt   → "switch" "(" expression ")" "{" switchCase* defaultCase? "}" ;
// breakStmt    → "break" ";" ;
// continueStmt → "continue" ";" ;
//
// switchCase   → "case" expression ":" statement* ;
// defaultCase  → "default" ":" statement* ;
//
// expression   → assignment ;
// assignment   → ( call ".")? IDENTIFIER "=" assignment | logic_or ;
// logic_or     → logic_and ( "or" logic_and )* ;
// logic_and    → equality ( "and" equality )* ;
// equality     → comparison ( ( "!=" | "==" ) comparison )* ;
// comparison   → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
// term         → factor ( ( "-" | "+" ) factor )* ;
// factor       → unary ( ( "/" | "*" ) unary )* ;
// unary        → ( "!" | "-" ) unary | call;
// call         → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
// primary      → "true" | "false" | "null" | "this" | NUMBER | STRING | IDENTIFIER
//                 | "(" expression ")" | "super" "." IDENTIFIER ;
//
// function     → IDENTIFIER "(" parameters? ")" blockStmt ;
// parameters   → IDENTIFIER ( "," IDENTIFIER )* ;
// arguments    → expression ( "," expression )* ;
//
// NUMBER       → DIGIT+ ( "." DIGIT+ )? ;
// STRING       → "\"" <any char except "\"">* "\"" ;
// IDENTIFIER   → ALPHA ( ALPHA | DIGIT )* ;
// ALPHA        → "a" ... "z" | "A" ... "Z" | "_" ;
// DIGIT        → "0" ... "9" ;

static identifier_cache ident_cache;

typedef struct {
    token current;
    token previous;
    bool had_error;
    bool panic_mode;
    bool first_token;
} token_parser;

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

typedef struct {
    token name;
    int depth;
    bool is_const;
} local_variable;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
} function_type;

typedef struct {
    object_function* function;
    function_type type;

    local_variable locals[UINT8_COUNT];
    int local_count;
    int scope_depth;
} compiler;

token_parser parser;
compiler* current_compiler = NULL;
bytecode_chunk* compiling_chunk;

static bytecode_chunk* current_chunk(void) { return &current_compiler->function->chunk; }

static void parse_expression(void);
static void statement(void);
static void declaration_statement(void);
static int identifier_constant(token* name);
static int resolve_local(compiler* comp, token* name);
static void and_(bool can_assign);
static void or_(bool can_assign);
static void variable_declaration(bool is_const);

static void error_at(token* t, const char* message) {
    if (parser.panic_mode) {
        return;
    }

    parser.panic_mode = true;

    fprintf(stderr, "[line %d] Error", t->line);

    if (t->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (t->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at '%.*s'", t->length, t->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void error(const char* message) { error_at(&parser.previous, message); }

static void error_at_current(const char* message) { error_at(&parser.current, message); }

static void advance_parser(void) {
    parser.previous = parser.current;

#ifdef DEBUG_TRACE_EXECUTION
    if (!parser.first_token) {
        printf("%s\n", token_type_tostr(parser.previous.type));
    }
    parser.first_token = false;
#endif

    while (true) {
        parser.current = lexer_scan_token();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        error_at_current(parser.current.start);
    }
}

static void consume_if_matches(token_type type, const char* message) {
    if (parser.current.type == type) {
        advance_parser();
        return;
    }

    error_at_current(message);
}

static bool check_token(token_type type) { return parser.current.type == type; }

static bool matches_token(token_type type) {
    if (!check_token(type)) {
        return false;
    }
    advance_parser();
    return true;
}

static void emit_byte(uint8_t byte) {
    write_to_bytecode_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes2(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_bytes3(uint8_t byte1, uint8_t byte2, uint8_t byte3) {
    emit_byte(byte1);
    emit_byte(byte2);
    emit_byte(byte3);
}

static void emit_bytes4(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {
    emit_byte(byte1);
    emit_byte(byte2);
    emit_byte(byte3);
    emit_byte(byte4);
}

static void emit_loop(int loop_start) {
    emit_byte(OP_LOOP);

    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emit_byte((offset >> 8) & 0xFF);
    emit_byte(offset & 0xFF);
}

// Writes the jump instruction and returns the index of the first 0xFF placeholder byte.
static int emit_jump(uint8_t instruction) {
    emit_bytes3(instruction, 0xFF, 0xFF);
    return current_chunk()->count - 2;
}

static void emit_return(void) { emit_byte(OP_RETURN); }

static void emit_constant(clox_value val) {
    int index = add_constant(current_chunk(), val);

    if ((unsigned int)index >= U24T_MAX) {
        error("Too many constants in one chunk.");
    }

    bool long_instr = index > 255;

    if (!long_instr) {
        emit_bytes2(OP_CONSTANT, index);
    } else {
        u24_t i = construct_u24_t(index);
        emit_bytes4(OP_CONSTANT_LONG, i.hi, i.mid, i.lo);
    }
}

static void patch_jump(int offset) {
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    current_chunk()->code[offset] = (jump >> 8) & 0xFF;
    current_chunk()->code[offset + 1] = jump & 0xFF;
}

static void init_compiler(compiler* comp, function_type type) {
    comp->function = NULL;
    comp->type = type;
    comp->local_count = 0;
    comp->scope_depth = 0;
    comp->function = new_function();
    current_compiler = comp;

    // The compiler claims slot 0 in the locals array for its own internal use.
    local_variable* local = &current_compiler->locals[current_compiler->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

static object_function* end_compilation(void) {
    emit_return();
    object_function* function = current_compiler->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        disassemble_chunk(current_chunk(),
                          function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    return function;
}

static void begin_scope(void) { ++current_compiler->scope_depth; }

static void end_scope(void) {
    --current_compiler->scope_depth;
    compiler* comp = current_compiler;

    while (comp->local_count > 0 && comp->locals[comp->local_count - 1].depth > comp->scope_depth) {
        emit_byte(OP_POP);
        --comp->local_count;
    }
}

static void parse_expression(void);
static parse_rule* get_rule(token_type type);
static void parse_precedence(precedence prec);

static void binary(bool can_assign) {
    token_type operator_type = parser.previous.type;
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
    switch (parser.previous.type) {
        case TOKEN_NULL: {
            emit_byte(OP_NULL);
        } break;
        case TOKEN_TRUE: {
            emit_byte(OP_TRUE);
        } break;
        case TOKEN_FALSE: {
            emit_byte(OP_FALSE);
        } break;
        default:
            return;
    }
}

static void grouping(bool can_assign) {
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void number(bool can_assign) {
    double val = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VALUE(val));
}

static void string(bool can_assign) {
    emit_constant(OBJECT_VALUE(copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void named_variable(token name, bool can_assign) {
    int local = resolve_local(current_compiler, &name);

    bool is_set = can_assign && matches_token(TOKEN_EQUAL);

    // Local path
    if (local != -1) {
        local_variable* lv = &current_compiler->locals[local];
        bool is_const = lv->is_const;

        if (is_set && is_const) {
            error("Cannot reassign to a local variable marked 'const'.");
        }

        if (is_set) {
            parse_expression();
        }

        emit_bytes2(is_set ? OP_SET_LOCAL : OP_GET_LOCAL, (uint8_t)local);
        return;
    }

    // Global path
    int global_index = identifier_constant(&name);
    bool long_instr = global_index > 255;

    if (is_set) {
        parse_expression();
    }

    if (!long_instr) {
        emit_bytes2(is_set ? OP_SET_GLOBAL : OP_GET_GLOBAL, (uint8_t)global_index);
    } else {
        u24_t i = construct_u24_t(global_index);
        emit_bytes4(is_set ? OP_SET_GLOBAL_LONG : OP_GET_GLOBAL_LONG, i.hi, i.mid, i.lo);
    }
}

static void variable(bool can_assign) { named_variable(parser.previous, can_assign); }

static void unary(bool can_assign) {
    token_type operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY);

    switch (operator_type) {
        case TOKEN_BANG: {
            emit_byte(OP_NOT);
        } break;
        case TOKEN_MINUS: {
            emit_byte(OP_NEGATE);
        } break;
        default:
            return;
    }
}

static void debug_statement(void) {
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after debug statement.");
    emit_byte(OP_DEBUG);
}

parse_rule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CASE] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_CONST] = {NULL, NULL, PREC_NONE},
    [TOKEN_DEFAULT] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNC] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NULL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_SWITCH] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},

    [TOKEN_DEBUG] = {NULL, NULL, PREC_NONE},
};

static void parse_precedence(precedence prec) {
    advance_parser();

    parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expected expression.");
        return;
    }

    bool can_assign = prec <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while (prec <= get_rule(parser.current.type)->prec) {
        advance_parser();
        parse_fn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && matches_token(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static int make_constant(clox_value val) {
    int constant = add_constant(current_chunk(), val);

    if ((unsigned int)constant >= U24T_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return constant;
}

static int identifier_constant(token* name) {
    object_string* str = copy_string(name->start, name->length);

    // To prevent adding an identifier to the chunk's constant table every time we see it, we keep
    // an 'identifier cache' hashmap of object_string* to index.  This way we can look up where it
    // exists, if it does already.
    int index;
    if (identifier_cache_get(&ident_cache, str, &index)) {
        return index;
    }

    index = make_constant(OBJECT_VALUE(str));
    identifier_cache_set(&ident_cache, str, index);

    return index;
}

static bool identifiers_equal(token* a, token* b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(compiler* comp, token* name) {
    for (int i = comp->local_count - 1; i >= 0; --i) {
        local_variable* local = &comp->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }

            return i;
        }
    }

    return -1;
}

static void add_local(token name, bool is_const) {
    if (current_compiler->local_count == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    local_variable* local = &current_compiler->locals[current_compiler->local_count++];
    local->name = name;
    local->depth = -1;
    local->is_const = is_const;
}

static void declare_variable(bool is_const) {
    // If it's a global we don't do anything here.
    if (current_compiler->scope_depth == 0) {
        return;
    }

    token* name = &parser.previous;

    for (int i = current_compiler->local_count - 1; i >= 0; --i) {
        local_variable* local = &current_compiler->locals[i];

        if (local->depth != -1 && local->depth < current_compiler->scope_depth) {
            break;
        }

        if (identifiers_equal(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    add_local(*name, is_const);
}

static int parse_variable(bool is_const) {
    if (is_const) {
        consume_if_matches(TOKEN_VAR, "Expected 'var' keyword after const declaration.");
    }

    consume_if_matches(TOKEN_IDENTIFIER, "Expected variable name.");

    // If this is a global variable we will fall through to the function below, since globals are
    // late bound.  If it's a local, we need to declare it.
    declare_variable(is_const);
    if (current_compiler->scope_depth > 0) {
        return 0;
    }

    // Add the global to the bytecode constant table.
    return identifier_constant(&parser.previous);
}

static void mark_initialized(void) {
    current_compiler->locals[current_compiler->local_count - 1].depth =
        current_compiler->scope_depth;
}

static void define_variable(int global, bool is_const) {
    // No runtime code to actually create for local variables, so we leave.
    if (current_compiler->scope_depth > 0) {
        mark_initialized();
        return;
    }

    bool long_instr = global > 255;

    if (!long_instr) {
        emit_bytes2(is_const ? OP_DEFINE_GLOBAL_CONST : OP_DEFINE_GLOBAL, global);
    } else {
        u24_t i = construct_u24_t(global);
        emit_bytes4(is_const ? OP_DEFINE_GLOBAL_LONG_CONST : OP_DEFINE_GLOBAL_LONG, i.hi, i.mid,
                    i.lo);
    }
}

// Left operand expression
// OP_JUMP_IF_FALSE ---------v
// OP_POP                    |
// Right operand expression  |
// OP_x continues   <---------
static void and_(bool can_assign) {
    // The left hand side of the expression is already on the stack.
    // Semantics: if A is falsey -> result is A; else -> result is B

    // If that expression is false, jump over the rest of the expression to short circuit.
    int end_jump = emit_jump(OP_JUMP_IF_FALSE);

    // Compile the expression
    emit_byte(OP_POP);
    parse_precedence(PREC_AND);

    // Patch in how much to skip if the expression was false.
    patch_jump(end_jump);
}

// Left operand expression
// OP_JUMP_IF_FALSE --v
// OP_JUMP          --|------v
// OP_POP           <--      |
// Right operand expression  |
// OP_x continues   <---------
static void or_(bool can_assign) {
    // The left hand side of the expression is already on the stack.
    // Semantics: if A is truthy -> result is A; else -> result is B

    // If the lhs value is truthy, JUMP_IF_FALSE does nothing.  Then the JUMP instruction skips the
    // rhs part.
    int else_jump = emit_jump(OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(OP_JUMP);

    patch_jump(else_jump);
    emit_byte(OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static parse_rule* get_rule(token_type type) { return &rules[type]; }

static void parse_expression(void) { parse_precedence(PREC_ASSIGNMENT); }

static void print_statement(void) {
    parse_expression();
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_PRINT);
}

static void while_statement(void) {
    int loop_start = current_chunk()->count;
    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after while.");
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after while condition.");

    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);

    statement();
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_byte(OP_POP);
}

static void block_statement(void) {
    while (!check_token(TOKEN_RIGHT_BRACE) && !check_token(TOKEN_EOF)) {
        declaration_statement();
    }

    consume_if_matches(TOKEN_RIGHT_BRACE, "Expected '}' to end block statement.");
}

static void switch_statement(void) {
    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after switch statement.");
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after switch expression.");

    consume_if_matches(TOKEN_LEFT_BRACE, "Expected '{' to begin switch body.");

    // We need an array of jump points, since we don't know which case will end up matching.  Each
    // one of the possible cases, if matched, needs to jump past the 'default' case.  Since we don't
    // know what this will be, we need to keep an array of them and patch all of them.
    int end_jumps[UINT8_COUNT];
    int end_jump_count = 0;

    while (matches_token(TOKEN_CASE)) {
        // Duplicate the expression at the bottom of the stack every time we find a case, since it
        // will be eaten by the EQUAL opcode.
        emit_byte(OP_DUP);
        parse_expression();
        emit_byte(OP_EQUAL);

        // If false, we jump over the body of the case.  Otherwise we fall through.
        int next_case = emit_jump(OP_JUMP_IF_FALSE);

        emit_byte(OP_POP);
        consume_if_matches(TOKEN_COLON, "Expected ':' after case expression.");
        statement();

        if (end_jump_count == UINT8_COUNT) {
            error("Cannot have more than 255 cases in switch statement.");
            return;
        }

        end_jumps[end_jump_count++] = emit_jump(OP_JUMP);

        patch_jump(next_case);
        emit_byte(OP_POP);
    }

    if (matches_token(TOKEN_DEFAULT)) {
        consume_if_matches(TOKEN_COLON, "Expected ':' after default.");
        statement();
    }

    for (int i = 0; i < end_jump_count; ++i) {
        patch_jump(end_jumps[i]);
    }

    emit_byte(OP_POP);
    consume_if_matches(TOKEN_RIGHT_BRACE, "Expected '}' to end switch body.");
}

static void expression_statement(void) {
    parse_expression();
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_POP);
}

// A diagram of this is provided in images/for_statement.png.
static void for_statement(void) {
    begin_scope();
    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");

    // For loop initializer clause (optional)
    if (matches_token(TOKEN_SEMICOLON)) {
        // No initializer
    } else if (matches_token(TOKEN_VAR)) {
        variable_declaration(false);
    } else if (matches_token(TOKEN_CONST)) {
        variable_declaration(true);
    } else {
        expression_statement();
    }

    // For loop conditional expression. (optional)
    int loop_start = current_chunk()->count;
    int exit_jump = -1;
    if (!matches_token(TOKEN_SEMICOLON)) {
        parse_expression();
        consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after for loop condition.");

        // Jump out of the loop if the condition is false.
        exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
    }

    // For loop post increment expression (optional)
    if (!matches_token(TOKEN_RIGHT_PAREN)) {
        // Skip the increment the first time, by jumping over it and running the body.
        int body_jump = emit_jump(OP_JUMP);

        // Save the increment start to patch in later.
        int increment_start = current_chunk()->count;
        parse_expression();
        emit_byte(OP_POP);
        consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");

        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    // Loop body
    statement();
    emit_loop(loop_start);

    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit_byte(OP_POP);
    }

    end_scope();
}

static void if_statement(void) {
    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after 'if' statement.");
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after 'if' statement condition.");

    // Save the index of the first placeholder byte to later backpatch.
    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    // Compile as much bytecode as needed to know how much bytecode to skip.
    statement();

    // To prevent fallthrough to the else condition, we also need to emit a non conditional jump
    // here.
    int else_jump = emit_jump(OP_JUMP);

    // Backpatch in the amount of bytecode to skip if the condition evaluates to false.
    patch_jump(then_jump);
    emit_byte(OP_POP);

    // If there's an else clause, we compile that statement too, to backpatch in the 'else_jump'
    // bytes to skip.
    if (matches_token(TOKEN_ELSE)) {
        statement();
    }

    patch_jump(else_jump);
}

static void variable_declaration(bool is_const) {
    int var_index = parse_variable(is_const);

    if (matches_token(TOKEN_EQUAL)) {
        parse_expression();
    } else {
        if (is_const) {
            error("Const variables must be initialized.");
        }
        emit_byte(OP_NULL);
    }

    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    define_variable(var_index, is_const);
}

static void synchronize(void) {
    parser.panic_mode = false;

    // When we are in panic mode, we want to continuously discard tokens until we hit a natural
    // point to start again.  This means finding a semicolon, or the beginning of any declaration
    // statements.
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUNC:
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

static void declaration_statement(void) {
    if (matches_token(TOKEN_VAR)) {
        variable_declaration(false);
    } else if (matches_token(TOKEN_CONST)) {
        variable_declaration(true);
    } else {
        statement();
    }

    if (parser.panic_mode) {
        synchronize();
    }
}

static void statement(void) {
    if (matches_token(TOKEN_DEBUG)) {
        debug_statement();
        return;
    }

    if (matches_token(TOKEN_PRINT)) {
        print_statement();
    } else if (matches_token(TOKEN_FOR)) {
        for_statement();
    } else if (matches_token(TOKEN_IF)) {
        if_statement();
    } else if (matches_token(TOKEN_WHILE)) {
        while_statement();
    } else if (matches_token(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block_statement();
        end_scope();
    } else if (matches_token(TOKEN_SWITCH)) {
        switch_statement();
    } else {
        expression_statement();
    }
}

object_function* compile(const char* source_code) {
    init_lexer(source_code);
    init_identifier_cache(&ident_cache);
    compiler comp;
    init_compiler(&comp, TYPE_SCRIPT);

    parser.had_error = false;
    parser.panic_mode = false;
    parser.first_token = true;

    advance_parser();

    while (!matches_token(TOKEN_EOF)) {
        declaration_statement();
    }

    object_function* function = end_compilation();
    free_identifier_cache(&ident_cache);

    return parser.had_error ? NULL : function;
}
