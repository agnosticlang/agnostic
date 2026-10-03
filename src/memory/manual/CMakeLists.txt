# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_memory_manual STATIC allocator.cpp rt_shim.cpp)
target_include_directories(agn_memory_manual PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_memory_manual PUBLIC agn_platform_linux)
target_compile_options(agn_memory_manual PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

add_library(agn_memory_manual_freebsd STATIC allocator.cpp rt_shim.cpp)
target_include_directories(agn_memory_manual_freebsd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_memory_manual_freebsd PUBLIC agn_platform_freebsd)
target_compile_definitions(agn_memory_manual_freebsd PRIVATE AGN_TARGET_FREEBSD)
target_compile_options(agn_memory_manual_freebsd PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

add_library(agn_memory_manual_hurd STATIC allocator.cpp rt_shim.cpp)
target_include_directories(agn_memory_manual_hurd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_memory_manual_hurd PUBLIC agn_platform_hurd)
target_compile_definitions(agn_memory_manual_hurd PRIVATE AGN_TARGET_HURD)
target_compile_options(agn_memory_manual_hurd PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

if(AGN_WINDOWS_TARGET)
  agn_add_windows_library(agn_memory_manual_windows SOURCES allocator.cpp rt_shim.cpp DEFINITIONS -DAGN_TARGET_WINDOWS)
endif()
