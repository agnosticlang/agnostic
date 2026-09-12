// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "ast/ast.hpp"

#include <string>

namespace agn::parser {

void monomorphizeGenerics(ast::Program& program, const std::string& targetOs, const std::string& targetArch,
                           const std::string& memMode);

} // namespace agn::parser
