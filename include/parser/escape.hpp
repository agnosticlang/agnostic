// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "parser/typechecker.hpp"

#include <vector>

namespace agn::parser {

bool canUseOwnRegion(const TypeChecker& checker, const std::vector<Type>& params, const Type& returnType,
                     const std::vector<Type>& captures);

} // namespace agn::parser
