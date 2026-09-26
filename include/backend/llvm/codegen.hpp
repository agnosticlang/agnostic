// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "ast/ast.hpp"
#include "parser/typechecker.hpp"

#include <memory>
#include <string>

namespace agn::backend::llvm_backend {

enum class MemMode { Arc, Manual, Orc };

class Codegen {
public:
    Codegen(agn::parser::TypeChecker& checker, MemMode mode, const std::string& moduleName);
    ~Codegen();

    bool generate(agn::ast::Program& program, std::string& errorOut);
    bool emitObjectFile(const std::string& path, std::string& errorOut);
    void dumpIR() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace agn::backend::llvm_backend
