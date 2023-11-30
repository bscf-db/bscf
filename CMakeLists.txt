cmake_minimum_required(VERSION 3.26)
project(bscf)

set(CMAKE_CXX_STANDARD 17)

add_executable(bscf src/main.cpp
        src/compiler.h
        src/builtins.h
        src/util.h
        src/versioning.h)
