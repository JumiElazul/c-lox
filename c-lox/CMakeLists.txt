set(CLOX_SOURCES
    "src/main.c"
    "src/common.h"

    "src/bytecode_chunk.h"
    "src/bytecode_chunk.c"
    "src/clox_value.h"
    "src/clox_value.c"
    "src/clox_object.h"
    "src/clox_object.c"
    "src/compiler.h"
    "src/compiler.c"
    "src/disassembler.h"
    "src/disassembler.c"
    "src/hash_table.h"
    "src/hash_table.c"
    "src/lexer.h"
    "src/lexer.c"
    "src/memory.h"
    "src/memory.c"
    "src/std_library.h"
    "src/std_library.c"
    "src/virtual_machine.h"
    "src/virtual_machine.c"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libedit REQUIRED IMPORTED_TARGET libedit)

set(CLOX_EXE_NAME c-lox)
add_executable(${CLOX_EXE_NAME} ${CLOX_SOURCES})

target_link_libraries(${CLOX_EXE_NAME} PRIVATE PkgConfig::libedit)

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

target_compile_definitions(${CLOX_EXE_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG_PRINT_CODE;DEBUG_TRACE_EXECUTION>
)

message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")
get_target_property(_CFLAGS ${CLOX_EXE_NAME} COMPILE_OPTIONS)
message(STATUS "${CLOX_EXE_NAME} compile options: ${_CFLAGS}")
get_target_property(_LFLAGS ${CLOX_EXE_NAME} LINK_OPTIONS)
message(STATUS "${CLOX_EXE_NAME} link options: ${_LFLAGS}")
