# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_backend_gcc STATIC backend.cpp)
target_include_directories(agn_backend_gcc PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_backend_gcc PUBLIC agn_ast)
