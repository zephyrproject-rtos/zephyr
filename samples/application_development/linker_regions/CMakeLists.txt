# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(linker_regions)

zephyr_linker_sources(SECTIONS partition_first.ld)
zephyr_linker_sources_ifdef(SECOND_REGION SECTIONS partition_second.ld)

target_sources(app PRIVATE src/main.c)
