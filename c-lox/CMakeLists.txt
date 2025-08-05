set(CLOX_SOURCES
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
    "src/clox_value.h"
    "src/clox_value.c"
    "src/virtual_machine.h"
    "src/virtual_machine.c"
)

set(CLOX_EXE_NAME c-lox)

add_executable(${CLOX_EXE_NAME} ${CLOX_SOURCES})

target_compile_options(${CLOX_EXE_NAME} PRIVATE
    -Wall
    -Wextra
    -pedantic-errors
    -Wshadow
    -Wno-unused-parameter
    -Wno-conversion
    -Wno-switch
    -Wno-unused-function
    -fsigned-char

    $<$<CONFIG:Debug>:-fsanitize=address;-fsanitize=undefined>

    $<$<CONFIG:Release>:-O3>
)

target_link_options(${CLOX_EXE_NAME} PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address;-fsanitize=undefined>
)

message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")
get_target_property(_CFLAGS ${CLOX_EXE_NAME} COMPILE_OPTIONS)
message(STATUS "${CLOX_EXE_NAME} compile options: ${_CFLAGS}")
get_target_property(_LFLAGS ${CLOX_EXE_NAME} LINK_OPTIONS)
message(STATUS "${CLOX_EXE_NAME} link options: ${_LFLAGS}")
