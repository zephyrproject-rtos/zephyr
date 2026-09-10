# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 sensry.io

zephyr_include_directories(common)

zephyr_sources(
        common/crt0.S
        common/soc.c
        common/udma.c
)

zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors common/vector_table.ld)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/common/linker.ld CACHE INTERNAL "")
