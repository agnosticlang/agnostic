// SPDX-License-Identifier: Apache-2.0
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

    void generate(agn::ast::Program& program);
    bool emitObjectFile(const std::string& path, std::string& errorOut);
    void dumpIR() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace agn::backend::llvm_backend
