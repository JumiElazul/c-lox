set(CLOX_SOURCES
    "src/main.c"
    "src/common.h"

    "src/bytecode_chunk.h"
    "src/bytecode_chunk.c"
    "src/disassembler.h"
    "src/disassembler.c"
    "src/memory.h"
    "src/memory.c"
)

set(CLOX_EXE_NAME c-lox)

add_executable(${CLOX_EXE_NAME} ${CLOX_SOURCES})
target_compile_features(${CLOX_EXE_NAME} PRIVATE c_std_17)
set_target_properties(${CLOX_EXE_NAME} PROPERTIES C_EXTENSIONS OFF)
