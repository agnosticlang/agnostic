// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "ast/ast.hpp"
#include "backend/gcc/backend.hpp"
#include "backend/llvm/codegen.hpp"
#include "lexer/lexer.hpp"
#include "parser/monomorphize.hpp"
#include "parser/parser.hpp"
#include "parser/typechecker.hpp"
#include "misc/diagnostic.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <spawn.h>
#include <sys/wait.h>

extern char** environ;

namespace fs = std::filesystem;

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "error: could not read file: " << path << "\n";
        std::exit(1);
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

agn::ast::Program parseSource(const std::string& source, const std::string& file) {
    agn::lexer::Lexer lexer(source, file);
    auto tokens = lexer.tokenize();
    agn::parser::Parser parser(tokens, file, source);
    return parser.parse();
}

fs::path findRuntimeLib(const fs::path& exeDir, const std::string& buildRelPath, const std::string& fileName) {
    fs::path installed = exeDir.parent_path() / "lib" / "agnostic" / fileName;
    if (fs::exists(installed)) return installed;
    fs::path fromBuild = exeDir.parent_path().parent_path() / buildRelPath;
    if (fs::exists(fromBuild)) return fromBuild;
    return installed;
}

fs::path findModuleFile(const std::string& name, const fs::path& sourceDir, const fs::path& exeDir) {
    const fs::path searchDirs[] = {
        sourceDir,
        exeDir / "stdlib",
        exeDir.parent_path() / "share" / "agnostic" / "stdlib",
        AGNOSTIC_SOURCE_STDLIB_DIR,
    };
    for (auto& dir : searchDirs) {
        for (auto ext : {".agn", ".per"}) {
            fs::path candidate = dir / (name + ext);
            if (fs::exists(candidate)) return candidate;
        }
    }
    return {};
}

fs::path executableDir(const char* argv0) {
    std::error_code ec;
    fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (ec) self = fs::weakly_canonical(fs::absolute(argv0));
    return self.parent_path();
}

void loadModules(agn::ast::Program& program, const fs::path& sourceDir, const fs::path& exeDir,
                  std::set<std::string>& loaded) {
    auto imports = program.imports;
    for (auto& imp : imports) {
        if (loaded.count(imp.path)) continue;
        loaded.insert(imp.path);

        fs::path file = findModuleFile(imp.path, sourceDir, exeDir);
        if (file.empty()) {
            std::cerr << "error: could not find module '" << imp.path << "'\n";
            std::exit(1);
        }

        auto modSource = readFile(file.string());
        auto modProgram = parseSource(modSource, file.string());
        loadModules(modProgram, sourceDir, exeDir, loaded);

        for (auto& [name, sub] : modProgram.modules) program.modules[name] = std::move(sub);

        agn::ast::Module mod;
        mod.name = imp.path;
        mod.functions = std::move(modProgram.functions);
        program.modules[imp.path] = std::move(mod);
    }
}

void reportErrors(const std::string& summary, const std::vector<agn::parser::TypeError>& errors,
                  const std::string& sourceFile, const std::string& source) {
    std::cerr << summary << " with " << errors.size() << " error(s):\n";
    for (auto& e : errors) {
        agn::misc::CompileError err(agn::misc::ErrorKind::Type, e.message + " (in " + e.location + ")", sourceFile,
                                     e.line, e.column);
        err.withSourceLine(agn::misc::extractSourceLine(source, e.line));
        err.display();
    }
}

