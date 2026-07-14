// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "backend/gcc/backend.hpp"

namespace agn::backend::gcc {

bool GccBackend::generate(agn::ast::Program&, const std::string&, std::string& errorOut) {
    errorOut = "the gcc/libgccjit backend is not implemented yet";
    return false;
}

} // namespace agn::backend::gcc
