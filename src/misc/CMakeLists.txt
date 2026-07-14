# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_misc STATIC diagnostic.cpp)
target_include_directories(agn_misc PUBLIC ${CMAKE_SOURCE_DIR}/include)
