// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "misc/diagnostic.hpp"

#include <cstdio>

namespace agn::misc {

void CompileError::display() const {
    const char* kindStr = "error";
    switch (kind_) {
        case ErrorKind::Lexer:   kindStr = "lexer error"; break;
        case ErrorKind::Parser:  kindStr = "parser error"; break;
        case ErrorKind::Type:    kindStr = "type error"; break;
        case ErrorKind::Module:  kindStr = "module error"; break;
        case ErrorKind::CodeGen: kindStr = "codegen error"; break;
    }

    std::fprintf(stderr, "\x1b[1;31merror\x1b[0m: %s\n", message_.c_str());
    std::fprintf(stderr, "  \x1b[1;34m-->\x1b[0m %s:%zu:%zu\n", file_.c_str(), line_, column_);

    if (sourceLine_) {
        std::fprintf(stderr, "\x1b[1;34m%4zu |\x1b[0m\n", line_);
        std::fprintf(stderr, "\x1b[1;34m     |\x1b[0m %s\n", sourceLine_->c_str());
        std::string pad(column_ > 0 ? column_ - 1 : 0, ' ');
        std::fprintf(stderr, "\x1b[1;34m     |\x1b[0m %s\x1b[1;31m^\x1b[0m %s\n", pad.c_str(), kindStr);
    }
    std::fprintf(stderr, "\n");
}

} // namespace agn::misc
