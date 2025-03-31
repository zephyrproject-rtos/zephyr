# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(UI_Zephyr_test4)

target_sources(app PRIVATE src/main.c)
# zephyr_library_include_directories(include(zephyr(drivers(gpio_shakti.h))))
if(CONFIG_XIP)
zephyr_code_relocate(FILES src/main.c LOCATION ROM_TEXT NOCOPY)
zephyr_code_relocate(FILES src/main.c LOCATION ROM_DATA)
zephyr_code_relocate(FILES src/main.c LOCATION ROM_BSS)
endif()