# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(chan_blen_transfer)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_code_relocate(FILES src/test_dma.c LOCATION ${CONFIG_DMA_LOOP_TRANSFER_RELOCATE_SECTION}_RODATA_BSS)
