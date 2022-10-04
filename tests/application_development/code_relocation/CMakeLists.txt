# SPDX-License-Identifier: Apache-2.0

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

zephyr_code_relocate(FILES src/test_file2.c ${RAM_PHDR} LOCATION RAM)

zephyr_code_relocate(FILES src/test_file3.c LOCATION SRAM2_LITERAL)
zephyr_code_relocate(FILES src/test_file3.c LOCATION SRAM2_TEXT)
zephyr_code_relocate(FILES src/test_file3.c LOCATION RAM_DATA)
zephyr_code_relocate(FILES src/test_file3.c LOCATION SRAM2_BSS)

zephyr_code_relocate(FILES ../../../kernel/sem.c ${RAM_PHDR} LOCATION RAM)

if (CONFIG_RELOCATE_TO_ITCM)
zephyr_code_relocate(FILES ../../../lib/libc/minimal/source/string/string.c
	LOCATION ITCM_TEXT)
endif()

zephyr_linker_sources(SECTIONS custom-sections.ld)
