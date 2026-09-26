# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
option(AGNOSTIC_BOOTSTRAP
       "Build with the plain C++ lexer (stage 0) instead of the self-hosted lexer.agn" OFF)
set(AGNOSTIC_BOOTSTRAP_COMPILER "" CACHE FILEPATH
    "Existing agnostic compiler to build lexer.agn with, instead of building stage 0 in this tree")

add_library(agn_lexer_bootstrap STATIC EXCLUDE_FROM_ALL ${PROJECT_SOURCE_DIR}/old/lexer/lexer.cpp)
target_include_directories(agn_lexer_bootstrap PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(agn_lexer_bootstrap PUBLIC agn_misc)

if(AGNOSTIC_BOOTSTRAP)
  add_library(agn_lexer ALIAS agn_lexer_bootstrap)
else()
  if(AGNOSTIC_BOOTSTRAP_COMPILER)
    set(STAGE0_COMPILER ${AGNOSTIC_BOOTSTRAP_COMPILER})
    set(STAGE0_TARGET "")
  else()
    set(STAGE0_COMPILER $<TARGET_FILE:agnostic_stage0>)
    set(STAGE0_TARGET agnostic_stage0)
  endif()

  set(LEXER_AGN_OBJ ${CMAKE_CURRENT_BINARY_DIR}/lexer_agn.o)
  add_custom_command(
    OUTPUT ${LEXER_AGN_OBJ}
    COMMAND ${STAGE0_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/lexer.agn
            --backend=llvm --mem=manual -c --output=${CMAKE_CURRENT_BINARY_DIR}/lexer_agn
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lexer.agn ${STAGE0_TARGET}
    COMMENT "Self-hosting: compiling lexer.agn with the stage 0 compiler"
    VERBATIM
  )
  add_custom_target(agn_lexer_agn_obj DEPENDS ${LEXER_AGN_OBJ})
  set_source_files_properties(${LEXER_AGN_OBJ} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

  add_library(agn_lexer STATIC lexer.cpp ${LEXER_AGN_OBJ})
  add_dependencies(agn_lexer agn_lexer_agn_obj)
  target_include_directories(agn_lexer PUBLIC ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(agn_lexer PUBLIC agn_misc agn_llvm_runtime agn_memory_manual agn_platform_linux)

  set(AGNOSTIC_EMBED_SHIM ${CMAKE_CURRENT_SOURCE_DIR}/agn_embed_shim.cpp PARENT_SCOPE)
endif()
