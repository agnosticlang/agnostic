# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
enable_language(ASM)

add_library(agn_platform_linux STATIC syscalls.cpp start.S)
target_include_directories(agn_platform_linux PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_options(agn_platform_linux PRIVATE -ffreestanding -fno-stack-protector -fno-exceptions -fno-rtti)
