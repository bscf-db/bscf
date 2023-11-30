/*
BSCF: Build System Configuration File
A new file format for storing build system configuration for C/C++ projects.
The file will be named proj.bscf and will be stored in the project root.
The projects aim for a clean easy to read format, as well as a clean easy to understand file structure.
A project (by default, these dirs can be change in the bscf) will have the following structure:
    proj/
        build/ (this is where the build system will store all build files)
        lib/ (this is where all libraries (SUB PROJECTS) will be stored)
        blib/ (this is where all binary libraries will be stored) blib/static/ blib/dynamic/ # not implemented yet
        src/ (this is where all source files for this project will be stored, including headers)
        proj.bscf (this is the build system configuration file)

An example with a sub project:
    proj/
        build/
        lib/
            subproj/
                build/
                src/
                subproj.bscf
        src/
        proj.bscf

All you need to put in the proj.bscf (for a c/c++ project with no dependencies) is:
TARGET EXEC test ALL

For a c/c++ project with a subproject dependency:
TARGET EXEC test ALL
INCLUDE subproj // include all targets from subproj
DEPEND test sometarget // add sometarget from subproj as a dependency to test

For a c/c++ project with a static library dependency:
TARGET EXEC test ALL
SLIB test mylibname

For a c/c++ project with a dynamic library dependency:
TARGET EXEC test ALL
DLIB test mylibname

DLIB doesnt really do much other than copy the library to the build dir.

This python script will read the proj.bscf and compile everything.
if you have this file structure:
    proj/
        build/
        src/
            main.c
            test.c
            test.h
        proj.bscf
and this in proj.bscf:
TARGET EXEC test ALL

then the build dir will contain the following:
    build/
        bin/
            test.exe
        obj/
            main_c.o
            test_c.o
        cache/
            test.target // all commands required to compile the test target
*/

/* Usage:
 * bscf [folder] [command(s)]
 * commands:
 * c, clean: clean the build dir (remove all build files)
 * sc, softclean: clean the build dir, but leave executables and libraries
 * b, build: build all targets
 * bc, buildcache: generate cahce files, but don't compile anything
 * gnu, msvc, clang: set the compiler
 * e, echo: echo commands
 * ne, noecho: don't echo commands (default)
 * [target(s)]: build the specified target(s)
 *
 * this means that you cannot have a target named "c" or "clean" or "sc" or "softclean" or "b" or "build" or "gnu" or "msvc" or "clang" or "bc" or "buildcache" or "e" or "echo" or "ne" or "noecho"
 * because then the build system will think that you are trying to run a command
 *
 * commands will be run in the order that they are specified
 * so you could build target a with gnu and target b with clang like this:
 * bscf . gnu a clang b
 * you can clean, then rebuild all like this:
 * bscf . c b
 *
 * the . is default, so you can do this:
 * bscf
 * this will build all targets (b is default if no command(s) is/are specified)
 *
 * the dir is required if you are doing a command other than the default (build)
 * so you can do this:
 * bscf . c
 * but you cannot do this:
 * bscf c
 * because then the build system will think that you are trying to build in a folder named c
 */

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>

#include "compiler.h"
#include "builtins.h"
#include "util.h"
#include "versioning.h"

bool echo = false;

enum class Command {
    TARGET,
    INCLUDE,
    DEPEND,
    GITINCLUDE,
    // 2 types of custom build commands: prebuild and postbuild
    // the commands are added to the target file either before or after the build commands
    // the commands are added to the target file in the order that they are specified
    PREBUILD,
    POSTBUILD,
    // we need platform specific commands
    // so we need to be able to specify a command for a specific platform
    // IF PLATFORM [windows/linux/macos/bsd/unix] [command] ENDIF
    // unix is all unix based systems (linux, macos, bsd, etc)
    IF,
    ENDIF,
    DEFINE, // define a preprocessor macro for a specific target
    LIB, // add a library to link to basically just adds -l[libname] to the link command
    INCDIR, // add an include directory to the target, basically just adds -I[dir] to the compile command for this target and dependent targets
    BUILTIN, // include a builtin library, very similar to GITINCLUDE but it does it from my github repo and the source
};

