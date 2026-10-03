// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "parser/monomorphize.hpp"
#include "parser/comptime_eval.hpp"

#include <cctype>
#include <unordered_map>
#include <unordered_set>

namespace agn::parser {

namespace {

using Subst = std::unordered_map<std::string, std::string>;
using ValueSubst = std::unordered_map<std::string, ComptimeValue>;

struct GenSubst {
    const Subst* types = nullptr;
    const ValueSubst* values = nullptr;
};

struct GenericArg {
    bool isType = false;
    std::string typeName;
    ComptimeValue value;
};

struct SourcePos {
    std::string location;
    size_t line = 0;
    size_t column = 0;
};

struct MonoState {
    std::unordered_map<std::string, ast::StructDecl> templates;
    std::unordered_set<std::string> registered;
    std::unordered_set<std::string> inProgress;
    std::vector<ast::StructDecl> newStructs;

    std::unordered_map<std::string, ast::Function> functionTemplates;
    std::unordered_set<std::string> registeredFns;
    std::unordered_set<std::string> inProgressFns;
    std::vector<ast::Function> newFunctions;

    ast::Program* program = nullptr;
    std::string targetOs;
    std::string targetArch;
    std::string memMode;

    SourcePos pos;
    std::vector<TypeError> errors;
};

void addError(MonoState& st, std::string message) {
    st.errors.push_back(TypeError{std::move(message), st.pos.location, st.pos.line, st.pos.column});
}

std::string rewriteTypeString(const std::string& s, MonoState& st, const Subst* subst);
void rewriteStmt(ast::Statement& s, MonoState& st, const GenSubst& subst);
void rewriteExpr(ast::Expression& e, MonoState& st, const GenSubst& subst);
std::string instantiateFunction(const std::string& name, const std::vector<GenericArg>& args, MonoState& st);

ast::Expression cloneExpr(const ast::Expression& e);
ast::Statement cloneStmt(const ast::Statement& s);

std::vector<ast::Expression> cloneExprs(const std::vector<ast::Expression>& v) {
    std::vector<ast::Expression> out;
    out.reserve(v.size());
    for (auto& e : v) out.push_back(cloneExpr(e));
    return out;
}

std::vector<ast::Statement> cloneStmts(const std::vector<ast::Statement>& v) {
    std::vector<ast::Statement> out;
    out.reserve(v.size());
    for (auto& s : v) out.push_back(cloneStmt(s));
    return out;
}

ast::Expression cloneExpr(const ast::Expression& e) {
    ast::Expression out;
    std::visit(
        [&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::NumberExpr> || std::is_same_v<T, ast::FloatExpr> ||
                          std::is_same_v<T, ast::BoolExpr> || std::is_same_v<T, ast::StringExpr> ||
                          std::is_same_v<T, ast::IdentifierExpr>) {
                out.node = node;
            } else if constexpr (std::is_same_v<T, ast::TemplateStringExpr>) {
                ast::TemplateStringExpr copy;
                for (auto& part : node.parts) {
                    if (auto* lit = std::get_if<ast::TemplateLiteralPart>(&part)) {
                        copy.parts.push_back(*lit);
                    } else {
                        auto* ep = std::get_if<ast::TemplateExprPart>(&part);
                        ast::TemplateExprPart newPart;
                        newPart.expr = std::make_unique<ast::Expression>(cloneExpr(*ep->expr));
                        newPart.format = ep->format;
                        copy.parts.push_back(std::move(newPart));
                    }
                }
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
                ast::BinaryExpr copy;
                copy.op = node.op;
                copy.left = std::make_unique<ast::Expression>(cloneExpr(*node.left));
                copy.right = std::make_unique<ast::Expression>(cloneExpr(*node.right));
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
                ast::UnaryExpr copy;
                copy.op = node.op;
                copy.operand = std::make_unique<ast::Expression>(cloneExpr(*node.operand));
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
                ast::CallExpr copy;
                copy.function = node.function;
                copy.args = cloneExprs(node.args);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
                ast::MethodCallExpr copy;
                copy.object = node.object;
                copy.member = node.member;
                copy.args = cloneExprs(node.args);
                copy.kind = node.kind;
                copy.resolvedStructName = node.resolvedStructName;
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::ArrayAccessExpr>) {
                ast::ArrayAccessExpr copy;
                copy.name = node.name;
                copy.index = std::make_unique<ast::Expression>(cloneExpr(*node.index));
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::StringIndexExpr>) {
                ast::StringIndexExpr copy;
                copy.str = std::make_unique<ast::Expression>(cloneExpr(*node.str));
                copy.index = std::make_unique<ast::Expression>(cloneExpr(*node.index));
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::AddressOfExpr>) {
                ast::AddressOfExpr copy;
                copy.operand = std::make_unique<ast::Expression>(cloneExpr(*node.operand));
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
                ast::DerefExpr copy;
                copy.operand = std::make_unique<ast::Expression>(cloneExpr(*node.operand));
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::CastExpr>) {
                ast::CastExpr copy;
                copy.operand = std::make_unique<ast::Expression>(cloneExpr(*node.operand));
                copy.targetType = node.targetType;
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
                ast::FieldAccessExpr copy;
                copy.object = std::make_unique<ast::Expression>(cloneExpr(*node.object));
                copy.field = node.field;
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::FunctionLiteralExpr>) {
                ast::FunctionLiteralExpr copy;
                copy.params = node.params;
                copy.returnType = node.returnType;
                copy.body = cloneStmts(node.body);
                copy.capturedVars = node.capturedVars;
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::StructLiteralExpr>) {
                ast::StructLiteralExpr copy;
                copy.structName = node.structName;
                for (auto& [fname, fexpr] : node.fields) copy.fields.emplace_back(fname, cloneExpr(fexpr));
                out.node = std::move(copy);
            }
        },
        e.node);
    return out;
}

ast::Statement cloneStmt(const ast::Statement& s) {
    ast::Statement out;
    out.line = s.line;
    out.column = s.column;
    std::visit(
        [&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::VarDeclStmt>) {
                ast::VarDeclStmt copy;
                copy.name = node.name;
                copy.varType = node.varType;
                if (node.value) copy.value = cloneExpr(*node.value);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::ArrayDeclStmt>) {
                out.node = node;
            } else if constexpr (std::is_same_v<T, ast::AssignmentStmt>) {
                ast::AssignmentStmt copy;
                copy.name = node.name;
                copy.value = cloneExpr(node.value);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::ArrayAssignmentStmt>) {
                ast::ArrayAssignmentStmt copy;
                copy.name = node.name;
                copy.index = cloneExpr(node.index);
                copy.value = cloneExpr(node.value);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::PointerAssignmentStmt>) {
                ast::PointerAssignmentStmt copy;
                copy.target = cloneExpr(node.target);
                copy.value = cloneExpr(node.value);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::FieldAssignmentStmt>) {
                ast::FieldAssignmentStmt copy;
                copy.object = cloneExpr(node.object);
                copy.field = node.field;
                copy.value = cloneExpr(node.value);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::IfStmt>) {
                ast::IfStmt copy;
                copy.condition = cloneExpr(node.condition);
                copy.thenBody = cloneStmts(node.thenBody);
                if (node.elseBody) copy.elseBody = cloneStmts(*node.elseBody);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::ForStmt>) {
                ast::ForStmt copy;
                if (node.condition) copy.condition = cloneExpr(*node.condition);
                copy.body = cloneStmts(node.body);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::ReturnStmt>) {
                ast::ReturnStmt copy;
                if (node.value) copy.value = cloneExpr(*node.value);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::ExpressionStmt>) {
                ast::ExpressionStmt copy;
                copy.expr = cloneExpr(node.expr);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::ComptimeStmt>) {
                ast::ComptimeStmt copy;
                copy.body = cloneStmts(node.body);
                out.node = std::move(copy);
            } else if constexpr (std::is_same_v<T, ast::BreakStmt> || std::is_same_v<T, ast::ContinueStmt>) {
                out.node = node;
            }
        },
        s.node);
    return out;
}

