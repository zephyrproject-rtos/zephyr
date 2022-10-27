# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(code_relocation)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Code relocation feature
zephyr_code_relocate(src/test_file1.c SRAM2)

zephyr_code_relocate(src/test_file2.c RAM)

zephyr_code_relocate(src/test_file3.c SRAM2_TEXT)
zephyr_code_relocate(src/test_file3.c RAM_DATA)
zephyr_code_relocate(src/test_file3.c SRAM2_BSS)

zephyr_code_relocate(../../../kernel/sem.c RAM)

if (CONFIG_RELOCATE_TO_ITCM)
zephyr_code_relocate(../../../lib/libc/minimal/source/string/string.c ITCM_TEXT)
endif()

zephyr_linker_sources(SECTIONS custom-sections.ld)
