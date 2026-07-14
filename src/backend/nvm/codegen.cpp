// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "backend/nvm/codegen.hpp"
#include "backend/nvm/opcodes.hpp"

namespace agn::backend::nvm {

bool NVMCodeGen::hasReturnOrExit(const std::vector<ast::Statement>& stmts) {
    for (auto& stmt : stmts) {
        if (std::get_if<ast::ReturnStmt>(&stmt.node)) return true;
        if (auto* n = std::get_if<ast::InlineAsmStmt>(&stmt.node)) {
            for (auto& part : n->parts) {
                if (part.kind == ast::AsmPart::Kind::Literal && part.text.find("syscall") != std::string::npos &&
                    part.text.find("exit") != std::string::npos) {
                    return true;
                }
            }
        }
        if (auto* n = std::get_if<ast::IfStmt>(&stmt.node)) {
            if (hasReturnOrExit(n->thenBody)) return true;
            if (n->elseBody && hasReturnOrExit(*n->elseBody)) return true;
        }
        if (auto* n = std::get_if<ast::ForStmt>(&stmt.node)) {
            if (hasReturnOrExit(n->body)) return true;
        }
    }
    return false;
}

void NVMCodeGen::emitByte(uint8_t b) { bytecode_.push_back(b); }

void NVMCodeGen::emitPush32(int32_t value) {
    emitByte(PUSH32);
    uint32_t u = static_cast<uint32_t>(value);
    bytecode_.push_back(uint8_t(u >> 24));
    bytecode_.push_back(uint8_t(u >> 16));
    bytecode_.push_back(uint8_t(u >> 8));
    bytecode_.push_back(uint8_t(u));
}

void NVMCodeGen::pushLabelAddress(const std::string& label) {
    emitPush32(0);
    labelPatches_.push_back({uint32_t(bytecode_.size() - 4), label});
}

void NVMCodeGen::emitLabelRef(const std::string& label) {
    labelPatches_.push_back({uint32_t(bytecode_.size()), label});
    bytecode_.insert(bytecode_.end(), {0, 0, 0, 0});
}

void NVMCodeGen::addLabel(const std::string& label) { labels_[label] = uint32_t(bytecode_.size()); }

std::string NVMCodeGen::generateLabel(const std::string& prefix) {
    return prefix + "_" + currentFunction_ + "_" + std::to_string(labelCounter_++);
}

void NVMCodeGen::patchLabels() {
    for (auto& [pos, label] : labelPatches_) {
        auto it = labels_.find(label);
        if (it == labels_.end()) continue;
        uint32_t target = it->second;
        bytecode_[pos] = uint8_t(target >> 24);
        bytecode_[pos + 1] = uint8_t(target >> 16);
        bytecode_[pos + 2] = uint8_t(target >> 8);
        bytecode_[pos + 3] = uint8_t(target);
    }
}

int NVMCodeGen::fieldOffset(const std::string& structName, const std::string& field) {
    auto it = checker_.structs().find(structName);
    if (it == checker_.structs().end()) return -1;
    for (size_t i = 0; i < it->second.size(); i++) {
        if (it->second[i].first == field) return int(i) * 4;
    }
    return -1;
}

void NVMCodeGen::emitReservedBlocks() {
    for (auto& [label, size] : reservedBlocks_) {
        addLabel(label);
        for (size_t i = 0; i < size; i++) emitByte(0);
    }
}

void NVMCodeGen::emitStringLiterals() {
    for (auto& [label, content] : stringLiterals_) {
        addLabel(label);
        for (unsigned char ch : content) emitByte(ch);
        emitByte(0);
    }
}

std::vector<uint8_t> NVMCodeGen::generate(ast::Program& program) {
    bytecode_.insert(bytecode_.end(), {'N', 'V', 'M', '0'});

    ast::Function* mainFn = nullptr;
    for (auto& f : program.functions) if (f.name == "main") mainFn = &f;
    if (mainFn) generateFunction(*mainFn, "main");

    for (auto& f : program.functions) {
        if (&f == mainFn) continue;
        std::string name = f.receiver ? (f.receiver->type + "_" + f.name) : f.name;
        generateFunction(f, name);
    }

    for (auto& [modName, mod] : program.modules) {
        if (modName == "stdio") continue;
        for (auto& f : mod.functions) {
            if (f.isExported) generateFunction(f, modName + "_" + f.name);
        }
    }

    if (program.modules.count("stdio")) generatePrintIntHelper();

    emitStringLiterals();
    emitReservedBlocks();
    patchLabels();
    return bytecode_;
}

void NVMCodeGen::generateFunction(ast::Function& func, const std::string& fullName) {
    currentFunction_ = fullName;
    localVars_.clear();
    compileTimeStrings_.clear();
    nextLocal_ = 0;

    addLabel("func_" + fullName);

    if (func.receiver) {
        localVars_[func.receiver->name] = nextLocal_++;
    }
    for (auto& p : func.params) localVars_[p.name] = nextLocal_++;

    for (auto& stmt : func.body) generateStatement(stmt);

    if (func.name == "main" && !hasReturnOrExit(func.body)) {
        emitPush32(0);
        emitByte(SYSCALL);
        emitByte(SYSCALL_EXIT);
    }
    emitByte(RET);
}

void NVMCodeGen::generateStatement(ast::Statement& stmt) {
    if (auto* n = std::get_if<ast::VarDeclStmt>(&stmt.node)) {
        uint8_t slot = nextLocal_++;
        localVars_[n->name] = slot;
        if (n->value) {
            if (auto* s = std::get_if<ast::StringExpr>(&n->value->node)) compileTimeStrings_[n->name] = s->value;
            if (auto* sl = std::get_if<ast::StructLiteralExpr>(&n->value->node)) {
                localStructType_[n->name] = sl->structName;
                generateStructLiteralInto(*sl, slot);
                return;
            }
            generateExpression(*n->value);
        } else {
            emitPush32(0);
        }
        emitByte(STORE);
        emitByte(slot);
        return;
    }
    if (auto* n = std::get_if<ast::AssignmentStmt>(&stmt.node)) {
        generateExpression(n->value);
        auto it = localVars_.find(n->name);
        if (it != localVars_.end()) {
            emitByte(STORE);
            emitByte(it->second);
        }
        return;
    }
    if (auto* n = std::get_if<ast::IfStmt>(&stmt.node)) {
        generateExpression(n->condition);
        std::string elseLbl = generateLabel("else");
        std::string endLbl = generateLabel("endif");
        emitByte(JZ32);
        emitLabelRef(elseLbl);
        for (auto& s : n->thenBody) generateStatement(s);
        emitByte(JMP32);
        emitLabelRef(endLbl);
        addLabel(elseLbl);
        if (n->elseBody) for (auto& s : *n->elseBody) generateStatement(s);
        addLabel(endLbl);
        return;
    }
    if (auto* n = std::get_if<ast::ForStmt>(&stmt.node)) {
        std::string startLbl = generateLabel("for_start");
        std::string endLbl = generateLabel("for_end");
        addLabel(startLbl);
        if (n->condition) {
            generateExpression(*n->condition);
            emitByte(JZ32);
            emitLabelRef(endLbl);
        }
        for (auto& s : n->body) generateStatement(s);
        emitByte(JMP32);
        emitLabelRef(startLbl);
        addLabel(endLbl);
        return;
    }
    if (auto* n = std::get_if<ast::ReturnStmt>(&stmt.node)) {
        if (n->value) generateExpression(*n->value);
        emitByte(RET);
        return;
    }
    if (auto* n = std::get_if<ast::ExpressionStmt>(&stmt.node)) {
        generateExpression(n->expr);
        emitByte(POP);
        return;
    }
    if (auto* n = std::get_if<ast::PointerAssignmentStmt>(&stmt.node)) {
        generateExpression(n->target);
        generateExpression(n->value);
        emitByte(STORE_ABS);
        return;
    }
    if (auto* n = std::get_if<ast::InlineAsmStmt>(&stmt.node)) {
        std::string asmText;
        for (auto& part : n->parts) {
            if (part.kind == ast::AsmPart::Kind::Literal) {
                asmText += part.text;
            } else {
                auto it = compileTimeStrings_.find(part.text);
                if (it != compileTimeStrings_.end()) {
                    asmText += it->second;
                    asmText += '\n';
                } else {
                    auto lv = localVars_.find(part.text);
                    if (lv != localVars_.end()) asmText += "load " + std::to_string(lv->second) + "\n";
                }
            }
        }
        size_t pos = 0;
        while (pos < asmText.size()) {
            size_t nl = asmText.find('\n', pos);
            if (nl == std::string::npos) nl = asmText.size();
            std::string line = asmText.substr(pos, nl - pos);
            size_t start = line.find_first_not_of(" \t");
            if (start != std::string::npos) {
                std::string trimmed = line.substr(start);
                if (!trimmed.empty() && trimmed[0] != ';') {
                    size_t comment = trimmed.find(';');
                    if (comment != std::string::npos) trimmed = trimmed.substr(0, comment);
                    if (!trimmed.empty()) emitAsmInstruction(trimmed);
                }
            }
            pos = nl + 1;
        }
        return;
    }
    if (auto* n = std::get_if<ast::ComptimeStmt>(&stmt.node)) {
        for (auto& s : n->body) generateStatement(s);
        return;
    }
    if (auto* n = std::get_if<ast::FieldAssignmentStmt>(&stmt.node)) {
        auto* id = std::get_if<ast::IdentifierExpr>(&n->object.node);
        if (!id) return;
        auto slotIt = localVars_.find(id->name);
        auto typeIt = localStructType_.find(id->name);
        if (slotIt == localVars_.end() || typeIt == localStructType_.end()) return;
        int offset = fieldOffset(typeIt->second, n->field);
        if (offset < 0) return;

        emitByte(LOAD);
        emitByte(slotIt->second);
        if (offset != 0) { emitPush32(offset); emitByte(ADD); }
        generateExpression(n->value);
        emitByte(STORE_ABS);
        return;
    }
}

void NVMCodeGen::generateMethodCall(const std::string& object, const std::string& member,
                                    std::vector<ast::Expression>& args) {
    if (object == "stdio") {
        if (member == "Print" || member == "Println") {
            if (!args.empty()) {
                if (auto* s = std::get_if<ast::StringExpr>(&args[0].node)) {
                    for (unsigned char ch : s->value) {
                        emitPush32(ch);
                        emitByte(SYSCALL);
                        emitByte(SYSCALL_PRINT);
                    }
                    if (member == "Println") {
                        emitPush32('\n');
                        emitByte(SYSCALL);
                        emitByte(SYSCALL_PRINT);
                    }
                    emitPush32(0);
                    return;
                }
                generateExpression(args[0]);
                emitByte(CALL32);
                emitLabelRef("__print_int");
                if (member == "Println") {
                    emitPush32('\n');
                    emitByte(SYSCALL);
                    emitByte(SYSCALL_PRINT);
                }
                emitPush32(0);
                return;
            }
        }
    }

    auto it = localVars_.find(object);
    if (it != localVars_.end()) {
        for (auto argIt = args.rbegin(); argIt != args.rend(); ++argIt) generateExpression(*argIt);
        emitByte(LOAD);
        emitByte(it->second);
        emitByte(CALL32);
        emitLabelRef("func_" + object + "_" + member);
        return;
    }

    for (auto& arg : args) generateExpression(arg);
    emitByte(CALL32);
    emitLabelRef("func_" + object + "_" + member);
}

void NVMCodeGen::generateStructLiteralInto(ast::StructLiteralExpr& lit, uint8_t destSlot) {
    auto structIt = checker_.structs().find(lit.structName);
    size_t numFields = structIt != checker_.structs().end() ? structIt->second.size() : lit.fields.size();
    std::string blockLabel = generateLabel("struct_" + lit.structName);
    reservedBlocks_.push_back({blockLabel, numFields * 4});

    for (auto& [fieldName, fieldExpr] : lit.fields) {
        int offset = fieldOffset(lit.structName, fieldName);
        if (offset < 0) continue;
        pushLabelAddress(blockLabel);
        if (offset != 0) { emitPush32(offset); emitByte(ADD); }
        generateExpression(fieldExpr);
        emitByte(STORE_ABS);
    }

    pushLabelAddress(blockLabel);
    emitByte(STORE);
    emitByte(destSlot);
}

void NVMCodeGen::generateExpression(ast::Expression& expr) {
    if (auto* n = std::get_if<ast::NumberExpr>(&expr.node)) {
        emitPush32(int32_t(n->value));
        return;
    }
    if (auto* n = std::get_if<ast::StringExpr>(&expr.node)) {
        std::string label = generateLabel("str");
        stringLiterals_.push_back({label, n->value});
        pushLabelAddress(label);
        return;
    }
    if (auto* n = std::get_if<ast::IdentifierExpr>(&expr.node)) {
        auto it = localVars_.find(n->name);
        if (it != localVars_.end()) {
            emitByte(LOAD);
            emitByte(it->second);
        } else {
            emitPush32(0);
        }
        return;
    }
    if (auto* n = std::get_if<ast::BinaryExpr>(&expr.node)) {
        generateExpression(*n->left);
        generateExpression(*n->right);
        switch (n->op) {
            case ast::BinaryOp::Add: emitByte(ADD); break;
            case ast::BinaryOp::Sub: emitByte(SUB); break;
            case ast::BinaryOp::Mul: emitByte(MUL); break;
            case ast::BinaryOp::Div: emitByte(DIV); break;
            case ast::BinaryOp::Mod: emitByte(MOD); break;
            case ast::BinaryOp::Equal: emitByte(EQ); break;
            case ast::BinaryOp::NotEqual: emitByte(NEQ); break;
            case ast::BinaryOp::Less: emitByte(LT); break;
            case ast::BinaryOp::Greater: emitByte(GT); break;
            case ast::BinaryOp::LessEqual: emitByte(GT); emitPush32(0); emitByte(EQ); break;
            case ast::BinaryOp::GreaterEqual: emitByte(LT); emitPush32(0); emitByte(EQ); break;
            default: break;
        }
        return;
    }
    if (auto* n = std::get_if<ast::UnaryExpr>(&expr.node)) {
        generateExpression(*n->operand);
        if (n->op == ast::UnaryOp::Neg) {
            emitPush32(0);
            emitByte(SWAP);
            emitByte(SUB);
        } else {
            emitPush32(0);
            emitByte(EQ);
        }
        return;
    }
    if (auto* n = std::get_if<ast::CallExpr>(&expr.node)) {
        for (auto argIt = n->args.rbegin(); argIt != n->args.rend(); ++argIt) generateExpression(*argIt);
        emitByte(CALL32);
        emitLabelRef("func_" + n->function);
        return;
    }
    if (auto* n = std::get_if<ast::MethodCallExpr>(&expr.node)) {
        generateMethodCall(n->object, n->member, n->args);
        return;
    }
    if (auto* n = std::get_if<ast::DerefExpr>(&expr.node)) {
        generateExpression(*n->operand);
        emitByte(LOAD_ABS);
        return;
    }
    if (auto* n = std::get_if<ast::EvalExpr>(&expr.node)) {
        if (auto* s = std::get_if<ast::StringExpr>(&n->instruction->node)) {
            emitAsmInstruction(s->value);
        }
        return;
    }
    if (auto* n = std::get_if<ast::FieldAccessExpr>(&expr.node)) {
        auto* id = std::get_if<ast::IdentifierExpr>(&n->object->node);
        auto slotIt = id ? localVars_.find(id->name) : localVars_.end();
        auto typeIt = id ? localStructType_.find(id->name) : localStructType_.end();
        if (id && slotIt != localVars_.end() && typeIt != localStructType_.end()) {
            int offset = fieldOffset(typeIt->second, n->field);
            if (offset >= 0) {
                emitByte(LOAD);
                emitByte(slotIt->second);
                if (offset != 0) { emitPush32(offset); emitByte(ADD); }
                emitByte(LOAD_ABS);
                return;
            }
        }
        emitPush32(0);
        return;
    }
    // AddressOf, ArrayAccess/StringIndex, FunctionLiteral: not supported by
    // this bounded NVM port (see backend/nvm/codegen.hpp notes on closures).
    emitPush32(0);
}

void NVMCodeGen::emitAsmInstruction(const std::string& line) {
    size_t start = line.find_first_not_of(' ');
    if (start == std::string::npos) return;
    size_t end = line.find(' ', start);
    std::string instr = line.substr(start, end == std::string::npos ? std::string::npos : end - start);
    for (auto& c : instr) c = char(std::tolower(static_cast<unsigned char>(c)));

    std::string rest = end == std::string::npos ? "" : line.substr(end + 1);
    size_t restStart = rest.find_first_not_of(' ');
    if (restStart != std::string::npos) rest = rest.substr(restStart);
    else rest.clear();

    if (instr == "push32" || instr == "push") {
        if (!rest.empty()) emitPush32(std::atoi(rest.c_str()));
    } else if (instr == "pop") emitByte(POP);
    else if (instr == "add") emitByte(ADD);
    else if (instr == "sub") emitByte(SUB);
    else if (instr == "mul") emitByte(MUL);
    else if (instr == "div") emitByte(DIV);
    else if (instr == "mod") emitByte(MOD);
    else if (instr == "ret") emitByte(RET);
    else if (instr == "syscall") {
        emitByte(SYSCALL);
        if (!rest.empty()) {
            for (auto& c : rest) c = char(std::tolower(static_cast<unsigned char>(c)));
            uint8_t num = 0;
            if (rest == "exit") num = SYSCALL_EXIT;
            else if (rest == "exec") num = SYSCALL_EXEC;
            else if (rest == "read") num = SYSCALL_READ;
            else if (rest == "write") num = SYSCALL_WRITE;
            else if (rest == "create") num = SYSCALL_CREATE;
            else if (rest == "delete") num = SYSCALL_DELETE;
            else if (rest == "print") num = SYSCALL_PRINT;
            else num = uint8_t(std::atoi(rest.c_str()));
            emitByte(num);
        } else {
            emitByte(0);
        }
    }
}

void NVMCodeGen::generatePrintIntHelper() {
    addLabel("__print_int");
    emitByte(STORE); emitByte(255);
    emitByte(STORE); emitByte(250);
    emitByte(LOAD); emitByte(250);
    emitPush32(0);
    emitByte(LT);

    std::string notNeg = generateLabel("not_negative");
    emitByte(JZ32);
    emitLabelRef(notNeg);
    emitPush32('-');
    emitByte(SYSCALL);
    emitByte(SYSCALL_PRINT);
    emitByte(LOAD); emitByte(250);
    emitPush32(0);
    emitByte(SWAP);
    emitByte(SUB);
    emitByte(STORE); emitByte(250);
    addLabel(notNeg);

    emitByte(LOAD); emitByte(250);
    emitPush32(0);
    emitByte(EQ);
    std::string notZero = generateLabel("not_zero");
    emitByte(JZ32);
    emitLabelRef(notZero);
    emitPush32('0');
    emitByte(SYSCALL);
    emitByte(SYSCALL_PRINT);
    emitByte(LOAD); emitByte(255);
    emitByte(RET);
    addLabel(notZero);

    emitPush32(1);
    emitByte(STORE); emitByte(251);
    std::string findPower = generateLabel("find_power");
    std::string findPowerDone = generateLabel("find_power_done");
    addLabel(findPower);
    emitByte(LOAD); emitByte(251);
    emitPush32(10);
    emitByte(MUL);
    emitByte(LOAD); emitByte(250);
    emitByte(GT);
    emitByte(JNZ32);
    emitLabelRef(findPowerDone);
    emitByte(LOAD); emitByte(251);
    emitPush32(10);
    emitByte(MUL);
    emitByte(STORE); emitByte(251);
    emitByte(JMP32);
    emitLabelRef(findPower);
    addLabel(findPowerDone);

    std::string printLoop = generateLabel("print_digit_loop");
    std::string printDone = generateLabel("print_done");
    addLabel(printLoop);
    emitByte(LOAD); emitByte(251);
    emitPush32(0);
    emitByte(GT);
    emitByte(JZ32);
    emitLabelRef(printDone);
    emitByte(LOAD); emitByte(250);
    emitByte(LOAD); emitByte(251);
    emitByte(DIV);
    emitPush32('0');
    emitByte(ADD);
    emitByte(SYSCALL);
    emitByte(SYSCALL_PRINT);
    emitByte(LOAD); emitByte(250);
    emitByte(LOAD); emitByte(251);
    emitByte(MOD);
    emitByte(STORE); emitByte(250);
    emitByte(LOAD); emitByte(251);
    emitPush32(10);
    emitByte(DIV);
    emitByte(STORE); emitByte(251);
    emitByte(JMP32);
    emitLabelRef(printLoop);
    addLabel(printDone);

    emitByte(LOAD); emitByte(255);
    emitByte(RET);
}

} // namespace agn::backend::nvm
