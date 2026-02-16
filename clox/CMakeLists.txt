set(SOURCES
    "src/main.c"
    "src/common.h"

    "src/bytecode_chunk.h"
    "src/bytecode_chunk.c"
    "src/clox_value.h"
    "src/clox_value.c"
    "src/compiler.h"
    "src/compiler.c"
    "src/debug.h"
    "src/debug.c"
    "src/hash_table.h"
    "src/hash_table.c"
    "src/lexer.h"
    "src/lexer.c"
    "src/memory.h"
    "src/memory.c"
    "src/object.h"
    "src/object.c"
    "src/vm.h"
    "src/vm.c"
)

add_executable(clox ${SOURCES})
target_compile_features(clox PRIVATE c_std_17)
set_target_properties(clox PROPERTIES C_EXTENSIONS OFF)

target_compile_options(clox PRIVATE
    -Wall
    -Wextra
    -pedantic-errors
    -Wno-unused-parameter
    -Wshadow
    -Wconversion
    -Wno-switch-enum
    -Wno-switch
    -fsigned-char

    $<$<CONFIG:Debug>:-fsanitize=address;-fsanitize=undefined>

    $<$<CONFIG:Release>:-O3>
)

target_link_options(clox PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address;-fsanitize=undefined>
)

message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")
get_target_property(_CFLAGS clox COMPILE_OPTIONS)
message(STATUS "main compile options: ${_CFLAGS}")
get_target_property(_LFLAGS clox LINK_OPTIONS)
message(STATUS "main link options: ${_LFLAGS}")
