# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_backend_nvm STATIC codegen.cpp)
target_include_directories(agn_backend_nvm PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_backend_nvm PUBLIC agn_ast agn_parser)
