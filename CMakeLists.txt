cmake_minimum_required(VERSION 3.26)
project(bscf)

set(CMAKE_CXX_STANDARD 17)

add_executable(bscf src/main.cpp
        src/compiler.h
        src/builtins.h
        src/util.h
        src/versioning.h
    lib/whereami/src/whereami.c
        lib/whereami/src/whereami.h)

target_include_directories(bscf PRIVATE lib/whereami/src)

# postbuild copy version.txt to exe dir
add_custom_command(TARGET bscf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/version.txt
        ${CMAKE_CURRENT_BINARY_DIR}/version.txt)

# if source dir/lib/whereami doesn't exist, clone it from https://github.com/gpakosz/whereami
# if it does, pull latest

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/lib/whereami)
    execute_process(COMMAND git clone https://github.com/gpakosz/whereami lib/whereami
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

else()
    execute_process(COMMAND git pull --force
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/whereami)
endif()