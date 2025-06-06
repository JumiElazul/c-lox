#include "compiler.h"
#include "common.h"
#include "bytecode_chunk.h"
#include "disassembler.h"
#include "lexer.h"
#include "object.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// program              -> declaration* EOF ;
// declaration          -> class_declaration | func_declaration | variable_declaration | statement ;
// class_declaration    -> "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
// func_declaration     -> "func" function ;
// function             -> IDENTIFIER "(" parameters? ")" block ;
// parameters           -> IDENTIFIER ( "," IDENTIFIER)* ;
// variable_declaration -> "var" IDENTIFIER ( "=" expression )? ";" ;
// statement            -> print_statement
//                       | if_statement
//                       | while_statement
//                       | for_statement
//                       | block_statement
//                       | break_statement
//                       | continue_statement
//              		 | return_statement
//     		             | expression_statement ;
//
// print_statement      -> "print" "(" expression ")" ";" ;

// if_statement         -> "if" "(" expression ")" statement ( "else if" statement )? ( "else" statement )? ;
// while_statement      -> "while" "(" expression ")" statement ;
// for_statement        -> "for" "(" ( variable_declaration | expression_statement | ";" )
//    						expression? ";"
//    						expression? ")" statement ;
//
// block_statement      -> "{" declaration* "}" ;
// break_statement      -> "break" ";" ;
// continue_statement   -> "continue" ";" ;
// return_statement     -> "return" expression? ";" ;
// expression_statement -> expression ";" ;

// expression           -> assignment ;
// assignment           -> ( call "." )? IDENTIFIER "=" assignment | logic_or ;
// logic_or             -> logic_and ( "or" logic_and )* ;
// logic_and            -> equality ( "and" equality )* ;
// equality             -> comparison ( ( "!=" | "==" ) comparison )* ;
// comparison           -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
// term                 -> factor ( ( "-" | "+" ) factor )* ;
// factor               -> unary ( ( "*" | "/" ) unary )* ;
// unary                -> ( "!" | "-" ) unary | call ;
// call                 -> primary ( "(" arguments? ") | "." IDENTIFIER )* ;
// arguments            -> expression ( "," expression )* ;

// primary              -> NUMBER | STRING | "true" | "false" | "null" | "(" expression ")" | IDENTIFIER | "super" "." IDENTIFIER ;

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

typedef void (*parse_fn)(bool can_assign);

typedef struct parse_rule {
    parse_fn prefix;
    parse_fn infix;
    precedence prec;
} parse_rule;

typedef struct {
    token name;
    int depth;
} local_variable;

// The compiler holds an implicit "main" function. TYPE_SCRIPT is used to denote that 
// function, while all user defined/standard library functions are defined with TYPE_FUNCTION.
typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
} function_type;

typedef struct compiler {
    struct compiler* enclosing_compiler;
    obj_function* function;
    function_type type;

    local_variable locals[UINT8_COUNT];
    int local_count;
    int local_depth;
    int scope_depth;
} compiler;

token_parser parser;
compiler* current_compiler = NULL;
bytecode_chunk* compiling_chunk;

static parse_rule* get_rule(token_type type);
static void parse_expression(void);
static void statement(void);
static void declaration(void);
static void parse_precedence(precedence prec);
static uint8_t identifier_constant(token* name);
static int resolve_local(compiler* comp, token* name);
static uint8_t argument_list(void);
static void and_(bool can_assign);
static void or_(bool can_assign);

