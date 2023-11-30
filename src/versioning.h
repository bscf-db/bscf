#pragma once
#ifndef SRC_VERSIONING_H
#define SRC_VERSIONING_H

#include <fstream>

#include "util.h"

void versionSystem(const char* exe) {
    // check if version.txt exists in the same directory as the executable
    // if not print an error message and exit
    // if it does, read the file and print the contents

    std::path executablePath = std::path(exe);
    std::path versionPath = executablePath.parent_path() / "version.txt";

    if (!std::filesystem::exists(versionPath)) {
        std::cout << "version.txt not found in the same directory as the executable" << std::endl;
        exit(1);
    }

    std::ifstream versionFile(versionPath);
    std::string version;
    std::getline(versionFile, version);
    std::cout << version << std::endl;

    // TODO: check if the version on the server is newer than the local version
    // TODO: prompt the user to update if the server version is newer
    // TODO: if they accept, we update by downloading the new source code and compiling it using bscf
    // TODO: if they decline, we continue as normal
    // we check by looking at the version.txt file on the github repo





}

#endif //SRC_VERSIONING_H
