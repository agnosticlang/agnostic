# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_llvm_runtime STATIC stdio.cpp fileio.cpp strext.cpp)
target_include_directories(agn_llvm_runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_llvm_runtime PUBLIC agn_platform_linux)
target_compile_options(agn_llvm_runtime PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

add_library(agn_llvm_runtime_freebsd STATIC stdio.cpp fileio.cpp strext.cpp)
target_include_directories(agn_llvm_runtime_freebsd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_llvm_runtime_freebsd PUBLIC agn_platform_freebsd)
target_compile_definitions(agn_llvm_runtime_freebsd PRIVATE AGN_TARGET_FREEBSD)
target_compile_options(agn_llvm_runtime_freebsd PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

if(AGN_WINDOWS_TARGET)
  agn_add_windows_library(agn_llvm_runtime_windows SOURCES stdio.cpp fileio.cpp strext.cpp
                          DEFINITIONS -DAGN_TARGET_WINDOWS)
endif()
