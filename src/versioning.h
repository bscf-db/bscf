#pragma once
#ifndef SRC_VERSIONING_H
#define SRC_VERSIONING_H

#include <fstream>

#include "util.h"

void versionSystem(const char* exe) {
    // check if version.txt exists in the same directory as the executable
    // if not print an error message and exit
    // if it does, read the file and print the contents

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

    // TODO: check if the version on the server is newer than the local version
    // TODO: prompt the user to update if the server version is newer
    // TODO: if they accept, we update by downloading the new source code and compiling it using bscf
    // TODO: if they decline, we continue as normal

    // clone the bscf repo in exepath/bscf_repo
    // compare the version in version.txt to the version in bscf_repo/version.txt
    // if the version in bscf_repo/version.txt is newer, prompt the user to update
    // if they accept, run bscf on the bscf_repo directory to compile the new version
    // once the new version is compiled, replace the old version with the new version

    std::path bscfRepoPath = executablePath / "bscf_repo";
    std::cout << "Checking for updates..." << std::endl;
    std::string cmd;
    if (std::filesystem::exists(bscfRepoPath)) {
        cmd = "cd " + bscfRepoPath.string() + " && git pull";
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
            std::cout << "Update complete. Please restart the program." << std::endl;
            exit(0);
        } else {
            std::cout << "Update declined." << std::endl;
        }
    }
}

#endif //SRC_VERSIONING_H
