# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_memory_arc STATIC arc.cpp rt_shim.cpp)
target_include_directories(agn_memory_arc PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(agn_memory_arc PUBLIC agn_memory_manual)
target_compile_options(agn_memory_arc PRIVATE -ffreestanding -fno-exceptions -fno-rtti)
