// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <optional>
#include <stdexcept>
#include <string>

namespace agn::misc {

enum class ErrorKind { Lexer, Parser, Type, Module, CodeGen };

class CompileError : public std::exception {
public:
    CompileError(ErrorKind kind, std::string message, std::string file,
                 size_t line, size_t column)
        : kind_(kind), message_(std::move(message)), file_(std::move(file)),
          line_(line), column_(column) {}

    CompileError& withSourceLine(std::string line) {
        sourceLine_ = std::move(line);
        return *this;
    }

    void display() const;

    const char* what() const noexcept override { return message_.c_str(); }
    ErrorKind kind() const { return kind_; }

private:
    ErrorKind kind_;
    std::string message_;
    std::string file_;
    size_t line_;
    size_t column_;
    std::optional<std::string> sourceLine_;
};

} // namespace agn::misc
