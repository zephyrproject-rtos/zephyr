# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(code_relocation)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Code relocation feature
zephyr_code_relocate(src/test_file1.c SRAM2)

zephyr_code_relocate(src/test_file2.c SRAM)

zephyr_code_relocate(src/test_file3.c SRAM2_TEXT)
zephyr_code_relocate(src/test_file3.c SRAM_DATA)
zephyr_code_relocate(src/test_file3.c SRAM2_BSS)

zephyr_code_relocate(../../../kernel/sem.c SRAM)

zephyr_linker_sources(SECTIONS custom-sections.ld)
