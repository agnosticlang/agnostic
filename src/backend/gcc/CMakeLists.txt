# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
find_path(GCCJIT_INCLUDE_DIR NAMES libgccjit.h)
find_library(GCCJIT_LIBRARY NAMES gccjit)

if(NOT GCCJIT_INCLUDE_DIR OR NOT GCCJIT_LIBRARY)
  message(FATAL_ERROR "libgccjit not found (need libgccjit.h and libgccjit.so)")
endif()

add_library(agn_backend_gcc STATIC backend.cpp)
target_include_directories(agn_backend_gcc PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(agn_backend_gcc PRIVATE ${GCCJIT_INCLUDE_DIR})
target_link_libraries(agn_backend_gcc PUBLIC agn_ast agn_parser PRIVATE ${GCCJIT_LIBRARY})
