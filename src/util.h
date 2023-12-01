#pragma once
#ifndef SRC_UTIL_H
#define SRC_UTIL_H

#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <regex>

#ifdef _WIN32
#define NULLIFY_CMD " > NUL 2>&1"
// NULLIFY_CMD supresses all output
// NULLIFY_ERR supresses only normal output, but allows errors
#define NULLIFY_ERR " > NUL"
#else
#define NULLIFY_CMD " > /dev/null 2>&1"
// NULLIFY_CMD supresses all output
// NULLIFY_ERR supresses only normal output, but allows errors
#define NULLIFY_ERR " > /dev/null"
#endif

#ifdef _WIN32
#define LIB_PREFIX ""
#define LIB_SUFFIX ".lib"
#else
#define LIB_PREFIX "lib"
#define LIB_SUFFIX ".a"
#endif

namespace std {
    using namespace std::filesystem;
}


std::string replace(std::string str, const std::string& from, const std::string& to) {
    size_t pos = str.find(from);
    while (pos != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos = str.find(from, pos + to.length());
    }
    return str;
}

std::vector<std::path> recurseDir(const std::path& dir) {
    std::vector<std::path> files;
    for (const auto& entry : std::directory_iterator(dir)) {
        if (entry.is_directory()) {
            std::vector<std::path> subFiles = recurseDir(entry.path());
            files.insert(files.end(), subFiles.begin(), subFiles.end());
        } else {
            files.push_back(entry.path());
        }
    }
    return files;
}

std::vector<std::path> globDir(const std::path& dir) {
    std::vector<std::path> files;
    for (const auto& entry : std::directory_iterator(dir)) {
        if (!entry.is_directory()) {
            files.push_back(entry.path());
        }
    }
    return files;
}

std::string strip(std::string s) {
    // remove leading and trailing whitespace /n/r/t etc
    s = std::regex_replace(s, std::regex("^\\s+"), "");
    s = std::regex_replace(s, std::regex("\\s+$"), "");
    return s;

}

#endif //SRC_UTIL_H
