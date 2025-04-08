set(SOURCES
    "src/main.c"
    "src/common.h"

    "src/bytecode_chunk.h"
    "src/bytecode_chunk.c"
    "src/disassembler.h"
    "src/disassembler.c"
    "src/memory.h"
    "src/memory.c"
    "src/value.h"
    "src/value.c"
)

add_executable(c-lox ${SOURCES})