static bytecode_chunk* current_chunk(void) {
    return &current_compiler->function->chunk;
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

static bool check(token_type type) {
    return parser.current.type == type;
}

static void consume_if_matches(token_type type, const char* msg) {
    if (check(type)) {
        advance_parser();
        return;
    }

    error_at_current(msg);
}

static bool matches_token(token_type type) {
    if (!check(type)) {
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

static void emit_loop(int loop_start) {
    emit_byte(OP_LOOP);

    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emit_byte((offset) >> 8 & 0xFF);
    emit_byte(offset & 0xFF);
}

static int emit_jump(uint8_t instruction) {
    emit_byte(instruction);
    emit_byte(0xFF);
    emit_byte(0xFF);
    return current_chunk()->count - 2;
} 

static void emit_return(void) {
    emit_byte(OP_NULL);
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

static void patch_jump(int offset) {
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    current_chunk()->code[offset] = (jump >> 8) & 0xFF;
    current_chunk()->code[offset + 1] = jump & 0xFF;
}

static void init_compiler(compiler* comp, function_type type) {
    comp->enclosing_compiler = current_compiler;
    comp->function = NULL;
    comp->type = type;
    comp->local_count = 0;
    comp->scope_depth = 0;
    comp->function = new_function();
    current_compiler = comp;

    if (type != TYPE_SCRIPT) {
        current_compiler->function->name = copy_string(
                parser.previous.start, parser.previous.length);
    }

    // The compiler claims stack slot 0 for the vm's own internal use.  It's name is empty
    // so that the user can't write an identifier that refers to it.
    local_variable* local = &current_compiler->locals[current_compiler->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

static obj_function* end_compiler(void) {
    emit_return();
    obj_function* function = current_compiler->function;

    if (debug_print_code) {
        if (!parser.had_error) {
            disassemble_bytecode_chunk(current_chunk(),
                    function->name != NULL ?
                    function->name->chars  :
                    "<script>");
        }
    }

    current_compiler = current_compiler->enclosing_compiler;
    return function;
}

static void begin_scope(void) {
    ++current_compiler->scope_depth;
}

static void end_scope(void) {
    --current_compiler->scope_depth;

    while (current_compiler->local_count > 0 &&
           current_compiler->locals[current_compiler->local_count - 1].depth >
           current_compiler->scope_depth) {
        emit_byte(OP_POP);
        --current_compiler->local_count;
    }
}

static void binary(bool can_assign) {
    // When a prefix parser function is called, the leading token has already been consumed.  In an infix
    // parser function, the entire left-hand operand expression has already been compiled and the subsequent infix
    // operator consumed.
    token_type operator_type = parser.previous.type;
    parse_rule* rule = get_rule(operator_type);
    parse_precedence((precedence)(rule->prec + 1));

    switch (operator_type) {
        case TOKEN_BANG_EQUAL:    emit_bytes2(OP_EQUAL, OP_NOT);   break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL);             break;
        case TOKEN_GREATER:       emit_byte(OP_GREATER);           break;
        case TOKEN_GREATER_EQUAL: emit_bytes2(OP_LESS, OP_NOT);    break;
        case TOKEN_LESS:          emit_byte(OP_LESS);              break;
        case TOKEN_LESS_EQUAL:    emit_bytes2(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:          emit_byte(OP_ADD);               break;
        case TOKEN_MINUS:         emit_byte(OP_SUBTRACT);          break;
        case TOKEN_STAR:          emit_byte(OP_MULTIPLY);          break;
        case TOKEN_SLASH:         emit_byte(OP_DIVIDE);            break;
        default: return;
    }
}

static void call(bool can_assign) {
    uint8_t arg_count = argument_list();
    emit_bytes2(OP_CALL, arg_count);
}

static void grouping(bool can_assign) {
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after grouping expression.");
}

// When we find a number literal, there's nothing else to do but add the constant to the chunk's
// constant table and write the proper opcode.
static void number(bool can_assign) {
    double val = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(val));
}

static void string(bool can_assign) {
    emit_constant(
            OBJ_VAL(
                copy_string(
                    parser.previous.start + 1,
                    parser.previous.length - 2)));
}

static void named_variable(token name, bool can_assign) {
    uint8_t get_op, set_op;

    int arg = resolve_local(current_compiler, &name);
    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg = identifier_constant(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    
    if (can_assign && matches_token(TOKEN_EQUAL)) {
        parse_expression();
        emit_bytes2(set_op, (uint8_t)arg);
    } else {
        emit_bytes2(get_op, (uint8_t)arg);
    }
}

static void variable(bool can_assign) {
    named_variable(parser.previous, can_assign);
}

static void literal(bool can_assign) {
    switch (parser.previous.type) {
        case TOKEN_NULL:  emit_byte(OP_NULL);  break;
        case TOKEN_TRUE:  emit_byte(OP_TRUE);  break;
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        default: return;
    }
}

static void unary(bool can_assign) {
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
    [TOKEN_LEFT_PAREN]    = { grouping, call,     PREC_NONE       },
    [TOKEN_RIGHT_PAREN]   = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_LEFT_BRACE]    = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_RIGHT_BRACE]   = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_COMMA]         = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_DOT]           = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_MINUS]         = { unary,    binary,   PREC_TERM       },
    [TOKEN_PLUS]          = { NULL,     binary,   PREC_TERM       },
    [TOKEN_SEMICOLON]     = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_SLASH]         = { NULL,     binary,   PREC_FACTOR     },
    [TOKEN_STAR]          = { NULL,     binary,   PREC_FACTOR     },
    [TOKEN_BANG]          = { unary,    NULL,     PREC_NONE       },
    [TOKEN_BANG_EQUAL]    = { NULL,     binary,   PREC_EQUALITY   },
    [TOKEN_EQUAL]         = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_EQUAL_EQUAL]   = { NULL,     binary,   PREC_EQUALITY   },
    [TOKEN_GREATER]       = { NULL,     binary,   PREC_COMPARISON },
    [TOKEN_GREATER_EQUAL] = { NULL,     binary,   PREC_COMPARISON },
    [TOKEN_LESS]          = { NULL,     binary,   PREC_COMPARISON },
    [TOKEN_LESS_EQUAL]    = { NULL,     binary,   PREC_COMPARISON },
    [TOKEN_IDENTIFIER]    = { variable, NULL,     PREC_NONE       },
    [TOKEN_STRING]        = { string,   NULL,     PREC_NONE       },
    [TOKEN_NUMBER]        = { number,   NULL,     PREC_NONE       },
    [TOKEN_AND]           = { NULL,     and_,     PREC_AND        },
    [TOKEN_CLASS]         = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_ELSE]          = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_FALSE]         = { literal,  NULL,     PREC_NONE       },
    [TOKEN_FOR]           = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_FUNC]          = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_IF]            = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_NULL]          = { literal,  NULL,     PREC_NONE       },
    [TOKEN_OR]            = { NULL,     or_,      PREC_OR         },
    [TOKEN_PRINT]         = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_RETURN]        = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_SUPER]         = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_THIS]          = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_TRUE]          = { literal,  NULL,     PREC_NONE       },
    [TOKEN_VAR]           = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_WHILE]         = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_ERROR]         = { NULL,     NULL,     PREC_NONE       },
    [TOKEN_EOF]           = { NULL,     NULL,     PREC_NONE       },
};

