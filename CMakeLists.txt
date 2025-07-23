cmake_minimum_required(VERSION 3.25)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC runtime" FORCE)

project(c-lox VERSION 0.0.1 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(c-lox)