std::string mangle(const std::string& name, const std::vector<std::string>& args) {
    std::string m = "__gen_" + name;
    for (auto& a : args) m += "_" + a;
    return m;
}

std::string argKey(const GenericArg& a) {
    if (a.isType) return a.typeName;
    switch (a.value.kind) {
        case ComptimeValue::Kind::I64: return std::to_string(a.value.i);
        case ComptimeValue::Kind::F64: return std::to_string(a.value.f);
        case ComptimeValue::Kind::Bool: return a.value.b ? "true" : "false";
        case ComptimeValue::Kind::String: return a.value.s;
        case ComptimeValue::Kind::Void: return "void";
    }
    return "";
}

std::string instantiate(const std::string& name, const std::vector<std::string>& args, MonoState& st) {
    std::string mangled = mangle(name, args);
    if (st.registered.count(mangled)) return mangled;
    if (st.inProgress.count(mangled)) {
        addError(st, "recursive generic instantiation of '" + name + "'");
        return mangled;
    }
    auto tmplIt = st.templates.find(name);
    if (tmplIt == st.templates.end()) {
        addError(st, "'" + name + "' is not a declared generic struct");
        return mangled;
    }
    const ast::StructDecl& tmpl = tmplIt->second;
    if (tmpl.typeParams.size() != args.size()) {
        addError(st, "'" + name + "' expects " + std::to_string(tmpl.typeParams.size()) + " type argument(s), got " +
                         std::to_string(args.size()));
        return mangled;
    }

    Subst subst;
    for (size_t i = 0; i < tmpl.typeParams.size(); i++) subst[tmpl.typeParams[i]] = args[i];

    st.inProgress.insert(mangled);
    SourcePos savedPos = st.pos;
    std::vector<ast::Parameter> concreteFields;
    for (auto& f : tmpl.fields) {
        st.pos = SourcePos{tmpl.name, f.line, f.column};
        concreteFields.push_back(
            ast::Parameter{f.name, rewriteTypeString(f.type, st, &subst), false, f.line, f.column});
    }
    st.pos = savedPos;
    st.inProgress.erase(mangled);
    st.registered.insert(mangled);

    st.newStructs.push_back(ast::StructDecl{mangled, {}, std::move(concreteFields)});
    return mangled;
}

bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

std::string rewriteOne(const std::string& s, size_t& pos, MonoState& st, const Subst* subst) {
    if (pos < s.size() && s[pos] == '*') {
        pos++;
        return "*" + rewriteOne(s, pos, st, subst);
    }
    if (s.compare(pos, 5, "func(") == 0) {
        pos += 5;
        std::string out = "func(";
        bool first = true;
        while (pos < s.size() && s[pos] != ')') {
            if (!first) out += ",";
            first = false;
            out += rewriteOne(s, pos, st, subst);
            if (pos < s.size() && s[pos] == ',') pos++;
        }
        if (pos < s.size()) pos++; // ')'
        out += ")";
        if (s.compare(pos, 2, "->") == 0) {
            pos += 2;
            out += "->" + rewriteOne(s, pos, st, subst);
        }
        return out;
    }

    size_t start = pos;
    while (pos < s.size() && isIdentChar(s[pos])) pos++;
    std::string name = s.substr(start, pos - start);

    if (pos < s.size() && s[pos] == '<') {
        pos++;
        std::vector<std::string> args;
        while (pos < s.size() && s[pos] != '>') {
            args.push_back(rewriteOne(s, pos, st, subst));
            if (pos < s.size() && s[pos] == ',') pos++;
        }
        if (pos < s.size()) pos++; // '>'
        return instantiate(name, args, st);
    }

    if (subst) {
        auto it = subst->find(name);
        if (it != subst->end()) return it->second;
    }
    return name;
}

