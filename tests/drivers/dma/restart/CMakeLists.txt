# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dma_restart)

target_sources(app PRIVATE src/main.c)

if(CONFIG_CODE_DATA_RELOCATION)
  zephyr_code_relocate(FILES src/main.c LOCATION ${CONFIG_DMA_RESTART_RELOCATE_SECTION}_DATA_BSS)
endif()
