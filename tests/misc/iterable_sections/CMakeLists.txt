# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iterable_sections)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_linker_sources(DATA_SECTIONS sections-ram.ld)
zephyr_iterable_section(NAME test_ram GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME test_ram2 GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME test_ram_named GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME test_ram_numeric NUMERIC GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_iterable_section(NAME test_rom KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
zephyr_iterable_section(NAME test_rom2 KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
zephyr_iterable_section(NAME test_rom_named KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
zephyr_iterable_section(NAME test_rom_numeric NUMERIC KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
