// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "lexer/token.hpp"

#include <string>
#include <vector>

namespace agn::lexer {

class Lexer {
public:
    Lexer(std::string input, std::string file);

    std::vector<Token> tokenize();

private:
    std::string input_;
    std::string file_;
};

} // namespace agn::lexer