std::unordered_map<std::string, Command> commandMap = {
        {"TARGET", Command::TARGET},
        {"INCLUDE", Command::INCLUDE},
        {"DEPEND", Command::DEPEND},
        {"GITINCLUDE", Command::GITINCLUDE},
        {"PREBUILD", Command::PREBUILD},
        {"POSTBUILD", Command::POSTBUILD},
        {"IF", Command::IF},
        {"ENDIF", Command::ENDIF},
        {"DEFINE", Command::DEFINE},
        {"LIB", Command::LIB},
        {"INCDIR", Command::INCDIR},
        {"BUILTIN", Command::BUILTIN},
};

// A FileLib is not a target type, but it is a way of specifying a dependency on a sub project that either generates a static or dynamic library.

enum class TargetType {
    EXEC,
    SLIB,
    DLIB,
};

struct Target {
    TargetType type;
    std::string name;
    std::path path;
    std::vector<std::string> sources; // relative to proj root
    std::vector<std::string> dependencies; // other targets
    std::vector<std::string> prebuildcmds;
    std::vector<std::string> postbuildcmds;
    std::vector<std::string> defines;
    std::vector<std::string> libs; // libs to link
    std::vector<std::string> includes; // include dirs // meant for libs

};

std::string bscfRead(const std::path& p) {
    std::path projPath = p / "proj.bscf";
    if (!std::exists(projPath)) {
        std::cout << "proj.bscf does not exist in " << p << std::endl;
        exit(1);
    }
    std::ifstream file(projPath);
    std::string contents;
    std::stringstream buffer;
    buffer << file.rdbuf();
    contents = buffer.str();

    // remove all comments (lines that start with #) and all empty lines
    // also remove inline comments

    contents = std::regex_replace(contents, std::regex("#.*"), "");
    contents = std::regex_replace(contents, std::regex("\n[\f\n\r\t\v]*\n"), "\n");
    contents = std::regex_replace(contents, std::regex("\n[\f\n\r\t\v]*"), "\n");
    contents = std::regex_replace(contents, std::regex("[\f\n\r\t\v]*\n"), "\n");


    return contents;
}

std::string bscfRead(const std::string& p) {
    return bscfRead(std::path(p));
}


