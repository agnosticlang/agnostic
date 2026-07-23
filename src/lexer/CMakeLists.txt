# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_lexer STATIC lexer.cpp)
target_include_directories(agn_lexer PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_lexer PUBLIC agn_misc)
