#pragma once
#ifndef SRC_VERSIONING_H
#define SRC_VERSIONING_H

#include <fstream>

#include "whereami.h"
#include "util.h"

void versionSystem() {
    // check if version.txt exists in the same directory as the executable
    // if not print an error message and exit
    // if it does, read the file and print the contents

    int length = wai_getExecutablePath(NULL, 0, NULL);
    int dirname_length;
    char* path = (char*)malloc(length + 1);
    wai_getExecutablePath(path, length, &dirname_length);
    path[length] = '\0';
    std::string exe = std::string(path);
    free(path);


    std::path executablePath = std::path(exe).parent_path();
    std::path versionPath = executablePath / "version.txt";
    // executableName
    std::string executableName = std::path(exe).filename().string();

    if (!std::filesystem::exists(versionPath)) {
        std::cout << "version.txt not found in the same directory as the executable" << std::endl;
        exit(1);
    }

    std::ifstream versionFile(versionPath);
    std::string version;
    std::getline(versionFile, version);
    version = strip(version);
    std::cout << version << std::endl;
    versionFile.close();

    if (std::exists(executablePath / ("old_" + executableName))) {
        std::filesystem::remove(executablePath / ("old_" + executableName));
    }

    std::path bscfRepoPath = executablePath / "bscf_repo";
    std::cout << "Checking for updates..." << std::endl;
    std::string cmd;
    if (std::filesystem::exists(bscfRepoPath)) {
        cmd = "cd " + bscfRepoPath.string() + " && git pull --force" + NULLIFY_CMD;
    } else {
        cmd = "git clone https://github.com/bscf-db/bscf " + bscfRepoPath.string() + NULLIFY_CMD;
    }
    system(cmd.c_str());

    std::ifstream bscfVersionFile(bscfRepoPath / "version.txt");
    std::string bscfVersion;
    std::getline(bscfVersionFile, bscfVersion);
    bscfVersion = strip(bscfVersion);
    bscfVersionFile.close();

    if (version != bscfVersion) {
        std::cout << "New version available: '" << bscfVersion << "'" << std::endl;
        std::cout << "Current version: '" << version << "'" << std::endl;

        std::cout << "A new version of bscf is available. Would you like to update? (y/n)" << std::endl;
        std::string response;
        std::cin >> response;
        if (response == "y") {
            std::cout << "Building new version..." << std::endl;
#ifdef _WIN32
            cmd = "cd " + bscfRepoPath.string() + " && ..\\bscf.exe NOUPDATE";
#else
            cmd = "cd " + bscfRepoPath.string() + " && ../bscf NOUPDATE";
#endif
            system(cmd.c_str());
            std::cout << "Build complete." << std::endl;
            // now replace the old version with the new version
            // wait, how do we do that?
            // we can't just replace the executable because it's running
            // we need to replace the executable that will be run next time
            // we need to rename the current executable (if already exist, delete), then copy the new executable to the old executable's name
            // then exit
            if (std::filesystem::exists(executablePath / ("old_" + executableName))) {
                std::filesystem::remove(executablePath / ("old_" + executableName));
            }
            std::filesystem::rename(executablePath / executableName, executablePath / ("old_" + executableName));
            std::filesystem::copy(bscfRepoPath / "build" / "bin" / executableName, executablePath / executableName);
            // delete old version.txt
            std::filesystem::remove(versionPath);
            // copy new version.txt
            std::filesystem::copy(bscfRepoPath / "version.txt", versionPath);
            std::cout << "Update complete. Please restart the program." << std::endl;
            exit(0);
        } else {
            std::cout << "Update declined." << std::endl;
        }
    }
}

#endif //SRC_VERSIONING_H
