# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gc_disp_test)
target_sources(app PRIVATE src/main.c)
if(CONFIG_XIP)
    zephyr_code_relocate(src/main.c ROM_TEXT NOCOPY)
    zephyr_code_relocate(src/main.c ROM_DATA)
    zephyr_code_relocate(src/main.c ROM_BSS)
endif()