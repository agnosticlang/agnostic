// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "ast/ast.hpp"
#include "parser/typechecker.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace agn::backend::nvm {

// closures unsupported here: no interpreter source to define the calling convention against
class NVMCodeGen {
public:
    explicit NVMCodeGen(agn::parser::TypeChecker& checker) : checker_(checker) {}
    std::vector<uint8_t> generate(agn::ast::Program& program);

private:
    bool hasReturnOrExit(const std::vector<agn::ast::Statement>& stmts);
    void generateFunction(agn::ast::Function& func, const std::string& fullName);
    void generateStatement(agn::ast::Statement& stmt);
    void generateExpression(agn::ast::Expression& expr);
    void generateMethodCall(const std::string& object, const std::string& member, std::vector<agn::ast::Expression>& args);
    void generateStructLiteralInto(agn::ast::StructLiteralExpr& lit, uint8_t destSlot);
    int fieldOffset(const std::string& structName, const std::string& field);

    void emitByte(uint8_t b);
    void emitPush32(int32_t value);
    void pushLabelAddress(const std::string& label);
    void emitLabelRef(const std::string& label);
    void addLabel(const std::string& label);
    std::string generateLabel(const std::string& prefix);
    void patchLabels();
    void emitStringLiterals();
    void emitReservedBlocks();
    void emitAsmInstruction(const std::string& line);
    void generatePrintIntHelper();

    agn::parser::TypeChecker& checker_;
    std::vector<uint8_t> bytecode_;
    std::unordered_map<std::string, uint32_t> labels_;
    std::vector<std::pair<uint32_t, std::string>> labelPatches_;
    std::unordered_map<std::string, uint8_t> localVars_;
    std::unordered_map<std::string, std::string> localStructType_;
    std::vector<std::pair<std::string, size_t>> reservedBlocks_;
    uint8_t nextLocal_ = 0;
    std::string currentFunction_;
    std::vector<std::pair<std::string, std::string>> stringLiterals_;
    std::unordered_map<std::string, std::string> compileTimeStrings_;
    int labelCounter_ = 0;
};

} // namespace agn::backend::nvm