static void parse_precedence(precedence prec) {
    advance_parser();
    // By definition, the previous token needs to have a prefix rule or else its not a valid expression.
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
        error("Invalid assignment target");
    }
}

static uint8_t identifier_constant(token* name) {
    return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
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

static bool is_local_variable(void) {
    return current_compiler->scope_depth > 0;
}

static void add_local_variable(token name) {
    if (current_compiler->local_count == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    local_variable* local = &current_compiler->locals[current_compiler->local_count++];
    local->name = name;
    local->depth = -1;
}

static void declare_variable(void) {
    if (!is_local_variable()) {
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

    add_local_variable(*name);
}

static uint8_t parse_variable(const char* err_msg) {
    consume_if_matches(TOKEN_IDENTIFIER, err_msg);

    if (is_local_variable()) {
        declare_variable();
        return 0;
    }

    return identifier_constant(&parser.previous);
}

static void mark_initialized(void) {
    compiler* c = current_compiler;
    if (c->scope_depth == 0) {
        return;
    }
    c->locals[c->local_count - 1].depth = c->scope_depth;
}

static void define_variable(uint8_t index) {
    if (is_local_variable()) {
        mark_initialized();
        return;
    }

    emit_bytes2(OP_DEFINE_GLOBAL, index);
}

static uint8_t argument_list(void) {
    uint8_t arg_count = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            parse_expression();
            if (arg_count == 255) {
                error("Can't have more than 255 arguments.");
            }
            ++arg_count;
        } while (matches_token(TOKEN_COMMA));
    }
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
    return arg_count;
}

static void and_(bool can_assign) {
    int end_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    parse_precedence(PREC_AND);
    patch_jump(end_jump);
}

static void or_(bool can_assign) {
    int else_jump = emit_jump(OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(OP_JUMP);

    patch_jump(else_jump);
    emit_byte(OP_POP);

    parse_precedence(PREC_OR);
    patch_jump(end_jump);
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

static void variable_declaration(void) {
    uint8_t index = parse_variable("Expected variable name.");

    if (matches_token(TOKEN_EQUAL)) {
        parse_expression();
    } else {
        emit_byte(OP_NULL);
    }

    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    define_variable(index);
}

static void print_statement(void) {
    parse_expression();
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_PRINT);
}

static void return_statement(void) {
    if (current_compiler->type == TYPE_SCRIPT) {
        error("Can't use 'return' statement from top-level code.");
    }

    if (matches_token(TOKEN_SEMICOLON)) {
        emit_return();
    } else {
        parse_expression();
        consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after return value.");
        emit_byte(OP_RETURN);
    }
}

static void while_statement(void) {
    int loop_start = current_chunk()->count;
    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after while.");
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected '(' after while.");

    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    // Loop body, ensure to pop the stack expression.  Compile the user's statements
    // then loop back to the beginning to retest the expression.
    emit_byte(OP_POP);
    statement();
    emit_loop(loop_start);

    // Loop exit after body, patch the exit jump and pop the stack expression.
    patch_jump(exit_jump);
    emit_byte(OP_POP);
}

static void synchronize(void) {
    parser.panic_mode = false;

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

            default:
                ;
        }

        advance_parser();
    }
}

