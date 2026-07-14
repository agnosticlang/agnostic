// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "ast/ast.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace agn::backend::nvm {

class NVMAssemblyGenerator {
public:
    std::string generate(agn::ast::Program& program);

private:
    bool hasReturnOrExit(const std::vector<agn::ast::Statement>& stmts);
    void generateFunction(agn::ast::Function& func, const std::string& fullName);
    void generateStatement(agn::ast::Statement& stmt);
    void generateExpression(agn::ast::Expression& expr);
    std::string generateLabel(const std::string& prefix);
    void emitVgaChar(uint8_t ch, uint8_t attr);
    void emitVgaNewline();

    std::string output_;
    uint32_t labelCounter_ = 0;
    std::unordered_map<std::string, uint8_t> localVars_;
    uint8_t nextLocal_ = 0;
    std::string currentFunction_;
    uint32_t vgaCursor_ = 0xB8000 + (18 * 160);
};

} // namespace agn::backend::nvm
