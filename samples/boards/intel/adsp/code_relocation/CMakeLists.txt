# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(adsp_cavs_code_relocation)

target_sources(app PRIVATE src/main.c src/reloc.c)

zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM2_LITERAL)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM2_TEXT)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM3_DATA)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM3_RODATA)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM4_BSS)
