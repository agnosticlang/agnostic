# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
enable_language(ASM)

add_library(agn_platform_hurd STATIC rpc.cpp traps.S start.S)
target_include_directories(agn_platform_hurd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_options(agn_platform_hurd PRIVATE -ffreestanding -fno-stack-protector -fno-exceptions -fno-rtti)
