# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
find_package(LLVM CONFIG REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_library(agn_backend_llvm STATIC codegen.cpp)
target_include_directories(agn_backend_llvm PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(agn_backend_llvm PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(agn_backend_llvm PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(agn_backend_llvm PUBLIC agn_ast agn_parser)

if(LLVM_LINK_LLVM_DYLIB)
  target_link_libraries(agn_backend_llvm PRIVATE LLVM)
else()
  llvm_map_components_to_libnames(llvm_libs core support mc target x86codegen x86asmparser x86desc x86info asmprinter)
  target_link_libraries(agn_backend_llvm PRIVATE ${llvm_libs})
endif()

add_subdirectory(runtime)
