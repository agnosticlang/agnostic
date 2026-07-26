# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
cmake_minimum_required(VERSION 3.20)

if(NOT AGNOSTIC_VERSION)
  set(AGNOSTIC_VERSION "0.0.0")
endif()

project(agnostic VERSION ${AGNOSTIC_VERSION} LANGUAGES C CXX)

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

install(TARGETS agnostic RUNTIME DESTINATION bin)
install(TARGETS agn_llvm_runtime agn_memory_arc agn_memory_manual agn_memory_orc agn_platform_linux
                agn_llvm_runtime_freebsd agn_memory_arc_freebsd agn_memory_manual_freebsd
                agn_memory_orc_freebsd agn_platform_freebsd
        ARCHIVE DESTINATION lib/agnostic)
install(DIRECTORY stdlib/ DESTINATION share/agnostic/stdlib)

set(CPACK_PACKAGE_NAME "agnostic")
set(CPACK_PACKAGE_VERSION ${AGNOSTIC_VERSION})
set(CPACK_PACKAGE_CONTACT "AnmiTaliDev <anmitalidev@nuros.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Agnostic programming language compiler")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)

include(CPack)