static void expression_statement(void) {
    parse_expression();
    consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after expression.");
    emit_byte(OP_POP);
}

static void for_statement(void) {
    begin_scope();
    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");

    // Initializer expression
    if (matches_token(TOKEN_SEMICOLON)) {
        // No initializer
    } else if (matches_token(TOKEN_VAR)) {
        variable_declaration();
    } else {
        expression_statement();
    }

    // Conditional expression
    int loop_start = current_chunk()->count;
    int exit_jump = -1;
    if (!matches_token(TOKEN_SEMICOLON)) {
        parse_expression();
        consume_if_matches(TOKEN_SEMICOLON, "Expected ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
    }

    // Increment expression
    if (!matches_token(TOKEN_RIGHT_PAREN)) {
        // Skip over the increment expression.  Textually this appears before the loop body,
        // but in bytecode it executes AFTER the body.
        int body_jump = emit_jump(OP_JUMP);
        int increment_start = current_chunk()->count;
        parse_expression();
        emit_byte(OP_POP);
        consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after 'for' clauses.");

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

static void block_statement(void) {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume_if_matches(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void function(function_type type) {
    compiler comp;
    init_compiler(&comp, type);
    begin_scope();

    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            ++current_compiler->function->arity;
            if (current_compiler->function->arity > 255) {
                error_at_current("Can't have more than 255 parameters.");
            }
            uint8_t constant = parse_variable("Expected parameter name.");
            define_variable(constant);
        } while (matches_token(TOKEN_COMMA));
    }
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consume_if_matches(TOKEN_LEFT_BRACE, "Expected '{' before function body.");
    block_statement();

    obj_function* function = end_compiler();
    emit_bytes2(OP_CONSTANT, make_constant(OBJ_VAL(function)));
}

static void function_declaration(void) {
    uint8_t global = parse_variable("Expected function name.");
    mark_initialized();
    function(TYPE_FUNCTION);
    define_variable(global);
}

static void if_statement(void) {
    consume_if_matches(TOKEN_LEFT_PAREN, "Expected '(' after if statement.)");
    parse_expression();
    consume_if_matches(TOKEN_RIGHT_PAREN, "Expected ')' after if statement condition.)");

    // The jif instruction is a 3-byte instruction.  The first byte is the instruction,
    // and the next two bytes are the amount of bytes to skip over if the condition is
    // false.  We initially set those two bytes as 0xFF as placeholders, but return the
    // index into the bytecode where they reside.  Then, once we know how many bytes to
    // skip by compiling the next statement(), we "backpatch" those two bytes.
    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();

    // To prevent fallthrough from when the if statement is true, we need to skip the else block.
    int else_jump = emit_jump(OP_JUMP);

    patch_jump(then_jump);
    emit_byte(OP_POP);

    if (matches_token(TOKEN_ELSE)) {
        statement();
    }

    patch_jump(else_jump);
}

static void declaration(void) {
    if (matches_token(TOKEN_FUNC)) {
        function_declaration();
    } else if (matches_token(TOKEN_VAR)) {
        variable_declaration();
    } else {
        statement();
    }

    if (parser.panic_mode) {
        synchronize();
    }
}

static void statement(void) {
    if (matches_token(TOKEN_PRINT)) {
        print_statement();
    } else if (matches_token(TOKEN_FOR)) {
        for_statement();
    } else if (matches_token(TOKEN_IF)) {
        if_statement();
    } else if (matches_token(TOKEN_RETURN)) {
        return_statement();
    } else if (matches_token(TOKEN_WHILE)) {
        while_statement();
    } else if (matches_token(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block_statement();
        end_scope();
    } else {
        expression_statement();
    }
}

obj_function* compile(const char* source) {
    init_lexer(source);
    compiler compiler;
    init_compiler(&compiler, TYPE_SCRIPT);

    parser.had_error = false;
    parser.panic_mode = false;

    advance_parser();

    while (!matches_token(TOKEN_EOF)) {
        declaration();
    }

    obj_function* function = end_compiler();
    return parser.had_error ? NULL : function;
}
