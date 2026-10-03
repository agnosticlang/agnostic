// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "parser/escape.hpp"

namespace agn::parser {

namespace {

bool holdsHeapRef(const TypeChecker& checker, const Type& type) {
    switch (type.kind) {
        case TypeKind::String: case TypeKind::Ptr: case TypeKind::Function:
            return true;
        case TypeKind::Array:
            return holdsHeapRef(checker, *type.elementType);
        case TypeKind::Struct:
            for (auto& [name, fieldType] : checker.structs().at(type.structName)) {
                if (holdsHeapRef(checker, fieldType)) return true;
            }
            return false;
        default:
            return false;
    }
}

bool storesHeapRef(const TypeChecker& checker, const Type& type) {
    switch (type.kind) {
        case TypeKind::Function:
            return true;
        case TypeKind::Ptr:
            return holdsHeapRef(checker, *type.pointee);
        case TypeKind::Array:
            return storesHeapRef(checker, *type.elementType);
        case TypeKind::Struct:
            for (auto& [name, fieldType] : checker.structs().at(type.structName)) {
                if (storesHeapRef(checker, fieldType)) return true;
            }
            return false;
        default:
            return false;
    }
}

} // namespace

bool canUseOwnRegion(const TypeChecker& checker, const std::vector<Type>& params, const Type& returnType,
                     const std::vector<Type>& captures) {
    if (holdsHeapRef(checker, returnType)) return false;
    for (auto& p : params) {
        if (storesHeapRef(checker, p)) return false;
    }
    for (auto& c : captures) {
        if (holdsHeapRef(checker, c)) return false;
    }
    return true;
}

} // namespace agn::parser
