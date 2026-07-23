# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
cmake_minimum_required(VERSION 3.20)
project(agnostic C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra)

add_subdirectory(src/ast)
add_subdirectory(src/misc)
add_subdirectory(src/lexer)
add_subdirectory(src/parser)
add_subdirectory(src/platform/linux)
add_subdirectory(src/platform/freebsd)
add_subdirectory(src/platform/windows)
add_subdirectory(src/platform/hurd)
add_subdirectory(src/memory/manual)
add_subdirectory(src/memory/arc)
add_subdirectory(src/memory/orc)
add_subdirectory(src/backend/llvm)
add_subdirectory(src/backend/nvm)
add_subdirectory(src/backend/gcc)
add_subdirectory(src/cli)

add_library(agnostic_lib INTERFACE)
target_link_libraries(agnostic_lib INTERFACE
    agn_ast
    agn_misc
    agn_lexer
    agn_parser
    agn_backend_llvm
    agn_backend_nvm
    agn_backend_gcc
)
set_target_properties(agnostic_lib PROPERTIES EXPORT_NAME agnostic)
