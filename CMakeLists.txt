cmake_minimum_required(VERSION 3.26)
project(bscf)

set(CMAKE_CXX_STANDARD 17)

add_executable(bscf src/main.cpp
        src/compiler.h
        src/builtins.h
        src/util.h
        src/versioning.h)

# postbuild copy version.txt to exe dir
add_custom_command(TARGET bscf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/version.txt
        ${CMAKE_CURRENT_BINARY_DIR}/version.txt)
