# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sdram_test)

target_sources(app PRIVATE src/main.c)

# Add custom linker script for SDRAM sections
zephyr_linker_sources(SECTIONS sdram.ld)