bool runTool(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid;
    int err = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
    if (err != 0) {
        std::cerr << "error: could not run '" << args[0] << "': " << std::strerror(err) << "\n";
        return false;
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " <source.agn> [options]\n"
              << "  --backend=llvm|gcc       select codegen backend (default: llvm)\n"
              << "  --mem=arc|manual|orc     select memory management mode (default: arc)\n"
              << "  --target-os=linux|freebsd|windows|hurd  (default: linux)\n"
              << "  --output=<path>          output executable path\n"
              << "  -c, --compile-only       emit an object file (<output>.o) instead of linking an executable\n"
              << "  --version                print version and exit\n"
              << "  --help                   print this message and exit\n"
              << "\n"
              << "Example: " << argv0 << " hello.agn --backend=llvm --output=hello\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string sourceFile;
    std::string backend = "llvm";
    std::string memMode = "arc";
    std::string targetOs = "linux";
    std::string output;
    bool compileOnly = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--version") {
            std::cout << "agnostic " << AGNOSTIC_VERSION << "\n";
            return 0;
        }
        else if (arg.rfind("--backend=", 0) == 0) backend = arg.substr(10);
        else if (arg.rfind("--mem=", 0) == 0) memMode = arg.substr(6);
        else if (arg.rfind("--target-os=", 0) == 0) targetOs = arg.substr(12);
        else if (arg.rfind("--output=", 0) == 0) output = arg.substr(9);
        else if (arg == "--llvm") backend = "llvm";
        else if (arg == "--gcc") backend = "gcc";
        else if (arg == "-c" || arg == "--compile-only") compileOnly = true;
        else if (sourceFile.empty()) sourceFile = arg;
        else {
            std::cerr << "error: unrecognized argument: " << arg << "\n";
            return 1;
        }
    }

    if (sourceFile.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    fs::path exeDir = executableDir(argv[0]);
    fs::path sourceDir = fs::path(sourceFile).parent_path();
    if (sourceDir.empty()) sourceDir = ".";

    std::string source = readFile(sourceFile);
    agn::ast::Program program;
    try {
        program = parseSource(source, sourceFile);

        fs::path resultFile = findModuleFile("result", sourceDir, exeDir);
        if (resultFile.empty()) {
            std::cerr << "error: could not find builtin stdlib module 'result' (result.agn)\n";
            return 1;
        }
        auto resultProgram = parseSource(readFile(resultFile.string()), resultFile.string());
        for (auto& s : resultProgram.structs) program.structs.push_back(std::move(s));

        std::set<std::string> loaded;
        loadModules(program, sourceDir, exeDir, loaded);
    } catch (const agn::misc::CompileError& err) {
        err.display();
        return 1;
    }

    auto monoErrors = agn::parser::monomorphizeGenerics(program, targetOs, "x86_64", memMode);
    if (!monoErrors.empty()) {
        reportErrors("generic instantiation failed", monoErrors, sourceFile, source);
        return 1;
    }

    agn::parser::TypeChecker checker(targetOs, "x86_64", memMode);
    if (!checker.checkProgram(program)) {
        reportErrors("type checking failed", checker.errors(), sourceFile, source);
        return 1;
    }

    std::string stem = sourceFile;
    auto dot = stem.rfind(".agn");
    if (dot != std::string::npos && dot == stem.size() - 4) stem = stem.substr(0, dot);
    std::string finalOutput = output.empty() ? stem : output;

    if (backend != "llvm" && backend != "gcc") {
        std::cerr << "error: unknown --backend= value '" << backend << "'\n";
        return 1;
    }

    if (targetOs != "linux" && targetOs != "freebsd" && targetOs != "windows" && targetOs != "hurd") {
        std::cerr << "error: unknown --target-os= value '" << targetOs << "'\n";
        return 1;
    }

    if (backend == "gcc" && targetOs == "windows") {
        std::cerr << "error: --backend=gcc cannot target windows (libgccjit only generates code for the host); "
                     "use --backend=llvm\n";
        return 1;
    }

    if (memMode != "arc" && memMode != "manual" && memMode != "orc") {
        std::cerr << "error: unknown --mem= value '" << memMode << "'\n";
        return 1;
    }

    std::string objPath = finalOutput + (targetOs == "windows" ? ".obj" : ".o");
    std::string codegenError;
    if (backend == "gcc") {
        agn::backend::gcc::MemMode mode = agn::backend::gcc::MemMode::Arc;
        if (memMode == "manual") mode = agn::backend::gcc::MemMode::Manual;
        else if (memMode == "orc") mode = agn::backend::gcc::MemMode::Orc;

        agn::backend::gcc::GccBackend codegen(checker, mode, fs::path(sourceFile).filename().string());
        codegen.generate(program);
        if (!codegen.emitObjectFile(objPath, codegenError)) {
            std::cerr << "error: " << codegenError << "\n";
            return 1;
        }
    } else {
        agn::backend::llvm_backend::MemMode mode = agn::backend::llvm_backend::MemMode::Arc;
        if (memMode == "manual") mode = agn::backend::llvm_backend::MemMode::Manual;
        else if (memMode == "orc") mode = agn::backend::llvm_backend::MemMode::Orc;

        agn::backend::llvm_backend::Codegen codegen(checker, mode, fs::path(sourceFile).filename().string(), targetOs);
        codegen.generate(program);
        if (!codegen.emitObjectFile(objPath, codegenError)) {
            std::cerr << "error: " << codegenError << "\n";
            return 1;
        }
    }

    if (compileOnly) {
        std::cout << "Compilation successful: " << objPath << "\n";
        return 0;
    }

    std::string osSuffix = targetOs == "linux" ? "" : "_" + targetOs;
    fs::path runtimeLib = findRuntimeLib(exeDir, "src/backend/llvm/runtime/libagn_llvm_runtime" + osSuffix + ".a",
                                          "libagn_llvm_runtime" + osSuffix + ".a");
    fs::path memoryLib = findRuntimeLib(exeDir, "src/memory/" + memMode + "/libagn_memory_" + memMode + osSuffix + ".a",
                                         "libagn_memory_" + memMode + osSuffix + ".a");
    fs::path manualLib = findRuntimeLib(exeDir, "src/memory/manual/libagn_memory_manual" + osSuffix + ".a",
                                         "libagn_memory_manual" + osSuffix + ".a");
    fs::path platformLib = findRuntimeLib(exeDir, "src/platform/" + targetOs + "/libagn_platform_" + targetOs + ".a",
                                           "libagn_platform_" + targetOs + ".a");

    if (!fs::exists(platformLib)) {
        std::cerr << "error: the " << targetOs << " runtime was not built with this compiler"
                  << (targetOs == "windows" ? " (building it needs clang, llvm-lib, and llvm-dlltool)" : "") << "\n";
        std::remove(objPath.c_str());
        return 1;
    }

    std::vector<std::string> libs = {runtimeLib.string(), memoryLib.string()};
    if (memMode != "manual") libs.push_back(manualLib.string());
    libs.push_back(platformLib.string());

    std::string exePath = finalOutput;
    std::vector<std::string> linkArgs;
    if (targetOs == "windows") {
        if (fs::path(exePath).extension() != ".exe") exePath += ".exe";
        linkArgs = {"lld-link", "-nologo", "-subsystem:console", "-entry:agn_start", "-nodefaultlib",
                    "-out:" + exePath, objPath};
        linkArgs.insert(linkArgs.end(), libs.begin(), libs.end());
    } else {
        linkArgs = {"cc", "-nostdlib", "-static", "-no-pie", "-e", "_start", "-o", exePath, objPath, "-Wl,--start-group"};
        linkArgs.insert(linkArgs.end(), libs.begin(), libs.end());
        linkArgs.push_back("-Wl,--end-group");
    }
    if (!runTool(linkArgs)) {
        std::cerr << "error: linking failed (object file kept at " << objPath << ")\n";
        return 1;
    }

    std::remove(objPath.c_str());
    std::cout << "Compilation successful: " << exePath << "\n";
    return 0;
}
