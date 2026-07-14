# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_llvm_runtime STATIC stdio.cpp)
target_include_directories(agn_llvm_runtime PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(agn_llvm_runtime PUBLIC agn_platform_linux)
target_compile_options(agn_llvm_runtime PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)
