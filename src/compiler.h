#pragma once
#ifndef SRC_COMPILER_H
#define SRC_COMPILER_H

#include <string>
#include <vector>

#include "util.h"

enum class CompilerType { // order of preference
    GNU,
    CLANG,
    MSVC,
    UNKNOWN
};

struct Compiler {
    CompilerType type;
    std::string cc;
    std::string cxx;
    std::string link;
    std::string ar;
};

Compiler defaultGNUCompiler() {
    return Compiler{CompilerType::GNU, "gcc", "g++", "g++", "ar"};
}

Compiler defaultClangCompiler() {
    return Compiler{CompilerType::CLANG, "clang", "clang++", "clang++", "ar"};
}

Compiler defaultMSVCCompiler() {
    return Compiler{CompilerType::MSVC, "cl", "cl", "link", "lib"};
}

bool isGNUCompilerAvailable() {
    return system("gcc --version" NULLIFY_CMD) == 0;
}

bool isClangCompilerAvailable() {
    return system("clang --version" NULLIFY_CMD) == 0;
}

bool isMSVCCompilerAvailable() {
    return system("cl" NULLIFY_CMD) == 0;
}

Compiler defaultCompiler() {
     if (isGNUCompilerAvailable()) {
         return defaultGNUCompiler();
     } else if (isClangCompilerAvailable()) {
         return defaultClangCompiler();
     } else if (isMSVCCompilerAvailable()) {
         return defaultMSVCCompiler();
     } else {
         std::cerr << "No compiler found" << std::endl;
         exit(1);
     }
}

#endif //SRC_COMPILER_H