std::string rewriteTypeString(const std::string& s, MonoState& st, const Subst* subst) {
    if (s.empty()) return s;
    size_t pos = 0;
    return rewriteOne(s, pos, st, subst);
}

void rewriteStmt(ast::Statement& s, MonoState& st, const GenSubst& subst) {
    st.pos.line = s.line;
    st.pos.column = s.column;
    std::visit(
        [&](auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::VarDeclStmt>) {
                if (!node.varType.empty()) node.varType = rewriteTypeString(node.varType, st, subst.types);
                if (node.value) rewriteExpr(*node.value, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ArrayDeclStmt>) {
                node.elementType = rewriteTypeString(node.elementType, st, subst.types);
            } else if constexpr (std::is_same_v<T, ast::AssignmentStmt>) {
                rewriteExpr(node.value, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ArrayAssignmentStmt>) {
                rewriteExpr(node.index, st, subst);
                rewriteExpr(node.value, st, subst);
            } else if constexpr (std::is_same_v<T, ast::PointerAssignmentStmt>) {
                rewriteExpr(node.target, st, subst);
                rewriteExpr(node.value, st, subst);
            } else if constexpr (std::is_same_v<T, ast::FieldAssignmentStmt>) {
                rewriteExpr(node.object, st, subst);
                rewriteExpr(node.value, st, subst);
            } else if constexpr (std::is_same_v<T, ast::IfStmt>) {
                rewriteExpr(node.condition, st, subst);
                for (auto& s2 : node.thenBody) rewriteStmt(s2, st, subst);
                if (node.elseBody)
                    for (auto& s2 : *node.elseBody) rewriteStmt(s2, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ForStmt>) {
                if (node.condition) rewriteExpr(*node.condition, st, subst);
                for (auto& s2 : node.body) rewriteStmt(s2, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ReturnStmt>) {
                if (node.value) rewriteExpr(*node.value, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ExpressionStmt>) {
                rewriteExpr(node.expr, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ComptimeStmt>) {
                for (auto& s2 : node.body) rewriteStmt(s2, st, subst);
            }
        },
        s.node);
}

void rewriteExpr(ast::Expression& e, MonoState& st, const GenSubst& subst) {
    std::visit(
        [&](auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
                if (subst.values) {
                    auto it = subst.values->find(node.name);
                    if (it != subst.values->end()) {
                        const ComptimeValue& v = it->second;
                        switch (v.kind) {
                            case ComptimeValue::Kind::I64: e.node = ast::NumberExpr{v.i}; return;
                            case ComptimeValue::Kind::F64: e.node = ast::FloatExpr{v.f}; return;
                            case ComptimeValue::Kind::Bool: e.node = ast::BoolExpr{v.b}; return;
                            case ComptimeValue::Kind::String: e.node = ast::StringExpr{v.s}; return;
                            case ComptimeValue::Kind::Void: break;
                        }
                    }
                }
            } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
                rewriteExpr(*node.left, st, subst);
                rewriteExpr(*node.right, st, subst);
            } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
                rewriteExpr(*node.operand, st, subst);
            } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
                auto tmplIt = st.functionTemplates.find(node.function);
                if (tmplIt != st.functionTemplates.end()) {
                    std::vector<size_t> comptimeIndices;
                    for (size_t i = 0; i < tmplIt->second.params.size() && i < node.args.size(); i++) {
                        if (tmplIt->second.params[i].isComptime) comptimeIndices.push_back(i);
                    }
                    std::vector<GenericArg> args;
                    for (size_t idx : comptimeIndices) {
                        rewriteExpr(node.args[idx], st, subst);
                        const ast::Parameter& param = tmplIt->second.params[idx];
                        if (param.type == "type") {
                            auto* ident = std::get_if<ast::IdentifierExpr>(&node.args[idx].node);
                            if (!ident) {
                                addError(st, "argument " + std::to_string(idx) + " of '" + node.function +
                                                 "' must be a type name");
                                return;
                            }
                            std::string typeName = ident->name;
                            if (subst.types) {
                                auto it = subst.types->find(typeName);
                                if (it != subst.types->end()) typeName = it->second;
                            }
                            args.push_back(GenericArg{true, typeName, ComptimeValue{}});
                        } else {
                            ComptimeEvaluator evaluator(*st.program, st.targetOs, st.targetArch, st.memMode);
                            auto val = evaluator.eval(node.args[idx]);
                            if (!val) {
                                addError(st, "argument " + std::to_string(idx) + " of '" + node.function +
                                                 "' must be a compile-time constant: " + evaluator.lastError());
                                return;
                            }
                            args.push_back(GenericArg{false, "", *val});
                        }
                    }
                    std::string mangled = instantiateFunction(node.function, args, st);
                    for (auto it = comptimeIndices.rbegin(); it != comptimeIndices.rend(); ++it) {
                        node.args.erase(node.args.begin() + static_cast<long>(*it));
                    }
                    node.function = mangled;
                }
                for (auto& a : node.args) rewriteExpr(a, st, subst);
            } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
                for (auto& a : node.args) rewriteExpr(a, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ArrayAccessExpr>) {
                rewriteExpr(*node.index, st, subst);
            } else if constexpr (std::is_same_v<T, ast::StringIndexExpr>) {
                rewriteExpr(*node.str, st, subst);
                rewriteExpr(*node.index, st, subst);
            } else if constexpr (std::is_same_v<T, ast::AddressOfExpr>) {
                rewriteExpr(*node.operand, st, subst);
            } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
                rewriteExpr(*node.operand, st, subst);
            } else if constexpr (std::is_same_v<T, ast::CastExpr>) {
                rewriteExpr(*node.operand, st, subst);
                node.targetType = rewriteTypeString(node.targetType, st, subst.types);
            } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
                rewriteExpr(*node.object, st, subst);
            } else if constexpr (std::is_same_v<T, ast::FunctionLiteralExpr>) {
                for (auto& p : node.params) p.type = rewriteTypeString(p.type, st, subst.types);
                node.returnType = rewriteTypeString(node.returnType, st, subst.types);
                for (auto& s2 : node.body) rewriteStmt(s2, st, subst);
            } else if constexpr (std::is_same_v<T, ast::StructLiteralExpr>) {
                node.structName = rewriteTypeString(node.structName, st, subst.types);
                for (auto& [fname, fexpr] : node.fields) rewriteExpr(fexpr, st, subst);
            } else if constexpr (std::is_same_v<T, ast::TemplateStringExpr>) {
                for (auto& part : node.parts) {
                    if (auto* e2 = std::get_if<ast::TemplateExprPart>(&part)) rewriteExpr(*e2->expr, st, subst);
                }
            }
        },
        e.node);
}

