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

class NVMCodeGen {
public:
    explicit NVMCodeGen(agn::parser::TypeChecker& checker) : checker_(checker) {}
    std::vector<uint8_t> generate(agn::ast::Program& program);

private:
    struct VarInfo {
        bool isArg;
        uint8_t index;
        std::string structType;
        bool isArray = false;
    };

    bool hasReturnOrExit(const std::vector<agn::ast::Statement>& stmts);
    void generateFunction(agn::ast::Function& func, const std::string& fullName, bool isMain);
    void generateStatement(agn::ast::Statement& stmt);
    void generateExpression(agn::ast::Expression& expr);
    void generateStdioCall(const std::string& member, std::vector<agn::ast::Expression>& args);
    void generateNovariaCall(const std::string& member, std::vector<agn::ast::Expression>& args);
    void generateNovariaRemove(agn::ast::Expression& filenameExpr);
    void generateMethodCall(const std::string& object, const std::string& member, std::vector<agn::ast::Expression>& args);
    void generateCall(const std::string& name, std::vector<agn::ast::Expression>& args);
    void generateStructLiteral(agn::ast::StructLiteralExpr& lit);
    void materializeString(const std::string& text);
    void writeExprAsString(agn::ast::Expression& expr);
    void writeHeapString(const std::string& text);
    void storeHeapI32();
    int fieldOffset(const std::string& structName, const std::string& field);
    void emitHeapAllocHelper();
    void emitPrintIntHelper();
    void emitAsmInstruction(const std::string& line);

    void emitByte(uint8_t b);
    void emitPush(int32_t value);
    void emitJumpRef(const std::string& label);
    void addLabel(const std::string& label);
    std::string generateLabel(const std::string& prefix);
    void patchJumps();

    agn::parser::TypeChecker& checker_;
    std::vector<uint8_t> bytecode_;
    std::unordered_map<std::string, uint32_t> labels_;
    std::vector<std::pair<uint32_t, std::string>> jumpPatches_;

    std::unordered_map<std::string, VarInfo> vars_;
    std::unordered_map<std::string, std::string> compileTimeStrings_;
    uint8_t nextLocal_ = 0;
    size_t enterOperandPos_ = 0;
    bool inMain_ = false;
    std::string currentFunction_;
    int labelCounter_ = 0;
    bool needsHeapAlloc_ = false;
    bool needsPrintInt_ = false;
};

} // namespace agn::backend::nvm
