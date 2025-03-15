# SPDX-License-Identifier: Apache-2.0
# Copyright 2022 NXP

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(code_relocation)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if (CONFIG_BOARD_QEMU_XTENSA)
    set(RAM_PHDR PHDR sram0_phdr)
    set(SRAM2_PHDR PHDR sram2_phdr)
endif()

# Code relocation feature
zephyr_code_relocate(FILES src/test_file1.c ${SRAM2_PHDR} LOCATION SRAM2)

zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_file2.c ${RAM_PHDR} LOCATION RAM FILTER ".*sram")

# Add custom library that we can relocate code for
add_subdirectory(test_lib)
target_link_libraries(app PUBLIC test_lib)
target_include_directories(app PUBLIC ${CMAKE_CURRENT_LIST_DIR}/test_lib)
# Relocate library code
zephyr_code_relocate(LIBRARY test_lib LOCATION SRAM2)

# Test support for a simple generator expression to relocate two files
set(reloc_files src/test_file4.c src/test_file5.c)
set(genex_expr
  ${CMAKE_CURRENT_LIST_DIR}/$<JOIN:${reloc_files},$<SEMICOLON>${CMAKE_CURRENT_LIST_DIR}/>)
zephyr_code_relocate(FILES ${genex_expr} LOCATION SRAM2)

zephyr_code_relocate(FILES src/test_file3.c LOCATION SRAM2_LITERAL)
zephyr_code_relocate(FILES src/test_file3.c LOCATION SRAM2_TEXT)
zephyr_code_relocate(FILES src/test_file3.c LOCATION RAM_DATA)
zephyr_code_relocate(FILES src/test_file3.c LOCATION SRAM2_BSS)

# Test NOKEEP support. Placing both KEEP and NOKEEP symbols in the same location
# (this and test_file2.c in RAM) should work fine.
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/sem.c ${RAM_PHDR} LOCATION RAM NOKEEP)

if (CONFIG_RELOCATE_TO_ITCM)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/lib/libc/minimal/source/string/string.c
	LOCATION ITCM_TEXT)
endif()

zephyr_linker_sources(SECTIONS custom-sections.ld)
