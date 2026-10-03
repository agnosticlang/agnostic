// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "ast/ast.hpp"
#include "parser/typechecker.hpp"

#include <string>
#include <vector>

namespace agn::parser {

std::vector<TypeError> monomorphizeGenerics(ast::Program& program, const std::string& targetOs,
                                            const std::string& targetArch, const std::string& memMode);

} // namespace agn::parser
