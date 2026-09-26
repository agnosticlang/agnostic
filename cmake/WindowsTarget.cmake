# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
find_package(LLVM CONFIG REQUIRED)
find_program(AGN_CLANG NAMES clang HINTS ${LLVM_TOOLS_BINARY_DIR})
find_program(AGN_LLVM_LIB NAMES llvm-lib HINTS ${LLVM_TOOLS_BINARY_DIR})
find_program(AGN_LLVM_DLLTOOL NAMES llvm-dlltool HINTS ${LLVM_TOOLS_BINARY_DIR})

if(AGN_CLANG AND AGN_LLVM_LIB AND AGN_LLVM_DLLTOOL)
  set(AGN_WINDOWS_TARGET ON)
else()
  set(AGN_WINDOWS_TARGET OFF)
  message(STATUS "Windows target disabled: clang, llvm-lib, or llvm-dlltool not found")
endif()

function(agn_add_windows_library name)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEFINITIONS;INPUTS" ${ARGN})
  set(objects "")
  foreach(source ${ARG_SOURCES})
    get_filename_component(stem ${source} NAME_WE)
    get_filename_component(extension ${source} EXT)
    set(object ${CMAKE_CURRENT_BINARY_DIR}/${name}.${stem}.obj)
    if(extension STREQUAL ".S")
      set(flags "")
    else()
      set(flags -std=c++20 -O2 -Wall -Wextra -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector)
    endif()
    add_custom_command(
      OUTPUT ${object}
      COMMAND ${AGN_CLANG} --target=x86_64-pc-windows-msvc ${flags} -I${PROJECT_SOURCE_DIR}/include
              ${ARG_DEFINITIONS} -MD -MF ${object}.d -c ${CMAKE_CURRENT_SOURCE_DIR}/${source} -o ${object}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
      DEPFILE ${object}.d
      COMMENT "Building ${source} for the windows target"
      VERBATIM)
    list(APPEND objects ${object})
  endforeach()

  set(library ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.a)
  add_custom_command(
    OUTPUT ${library}
    COMMAND ${AGN_LLVM_LIB} -nologo -out:${library} ${objects} ${ARG_INPUTS}
    DEPENDS ${objects} ${ARG_INPUTS}
    COMMENT "Archiving ${name}"
    VERBATIM)
  add_custom_target(${name} ALL DEPENDS ${library})
  install(FILES ${library} DESTINATION lib/agnostic)
endfunction()
