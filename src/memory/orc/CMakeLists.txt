# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_memory_orc STATIC orc.cpp rt_shim.cpp)
target_include_directories(agn_memory_orc PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_memory_orc PUBLIC agn_memory_manual)
target_compile_options(agn_memory_orc PRIVATE -ffreestanding -fno-exceptions -fno-rtti)
