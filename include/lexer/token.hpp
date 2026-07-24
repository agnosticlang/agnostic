// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <cstdint>
#include <string>

namespace agn::lexer {

enum class TokenKind {
    Package, Import, Func, Var, If, Else, For, Return, Asm, Struct, Comptime,
    Break, Continue,

    Identifier, Number, String,

    Plus, Minus, Star, Slash, Percent, Assign, Equal, NotEqual,
    Less, LessEqual, Greater, GreaterEqual, And, Or, Not, Pipe, Caret,
    LShift, RShift,

    LeftParen, RightParen, LeftBrace, RightBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, Dot, Arrow, Ampersand, DoublePlus, Dollar,

    Newline, Eof,
};

struct Token {
    TokenKind kind;
    std::string text;
    int64_t number = 0;

    bool operator==(const Token& other) const {
        if (kind != other.kind) return false;
        switch (kind) {
            case TokenKind::Identifier:
            case TokenKind::String:
                return text == other.text;
            case TokenKind::Number:
                return number == other.number;
            default:
                return true;
        }
    }
    bool operator==(TokenKind k) const { return kind == k; }
    bool operator!=(TokenKind k) const { return kind != k; }
};

const char* tokenKindName(TokenKind kind);

} // namespace agn::lexer
