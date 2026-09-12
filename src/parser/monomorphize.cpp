// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "parser/monomorphize.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>

namespace agn::parser {

namespace {

using Subst = std::unordered_map<std::string, std::string>;

struct MonoState {
    std::unordered_map<std::string, ast::StructDecl> templates;
    std::unordered_set<std::string> registered;
    std::unordered_set<std::string> inProgress;
    std::vector<ast::StructDecl> newStructs;

    std::unordered_map<std::string, ast::Function> functionTemplates;
    std::unordered_set<std::string> registeredFns;
    std::unordered_set<std::string> inProgressFns;
    std::vector<ast::Function> newFunctions;
};

std::string rewriteTypeString(const std::string& s, MonoState& st, const Subst* subst);
void rewriteStmt(ast::Statement& s, MonoState& st, const Subst* subst);
void rewriteExpr(ast::Expression& e, MonoState& st, const Subst* subst);
std::string instantiateFunction(const std::string& name, const std::vector<std::string>& typeArgs, MonoState& st);

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
            } else if constexpr (std::is_same_v<T, ast::EvalExpr>) {
                ast::EvalExpr copy;
                copy.instruction = std::make_unique<ast::Expression>(cloneExpr(*node.instruction));
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
            } else if constexpr (std::is_same_v<T, ast::InlineAsmStmt>) {
                out.node = node;
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

std::string instantiate(const std::string& name, const std::vector<std::string>& args, MonoState& st) {
    std::string mangled = mangle(name, args);
    if (st.registered.count(mangled)) return mangled;
    if (st.inProgress.count(mangled)) {
        std::fprintf(stderr, "error: recursive generic instantiation of '%s'\n", name.c_str());
        std::exit(1);
    }
    auto tmplIt = st.templates.find(name);
    if (tmplIt == st.templates.end()) {
        std::fprintf(stderr, "error: '%s' is not a declared generic struct\n", name.c_str());
        std::exit(1);
    }
    const ast::StructDecl& tmpl = tmplIt->second;
    if (tmpl.typeParams.size() != args.size()) {
        std::fprintf(stderr, "error: '%s' expects %zu type argument(s), got %zu\n", name.c_str(),
                     tmpl.typeParams.size(), args.size());
        std::exit(1);
    }

    Subst subst;
    for (size_t i = 0; i < tmpl.typeParams.size(); i++) subst[tmpl.typeParams[i]] = args[i];

    st.inProgress.insert(mangled);
    std::vector<ast::Parameter> concreteFields;
    for (auto& f : tmpl.fields) {
        concreteFields.push_back(ast::Parameter{f.name, rewriteTypeString(f.type, st, &subst), false});
    }
    st.inProgress.erase(mangled);
    st.registered.insert(mangled);

    st.newStructs.push_back(ast::StructDecl{mangled, {}, std::move(concreteFields)});
    return mangled;
}

bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

std::string rewriteOne(const std::string& s, size_t& pos, MonoState& st, const Subst* subst) {
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

void rewriteStmt(ast::Statement& s, MonoState& st, const Subst* subst) {
    std::visit(
        [&](auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::VarDeclStmt>) {
                if (!node.varType.empty()) node.varType = rewriteTypeString(node.varType, st, subst);
                if (node.value) rewriteExpr(*node.value, st, subst);
            } else if constexpr (std::is_same_v<T, ast::ArrayDeclStmt>) {
                node.elementType = rewriteTypeString(node.elementType, st, subst);
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

void rewriteExpr(ast::Expression& e, MonoState& st, const Subst* subst) {
    std::visit(
        [&](auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
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
                    std::vector<std::string> typeArgs;
                    for (size_t idx : comptimeIndices) {
                        auto* ident = std::get_if<ast::IdentifierExpr>(&node.args[idx].node);
                        if (!ident) {
                            std::fprintf(stderr, "error: argument %zu of '%s' must be a type name\n", idx,
                                         node.function.c_str());
                            std::exit(1);
                        }
                        std::string typeName = ident->name;
                        if (subst) {
                            auto it = subst->find(typeName);
                            if (it != subst->end()) typeName = it->second;
                        }
                        typeArgs.push_back(std::move(typeName));
                    }
                    std::string mangled = instantiateFunction(node.function, typeArgs, st);
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
            } else if constexpr (std::is_same_v<T, ast::EvalExpr>) {
                rewriteExpr(*node.instruction, st, subst);
            } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
                rewriteExpr(*node.object, st, subst);
            } else if constexpr (std::is_same_v<T, ast::FunctionLiteralExpr>) {
                for (auto& p : node.params) p.type = rewriteTypeString(p.type, st, subst);
                node.returnType = rewriteTypeString(node.returnType, st, subst);
                for (auto& s2 : node.body) rewriteStmt(s2, st, subst);
            } else if constexpr (std::is_same_v<T, ast::StructLiteralExpr>) {
                node.structName = rewriteTypeString(node.structName, st, subst);
                for (auto& [fname, fexpr] : node.fields) rewriteExpr(fexpr, st, subst);
            } else if constexpr (std::is_same_v<T, ast::TemplateStringExpr>) {
                for (auto& part : node.parts) {
                    if (auto* e2 = std::get_if<ast::TemplateExprPart>(&part)) rewriteExpr(*e2->expr, st, subst);
                }
            }
        },
        e.node);
}

std::string instantiateFunction(const std::string& name, const std::vector<std::string>& typeArgs, MonoState& st) {
    std::string mangled = mangle(name, typeArgs);
    if (st.registeredFns.count(mangled)) return mangled;
    if (st.inProgressFns.count(mangled)) {
        std::fprintf(stderr, "error: recursive generic instantiation of '%s'\n", name.c_str());
        std::exit(1);
    }
    auto tmplIt = st.functionTemplates.find(name);
    if (tmplIt == st.functionTemplates.end()) {
        std::fprintf(stderr, "error: '%s' is not a declared generic function\n", name.c_str());
        std::exit(1);
    }
    const ast::Function& tmpl = tmplIt->second;

    std::vector<std::string> comptimeNames;
    for (auto& p : tmpl.params) {
        if (p.isComptime) comptimeNames.push_back(p.name);
    }
    if (comptimeNames.size() != typeArgs.size()) {
        std::fprintf(stderr, "error: '%s' expects %zu compile-time type argument(s), got %zu\n", name.c_str(),
                     comptimeNames.size(), typeArgs.size());
        std::exit(1);
    }

    Subst subst;
    for (size_t i = 0; i < comptimeNames.size(); i++) subst[comptimeNames[i]] = typeArgs[i];

    st.inProgressFns.insert(mangled);

    ast::Function concrete;
    concrete.name = mangled;
    concrete.isExported = tmpl.isExported;
    for (auto& p : tmpl.params) {
        if (p.isComptime) continue;
        concrete.params.push_back(ast::Parameter{p.name, rewriteTypeString(p.type, st, &subst), false});
    }
    concrete.returnType = rewriteTypeString(tmpl.returnType, st, &subst);
    concrete.body = cloneStmts(tmpl.body);
    for (auto& s : concrete.body) rewriteStmt(s, st, &subst);

    st.inProgressFns.erase(mangled);
    st.registeredFns.insert(mangled);
    st.newFunctions.push_back(std::move(concrete));
    return mangled;
}

void rewriteFunction(ast::Function& f, MonoState& st) {
    if (f.receiver) f.receiver->type = rewriteTypeString(f.receiver->type, st, nullptr);
    for (auto& p : f.params) p.type = rewriteTypeString(p.type, st, nullptr);
    f.returnType = rewriteTypeString(f.returnType, st, nullptr);
    for (auto& s : f.body) rewriteStmt(s, st, nullptr);
}

} // namespace

void monomorphizeGenerics(ast::Program& program) {
    MonoState st;

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
        for (auto& f : s.fields) f.type = rewriteTypeString(f.type, st, nullptr);
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
}

} // namespace agn::parser
