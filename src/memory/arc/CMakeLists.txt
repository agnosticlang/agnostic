# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_memory_arc STATIC arc.cpp rt_shim.cpp)
target_include_directories(agn_memory_arc PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_memory_arc PUBLIC agn_memory_manual)
target_compile_options(agn_memory_arc PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

add_library(agn_memory_arc_freebsd STATIC arc.cpp rt_shim.cpp)
target_include_directories(agn_memory_arc_freebsd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_memory_arc_freebsd PUBLIC agn_memory_manual_freebsd)
target_compile_options(agn_memory_arc_freebsd PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

add_library(agn_memory_arc_hurd STATIC arc.cpp rt_shim.cpp)
target_include_directories(agn_memory_arc_hurd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_memory_arc_hurd PUBLIC agn_memory_manual_hurd)
target_compile_options(agn_memory_arc_hurd PRIVATE -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)

if(AGN_WINDOWS_TARGET)
  agn_add_windows_library(agn_memory_arc_windows SOURCES arc.cpp rt_shim.cpp)
endif()