std::vector<Target> bscfInclude(const std::path& path, const Compiler& c) {
    std::string bscf = bscfRead(path);
    std::vector<Target> targets;

    std::stringstream ss(bscf);
    std::string line;
    while (std::getline(ss, line)) {
        std::stringstream lineStream(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        std::string command;
        lineStream >> command;
        if (command == " " || command.empty()) continue;
        if (commandMap.find(command) == commandMap.end()) {
            std::cerr << "Invalid command: " << command << std::endl;
            continue;
        }
        switch (commandMap[command]) {
            case Command::TARGET: {
                std::string type;
                std::string name;
                lineStream >> type;
                lineStream >> name;
                Target target;
                target.path = path;
                if (type == "EXEC") {
                    target.type = TargetType::EXEC;
                } else if (type == "SLIB") {
                    target.type = TargetType::SLIB;
                } else if (type == "DLIB") {
                    target.type = TargetType::DLIB;
                } else {
                    std::cerr << "Invalid target type: " << type << std::endl;
                    continue;
                }
                target.name = name;
                std::string source;
                bool do_glob = false;
                bool is_recursive = false;
                while (lineStream >> source) {
                    if (do_glob) {
                        // we need to glob the source
                        target.includes.push_back((path / source).string());
                        if (is_recursive) {
                            std::vector<std::path> files = recurseDir(path / source);
                            for (std::path& file : files) {
                                std::string ext = file.extension().string();
                                if (ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
                                    if (file.string() != " " && std::exists(file)) {
                                        target.sources.push_back(file.string());
                                    }
                                }
                            }
                        } else {
                            std::vector<std::path> files = globDir(path / source);
                            for (std::path& file : files) {
                                std::string ext = file.extension().string();
                                if (ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
                                    if (file.string() != " " && std::exists(file)) {
                                        target.sources.push_back(file.string());
                                    }
                                }
                            }
                        }
                        do_glob = false;
                        continue;
                    }
                    if (source == "ALL") {
                        target.includes.push_back((path / "src").string());
                        // recurse all files in src/ that end in .c, .cpp, .cc, .cxx, .h, .hpp, .hh, .hxx
                        std::vector<std::path> files = recurseDir(path / "src");
                        for (std::path& file : files) {
                            std::string ext = file.extension().string();
                            if (ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
                                if (file.string() != " " && std::exists(file))
                                    target.sources.push_back(file.string());
                            }
                        }
                        break;
                    }
                    if (source == "GLOB") {
                        do_glob = true;
                        is_recursive = false;
                        continue;
                    }
                    if (source == "RECURSE") {
                        do_glob = true;
                        is_recursive = true;
                        continue;
                    }
                    if (source == " " || source.empty()) continue;
                    target.sources.push_back((path / source).string());
                }
                targets.push_back(target);
            } break;
            case Command::INCLUDE: {
                std::string p;
                lineStream >> p;
                std::path pt = path / "lib" / p;
                std::vector<Target> includedTargets = bscfInclude(pt, c);
                targets.insert(targets.end(), includedTargets.begin(), includedTargets.end());
            } break;
            case Command::DEPEND: {
                std::string targetName;
                std::string dependencyName;
                lineStream >> targetName;
                lineStream >> dependencyName;
                for (Target& target : targets) {
                    if (target.name == targetName) {
                        target.dependencies.push_back(dependencyName);
                        break;
                    }
                }
            } break;
            case Command::GITINCLUDE: {
                // if git isn't installed, then give an error
                if (system("git --version" NULLIFY_CMD) != 0) {
                    std::cout << "Git is not installed" << std::endl;
                    exit(1);
                }
                std::string link;
                std::string name;
                lineStream >> link;
                lineStream >> name;
                std::string gitDir = (path / "lib" / name).string();
                std::create_directories(path / "lib");
                // if the directory already exists, git fetch it else git clone it
                if (std::exists(gitDir)) {
                    std::cout << std::endl << "Updating " << name << std::endl;
                    // if the file was changed locally, then the git pull won't fail, ut will say that it is up to date
                    // we need to make it realize that it is not up to date
                    // so we need to git add
                    std::string cmd = "cd " + gitDir + " && git reset --hard " NULLIFY_CMD " && git pull origin" NULLIFY_CMD;
                    system(cmd.c_str());
                } else {
                    std::cout << "Cloning " << name << std::endl;
                    std::string cmd = "cd " + (path / "lib").string() + " && git clone " + link + " " + name + NULLIFY_CMD;
                    system(cmd.c_str());
                }
                std::vector<Target> includedTargets = bscfInclude(gitDir, c);
                targets.insert(targets.end(), includedTargets.begin(), includedTargets.end());

            } break;
            case Command::PREBUILD: {
                std::string targetName;
                std::string cmd;
                lineStream >> targetName;
                std::getline(lineStream, cmd);
                for (Target& target : targets) {
                    if (target.name == targetName) {
                        target.prebuildcmds.push_back(cmd);
                        break;
                    }
                }
            } break;
            case Command::POSTBUILD: {
                std::string targetName;
                std::string cmd;
                lineStream >> targetName;
                std::getline(lineStream, cmd);
                for (Target& target : targets) {
                    if (target.name == targetName) {
                        target.postbuildcmds.push_back(cmd);
                        break;
                    }
                }
            } break;
            case Command::IF: {
                // if the if is true, then we do nothing
                // if false, we need to keep track of the level of the ifs, and keep reading until we get to the matching endif
                // if we get to another if, raise level by 1, if we get to an endif, lower level by 1
                // if level is 0, then we are done
                std::string ifcmd;
                lineStream >> ifcmd;
                bool failed = true;
                if (ifcmd == "NOT") {
                    failed = false;
                    lineStream >> ifcmd;
                }
                if (ifcmd == "PLATFORM") {
                    std::string platform;
                    lineStream >> platform;
                    if (platform == "windows") {
#ifdef _WIN32
                        failed = !failed;
#endif
                    } else if (platform == "linux") {
#ifdef __linux__
                        failed = !failed;
#endif
                    } else if (platform == "macos") {
#ifdef __APPLE__
                        failed = !failed;
#endif
                    } else if (platform == "bsd") {
#ifdef __FreeBSD__
                        failed = !failed;
#endif
                    } else if (platform == "unix") {
#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
                        failed = !failed;
#endif
                    } else {
                        std::cerr << "Invalid platform: " << platform << std::endl;
                        failed = true;
                    }
                } else if (ifcmd == "COMPILER") {
                    std::string compiler;
                    lineStream >> compiler;
                    if (compiler == "gnu") {
                        if (c.type == CompilerType::GNU) {
                            failed = !failed;
                        }
                    } else if (compiler == "msvc") {
                        if (c.type == CompilerType::MSVC) {
                            failed = !failed;
                        }
                    } else if (compiler == "clang") {
                        if (c.type == CompilerType::CLANG) {
                            failed = !failed;
                        }
                    } else {
                        std::cerr << "Invalid compiler: " << compiler << std::endl;
                        failed = true;
                    }
                } else {
                    std::cerr << "Invalid if command: " << ifcmd << std::endl;
                }
                if (failed) {
                    // we need to keep reading until we get to the matching endif
                    int level = 0;
                    while (std::getline(ss, line)) {
                        std::stringstream iflineStream(line);
                        if (line.empty()) continue;
                        if (line[0] == '#') continue;
                        std::string ifcommand;
                        iflineStream >> ifcommand;
                        if (commandMap.find(ifcommand) == commandMap.end()) {
                            std::cerr << "Invalid if command: " << ifcommand << std::endl;
                            continue;
                        }
                        if (commandMap[ifcommand] == Command::IF) {
                            level++;
                        } else if (commandMap[ifcommand] == Command::ENDIF) {
                            level--;
                            if (level <= 0) {
                                break;
                            }
                        }
                    }
                }
            } break;
            case Command::ENDIF: {
                // do nothing
            } break;
            case Command::DEFINE: {
                // usage:
                // DEFINE target MACRO
                // DEFINE target MACRO=VALUE // no spaces
                std::string targetName;
                std::string macro; // macro=value or macro
                lineStream >> targetName;
                // get the rest of the line
                std::getline(lineStream, macro);
                // remove leading and trailing spaces
                macro = std::regex_replace(macro, std::regex("^ +| +$"), "");
                for (Target& target : targets) {
                    if (target.name == targetName) {
                        target.defines.push_back(macro);
                        break;
                    }
                }
            } break;
            case Command::LIB: {
                std::string targetName;
                std::string lib;
                lineStream >> targetName;
                lineStream >> lib;
                for (Target& target : targets) {
                    if (target.name == targetName) {
                        target.libs.push_back(lib);
                        break;
                    }
                }
            } break;
            case Command::INCDIR: {
                std::string targetName;
                std::string incdir;
                lineStream >> targetName;
                lineStream >> incdir;
                for (Target& target : targets) {
                    if (target.name == targetName) {
                        target.includes.push_back((path / incdir).string());
                        break;
                    }
                }
            } break;
            case Command::BUILTIN: {
                if (system("git --version" NULLIFY_CMD) != 0) {
                    std::cout << "Git is not installed" << std::endl;
                    exit(1);
                }
                std::string name;
                lineStream >> name;
                bool x = getBuiltin(name, path);
                if (!x) {
                    std::cout << "Builtin " << name << " failed" << std::endl;
                    exit(1);
                }
                std::vector<Target> includedTargets = bscfInclude(path / "lib" / name, c);
                targets.insert(targets.end(), includedTargets.begin(), includedTargets.end());
            } break;
            default:
                break;
        }

    }
    return targets;

}

std::string bscfSourceCmd(const Target& t, const Compiler& c, const std::string& source, std::vector<std::string>& objs) {
    // check cc or cxx
    std::string ext = std::path(source).extension().string();
    // get source relative to target path/src
    // and replace / with _
    // so if source is ex/src/testdir/main.c
    // output should be ex/build/obj/testdir_main.c
    // first, convert to relpath
    std::string relpath = std::relative(source, t.path).string();
    // then replace / with _
    std::string objname = replace(relpath, "/", "_");
    objname = replace(objname, "\\","_"); // windows
    // then add .o
    objname += ".o";
    if (ext == ".c" || ext == ".cc") {
        objs.push_back(objname);
        return c.cc + " -c " + source + " -o " + t.path.string() + "/build/obj/" + objname;
    } else if (ext == ".cpp" || ext == ".cxx") {
        objs.push_back(objname);
        return c.cxx + " -c " + source + " -o " + t.path.string() + "/build/obj/" + objname;
    } else {
        std::cout << "Skipping " << source << std::endl;
    }
    return "";
}

std::vector<std::string> bscfResolveIncludes(const Target& t, const std::vector<Target>& targets) {
    std::vector<std::string> includes;
    for (const std::string& dep : t.dependencies) {
        for (const Target& target : targets) {
            if (target.name == dep) {
                // add target.includes to includes
                std::vector<std::string> depIncludes = bscfResolveIncludes(target, targets);
                includes.insert(includes.end(), depIncludes.begin(), depIncludes.end());
            }
        }
    }

    for (const std::string& inc : t.includes) {
        includes.push_back(inc);
    }

    return includes;
}

std::vector<std::string> bscfGenCmd(const Target& t, const Compiler& c, const std::vector<Target>& targets) {
    std::vector<std::string> commands;
    std::string comp_flags = " ";
    std::string link_flags = " ";
    if (!t.prebuildcmds.empty()) {
        for (const std::string& cmd : t.prebuildcmds) {
            commands.push_back(cmd);
        }
    }
    if (!t.libs.empty()) {
        for (const std::string& lib : t.libs) {
            link_flags += "-l" + lib + " ";
        }
    }
    if (!t.defines.empty()) {
        for (const std::string& def : t.defines) {
            comp_flags += "-D" + def + " ";
        }
    }
    if (!t.dependencies.empty()) {
        for (const std::string& dep : t.dependencies) {
            for (const Target& target : targets) {
                if (target.name == dep) {
                    // the other target is a dependency, so it will be built first by the runner, so we don't need to include it's build commands
                    // all we need to do, is add the include flags
                    switch (target.type) {
                        case TargetType::EXEC:
                            break;
                        case TargetType::SLIB:
                            link_flags += "-L" + (target.path / "build" / "lib").string() + " ";
                            link_flags += "-l" + target.name + " ";
                            // if the dep has a folder called include, then add that to the include flags
                            if (!target.libs.empty()) {
                                for (const std::string& lib : target.libs) {
                                    link_flags += "-l" + lib + " ";
                                }
                            }
                            break;
                        case TargetType::DLIB:
                            link_flags += "-L" + (target.path / "build" / "bin").string() + " ";
                            link_flags += "-l" + target.name + " ";
                            // add the copy command
#ifdef _WIN32
                            commands.push_back("copy " + (target.path / "build" / "bin" / (target.name + ".dll")).string() + " " + (t.path / "build" / "bin" / (target.name + ".dll")).string());
#else
                            commands.push_back("cp " + (target.path / "build" / "bin" / ("lib" + target.name + ".so")).string() + " " + (t.path / "build" / "bin" / ("lib" + target.name + ".so")).string());
#endif
                            break;
                        default:
                            break;
                    }
                    break;
                }
            }
        }
    }

    std::vector<std::string> includes = bscfResolveIncludes(t, targets);
    for (const std::string& inc : includes) {
        comp_flags += "-I" + inc + " ";
    }

    switch (t.type) {
        case TargetType::EXEC: {
            std::vector<std::string> objs;
            for (const std::string& source : t.sources) {
                commands.push_back(bscfSourceCmd(t, c, source, objs) + comp_flags);
            }
            std::string linkCmd = c.link + " ";
            for (const std::string& obj : objs) {
                linkCmd += t.path.string() + "/build/obj/" + obj + " ";
            }
#ifdef _WIN32
            linkCmd += "-o " + t.path.string() + "/build/bin/" + t.name + ".exe";
#else
            linkCmd += "-o " + t.path.string() + "/build/bin/" + t.name;
#endif
            commands.push_back(linkCmd + link_flags);
            std::create_directories(t.path / "build" / "obj");
            std::create_directories(t.path / "build" / "bin");
        } break;
        case TargetType::SLIB: {
            std::vector<std::string> objs;
            for (const std::string& source : t.sources) {
                commands.push_back(bscfSourceCmd(t, c, source, objs) + comp_flags);
            }
            std::string arCmd = c.ar + " rcs " + t.path.string() + "/build/lib/" + LIB_PREFIX + t.name + LIB_SUFFIX + " ";
            for (const std::string& obj : objs) {
                arCmd += t.path.string() + "/build/obj/" + obj + " ";
            }
            commands.push_back(arCmd);
            std::create_directories(t.path / "build" / "obj");
            std::create_directories(t.path / "build" / "lib");
        } break;
        case TargetType::DLIB: {
            std::vector<std::string> objs;
            for (const std::string& source : t.sources) {
                commands.push_back(bscfSourceCmd(t, c, source, objs) + comp_flags + " -fPIC");
            }
            std::string linkCmd = c.link + " -shared ";
            for (const std::string& obj : objs) {
                linkCmd += t.path.string() + "/build/obj/" + obj + " ";
            }
#ifdef _WIN32
            linkCmd += "-o " + t.path.string() + "/build/bin/" + t.name + ".dll";
#else
            linkCmd += "-o " + t.path.string() + "/build/bin/" + t.name + ".so";
#endif
            commands.push_back(linkCmd + link_flags);
            std::create_directories(t.path / "build" / "obj");
            std::create_directories(t.path / "build" / "bin");
        } break;
        default:
            std::cout << "Not implemented yet" << std::endl;
            std::cout << "Target name: " << t.name << std::endl;
            break;
    }
    if (!t.postbuildcmds.empty()) {
        for (const std::string& cmd : t.postbuildcmds) {
            commands.push_back(cmd);
        }
    }
    return commands;
}

std::vector<Target> bscfGenCache(const std::path& dir, const Compiler& c) {
    std::vector<Target> targets = bscfInclude(dir, c);
    for (Target& t : targets) {
        std::create_directories(t.path / "build" / "cache");
        std::vector<std::string> commands = bscfGenCmd(t, c, targets);
        std::path out = t.path / "build" / "cache" / (t.name + ".target");
        std::ofstream file(out);
        for (std::string& cmd : commands) {
            file << cmd << std::endl;
        }
        file.close();

    }
    return targets;
}

class bscfBuilder {
private:
    std::vector<Target> targets;
    std::vector<Target> built;
    std::vector<Target> failed;
    // we need to keep track of the targets that have been built, so we don't build them again
    // but we can't just loop through the targets, because we need to build the dependencies first
    // so we need to keep track of the targets that have been built, and then loop through the targets, and if the target has been built, skip it
    // and if the target has not been built, build it, and add it to the built list

    bool buildTargetCMD(const Target& t) {
        std::cout << "# Building " << t.name << std::endl;
        // just run commands in the t.path/build/cache/t.name.cache file
        std::ifstream file(t.path / "build" / "cache" / (t.name + ".target"));
        std::string cmd;
        while (std::getline(file, cmd)) {
            if (echo)
                std::cout << cmd << std::endl;
            int s = system(cmd.c_str());
            if (s != 0) {
                std::cerr << "Failed to build " << t.name << std::endl;
                return false;
            }
        }
        file.close();
        built.push_back(t);
        return true;
    }

    bool buildTarget(const Target& t) {
        // first, check if the target has already been built
        for (const Target& b : built) {
            if (b.name == t.name) {
                // target has already been built
                return true;
            }
        }
        for (const Target& f : failed) {
            if (f.name == t.name) {
                // target has failed before
                return false;
            }
        }
        // target has not been built
        // first, build dependencies
        for (const std::string& dep : t.dependencies) {
            for (const Target& target : targets) {
                if (target.name == dep) {
                    bool ok = buildTarget(target);
                    if (!ok) {
                        failed.push_back(target);
                        failed.push_back(t);
                        return false;
                    }
                    break;
                }
            }
        }
        // now build the target
        bool good = buildTargetCMD(t);
        if (!good) {
            failed.push_back(t);
            return false;
        }
        return true;
    }

public:
    explicit bscfBuilder(const std::vector<Target>& targets) {
        this->targets = targets;
    }

    void build() {
        for (const Target& t : targets) {
            buildTarget(t);
        }
    }

    void buildTarget(const std::string& target) {
        for (const Target& t : targets) {
            if (t.name == target) {
                buildTarget(t);
                return;
            }
        }
        std::cout << "Target " << target << " not found" << std::endl;
    }
};

int main(int argc, char* argv[]) {

    std::path p = ".";
    Compiler c = defaultCompiler();
    std::vector<std::string> commands;

    if (argc > 1) {
        if (argv[1] == "NOUPDATE") {
            // build current project then exit (used for auto update)
            std::vector<Target> targets = bscfGenCache(p, c);
            bscfBuilder builder(targets);
            builder.build();
            return 0;
        }
        p = argv[1];

    }

    versionSystem(argv[0]);

    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            commands.emplace_back(argv[i]);
        }
    }

    if (commands.empty()) {
        commands.emplace_back("build");
    }

    for (const std::string& com : commands) {
        if (com == "clean" || com == "c") {
            std::vector<Target> targets = bscfInclude(p, c);
            for (Target& t : targets) {
                std::cout << "Cleaning " << t.name << std::endl;
                std::remove_all(t.path / "build");
            }
            std::cout << "Done cleaning" << std::endl;
        } else if (com == "softclean" || com == "sc") {
            // clean, but leave executables and libraries
            std::vector<Target> targets = bscfInclude(p, c);
            for (Target& t : targets) {
                std::cout << "Soft cleaning " << t.name << std::endl;
                std::remove_all(t.path / "build" / "obj");
                std::remove_all(t.path / "build" / "cache");
            }
        } else if (com == "build" || com == "b") {
            std::cout << "Generating build files... ";
            std::vector<Target> targets = bscfGenCache(p, c);
            std::cout << "Done" << std::endl;

            bscfBuilder builder(targets);
            builder.build();
        } else if (com == "buildcache" || com == "bc") {
            std::cout << "Generating build files... ";
            std::vector<Target> targets = bscfGenCache(p, c);
            std::cout << "Done" << std::endl;
        } else if (com == "gnu") {
            c = defaultGNUCompiler();
        } else if (com == "msvc") {
            c = defaultMSVCCompiler();
        } else if (com == "clang") {
            c = defaultClangCompiler();
        } else if (com == "echo" || com == "e") {
            echo = true;
        } else if (com == "noecho" || com == "ne") {
            echo = false;
        } else {
            std::cout << "Generating build files... ";
            std::vector<Target> targets = bscfGenCache(p, c);
            std::cout << "Done" << std::endl;
            bscfBuilder builder(targets);
            builder.buildTarget(com);
        }
    }

    return 0;
}
