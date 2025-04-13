set(SOURCES
    "src/main.c"
    "src/common.h"

    "src/bytecode_chunk.h"
    "src/bytecode_chunk.c"
    "src/compiler.h"
    "src/compiler.c"
    "src/disassembler.h"
    "src/disassembler.c"
    "src/lexer.h"
    "src/lexer.c"
    "src/memory.h"
    "src/memory.c"
    "src/object.h"
    "src/object.c"
    "src/value.h"
    "src/value.c"
    "src/virtual_machine.h"
    "src/virtual_machine.c"
)

add_executable(c-lox ${SOURCES})
