# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(adsp_cavs_code_relocation)

target_sources(app PRIVATE src/main.c src/reloc.c)

zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM2_LITERAL TARGET app)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM2_TEXT TARGET app)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM3_DATA TARGET app)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM3_RODATA TARGET app)
zephyr_code_relocate(FILES src/reloc.c LOCATION SRAM4_BSS TARGET app)
