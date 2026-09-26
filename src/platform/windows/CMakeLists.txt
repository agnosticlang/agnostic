# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
if(AGN_WINDOWS_TARGET)
  set(KERNEL32_IMPORTS ${CMAKE_CURRENT_BINARY_DIR}/kernel32.lib)
  add_custom_command(
    OUTPUT ${KERNEL32_IMPORTS}
    COMMAND ${AGN_LLVM_DLLTOOL} -m i386:x86-64 -d ${CMAKE_CURRENT_SOURCE_DIR}/kernel32.def -l ${KERNEL32_IMPORTS}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kernel32.def
    COMMENT "Generating the kernel32 import library"
    VERBATIM)
  agn_add_windows_library(agn_platform_windows SOURCES kernel32.cpp chkstk.S INPUTS ${KERNEL32_IMPORTS})
endif()
