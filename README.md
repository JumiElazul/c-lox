README

## c-lox Grammar:
```
program     → declaration_statement* EOF ;

declaration → classDecl
            | funcDecl
            | varDecl
            | statement ;

classDecl   → "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;
funcDecl    → "func" function ;
varDecl     → ( "const" )? "var" IDENTIFIER ( "=" expression )? ";" ;

statement   → exprStmt
            | forStmt
            | ifStmt
            | printStmt
            | returnStmt
            | whileStmt
            | blockStmt
            | switchStmt ;

exprStmt    → expression ";" ;
forStmt     → "for" "(" ( varDecl | exprStmt | ";" )
                          expression? ";"
                          expression? ")" statement ;
ifStmt      → "if" "(" expression ")" statement ( "else" statement )? ;
printStmt   → "print" expression ";" ;
returnStmt  → "return" expression? ";" ;
whileStmt   → "while" "(" expression ")" statement ;
blockStmt   → "{" declaration* "}" ;
switchStmt  → "switch" "(" expression ")" "{" switchCase* defaultCase? "}" ;
switchCase  → "case" expression ":" statement* ;
defaultCase → "default" ":" statement* ;

expression  → assignment ;
assignment  → ( call "." )? IDENTIFIER "=" assignment | logic_or ;
logic_or    → logic_and ( "or" logic_and )* ;
logic_and   → equality ( "and" equality )* ;
equality    → comparison ( ( "!=" | "==" ) comparison )* ;
comparison  → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term        → factor ( ( "-" | "+" ) factor )* ;
factor      → unary ( ( "/" | "*" ) unary )* ;
unary       → ( "!" | "-" ) unary | call ;
call        → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
primary     → "true" | "false" | "null" | "this" | NUMBER | STRING | IDENTIFIER
              | "(" expression ")" | "super" "." IDENTIFIER ;

function    → IDENTIFIER "(" parameters? ")" blockStmt ;
parameters  → IDENTIFIER ( "," IDENTIFIER )* ;
arguments   → expression ( "," expression )* ;

NUMBER      → DIGIT+ ( "." DIGIT+ )? ;
STRING      → "\"" <any char except "\"">* "\"" ;
IDENTIFIER  → ALPHA ( ALPHA | DIGIT )* ;
ALPHA       → "a" ... "z" | "A" ... "Z" | "_" ;
DIGIT       → "0" ... "9" ;
```

## TODO:

### compiler.c:
- [ ] Deduplicate constants other than user identifiers in the constant table.
- [ ] Implement 'break' statements for control flow constructs.
- [ ] Implement 'continue' statements for control flow constructs.

