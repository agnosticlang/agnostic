# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
add_library(agn_driver OBJECT main.cpp)
target_include_directories(agn_driver PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(agn_driver PRIVATE AGNOSTIC_VERSION="${AGNOSTIC_VERSION}"
                                              AGNOSTIC_SOURCE_STDLIB_DIR="${PROJECT_SOURCE_DIR}/stdlib")

add_executable(agnostic)
if(AGNOSTIC_EMBED_SHIM)
  target_sources(agnostic PRIVATE ${AGNOSTIC_EMBED_SHIM})
endif()
target_link_libraries(agnostic PRIVATE agn_driver agn_parser agn_backend_llvm agn_backend_gcc agn_lexer)

if(NOT AGNOSTIC_BOOTSTRAP AND NOT AGNOSTIC_BOOTSTRAP_COMPILER)
  add_executable(agnostic_stage0)
  target_link_libraries(agnostic_stage0 PRIVATE agn_driver agn_parser agn_backend_llvm agn_backend_gcc
                                                agn_lexer_bootstrap)
endif()
