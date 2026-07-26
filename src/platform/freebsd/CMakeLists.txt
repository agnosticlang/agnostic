# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
enable_language(ASM)

add_library(agn_platform_freebsd STATIC syscalls.cpp start.S)
target_include_directories(agn_platform_freebsd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_options(agn_platform_freebsd PRIVATE -ffreestanding -fno-stack-protector -fno-exceptions -fno-rtti)
