#pragma once
#ifndef SRC_BUILTINS_H
#define SRC_BUILTINS_H

#include <string>
#include <filesystem>
#include <map>

#include "util.h"

// two types of builtins:
// 1. builtins that have a source repo, and a seperate project file in the bscf-db (normally for projects that i didn't create)
// 2. builtins that are just a single repo (normally for projects that i created that already have a proj.bscf file) doesn't use the db at all


const std::string BSCF_GLFW_DB = "https://github.com/bscf-db/glfw"; // contains only proj.bscf
const std::string BSCF_GLFW_REPO = "https://github.com/glfw/glfw";

const std::string BSCF_WHEREAMI_DB = "https://github.com/bscf-db/whereami";
const std::string BSCF_WHEREAMI_REPO = "https://github.com/gpakosz/whereami";

struct bscfBuiltin {
    std::string db;
    std::string repo;
    bool singleRepo;
};

const std::map<std::string, bscfBuiltin> BSCF_BUILTINS = {
        {"glfw", {BSCF_GLFW_DB, BSCF_GLFW_REPO, false}},
        {"whereami", {BSCF_WHEREAMI_DB, BSCF_WHEREAMI_REPO, false}}
};


bool getBuiltin(const std::string& name, const std::path& path) {
    if (BSCF_BUILTINS.find(name) == BSCF_BUILTINS.end()) {
        return false;
    }
    // download and put in path/lib/name

    const bscfBuiltin& builtin = BSCF_BUILTINS.at(name);
    // if exists git reset --hard and git pull
    if (builtin.singleRepo) {
        if (std::filesystem::exists(path / "lib" / name)) {
            std::string cmd = "cd " + (path / "lib" / name).string() + " && git reset --hard " NULLIFY_CMD " && git pull" + NULLIFY_CMD;
            system(cmd.c_str());
            return true;
        }
        std::string url = builtin.repo;
        std::string cmd = "git clone " + url + " " + (path / "lib" / name).string() + NULLIFY_CMD;
        system(cmd.c_str());
    } else {
        if (std::filesystem::exists(path / "lib" / name)) {
            std::string cmd = "cd " + (path / "lib" / name).string() + " && git reset --hard " NULLIFY_CMD " && git pull" + NULLIFY_CMD;
            system(cmd.c_str());
            return true;
        } else {
            std::string url = builtin.repo;
            std::string cmd = "git clone " + url + " " + (path / "lib" / name).string() + NULLIFY_CMD;
            system(cmd.c_str());
        }
        // now we need to download the proj.bscf file
        std::string url = builtin.db;
        std::string cmd = "git clone " + url + " " + (path / "lib" / name / "bscf-db").string() + NULLIFY_CMD;
        // first create the bscf-db folder
        std::filesystem::create_directory(path / "lib" / name / "bscf-db");
        system(cmd.c_str());
        // now copy the proj.bscf file, but make sure to delete the old one first
        if (std::filesystem::exists(path / "lib" / name / "proj.bscf")) {
            std::filesystem::remove(path / "lib" / name / "proj.bscf");
        }
        std::filesystem::copy(path / "lib" / name / "bscf-db" / "proj.bscf", path / "lib" / name / "proj.bscf");
        // now delete the bscf-db folder
        // if for some reason we can't delete it, that's fine, it's not a big deal so we need to just ignore it
        try {
            std::filesystem::remove_all(path / "lib" / name / "bscf-db");
        } catch (...) {}

    }
    return true;
}


#endif //SRC_BUILTINS_H