std::string instantiateFunction(const std::string& name, const std::vector<GenericArg>& args, MonoState& st) {
    std::vector<std::string> keys;
    for (auto& a : args) keys.push_back(argKey(a));
    std::string mangled = mangle(name, keys);
    if (st.registeredFns.count(mangled)) return mangled;
    if (st.inProgressFns.count(mangled)) {
        addError(st, "recursive generic instantiation of '" + name + "'");
        return mangled;
    }
    auto tmplIt = st.functionTemplates.find(name);
    if (tmplIt == st.functionTemplates.end()) {
        addError(st, "'" + name + "' is not a declared generic function");
        return mangled;
    }
    const ast::Function& tmpl = tmplIt->second;

    std::vector<const ast::Parameter*> comptimeParams;
    for (auto& p : tmpl.params) {
        if (p.isComptime) comptimeParams.push_back(&p);
    }
    if (comptimeParams.size() != args.size()) {
        addError(st, "'" + name + "' expects " + std::to_string(comptimeParams.size()) +
                         " compile-time argument(s), got " + std::to_string(args.size()));
        return mangled;
    }

    Subst typeSubst;
    ValueSubst valueSubst;
    for (size_t i = 0; i < comptimeParams.size(); i++) {
        if (args[i].isType) typeSubst[comptimeParams[i]->name] = args[i].typeName;
        else valueSubst[comptimeParams[i]->name] = args[i].value;
    }
    GenSubst subst{&typeSubst, &valueSubst};

    st.inProgressFns.insert(mangled);
    SourcePos savedPos = st.pos;
    st.pos = SourcePos{name, tmpl.line, tmpl.column};

    ast::Function concrete;
    concrete.name = mangled;
    concrete.isExported = tmpl.isExported;
    concrete.line = tmpl.line;
    concrete.column = tmpl.column;
    for (auto& p : tmpl.params) {
        if (p.isComptime) continue;
        concrete.params.push_back(
            ast::Parameter{p.name, rewriteTypeString(p.type, st, subst.types), false, p.line, p.column});
    }
    concrete.returnType = rewriteTypeString(tmpl.returnType, st, subst.types);
    concrete.body = cloneStmts(tmpl.body);
    for (auto& s : concrete.body) rewriteStmt(s, st, subst);
    st.pos = savedPos;

    st.inProgressFns.erase(mangled);
    st.registeredFns.insert(mangled);
    st.newFunctions.push_back(std::move(concrete));
    return mangled;
}

