// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "ast/ast.hpp"
#include "parser/typechecker.hpp"

#include <memory>
#include <string>

namespace agn::backend::gcc {

enum class MemMode { Arc, Manual, Orc };

class GccBackend {
public:
    GccBackend(agn::parser::TypeChecker& checker, MemMode mode, const std::string& moduleName);
    ~GccBackend();

    bool generate(agn::ast::Program& program, std::string& errorOut);
    bool emitObjectFile(const std::string& path, std::string& errorOut);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace agn::backend::gcc
