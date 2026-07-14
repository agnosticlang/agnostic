// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "lexer/token.hpp"

#include <optional>
#include <string>
#include <vector>

namespace agn::lexer {

class Lexer {
public:
    Lexer(std::string input, std::string file);

    std::vector<Token> tokenize();

private:
    void advance();
    std::optional<char> peek(size_t offset) const;
    void skipWhitespace();
    void skipComment();
    Token readNumber();
    Token readIdentifier();
    Token readString();

    std::string input_;
    size_t position_ = 0;
    std::optional<char> currentChar_;
    size_t line_ = 1;
    size_t column_ = 1;
    std::string file_;
};

} // namespace agn::lexer
