// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "ast/ast.hpp"

#include <string>

namespace agn::backend::gcc {

// TODO: libgccjit-based codegen. Not implemented yet.
class GccBackend {
public:
    bool generate(agn::ast::Program& program, const std::string& outputPath, std::string& errorOut);
};

} // namespace agn::backend::gcc