void rewriteFunction(ast::Function& f, MonoState& st) {
    st.pos = SourcePos{f.name, f.line, f.column};
    GenSubst subst{};
    if (f.receiver) f.receiver->type = rewriteTypeString(f.receiver->type, st, subst.types);
    for (auto& p : f.params) p.type = rewriteTypeString(p.type, st, subst.types);
    f.returnType = rewriteTypeString(f.returnType, st, subst.types);
    for (auto& s : f.body) rewriteStmt(s, st, subst);
}

} // namespace

std::vector<TypeError> monomorphizeGenerics(ast::Program& program, const std::string& targetOs,
                                            const std::string& targetArch, const std::string& memMode) {
    MonoState st;
    st.program = &program;
    st.targetOs = targetOs;
    st.targetArch = targetArch;
    st.memMode = memMode;

    std::vector<ast::StructDecl> concrete;
    for (auto& s : program.structs) {
        if (!s.typeParams.empty()) {
            st.templates[s.name] = s;
        } else {
            concrete.push_back(std::move(s));
        }
    }
    program.structs = std::move(concrete);

    for (auto& s : program.structs) {
        for (auto& f : s.fields) {
            st.pos = SourcePos{s.name, f.line, f.column};
            f.type = rewriteTypeString(f.type, st, nullptr);
        }
    }

    std::vector<ast::Function> concreteFns;
    for (auto& f : program.functions) {
        bool isGeneric = false;
        for (auto& p : f.params) {
            if (p.isComptime) { isGeneric = true; break; }
        }
        if (isGeneric) {
            std::string fname = f.name;
            st.functionTemplates[fname] = std::move(f);
        } else {
            concreteFns.push_back(std::move(f));
        }
    }
    program.functions = std::move(concreteFns);

    for (auto& f : program.functions) rewriteFunction(f, st);
    for (auto& [modName, mod] : program.modules) {
        for (auto& f : mod.functions) rewriteFunction(f, st);
    }

    for (auto& s : st.newStructs) program.structs.push_back(std::move(s));
    for (auto& f : st.newFunctions) program.functions.push_back(std::move(f));
    return std::move(st.errors);
}

} // namespace agn::parser
