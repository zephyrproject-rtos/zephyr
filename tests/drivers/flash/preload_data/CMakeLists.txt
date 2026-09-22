#
# Copyright (c) 2026 Analog Devices Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(flash_preload)

target_sources(app PRIVATE src/main.c)

if(CONFIG_FLASH_SEMIHOST)
  # Write to "flash.bin" in the build folder. This is used by the flash semihost driver to simulate flash memory.
  file(WRITE "${CMAKE_BINARY_DIR}/flash.bin" "flash_data")
endif()
